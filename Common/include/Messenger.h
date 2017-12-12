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
#include <boost/progress.hpp>
#include <boost/crc.hpp>
#include <memory.h>
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

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
        auto header = async_receive_header(handler);

        if(header.type != type) {
            logger::write("Received bad message header");
            return std::string();
        }

        // Read the message body
        asio::streambuf buffer;
        asio::async_read(socket, buffer, asio::transfer_exactly(header.size), handler);

        // Construct a string from the message body
        std::string body((std::istreambuf_iterator<char>(&buffer)),
                         std::istreambuf_iterator<char>());

        return body;
    }

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

    template <typename Handler>
    void async_receive_file(boost::filesystem::path file_path, const Handler& handler, const bool print_progress=false) {
        std::ofstream file;

        // Get the socket receive buffer size and use that as the chunk size
        boost::asio::socket_base::receive_buffer_size option;
        socket.get_option(option);
        std::size_t chunk_size = option.value();

        // Openfile for writing
        file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
        if(!file) {
            logger::write("Error opening file: " + file_path.string() + " " + std::strerror(errno));
        }

        auto header = async_receive_header(handler);
        if(header.type != MessageType::file) {
            logger::write("Recieved message hader not of file type");
            return;
        }
        auto total_size = header.size;

        // If requested initialize a progress bar
        std::shared_ptr<boost::progress_display> progress_bar;
        if(print_progress) {
            progress_bar = std::make_shared<boost::progress_display>(total_size / chunk_size);
        }

        // Receive file in chunk_size messages
        auto bytes_remaining = total_size;
        std::vector<char> buffer_storage(chunk_size);
        auto buffer = asio::buffer(buffer_storage);
        boost::crc_32_type csc_result;

        do {
            auto bytes_to_receive = std::min(bytes_remaining, chunk_size);

            auto bytes_received = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_receive), handler);

            file.write(buffer_storage.data(), bytes_received);

            csc_result.process_bytes(buffer_storage.data(), bytes_received);

            bytes_remaining -= bytes_received;

            if(print_progress) {
                ++(*progress_bar);
            }

        } while (bytes_remaining);

        // After we've read the file we read the checksum and print a message if they do not match
        auto checksum = csc_result.checksum();
        auto checksum_size = sizeof(boost::crc_32_type::value_type);

        boost::crc_32_type::value_type sent_checksum;
        asio::async_read(socket, asio::buffer(&sent_checksum, checksum_size), asio::transfer_exactly(checksum_size), handler);

        if(checksum != sent_checksum) {
            logger::write("File checksums do not match, file corrupted!");
        }

        file.close();
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

    // Send a file as a message asynchronously
    template <typename Handler>
    void async_send_file(boost::filesystem::path file_path, const Handler& handler, const bool print_progress=false) {
        std::ifstream file;

        // Get the socket receive buffer size and use that as the chunk size
        boost::asio::socket_base::send_buffer_size option;
        socket.get_option(option);
        std::size_t chunk_size = option.value();

        // Open file and get size
        file.open(file_path.string(), std::fstream::in | std::fstream::binary);
        if(!file) {
            logger::write("Error opening file: " + file_path.string() + " " + std::strerror(errno));
        }
        auto file_size = boost::filesystem::file_size(file_path);

        async_send_header(file_size, MessageType::file, handler);

        // If requested initialize a progress bar
        std::shared_ptr<boost::progress_display> progress_bar;
        if(print_progress) {
            progress_bar = std::make_shared<boost::progress_display>(file_size / chunk_size);
        }

        // Send file in chunk_size bits
        auto bytes_remaining = file_size;
        std::vector<char> buffer_storage(chunk_size);
        auto buffer = asio::buffer(buffer_storage);
        boost::crc_32_type csc_result;
        do {
            auto bytes_to_send = std::min(bytes_remaining, chunk_size);

            file.read(buffer_storage.data(), bytes_to_send);

            csc_result.process_bytes(buffer_storage.data(), bytes_to_send);

            auto bytes_sent = asio::async_write(socket, buffer, asio::transfer_exactly(bytes_to_send), handler);

            bytes_remaining -= bytes_sent;

            if(print_progress) {
                ++(*progress_bar);
            }
        } while (bytes_remaining);

        file.close();

        // After we've sent the file we send the checksum
        // This would make more logical sense in the header but would require us to traverse a potentialy large file twice
        auto checksum = csc_result.checksum();
        auto checksum_size = sizeof(boost::crc_32_type::value_type);
        asio::async_write(socket, asio::buffer(&checksum, checksum_size), asio::transfer_exactly(checksum_size), handler);
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

private:
    tcp::socket &socket;

    // Send the header, which is the size and type of the message that will immediately follow, asynchronously
    template <typename Handler>
    void async_send_header(std::size_t message_size, MessageType type, const Handler& handler) {
        Header header = {message_size, type};
        auto header_buffer = asio::buffer(&header, header_size());
        asio::async_write(socket, header_buffer, asio::transfer_exactly(header_size()), handler);
    }

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