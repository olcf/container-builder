#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Messenger.h"


namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;

std::string queue_host() {
    auto env = std::getenv("QUEUE_HOST");
    if (!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_HOST not set!");
    }
    return std::string(env);
}

std::string queue_port() {
    auto env = std::getenv("QUEUE_PORT");
    if (!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_PORT not set!");
    }
    return std::string(env);
}
/*
void display_spinner(asio::io_service &io_service, asio::yield_context &yield, bool display) {
    static bool should_display;
    static boost::asio::deadline_timer timer(io_service);

    should_display = display;

    auto expire_time = boost::posix_time::milliseconds(250);

    asio::spawn(io_service,
                [&](asio::yield_context yield) {
                    while (should_display) {
                        std::cout << "\b--" << std::flush;
                        timer.expires_from_now(expire_time);
                        timer.async_wait(yield);
                        std::cout << "\b\\" << std::flush;
                        timer.expires_from_now(expire_time);
                        timer.async_wait(yield);
                        std::cout << "\b|" << std::flush;
                        timer.expires_from_now(expire_time);
                        timer.async_wait(yield);
                        timer.expires_from_now(expire_time);
                        std::cout << "\b/" << std::flush;
                        timer.async_wait(yield);
                    }
                });
}
*/

int main(int argc, char *argv[]) {

    try {

        // Check for correct number of arguments
        if (argc != 3) {
            std::cerr << "Usage: ContainerBuilder <definition path> <container path>\n";
            return 1;
        }
        std::string definition_path(argv[1]);
        std::string container_path(argv[2]);

        asio::io_service io_service;

        asio::spawn(io_service,
                    [&](asio::yield_context yield) {

                        std::cout << "Attempting to connect to BuilderQueue: " << queue_host() << ":" << queue_port()
                                  << std::endl;

                        tcp::socket queue_socket(io_service);
                        tcp::resolver queue_resolver(io_service);
                        boost::system::error_code ec;

                        asio::async_connect(queue_socket, queue_resolver.resolve({queue_host(), queue_port()}), yield);

                        std::cout << "Connected to BuilderQueue: " << queue_host() << ":" << queue_port() << std::endl;

                        Messenger queue_messenger(queue_socket);

                        // Initiate a build request
                        queue_messenger.async_send("checkout_builder_request", yield);

                        // Wait on a builder from the queue
                        auto builder = queue_messenger.async_receive_builder(yield);

                        std::cout << "Attempting to connect to build host: " << builder.host << ":" << builder.port
                                  << std::endl;

                        tcp::socket builder_socket(io_service);
                        tcp::resolver builder_resolver(io_service);
                        do {
                            asio::async_connect(builder_socket, builder_resolver.resolve({builder.host, builder.port}),
                                                yield[ec]);
                        } while (ec != boost::system::errc::success);

                        std::cout << "Connected to builder host: " << builder.host << ":" << builder.port << std::endl;

                        Messenger builder_messenger(builder_socket);

                        // Once we're connected to the builder start the client process
                        std::cout << "Sending definition: " << definition_path << std::endl;

                        // Send the definition file
                        builder_messenger.async_send_file(definition_path, yield, true);

                        std::cout << "Start of Singularity builder output:" << std::endl;

                        // Hide the cursor and disable buffering for cleaner output
                        std::cout << "\e[?25l" << std::flush;
                        std::cout.setf(std::ios::unitbuf);

                        std::string line;
                        do {
                            line = builder_messenger.async_receive(yield);
                            std::cout << line;
                        } while (!line.empty());

                        // Read the container image
                        std::cout << "Sending finished container: " << container_path << std::endl;

                        builder_messenger.async_receive_file(container_path, yield, true);

                        std::cout << "Container received: " << container_path << std::endl;

                        // Inform the queue we're done
                        queue_messenger.async_send(std::string("checkout_builder_complete"), yield);
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (std::exception &e) {
        std::cout << std::string() + "Build exception: " + e.what() << std::endl;
    }

    std::cout << "Client shutting down\n";

    // Show the cursor
    std::cout << "\e[?25h" << std::flush;


    return 0;
}