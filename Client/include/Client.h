#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Messenger.h"
#include "ClientData.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Client {
public:
    explicit Client(int argc, char **argv) {
        parse_environment();
        parse_arguments(argc, argv);

        // Hide the cursor and disable buffering for cleaner builder output
        std::cout << "\e[?25l" << std::flush;
        std::cout.setf(std::ios::unitbuf);

        // Full client connection - this will not run until the io_service is started
        asio::spawn(io_service,
                    [this](asio::yield_context yield) {

                        auto queue_messenger = connect_to_queue(yield);
                        if (queue_messenger.error) {
                            throw std::runtime_error("Error connecting to queue_messenger: " + queue_messenger.error.message());
                        }

                        auto builder_messenger = connect_to_builder(queue_messenger, yield);
                        if (builder_messenger.error) {
                            throw std::runtime_error("Error connecting to builder: " + builder_messenger.error.message());
                        }

                        // Send client data to builder
                        builder_messenger.async_send(client_data());
                        if (builder_messenger.error) {
                            throw std::runtime_error("Error sending client data to builder!");
                        }

                        logger::write("Sending definition: " + definition_path);
                        builder_messenger.async_send_file(definition_path, true);
                        if (builder_messenger.error) {
                            throw std::runtime_error("Error sending definition file to builder: " + builder_messenger.error.message());
                        }

                        std::string line;
                        do {
                            line = builder_messenger.async_receive();
                            if (builder_messenger.error) {
                                throw std::runtime_error("Error streaming build output!");
                            }
                            std::cout << line;
                        } while (!line.empty());

                        builder_messenger.async_receive_file(container_path, true);
                        if (builder_messenger.error) {
                            throw std::runtime_error("Error downloading container image: " + builder_messenger.error.message());
                        }

                        logger::write("Container received: " + container_path, logger::severity_level::success);

                        // Inform the queue we're done
                        queue_messenger.async_send(std::string("checkout_builder_complete"));
                        if (queue_messenger.error) {
                            throw std::runtime_error("Error ending build!");
                        }
                    });
    }

    ~Client() {
        // Restore the cursor
        std::cout << "\e[?25h" << std::flush;
    }

    Messenger connect_to_queue(asio::yield_context yield);

    Messenger connect_to_builder(Messenger &queue_messenger, asio::yield_context yield);

    ClientData client_data();

    // Start the IO service
    void run();
private:
    asio::io_service io_service;
    std::string definition_path;
    std::string container_path;
    std::string user_id;
    std::string queue_host;
    std::string queue_port;
    bool tty;
    Architecture arch;

    void parse_environment();
    void parse_arguments(int argc, char **argv);
};