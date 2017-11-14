#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace message {
    template<typename BufferSequence>
    // Read an integer header, of type size_t, containing the number of bytes to follow
    // followed by a read of the message. Each time a buffer sized chunk is read the user provided process_read function is called
    // Return the total message size
    std::size_t read(tcp::socket &socket, BufferSequence buffer, std::function<void(std::size_t)> process_read=[](std::size_t){}) {
        // Read the header containing the size of the message to be received
        std::size_t message_size;
        asio::read(socket, asio::buffer(&message_size, sizeof(std::size_t)));

        // Read message in buffer sized chunks
        std::size_t bytes_remaining = message_size;
        while (bytes_remaining > 0) {
            // Read the entire message if possible, otherwise read until buffer is full
            std::size_t bytes_to_read = std::min(asio::buffer_size(buffer), bytes_remaining);

            // Read into buffer
            auto bytes_read = asio::read(socket, buffer, asio::transfer_exactly(bytes_to_read));
            process_read(bytes_read);

            bytes_remaining -= bytes_read;
        }

        return message_size;
    }

    template<typename BufferSequence>
    // Read an integer header, of type size_t, containing the number of bytes to follow
    // followed by a read of the message. Each time a buffer sized chunk is read the user provided process_read function is called
    // Return the total message size
    std::size_t async_read(tcp::socket &socket, BufferSequence buffer, asio::yield_context yield,
                 std::function<void(std::size_t)> process_read=[](std::size_t){}) {
        // Read the header containing the size of the message to be received
        std::size_t message_size;
        asio::async_read(socket, asio::buffer(&message_size, sizeof(std::size_t)), yield);

        // Read message in buffer chunk_size chunks
        std::size_t bytes_remaining = message_size;
        while (bytes_remaining > 0) {
            // Read the entire message if possible, otherwise read until buffer is full
            std::size_t bytes_to_read = std::min(asio::buffer_size(buffer), bytes_remaining);

            // Read into buffer
            auto bytes_read = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_read), yield);

            process_read(bytes_read);

            bytes_remaining -= bytes_read;
        }

        return message_size;
    }
}

