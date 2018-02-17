#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include "BuilderQueue.h"
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket, BuilderQueue &queue) : stream(std::move(socket)),
                                                                   queue(queue) {
        // Use keep alive, which hopefully detect badly disconnected clients
        boost::asio::socket_base::keep_alive keep_alive(true);
        boost::system::error_code ec;
        socket.set_option(keep_alive, ec);
        if(ec) {
            Logger::error("Error setting keep alive on socket");
        }
    };

    ~Connection() {
        if (builder) {
            queue.return_builder(builder.get());
        }
        Logger::info("Connection ending");
    };

    void start();

private:
    websocket::stream<tcp::socket> stream;
    BuilderQueue &queue;
    beast::flat_buffer buffer;
    boost::optional<BuilderData> builder;

    void read_request_string();

    void request_builder();

    void request_queue_stats();

    void builder_ready(BuilderData builder);

    void wait_for_close();
};