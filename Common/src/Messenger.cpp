#include <functional>
#include <boost/asio/ip/tcp.hpp>
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

// Receive a string message asynchronously of the specified type
std::string Messenger::async_receive(MessageType type) {

    auto header = async_receive_header();
    if (error) {
        logger::write("Header receive error: " + error.message());
        return std::string();
    }

    if (header.type != type) {
        logger::write("Received bad message header");
        return std::string();
    }

    // Read the message body
    asio::streambuf buffer;
    asio::async_read(socket, buffer, asio::transfer_exactly(header.size), yield[error]);
    if (header.type != type) {
        logger::write("Received bad message: " + error.message());
        return std::string();
    }

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                     std::istreambuf_iterator<char>());

    return body;
}

std::string Messenger::async_receive(MessageType *type) {
    auto header = async_receive_header();
    if (error) {
        logger::write("Received bad header" + error.message());
        *type = MessageType::error;
        return std::string();
    }
    *type = header.type;

    // Read the message body
    asio::streambuf buffer;
    asio::async_read(socket, buffer, asio::transfer_exactly(header.size), yield[error]);
    if (error) {
        logger::write("Received bad message : " + error.message());
        *type = MessageType::error;
        return std::string();
    }

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                     std::istreambuf_iterator<char>());

    return body;
}

void Messenger::async_receive_file(boost::filesystem::path file_path, const bool print_progress) {
    std::ofstream file;

    // Get the socket receive buffer size and use that as the chunk size
    boost::asio::socket_base::receive_buffer_size option;
    socket.get_option(option);
    std::size_t chunk_size = option.value();

    // Openfile for writing
    file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
    if (!file) {
        logger::write("Error opening file: " + file_path.string() + " " + std::strerror(errno));
        return;
    }

    auto header = async_receive_header();
    if (error) {
        logger::write("Received bad header: " + error.message());
        return;
    }

    if (header.type != MessageType::file) {
        logger::write("Recieved message header not of file type");
        return;
    }
    auto total_size = header.size;

    // If requested initialize a progress bar
    std::shared_ptr <boost::progress_display> progress_bar;
    if (print_progress) {
        progress_bar = std::make_shared<boost::progress_display>(total_size / chunk_size);
    }

    // Receive file in chunk_size messages
    auto bytes_remaining = total_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);
    boost::crc_32_type csc_result;

    do {
        auto bytes_to_receive = std::min(bytes_remaining, chunk_size);

        auto bytes_received = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_receive), yield[error]);
        if (error) {
            logger::write("Invalid file chunk read: " + error.message());
            return;
        }

        file.write(buffer_storage.data(), bytes_received);

        csc_result.process_bytes(buffer_storage.data(), bytes_received);

        bytes_remaining -= bytes_received;

        if (print_progress) {
            ++(*progress_bar);
        }

    } while (bytes_remaining);

    file.close();
    if (!file) {
        logger::write("Error closing file: " + file_path.string() + " " + std::strerror(errno));
    }

    // After we've read the file we read the checksum and print a message if they do not match
    auto checksum = csc_result.checksum();
    auto checksum_size = sizeof(boost::crc_32_type::value_type);

    boost::crc_32_type::value_type sent_checksum;
    asio::async_read(socket, asio::buffer(&sent_checksum, checksum_size), asio::transfer_exactly(checksum_size),
                     yield[error]);
    if (error) {
        logger::write("Invalid file chunk read: " + error.message());
        return;
    }

    if (checksum != sent_checksum) {
        logger::write("File checksums do not match, file corrupted!");
        return;
    }
}

// Send a string message asynchronously
void Messenger::async_send(const std::string &message, MessageType type) {
    auto message_size = message.size();

    async_send_header(message_size, type);
    if (error) {
        logger::write("Bad header send: " + error.message());
        return;
    }

    // Write the message body
    auto body = asio::buffer(message.data(), message_size);
    asio::async_write(socket, body, asio::transfer_exactly(message_size), yield[error]);
    if (error) {
        logger::write("Bad message send: " + error.message());
        return;
    }
}

// Send a streambuf message asynchronously
void Messenger::async_send(asio::streambuf &message_body) {
    auto message_size = message_body.size();

    async_send_header(message_size, MessageType::string);
    if (error) {
        logger::write("Bad header send: " + error.message());
        return;
    }

    // Write the message body
    asio::async_write(socket, message_body, asio::transfer_exactly(message_size), yield[error]);
    if (error) {
        logger::write("Bad message send: " + error.message());
        return;
    }
}

// Send a file as a message asynchronously
void Messenger::async_send_file(boost::filesystem::path file_path, const bool print_progress) {
    std::ifstream file;

    // Get the socket receive buffer size and use that as the chunk size
    boost::asio::socket_base::send_buffer_size option;
    socket.get_option(option);
    std::size_t chunk_size = option.value();

    // Open file and get size
    file.open(file_path.string(), std::fstream::in | std::fstream::binary);
    if (!file) {
        logger::write("Error opening file: " + file_path.string() + " " + std::strerror(errno));
    }
    auto file_size = boost::filesystem::file_size(file_path);

    async_send_header(file_size, MessageType::file);
    if (error) {
        logger::write("Bad header send: " + error.message());
        return;
    }

    // If requested initialize a progress bar
    std::shared_ptr <boost::progress_display> progress_bar;
    if (print_progress) {
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

        auto bytes_sent = asio::async_write(socket, buffer, asio::transfer_exactly(bytes_to_send), yield[error]);
        if (error) {
            logger::write("Bad file chunk send: " + error.message());
            return;
        }

        bytes_remaining -= bytes_sent;

        if (print_progress) {
            ++(*progress_bar);
        }
    } while (bytes_remaining);

    file.close();
    if (!file) {
        logger::write("Error closing file: " + file_path.string() + " " + std::strerror(errno));
    }

    // After we've sent the file we send the checksum
    // This would make more logical sense in the header but would require us to traverse a potentialy large file twice
    auto checksum = csc_result.checksum();
    auto checksum_size = sizeof(boost::crc_32_type::value_type);
    asio::async_write(socket, asio::buffer(&checksum, checksum_size), asio::transfer_exactly(checksum_size),
                      yield[error]);
    if (error) {
        logger::write("Bad file checksum send: " + error.message());
        return;
    }
}

BuilderData Messenger::async_receive_builder() {
    // Read in the serialized builder as a string
    auto serialized_builder = async_receive(MessageType::builder);
    if (error) {
        logger::write("Received bad builder: " + error.message());
        BuilderData builder;
        return builder;
    }

    // de-serialize BuilderData
    BuilderData builder;
    std::istringstream archive_stream(serialized_builder);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> builder;

    return builder;
}

void Messenger::async_send(BuilderData builder) {
    // Serialize the builder into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << builder;
    auto serialized_builder = archive_stream.str();

    async_send(serialized_builder, MessageType::builder);
    if (error) {
        logger::write("Error sending builder: " + error.message());
    }
}

// Send the header, which is the size and type of the message that will immediately follow, asynchronously
void  Messenger::async_send_header(std::size_t message_size, MessageType type) {
    Header header = {message_size, type};
    auto header_buffer = asio::buffer(&header, header_size());

    asio::async_write(socket, header_buffer, asio::transfer_exactly(header_size()), yield[error]);
    if (error) {
        logger::write("Error sending header: " + error.message());
    }
}

// Receive the message header
Header  Messenger::async_receive_header() {
    Header header;
    auto header_buffer = asio::buffer(&header, header_size());
    asio::async_read(socket, header_buffer, asio::transfer_exactly(header_size()), yield[error]);
    if (error) {
        logger::write("Error receiving header: " + error.message());
        header.type = MessageType::error;
        header.size = 0;
    }
    return header;
}

ClientData Messenger::async_receive_client_data() {
    // Read in the serialized client data as a string
    auto serialized_client_data = async_receive(MessageType::client_data);
    if (error) {
        logger::write("Received bad client data: " + error.message());
        ClientData client_data;
        return client_data;
    }

    // de-serialize BuilderData
    std::istringstream archive_stream(serialized_client_data);
    boost::archive::text_iarchive archive(archive_stream);
    ClientData client_data;
    archive >> client_data;

    return client_data;
}

void Messenger::async_send(ClientData client_data) {
    // Serialize the client data into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << client_data;
    auto serialized_client_data = archive_stream.str();

    async_send(serialized_client_data, MessageType::client_data);
    if (error) {
        logger::write("Error sending client data: " + error.message());
    }
}