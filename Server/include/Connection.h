#pragma once

#include "ResourceQueue.h"
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket, ResourceQueue &queue) : socket(std::move(socket)),
                                                                    queue(queue) {
        logger::write(this->socket, "Established connection");
    }

    ~Connection() {
        logger::write(socket, "Ending connection");
    }

    void begin();

private:
    tcp::socket socket;
    ResourceQueue &queue;

    // Handle a request to build a new container
    void handle_build_request(asio::yield_context yield);

    // Send queue diagnostic information
    void handle_diagnostic_request(asio::yield_context yield);
};