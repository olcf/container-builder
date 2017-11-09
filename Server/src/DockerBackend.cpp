#include "DockerBackend.h"

void DockerBackend::build_singularity_container() {
    std::string docker_command;
    docker_command += "/usr/bin/docker run --device=" + resource.loop_device
                      + " --security-opt apparmor=docker-singularity --cap-add SYS_ADMIN --name "
                      + docker_name + " --mount type=bind,source=" + working_directory
                      + ",destination=/work_dir -w /work_dir singularity_builder";

//    std::cout<<"Creating fake container\n";
//    docker_command = "/bin/dd if=/dev/zero of=" + working_directory + "/container.img  bs=512  count=3";

    boost::process::child docker_proc(docker_command, (boost::process::std_out & boost::process::std_err) > std_pipe);
    docker_proc.detach();
}