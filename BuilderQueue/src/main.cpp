#include <boost/asio/io_service.hpp>
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
    asio::io_service io_service;

    BuilderQueue builder_queue(io_service);

    // Wait for connections from either Clients or Builders
    asio::spawn(io_service,
                [&](asio::yield_context yield) {
                    tcp::acceptor acceptor(io_service,
                                           tcp::endpoint(tcp::v4(), 8080));
                    for (;;) {
                        boost::system::error_code error;
                        tcp::socket socket(io_service);
                        acceptor.async_accept(socket, yield[error]);
                        if (error) {
                            logger::write(socket, "Error accepting new connection");
                        } else {
                            std::make_shared<Connection>(std::move(socket), builder_queue)->begin();
                        }
                    }
                });

    // Start the queue which ticks at the specified interval
    asio::spawn(io_service,
                [&](asio::yield_context yield) {
                    for (;;) {
                        try {
                            builder_queue.tick(yield);
                        } catch (...) {
                            logger::write("Unknown queue tick exception");
                        }
                    }
                });

    // Keep running even in the event of an exception
    for(;;) {
        try {
            io_service.run();
        } catch(...) {
            logger::write("Unknown io_service exception run");
        }
    }

    logger::write("Server shutting down");

    return 0;
}