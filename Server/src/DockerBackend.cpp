#include "DockerBackend.h"
#include "Logger.h"

#include "boost/process/extend.hpp"

namespace ex = boost::process::extend;

void DockerBackend::build_singularity_container() {
    std::string docker_command;
    docker_command += "/usr/bin/docker run --device=" + resource.loop_device
                      + " --security-opt apparmor=docker-singularity --cap-add SYS_ADMIN --name "
                      + docker_name + " --mount type=bind,source=" + working_directory
                      + ",destination=/work_dir -w /work_dir singularity_builder";

    // TODO: find out why OSX still throws an exception if error encountered
    std::error_code ec;
    boost::process::child docker_proc(docker_command, ec,
                                      (boost::process::std_out & boost::process::std_err) > std_pipe);
    // Check for an error launching before we detach the process
    if (ec) {
        logger::write("Docker backend launch failure: " + ec.message());
    } else {
        docker_proc.detach();
    }
}