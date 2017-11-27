#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include "Messenger.h"
#include "Builder.h"

namespace asio = boost::asio;
using asio::ip::tcp;

void create_def() {
    std::cout << "creating simple definition file\n";
    std::ofstream outfile("recipe.def");
    outfile << "BootStrap: docker\nFrom: ubuntu:zesty" << std::endl;
    outfile.close();
}

std::string queue_hostname() {
    auto env = std::getenv("QUEUE_HOSTNAME");
    if(!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_HOSTNAME");
    }

    return std::string(env);
}

std::string queue_port() {
    auto env = std::getenv("QUEUE_PORT");
    if(!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_HOSTNAME");
    }
    return std::string(env);
}


int main(int argc, char *argv[]) {
    try {
        asio::io_service io_service;
        tcp::socket queue_socket(io_service);
        tcp::resolver queue_resolver(io_service);
        asio::connect(queue_socket, queue_resolver.resolve({queue_hostname(), queue_port()}));

        Messenger queue_messenger(queue_socket);

        // Initiate a build request
        queue_messenger.send("checkout_builder_request");

        // Receive an available builder
        auto builder = queue_messenger.receive_builder();

        // Connect to the builder builder
        tcp::socket builder_socket(io_service);
        tcp::resolver builder_resolver(io_service);
        asio::connect(builder_socket, builder_resolver.resolve({builder.host, builder.port}));

        Messenger builder_messenger(builder_socket);

        // Create a fake definition for testing
        create_def();
        // Send the definition file
        builder_messenger.send_file("./recipe.def");

        // Read the build output until a zero length message is sent
        std::string line;
        do {
            line = builder_messenger.receive();
            std::cout << line;
        } while (!line.empty());

        // Read the container image
        builder_messenger.receive_file("./container.img");

        std::cout << "Container built!\n";

        // Inform the queue we're done
        queue_messenger.send("checkout_builder_complete");
    }
    catch (std::exception &e) {
        std::cerr << "\033[1;31m Failed to build container: " << e.what() << "\033[0m\n";
    }

    return 0;
}