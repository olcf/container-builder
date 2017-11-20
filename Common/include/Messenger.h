#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/filesystem.hpp>

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

    // Send entire contents of streambuf
    void async_send(asio::streambuf& message_body, asio::yield_context yield);

    // Send a file in multiple chunk_size, or smaller, messages
    void send_file(boost::filesystem::path, std::size_t chunk_size=1024);
    void async_send_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size=1024);


private:
    void send_header(std::size_t message_size);
    void send_header(std::size_t message_size, asio::yield_context yield);
    std::size_t receive_header();
    std::size_t receive_header(asio::yield_context yield);

    tcp::socket& socket;

};