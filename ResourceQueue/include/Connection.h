#pragma once

#include "BuilderQueue.h"
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket, BuilderQueue &queue) : socket(std::move(socket)),
                                                                                              queue(queue) {
        logger::write(this->socket, "Established connection");
    }

    ~Connection() {
        logger::write(socket, "Ending connection");
    }

    void begin();

private:
    tcp::socket socket;
    BuilderQueue &queue;

    // Checkout a builder
    void checkout_builder(asio::yield_context yield);

    // Make a new builder available
    void checkin_builder(asio::yield_context yield);
};