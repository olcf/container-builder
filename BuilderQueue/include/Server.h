#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "BuilderQueue.h"
#include "Connection.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Server {
public:
    Server(asio::io_context &io_context, BuilderQueue queue) :
            acceptor(io_context, tcp::endpoint(tcp::v4(), 8080)),
            queue(queue) {
        accept_connection();
    };
private:
    tcp::acceptor acceptor;
    BuilderQueue queue;

    void accept_connection() {
        acceptor.async_accept([this](boost::system::error_code error, tcp::socket socket) {
            if (!error) {
                std::make_shared<Connection>(std::move(socket), queue)->start();
            }
            accept_connection();
        });
    };
};