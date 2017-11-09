#pragma once

#include <iostream>
#include <boost/asio/ip/tcp.hpp>
#include "ResourceQueue.h"
#include <boost/asio/spawn.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <Resource.h>

namespace asio = boost::asio;
using asio::ip::tcp;

static std::string get_home_dir() {
    const char *home = getenv("HOME");
    if (home == NULL) {
        throw std::invalid_argument("ERROR: HOME environment variable not set");
    }
    return std::string(home);
}

static std::string get_build_dir(tcp::socket &socket) {
    // Create build directory consisting of remote endpoint and local enpoint
    // This should uniquely identify the connection as it includes the IP and port

    std::string build_dir(get_home_dir() + "/container_scratch/");
    build_dir += boost::lexical_cast<std::string>(socket.remote_endpoint())
            += std::string("_")
            += boost::lexical_cast<std::string>(socket.local_endpoint());
    return build_dir;
}

class Builder {
public:
    explicit Builder(tcp::socket &socket, ResourceQueue &queue, asio::yield_context &yield) : socket(socket),
                                                                                              queue(queue),
                                                                                              yield(yield),
                                                                                              build_directory(
                                                                                                      get_build_dir(
                                                                                                              socket)) {
        boost::filesystem::create_directory(build_directory);
    }

    ~Builder() {
        boost::system::error_code ec;
        boost::filesystem::remove_all(build_directory, ec);
        if (ec) {
            std::cerr << "Error removing directory: " << std::endl;
        }
    }

    void singularity_build();

private:
    void receive_definition();

    void build_container();

    void send_image();


    std::string definition_filename();

    tcp::socket &socket;
    ResourceQueue &queue;
    asio::yield_context &yield;
    const std::string build_directory;
    Resource resource;
};