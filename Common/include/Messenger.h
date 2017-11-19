#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class Messenger {
public:
    explicit Messenger(tcp::socket& socket): socket(socket)
    {}

    // Send a string as a message
    void send(const std::string& message);
    void async_send(const std::string& message, asio::yield_context yield);

    // Receive message as a string
    std::string receive();
    std::string async_receive(asio::yield_context yield);
    
private:
    asio::streambuf buffer;
    tcp::socket& socket;

    /*
    // Write an integer header, of type size_t, containing the number of bytes to follow
    // followed by a write of the message, potentially in multiple chunks
    void send(std::size_t message_size, std::function<void(asio::streambuf&)> fill_buffer);

    // Asynchronously Write an integer header, of type size_t, containing the number of bytes to follow
    // followed by a write of the message, potentially in multiple chunks
    void async_send(std::size_t message_size, asio::yield_context yield,
                              std::function<void(asio::streambuf&)> fill_buffer);

    std::size_t receive(std::size_t chunk_size, std::function<void(asio::streambuf&)> process_read);

    std::size_t async_receive(std::size_t chunk_size, asio::yield_context yield,
                              std::function<void(asio::streambuf&)> process_read);
                              */
};
