#include "Messenger.h"
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <iostream>

// TODO: move this into .cpp as it's not templated!

// Send a string message
void Messenger::send(const std::string& message) {
    std::size_t message_size = message.size();

    // Write message size to header
    auto header = asio::buffer(&message_size, sizeof(std::size_t));
    asio::write(socket, header, asio::transfer_exactly(message_size));

    // Write the message body
    auto body = asio::buffer(message.data(), message_size);
    asio::write(socket, body, asio::transfer_exactly(message_size));
}

// Send a string message asynchronously
void Messenger::async_send(const std::string& message, asio::yield_context yield) {
    std::size_t message_size = message.size();

    // Write message size to header
    auto header = asio::buffer(&message_size, sizeof(std::size_t));
    asio::async_write(socket, header, asio::transfer_exactly(message_size), yield);

    // Write the message body
    auto body = asio::buffer(message.data(), message_size);
    asio::async_write(socket, body, asio::transfer_exactly(message_size), yield);
}

// Receive a string message
std::string Messenger::receive() {
    std::size_t message_size;

    // Read message size from header
    auto header = asio::buffer(&message_size, sizeof(std::size_t));
    asio::read(socket, header, asio::transfer_exactly(message_size));

    // Read the message body
    asio::read(socket, buffer, asio::transfer_exactly(message_size));

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                      std::istreambuf_iterator<char>());

    return body;
}

// Receive a string message asynchronously
std::string Messenger::async_receive(asio::yield_context yield) {
    std::size_t message_size;

    // Read message size from header
    auto header = asio::buffer(&message_size, sizeof(std::size_t));
    asio::async_read(socket, header, asio::transfer_exactly(message_size), yield);

    // Read the message body
    asio::async_read(socket, buffer, asio::transfer_exactly(message_size), yield);

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                      std::istreambuf_iterator<char>());

    return body;
}

/*
void Messenger::send(std::size_t message_size, std::function<void(asio::streambuf&)> fill_buffer) {
    // Write total message size header
    asio::write(socket, asio::buffer(&message_size, sizeof(std::size_t)));

    // Call fill_buffer callback and write chunk until the total message has been sent
    std::size_t bytes_remaining = message_size;
    while (bytes_remaining) {
        // Get chunk of message from caller
        fill_buffer(buffer);
        auto bytes_to_write = buffer.size();

        // Write the buffer
        asio::write(socket, buffer, asio::transfer_exactly(bytes_to_write));

        bytes_remaining -= bytes_to_write;
    }
}

void Messenger::async_send(std::size_t message_size, asio::yield_context yield,
                          std::function<void(asio::streambuf&)> fill_buffer) {
    // Write total message size header
    asio::async_write(socket, asio::buffer(&message_size, sizeof(std::size_t)), yield);

    // Call fill_buffer callback and write chunk until the total message has been sent
    std::size_t bytes_remaining = message_size;
    while (bytes_remaining) {
        // Get chunk of messge from caller
        fill_buffer(buffer);
        auto bytes_to_write = buffer.size();

        // Write the buffer
        asio::async_write(socket, buffer, asio::transfer_exactly(bytes_to_write), yield);

        bytes_remaining -= bytes_to_write;
    }
}

// Read an integer header, of type size_t, containing the number of bytes to follow
// followed by a read of the message. Each time a buffer sized chunk is read the user provided process_read function is called
// Return the total message size
std::size_t Messenger::receive(std::size_t chunk_size, std::function<void(asio::streambuf&)> process_read) {
    // Read the header containing the size of the message to be received
    std::size_t message_size;
    asio::read(socket, asio::buffer(&message_size, sizeof(std::size_t)));

    // Read message in buffer sized chunks
    std::size_t bytes_remaining = message_size;
    while (bytes_remaining > 0) {
        // Read the entire message if possible, otherwise read until buffer is full
        std::size_t bytes_to_read = std::min(chunk_size, bytes_remaining);

        // Read into buffer
        auto bytes_read = asio::read(socket, buffer, asio::transfer_exactly(bytes_to_read));
        process_read(buffer);

        bytes_remaining -= bytes_read;
    }

    return message_size;
}

std::size_t Messenger::async_receive(std::size_t chunk_size, asio::yield_context yield,
                          std::function<void(asio::streambuf&)> process_read) {
    // Read the header containing the size of the message to be received
    std::size_t message_size;
    asio::async_read(socket, asio::buffer(&message_size, sizeof(std::size_t)), yield);

    // Read message in buffer sized chunks
    std::size_t bytes_remaining = message_size;
    while (bytes_remaining > 0) {
        // Read the entire message if possible, otherwise read until buffer is full
        std::size_t bytes_to_read = std::min(chunk_size, bytes_remaining);

        // Read into buffer
        auto bytes_read = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_read), yield);
        process_read(buffer);

        bytes_remaining -= bytes_read;
    }

    return message_size;
}
 */