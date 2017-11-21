#pragma once

#include <iostream>
#include <boost/asio/ip/tcp.hpp>
#include "ResourceQueue.h"
#include <boost/asio/spawn.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <Resource.h>
#include "Logger.h"
#include "Messenger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

// Create build directory insisde of `pwd`/container_scratch consisting of remote endpoint and local enpoint
// This should uniquely identify the connection as it includes the IP and port
static std::string get_build_dir(tcp::socket &socket) {
    auto pwd = boost::filesystem::current_path();
    std::string build_dir(pwd.string() + "/container_scratch/");

    auto remote_ip = socket.remote_endpoint().address().to_string();
    auto remote_port = std::to_string(socket.remote_endpoint().port());
    auto local_ip = socket.local_endpoint().address().to_string();
    auto local_port = std::to_string(socket.local_endpoint().port());
    build_dir += remote_ip + "_" + remote_port + "-" + local_ip + "_" + local_port;

    return build_dir;
}

class Builder {
public:
    // Create a unique build directory and set it as the current working directory
    explicit Builder(tcp::socket &socket,
                     ResourceQueue &queue,
                     asio::yield_context &yield) : socket(socket),
                                                   queue(queue),
                                                   yield(yield),
                                                   build_directory(get_build_dir(socket)),
                                                   messenger(socket) {

        boost::filesystem::create_directory(build_directory);

        boost::filesystem::current_path(build_directory);

        logger::write(socket, "New builder: " + build_directory);
    }

    // Remove the unique build directory
    ~Builder() {
        boost::system::error_code ec;

        auto pwd = boost::filesystem::current_path();
        std::string build_dir(pwd.string() + "/container_scratch/");

        boost::filesystem::remove_all(build_directory, ec);
        if (ec) {
            logger::write(socket, "Error removing directory: " + build_directory);
        } else {
            logger::write(socket, "Removing builder: " + build_directory);
        }
    }

    void singularity_build();

private:
    void receive_definition();

    void build_container();

    void send_image();


    std::string definition_path();

    tcp::socket &socket;
    ResourceQueue &queue;
    asio::yield_context &yield;
    const std::string build_directory;
    Resource resource;
    Messenger messenger;
};