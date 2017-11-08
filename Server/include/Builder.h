#pragma once

#include <iostream>
#include <boost/asio/ip/tcp.hpp>
#include "ResourceQueue.h"
#include <boost/asio/spawn.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class Builder {
public:
    explicit Builder(tcp::socket &socket, ResourceQueue &queue, asio::yield_context& yield) : socket(socket),
                                                                                             queue(queue),
                                                                                             yield(yield) {
        // Create build directory consisting of remote endpoint and local enpoint
        // This should uniquely identify the connection as it includes the IP and port
        build_directory = "/home/builder/container_scratch";
        build_directory += boost::lexical_cast<std::string>(socket.remote_endpoint())
                        += std::string("_")
                        += boost::lexical_cast<std::string>(socket.local_endpoint());
        boost::filesystem::create_directory(build_directory);
    }

    ~Builder() {
        boost::system::error_code ec;
        boost::filesystem::remove_all(build_directory, ec);
        if(ec) {
            std::cerr<<"Error removing directory: "<<std::endl;
        }
    }

    void singularity_build();
private:
    void receive_definition();
    void build_container();
    void send_image();

    tcp::socket& socket;
    ResourceQueue& queue;
    asio::yield_context& yield;
    std::string build_directory;
};