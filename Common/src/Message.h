#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class Message {
public:
    // Write an integer header, of type size_t, containing the number of bytes to follow
    // followed by a write of the message, potentially in multiple chunks
    void write(tcp::socket &socket, std::size_t message_size, std::function<void(asio::streambuf&)> fill_buffer);

    // Asynchronously Write an integer header, of type size_t, containing the number of bytes to follow
    // followed by a write of the message, potentially in multiple chunks
    void async_write(tcp::socket &socket, std::size_t message_size, asio::yield_context yield,
                              std::function<void(asio::streambuf&)> fill_buffer);

    std::size_t read(tcp::socket &socket, std::size_t chunk_size, std::function<void(asio::streambuf&)> process_read);

    std::size_t async_read(tcp::socket &socket, std::size_t chunk_size, asio::yield_context yield,
                              std::function<void(asio::streambuf&)> process_read);
private:
    asio::streambuf buffer;
};
