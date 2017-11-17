#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <iostream>
#include "Resource.h"
#include "Reservation.h"
#include "ResourceQueue.h"
#include "Connection.h"
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

int main(int argc, char *argv[]) {

    // Enable logging
    logger::init("ContainerBuilder.log");

    try {
        asio::io_service io_service;

        // Add a few resources
        ResourceQueue job_queue;
        Resource resource;
        resource.loop_device = "/dev/loop0";
        resource.host = std::string("localhost");
        resource.num_cores = 1;
        job_queue.add_resource(resource);
        resource.loop_device = "/dev/loop1";
        job_queue.add_resource(resource);

        // Wait for connections
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        tcp::acceptor acceptor(io_service,
                                               tcp::endpoint(tcp::v4(), 8080));

                        for (;;) {
                            boost::system::error_code ec;
                            tcp::socket socket(io_service);
                            acceptor.async_accept(socket, yield[ec]);
                            if (!ec) {
                                std::make_shared<Connection>(std::move(socket), job_queue)->begin();
                            } else {
                                logger::write("New connection error: " + ec.message());
                            }
                        }
                    });

        io_service.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}