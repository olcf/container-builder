#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/filesystem.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

using header_t = std::size_t;

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

    // Send a file in multiple chunk_size peices
    void send_file(boost::filesystem::path file_path, std::size_t chunk_size=1024);
    void async_send_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size=1024);

    // Read a file in multiple chunk_size peices
    void receive_file(boost::filesystem::path file_path, std::size_t chunk_size=1024);
    void async_receive_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size=1024);

private:
    tcp::socket& socket;

    void send_header(std::size_t message_size);
    void async_send_header(std::size_t message_size, asio::yield_context yield);
    std::size_t receive_header();
    std::size_t async_receive_header(asio::yield_context yield);

    constexpr std::size_t header_size() {
        return sizeof(header_t);
    }
};