#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Logger.h"
#include "Messenger.h"
#include <memory>

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;

std::string queue_host() {
    auto env = std::getenv("QUEUE_HOST");
    if(!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_HOST");
    }

    return std::string(env);
}

std::string queue_port() {
    auto env = std::getenv("QUEUE_PORT");
    if(!env) {
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
        asio::connect(queue_socket, queue_resolver.resolve({queue_host(), queue_port()}));

        std::cout << "Connected to BuilderQueue: " << queue_host() << ":" << queue_port() << std::endl;

        Messenger queue_messenger(queue_socket);

        // Initiate a build request
        queue_messenger.send("checkout_builder_request");

        // Receive an available builder
        auto builder = queue_messenger.receive_builder();

        std::cout << "Received build host: " << builder.host << ":" << builder.port << std::endl;

        // Block until the initial connection to the builder is made
        std::shared_ptr<tcp::socket> builder_socket = std::make_shared<tcp::socket>(io_service);
        tcp::resolver builder_resolver(io_service);
        boost::system::error_code ec;
        do {
            asio::connect(*builder_socket, builder_resolver.resolve({builder.host, builder.port}), ec);
        } while (ec != boost::system::errc::success);

        std::cout << "Connected to builder host: " << builder.host << ":" << builder.port << std::endl;

        std::shared_ptr<Messenger> builder_messenger = std::make_shared<Messenger>(*builder_socket);

        // Once we're connected start the client process in the coroutine
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        // Send the definition file
                        builder_messenger->async_send_file(definition_path, yield);

                        // Start listening for the heartbeat which is set by the builder every 10 seconds
                        // The heartbeat is guarded by a watchdog that if triggered will cause the connection to be restarted
                        asio::deadline_timer heartbeat(io_service);
                        asio::deadline_timer watchdog(io_service);
                        const auto timeout = boost::posix_time::seconds(15);

                        // When a hang is detected we destroy the socket and attempt to create a new one
                        // This is handled outside of the heartbeat coroutine as if it fires that coroutine will be jammed up
                        timer_callback_t heartbeat_hung = [&](const boost::system::error_code& ec) {
                            if (ec == boost::asio::error::operation_aborted) {
                                return;
                            }
                            // Destroy the current socket
                            builder_socket->cancel();
                            builder_socket->close();

                            // Try to resolve a new connection to the builder and reset the socket and messenger
                            builder_socket = std::make_shared<tcp::socket>(io_service);
                            tcp::resolver builder_resolver(io_service);
                            boost::system::error_code e;
                            do {
                                asio::connect(*builder_socket, builder_resolver.resolve({builder.host, builder.port}), e);
                            } while (e != boost::system::errc::success);
                            builder_messenger = std::make_shared<Messenger>(*builder_socket);
                        };

                        // Read the build output until a zero length message is sent
                        std::string line;
                        do {
                            // arm/disarm watchdog
                            watchdog.expires_from_now(timeout);
                            watchdog.async_wait(heartbeat_hung);

                            line = builder_messenger->async_receive_ignore_heartbeat(yield);
                            std::cout << line;
                        } while (!line.empty());
                        watchdog.cancel();

                        // Read the container image
                        builder_messenger->async_receive_file(container_path, yield);

                        // Inform the queue we're done
                        queue_messenger.async_send("checkout_builder_complete", yield);
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (std::exception &e) {
        logger::write(std::string() + "Build server exception: " + e.what());
    }

    logger::write("Server shutting down");

    return 0;
}