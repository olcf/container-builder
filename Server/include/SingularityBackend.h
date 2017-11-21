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

class SingularityBackend {
public:
    explicit SingularityBackend(tcp::socket &socket, Resource resource, boost::process::async_pipe &std_pipe,
                                std::string definition_filename)
            : socket(socket),
              resource(resource),
              std_pipe(std_pipe),
              definition_filename(definition_filename),
              working_directory(parent_directory(definition_filename)) {};

    ~SingularityBackend() {
        if (group.valid())
            group.terminate();
    }

    void build_singularity_container();

private:
    tcp::socket &socket;
    const Resource resource;
    boost::process::group group;
    boost::process::async_pipe &std_pipe;
    const std::string definition_filename;
    const std::string working_directory;
};
