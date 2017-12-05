#include "Messenger.h"
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

// Send the header, which is the size and type of the message that will immediately follow
void Messenger::send_header(std::size_t message_size, MessageType type) {
    Header header = {message_size, type};
    auto header_buffer = asio::buffer(&header, header_size());
    asio::write(socket, header_buffer, asio::transfer_exactly(header_size()));
}

// Send the header, which is the size and type of the message that will immediately follow, asynchronously
void Messenger::async_send_header(std::size_t message_size, MessageType type, asio::yield_context yield) {
    Header header = {message_size, type};
    auto header_buffer = asio::buffer(&header, header_size());
    asio::async_write(socket, header_buffer, asio::transfer_exactly(header_size()), yield);
}

// Receive the message header
Header Messenger::receive_header(const MessageType &type) {
    Header header;
    auto header_buffer = asio::buffer(&header, header_size());
    asio::read(socket, header_buffer, asio::transfer_exactly(header_size()));
    if (header.type == MessageType::error) {
        throw std::system_error(EBADMSG, std::generic_category(), "received message error!");
    } else if (type != header.type) {
        throw std::system_error(EBADMSG, std::generic_category(), "received bad message type");
    }
    return header;
}

// Receive the message header
Header Messenger::async_receive_header(const MessageType &type, asio::yield_context yield) {
    Header header;
    auto header_buffer = asio::buffer(&header, header_size());
    asio::async_read(socket, header_buffer, asio::transfer_exactly(header_size()), yield);
    if (header.type == MessageType::error) {
        throw std::system_error(EBADMSG, std::generic_category(), "received message error!");
    } else if (type != header.type) {
        throw std::system_error(EBADMSG, std::generic_category(), "received bad message type");
    }
    return header;
}

// Send a string message
void Messenger::send(const std::string &message, MessageType type) {
    auto message_size = message.size();

    send_header(message_size, MessageType::string);

    // Write the message body
    auto body = asio::buffer(message.data(), message_size);
    asio::write(socket, body, asio::transfer_exactly(message_size));
}

// Send a string message asynchronously
void Messenger::async_send(const std::string &message, asio::yield_context yield, MessageType type) {
    auto message_size = message.size();

    async_send_header(message_size, MessageType::string, yield);

    // Write the message body
    auto body = asio::buffer(message.data(), message_size);
    asio::async_write(socket, body, asio::transfer_exactly(message_size), yield);
}

// Send a streambuf message asynchronously
void Messenger::async_send(asio::streambuf &message_body, asio::yield_context yield) {
    auto message_size = message_body.size();

    async_send_header(message_size, MessageType::string, yield);

    // Write the message body
    asio::async_write(socket, message_body, asio::transfer_exactly(message_size), yield);
}

// Send a streambuf message
void Messenger::send(asio::streambuf &message_body) {
    auto message_size = message_body.size();

    send_header(message_size, MessageType::string);

    // Write the message body
    asio::write(socket, message_body, asio::transfer_exactly(message_size));
}


// Send a file as a message
void Messenger::send_file(boost::filesystem::path file_path, std::size_t chunk_size) {
    std::ifstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Open file and get size
    file.open(file_path.string(), std::fstream::in | std::fstream::binary);
    auto file_size = boost::filesystem::file_size(file_path);

    // Send message header
    send_header(file_size, MessageType::file);

    // Send file as message body, in chunk_size sections
    auto bytes_remaining = file_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);
    do {
        auto bytes_to_send = std::min(bytes_remaining, chunk_size);

        file.read(buffer_storage.data(), bytes_to_send);

        auto bytes_sent = asio::write(socket, buffer, asio::transfer_exactly(bytes_to_send));

        bytes_remaining -= bytes_sent;
    } while (bytes_remaining);

    file.close();
}

// Send a file as a message asynchronously
void Messenger::async_send_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size) {
    std::ifstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Open file and get size
    file.open(file_path.string(), std::fstream::in | std::fstream::binary);
    auto file_size = boost::filesystem::file_size(file_path);

    async_send_header(file_size, MessageType::file, yield);

    // Send file in chunk_size bits
    auto bytes_remaining = file_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);
    do {
        auto bytes_to_send = std::min(bytes_remaining, chunk_size);

        file.read(buffer_storage.data(), bytes_to_send);

        auto bytes_sent = asio::async_write(socket, buffer, asio::transfer_exactly(bytes_to_send), yield);

        bytes_remaining -= bytes_sent;
    } while (bytes_remaining);

    file.close();
}

// Receive a string message
std::string Messenger::receive(MessageType type) {
    auto header = receive_header(MessageType::string);

    // Read the message body
    asio::streambuf buffer;
    asio::read(socket, buffer, asio::transfer_exactly(header.size));

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                     std::istreambuf_iterator<char>());

    return body;
}

// Receive a string message asynchronously
std::string Messenger::async_receive(asio::yield_context yield, MessageType type) {
    auto header = async_receive_header(MessageType::string, yield);

    // Read the message body
    asio::streambuf buffer;
    asio::async_read(socket, buffer, asio::transfer_exactly(header.size), yield);

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                     std::istreambuf_iterator<char>());

    return body;
}

// Receive a string message with a timeout
std::string Messenger::receive(int timeout, int max_retries, MessageType type) {
    int retries = 0;

    // Create buffers to be used for header and body
    Header header;
    auto header_buffer = asio::buffer(&header, header_size());
    asio::streambuf message_buffer;
    std::string message;

    asio::deadline_timer header_watchdog(socket.get_io_service());
    asio::deadline_timer body_watchdog(socket.get_io_service());

    // Callbacks for header, body, and timeouts
    timer_callback_t header_timed_out = [&](const boost::system::error_code& ec) {
        if(ec == boost::asio::error::operation_aborted) {
            return;
        }
        else {
           std::cout<<"header watchdog!\n";
        }
    };
    timer_callback_t body_timed_out = [&](const boost::system::error_code& ec) {
        if(ec == boost::asio::error::operation_aborted) {
            return;
        }
        else {
           std::cout<<"body watchdog!\n";
        }
    };
    receive_callback_t received_body = [&](const boost::system::error_code& ec, std::size_t size) {
        if(ec || size != header.size) {
            throw std::system_error(EBADMSG, std::generic_category(), "received message error!");
        }
        else {
            body_watchdog.cancel();
            // Construct a string from the message body
            message = std::string((std::istreambuf_iterator<char>(&message_buffer)),
                                  std::istreambuf_iterator<char>());
        }

    };

    receive_callback_t received_header = [&](const boost::system::error_code& ec, std::size_t size) {
        if (ec || header.type == MessageType::error) {
            throw std::system_error(EBADMSG, std::generic_category(), "received message error!");
        }
        else if (type != header.type) {
            throw std::system_error(EBADMSG, std::generic_category(), "received bad message type");
        }
        else {
            header_watchdog.cancel();
            body_watchdog.async_wait(body_timed_out);
            asio::async_read(socket, message_buffer, asio::transfer_exactly(header.size), received_body);
        }
    };

    // Initiated header read
    header_watchdog.expires_from_now(boost::posix_time::seconds(timeout));
    header_watchdog.async_wait(header_timed_out);
    asio::async_read(socket, header_buffer, asio::transfer_exactly(header_size()), received_header);

    // Wait for all async operations to finish
    socket.get_io_service().run();

    return message;
}

void Messenger::receive_file(boost::filesystem::path file_path, std::size_t chunk_size) {
    std::ofstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Openfile for writing
    file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);

    auto header = receive_header(MessageType::file);
    auto total_size = header.size;

    // Receive file in chunk_size messages
    auto bytes_remaining = total_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);

    do {
        auto bytes_to_receive = std::min(bytes_remaining, chunk_size);

        auto bytes_received = asio::read(socket, buffer, asio::transfer_exactly(bytes_to_receive));

        file.write(buffer_storage.data(), bytes_received);

        bytes_remaining -= bytes_received;

    } while (bytes_remaining);

    file.close();
}

void
Messenger::async_receive_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size) {
    std::ofstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Openfile for writing
    file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);

    auto header = async_receive_header(MessageType::file, yield);
    auto total_size = header.size;

    // Receive file in chunk_size messages
    auto bytes_remaining = total_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);

    do {
        auto bytes_to_receive = std::min(bytes_remaining, chunk_size);

        auto bytes_received = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_receive), yield);

        file.write(buffer_storage.data(), bytes_received);

        bytes_remaining -= bytes_received;

    } while (bytes_remaining);

    file.close();
}

Builder Messenger::receive_builder() {
    Messenger messenger(socket);

    // Read in the serialized builder as a string
    auto serialized_builder = messenger.receive(MessageType::builder);

    // de-serialize Builder
    Builder builder;
    std::istringstream archive_stream(serialized_builder);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> builder;

    return builder;
}

Builder Messenger::async_receive_builder(asio::yield_context yield) {
    Messenger messenger(socket);

    // Read in the serialized builder as a string
    auto serialized_builder = messenger.async_receive(yield, MessageType::builder);

    // de-serialize Builder
    Builder builder;
    std::istringstream archive_stream(serialized_builder);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> builder;

    return builder;
}

void Messenger::async_send(Builder builder, asio::yield_context yield) {
    Messenger messenger(socket);

    // Serialize the builder into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << builder;
    auto serialized_builder = archive_stream.str();

    messenger.async_send(serialized_builder, yield, MessageType::builder);
}

void Messenger::send(Builder builder) {
    Messenger messenger(socket);

    // Serialize the builder into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << builder;
    auto serialized_builder = archive_stream.str();

    messenger.send(serialized_builder, MessageType::builder);
}