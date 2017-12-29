#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <iostream>
#include "Builder.h"
#include "Reservation.h"
#include "BuilderQueue.h"
#include "Connection.h"

namespace asio = boost::asio;
using asio::ip::tcp;

int main(int argc, char *argv[]) {
    asio::io_context io_context;

    BuilderQueue builder_queue(io_context);

    // Wait for connections from either Clients or Builders
    asio::spawn(io_context,
                [&](asio::yield_context yield) {
                    std::error_code error;
                    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));
                    for (;;) {
                        try {
                            // Wait for new connections
                            Messenger messenger(acceptor, yield, error);
                            if (error) {
                                logger::write("Error accepting new connection");
                            } else {
                                // Create a new connection and give it our messenger
                                std::make_shared<Connection>(builder_queue, std::move(messenger))->start(io_context);
                            }
                        } catch (...) {
                            logger::write("Unknown connection exception", logger::severity_level::error);
                        }
                    }
                });

    // Start the queue which ticks at the specified interval
    asio::spawn(io_context,
                [&](asio::yield_context yield) {
                    for (;;) {
                        try {
                            builder_queue.tick(yield);
                        } catch (...) {
                            logger::write("Unknown queue tick exception", logger::severity_level::error);
                        }
                    }
                });

    // Keep running even in the event of an exception
    for (;;) {
        try {
            io_context.run();
        } catch (...) {
            logger::write("Unknown io_service exception during run", logger::severity_level::error);
        }
    }

    logger::write("Server shutting down");
    return 0;
}