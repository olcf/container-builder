#include "DockerBackend.h"

void DockerBackend::build_singularity_container() {
    std::string docker_command;
    docker_command += "docker run --device=" + resource.loop_device +
                      " --security-opt apparmor=docker-singularity --cap-add SYS_ADMIN --name "
                      + docker_name + " --mount type=bind,source=" + working_directory
                      + ",destination=/work_dir -w /work_dir singularity_builder";

    boost::process::child docker_proc(docker_command, (boost::process::std_out & boost::process::std_err) > std_pipe);
    docker_proc.detach();
}