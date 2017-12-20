#pragma once

#include "BuilderQueue.h"
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(Messenger client, BuilderQueue &queue) : client(std::move(client)),
                                                                 queue(queue) {
        logger::write(client.socket, "Established connection");
    }

    ~Connection() {
        logger::write(client.socket, "Ending connection");
    }

    void begin();

private:
    Messenger client;
    BuilderQueue &queue;

    // Checkout a builder
    void checkout_builder(asio::yield_context yield);
};