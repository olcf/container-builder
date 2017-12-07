#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
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

    // Receive a string message asynchronously of the specified type
    template <typename Handler>
    std::string async_receive(const Handler& handler, MessageType type=MessageType::string) {
        auto header = async_receive_header(type, handler);

        // Read the message body
        asio::streambuf buffer;
        asio::async_read(socket, buffer, asio::transfer_exactly(header.size), handler);

        // Construct a string from the message body
        std::string body((std::istreambuf_iterator<char>(&buffer)),
                         std::istreambuf_iterator<char>());

        return body;
    }

    // TODO set chunk size to the size of the socket receive buffer?
    template <typename Handler>
    void async_receive_file(boost::filesystem::path file_path, const Handler& handler, std::size_t chunk_size=1024) {
        std::ofstream file;

        // Throw exception if we run into any file issues
        file.exceptions(std::fstream::failbit | std::ifstream::badbit);

        // Openfile for writing
        file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);

        auto header = async_receive_header(MessageType::file, handler);
        auto total_size = header.size;

        // Receive file in chunk_size messages
        auto bytes_remaining = total_size;
        std::vector<char> buffer_storage(chunk_size);
        auto buffer = asio::buffer(buffer_storage);

        do {
            auto bytes_to_receive = std::min(bytes_remaining, chunk_size);

            auto bytes_received = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_receive), handler);

            file.write(buffer_storage.data(), bytes_received);

            bytes_remaining -= bytes_received;

        } while (bytes_remaining);

        file.close();
    }

    // Receive a string message asynchronously
    template <typename Handler>
    std::string async_receive(const Handler& handler, MessageType* type) {
        auto header = async_receive_header(handler);
        *type = header.type;

        // Read the message body
        asio::streambuf buffer;
        asio::async_read(socket, buffer, asio::transfer_exactly(header.size), handler);

        // Construct a string from the message body
        std::string body((std::istreambuf_iterator<char>(&buffer)),
                         std::istreambuf_iterator<char>());

        return body;
    }

    // Send a string message asynchronously
    template <typename Handler>
    void async_send(const std::string &message, const Handler& handler, MessageType type=MessageType::string) {
        auto message_size = message.size();

        async_send_header(message_size, type, handler);

        // Write the message body
        auto body = asio::buffer(message.data(), message_size);
        asio::async_write(socket, body, asio::transfer_exactly(message_size), handler);
    }

    // Send a streambuf message asynchronously
    template <typename Handler>
    void async_send(asio::streambuf &message_body, const Handler& handler) {
        auto message_size = message_body.size();

        async_send_header(message_size, MessageType::string, handler);

        // Write the message body
        asio::async_write(socket, message_body, asio::transfer_exactly(message_size), handler);
    }

    // Send a string as a message
    void send(const std::string &message, MessageType type=MessageType::string);


    // Receive message as a string
    std::string receive(MessageType type);

    // Send entire contents of streambuf
    void send(asio::streambuf &message_body);

    // Send a file in multiple chunk_size peices
    void send_file(boost::filesystem::path file_path, std::size_t chunk_size = 1024);
    // Read a file in multiple chunk_size peices
    void receive_file(boost::filesystem::path file_path, std::size_t chunk_size = 1024);

    // TODO set chunk size to the size of the socket receive buffer?
    // Send a file as a message asynchronously
    template <typename Handler>
    void async_send_file(boost::filesystem::path file_path, const Handler& handler, std::size_t chunk_size=1024) {
        std::ifstream file;

        // Throw exception if we run into any file issues
        file.exceptions(std::fstream::failbit | std::ifstream::badbit);

        // Open file and get size
        file.open(file_path.string(), std::fstream::in | std::fstream::binary);
        auto file_size = boost::filesystem::file_size(file_path);

        async_send_header(file_size, MessageType::file, handler);

        // Send file in chunk_size bits
        auto bytes_remaining = file_size;
        std::vector<char> buffer_storage(chunk_size);
        auto buffer = asio::buffer(buffer_storage);
        do {
            auto bytes_to_send = std::min(bytes_remaining, chunk_size);

            file.read(buffer_storage.data(), bytes_to_send);

            auto bytes_sent = asio::async_write(socket, buffer, asio::transfer_exactly(bytes_to_send), handler);

            bytes_remaining -= bytes_sent;
        } while (bytes_remaining);

        file.close();
    }

    template <typename Handler>
    Builder async_receive_builder(const Handler& handler) {
        // Read in the serialized builder as a string
        auto serialized_builder = async_receive(handler, MessageType::builder);

        // de-serialize Builder
        Builder builder;
        std::istringstream archive_stream(serialized_builder);
        boost::archive::text_iarchive archive(archive_stream);
        archive >> builder;

        return builder;
    }

    template <typename Handler>
    void async_send(Builder builder, const Handler& handler) {
        // Serialize the builder into a string
        std::ostringstream archive_stream;
        boost::archive::text_oarchive archive(archive_stream);
        archive << builder;
        auto serialized_builder = archive_stream.str();

        async_send(serialized_builder, handler, MessageType::builder);
    }

    // Transfer builder objects
    void send(Builder builder);
    Builder receive_builder();

private:
    tcp::socket &socket;

    void send_header(std::size_t message_size, const MessageType& type);

    // Send the header, which is the size and type of the message that will immediately follow, asynchronously
    template <typename Handler>
    void async_send_header(std::size_t message_size, MessageType type, const Handler& handler) {
        Header header = {message_size, type};
        auto header_buffer = asio::buffer(&header, header_size());
        asio::async_write(socket, header_buffer, asio::transfer_exactly(header_size()), handler);
    }

    Header receive_header(const MessageType& type);

    // Receive the message header
    template <typename Handler>
    Header async_receive_header(const MessageType &type, const Handler& handler) {
        Header header;
        auto header_buffer = asio::buffer(&header, header_size());
        asio::async_read(socket, header_buffer, asio::transfer_exactly(header_size()), handler);
        if (header.type == MessageType::error) {
            throw std::system_error(EBADMSG, std::generic_category(), "received message error!");
        } else if (type != header.type) {
            throw std::system_error(EBADMSG, std::generic_category(), "received bad message type");
        }
        return header;
    }

    Header receive_header();

    // Receive the message header
    template <typename Handler>
    Header async_receive_header(const Handler& handler) {
        Header header;
        auto header_buffer = asio::buffer(&header, header_size());
        asio::async_read(socket, header_buffer, asio::transfer_exactly(header_size()), handler);
        return header;
    }

    static constexpr std::size_t header_size() {
        return sizeof(Header);
    }
};