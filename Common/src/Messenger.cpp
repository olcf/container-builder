#include "Messenger.h"

// Send the header, which is the size and type of the message that will immediately follow
void Messenger::send_header(std::size_t message_size, MessageType type) {
    Header header = {message_size, type};
    auto header_buffer = asio::buffer(&header, header_size());
    asio::write(socket, header_buffer, asio::transfer_exactly(header_size()));
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
Header Messenger::receive_header() {
    Header header;
    auto header_buffer = asio::buffer(&header, header_size());
    asio::read(socket, header_buffer, asio::transfer_exactly(header_size()));
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

Builder Messenger::receive_builder() {
    // Read in the serialized builder as a string
    auto serialized_builder = receive(MessageType::builder);

    // de-serialize Builder
    Builder builder;
    std::istringstream archive_stream(serialized_builder);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> builder;

    return builder;
}

void Messenger::send(Builder builder) {
    // Serialize the builder into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << builder;
    auto serialized_builder = archive_stream.str();

    send(serialized_builder, MessageType::builder);
}