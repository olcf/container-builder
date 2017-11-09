#pragma once

#include <iostream>
#include <boost/process.hpp>
#include "Resource.h"

// Return the parent directory of a file
static std::string parent_directory(std::string definition_filename) {
    boost::filesystem::path path(definition_filename);
    boost::filesystem::path parent_directory = path.parent_path();
    return parent_directory.string();
}

class DockerBackend {
public:
    explicit DockerBackend(Resource resource, boost::process::async_pipe std_pipe, std::string definition_filename)
            : resource(resource),
              std_pipe(std_pipe),
              definition_filename(definition_filename),
              docker_name("docker_"+ loop_device),
              working_directory(parent_directory(definition_filename))
    {};

    ~DockerBackend() {
        std::string stop_command;
        stop_command += "docker stop " + docker_name;
        boost::process::child stop_dock(stop_command);
        stop_dock.detach();

        std::string rm_command;
        rm_command += "docker rm " + docker_name;
        boost::process::child rm_dock(rm_command);
        rm_dock.detach();
    }

    void build_singularity_container();

private:
    const Resource resource;
    boost::process::async_pipe std_pipe;
    const std::string docker_name;
    const std::string definition_filename;
    const std::string loop_device;
    const std::string working_directory;
};
