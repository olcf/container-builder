#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Messenger.h"
#include <memory>

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;

std::string queue_host() {
    auto env = std::getenv("QUEUE_HOST");
    if (!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_HOST");
    }
    return std::string(env);
}

std::string queue_port() {
    auto env = std::getenv("QUEUE_PORT");
    if (!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_PORT");
    }
    return std::string(env);
}

int main(int argc, char *argv[]) {

    try {

        // Check for correct number of arguments
        if (argc != 3) {
            std::cerr << "Usage: ContainerBuilder <definition path> <container path>\n";
            return 1;
        }
        std::string definition_path(argv[1]);
        std::string container_path(argv[2]);

        std::cout << "Attempting to connect to BuilderQueue: " << queue_host() << ":" << queue_port() << std::endl;

        asio::io_service io_service;

        tcp::socket queue_socket(io_service);
        tcp::resolver queue_resolver(io_service);
        boost::system::error_code ec;
        asio::connect(queue_socket, queue_resolver.resolve({queue_host(), queue_port()}));

        std::cout << "Connected to BuilderQueue: " << queue_host() << ":" << queue_port() << std::endl;

        Messenger queue_messenger(queue_socket);

        // Initiate a build request
        queue_messenger.send("checkout_builder_request");

        // Receive an available builder
        auto builder = queue_messenger.receive_builder();

        std::cout << "Attempting to connect to build host: " << builder.host << ":" << builder.port << std::endl;

        // Block until the initial connection to the builder is made
        tcp::socket builder_socket(io_service);
        tcp::resolver builder_resolver(io_service);
        do {
            asio::connect(builder_socket, builder_resolver.resolve({builder.host, builder.port}), ec);
        } while (ec != boost::system::errc::success);

        std::cout << "Connected to builder host: " << builder.host << ":" << builder.port << std::endl;

        Messenger builder_messenger(builder_socket);

        // Once we're connected to the builder start the client process
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        std::cout << "Sending definition: " << definition_path << std::endl;

                        // Send the definition file
                        builder_messenger.async_send_file(definition_path, yield);

                        std::cout << "Start reading builder output:" << std::endl;
                        std::string line;
                        boost::system::error_code read_ec;

                        do {
                            auto line = builder_messenger.async_receive(yield);
                            std::cout << line;
                        } while (!line.empty());

                        // Read the container image
                        std::cout << "Begin receive of container: " << container_path << std::endl;

                        builder_messenger.async_receive_file(container_path, yield);

                        std::cout << "End receive of container: " << container_path << std::endl;

                        // Inform the queue we're done
                        queue_messenger.async_send(std::string("checkout_builder_complete"), yield);
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (std::exception &e) {
        std::cout << std::string() + "Build server exception: " + e.what() << std::endl;
    }

    std::cout << "Client shutting down\n";

    return 0;
}