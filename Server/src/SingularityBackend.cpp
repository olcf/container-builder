#include "SingularityBackend.h"
#include <boost/filesystem.hpp>
#include "Logger.h"

#include "boost/process/extend.hpp"
namespace ex = boost::process::extend;

void SingularityBackend::build_singularity_container() {
    boost::filesystem::current_path(working_directory);
    std::string singularity_command("sudo /usr/local/bin/singularity exec --containall /home/builder/singularity_backend.img /home/builder/build.sh");

    // TODO: find out why OSX still throws an exception if error encountered
    std::error_code ec;
    boost::process::child singularity_proc(singularity_command, ec, (boost::process::std_out & boost::process::std_err) > std_pipe, group);
    // Check for an error launching before we detach the process
    if (ec) {
        logger::write("Singularity backend launch failure: " + ec.message());
    } else {
        singularity_proc.detach();
    }
}