#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "BuilderQueue.h"
#include "Connection.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Server {
public:
    Server(asio::io_context &io_context, BuilderQueue& queue) :
            acceptor(io_context, tcp::endpoint(tcp::v4(), 8080)),
            queue(queue) {
        accept_connection();
    };

    // Don't allow the connection server to be copied or moved
    Server(const Server&)            =delete;
    Server& operator=(const Server&) =delete;
    Server(Server&&) noexcept        =delete;
    Server& operator=(Server&&)      =delete;

private:
    tcp::acceptor acceptor;
    BuilderQueue &queue;

    void accept_connection() {
        acceptor.async_accept([this](boost::system::error_code error, tcp::socket socket) {
            if (!error) {
                Logger::info("New connection created");
                std::make_shared<Connection>(std::move(socket), queue)->start();
            }
            accept_connection();
        });
    };
};