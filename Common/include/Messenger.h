#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/filesystem.hpp>
#include "Builder.h"

namespace asio = boost::asio;
using asio::ip::tcp;

using receive_callback_t = std::function<void(const boost::system::error_code &ec, std::size_t size)>;
using timer_callback_t = std::function<void(const boost::system::error_code &ec)>;

// Enum to handle message type, used to ensure
enum class MessageType : unsigned char {
    string,
    builder,
    file,
    heartbeat,
    error
};

// Message header
struct Header {
    std::size_t size;
    MessageType type;
};

class Messenger {

public:
    explicit Messenger(tcp::socket &socket) : socket(socket) {}

    // Send/receive a heartbeat message
    void async_send_heartbeat(asio::yield_context yield);
    void async_receive_heartbeat(asio::yield_context yield);

    // Send a string as a message
    void send(const std::string &message, MessageType type=MessageType::string);

    void async_send(const std::string &message, asio::yield_context yield, MessageType type=MessageType::string);

    // Receive message as a string
    std::string receive(MessageType type=MessageType::string);

    std::string async_receive(asio::yield_context yield, MessageType type=MessageType::string);

    // Send entire contents of streambuf
    void async_send(asio::streambuf &message_body, asio::yield_context yield);
    void send(asio::streambuf &message_body);

    // Send a file in multiple chunk_size peices
    void send_file(boost::filesystem::path file_path, std::size_t chunk_size = 1024);
    void async_send_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size = 1024);

    // Read a file in multiple chunk_size peices
    void receive_file(boost::filesystem::path file_path, std::size_t chunk_size = 1024);
    void  async_receive_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size = 1024);

    // Send a Builder
    void async_send(Builder builder, asio::yield_context yield);
    void send(Builder builder);

    // Receive a Builder
    Builder receive_builder();
    Builder async_receive_builder(asio::yield_context yield);

    std::string receive(int timeout, int max_retries, MessageType type=MessageType::string);
private:
    tcp::socket &socket;

    void send_header(std::size_t message_size, MessageType type);

    void async_send_header(std::size_t message_size, MessageType type, asio::yield_context yield);

    Header receive_header(const MessageType& type);

    Header async_receive_header(const MessageType& type, asio::yield_context yield);

    static constexpr std::size_t header_size() {
        return sizeof(Header);
    }
};