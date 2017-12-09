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

    try {
        asio::io_service io_service;

        BuilderQueue builder_queue(io_service);

        // Wait for connections from either Clients or Builders
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        tcp::acceptor acceptor(io_service,
                                               tcp::endpoint(tcp::v4(), 8080));

                        for (;;) {
                            try {
                                boost::system::error_code ec;
                                tcp::socket socket(io_service);
                                acceptor.async_accept(socket, yield[ec]);
                                std::make_shared<Connection>(std::move(socket), builder_queue)->begin();
                            } catch (std::exception &e) {
                                logger::write(std::string() + "New connection error: " + e.what());
                            }
                        }
                    });

        // Start the queue which ticks at the specified interval
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        for (;;) {
                            try {
                                builder_queue.tick(yield);
                            } catch (std::exception &e) {
                                logger::write(std::string() + "Queue tick error: " + e.what());
                            }
                        }
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (std::exception &e) {
        logger::write(std::string() + "Server Exception: " + e.what());
    }

    logger::write("Server shutting down");

    return 0;
}