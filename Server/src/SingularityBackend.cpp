#include "SingularityBackend.h"
#include <boost/filesystem.hpp>
#include "Logger.h"

#include "boost/process/extend.hpp"

namespace ex = boost::process::extend;

void SingularityBackend::build_singularity_container() {

    // Run the build script inside a contained container.
    // Only the backend image, build script, and work path are mounted
    std::string singularity_command( std::string()
                    + "/usr/local/bin/singularity exec --containall "
                    + "-B /home/builder/singularity_backend.img "
                    + "-B /home/builder/build.sh "
                    + "--workdir " + working_directory
                    + " /home/builder/singularity_backend.img /home/builder/build.sh ");

    std::error_code ec;
    boost::process::child singularity_proc(singularity_command, ec, boost::process::std_in.close(),
                                           (boost::process::std_out & boost::process::std_err) > std_pipe, group);

    // Check for an error launching before we detach the process
    if (ec) {
        logger::write(socket, "Singularity backend launch failure: " + ec.message());
    } else {
        logger::write(socket, "detaching Singularity process: " + singularity_command);
        singularity_proc.detach();
    }
}