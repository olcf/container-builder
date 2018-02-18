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
        enable_keep_alive();
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

    void enable_keep_alive();

    void read_request_string();

    void request_builder();

    void request_queue_stats();

    void request_image_list();

    void builder_ready(BuilderData builder);

    void wait_for_close();
};