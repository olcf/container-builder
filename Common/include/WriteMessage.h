#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace message {
    // Write an integer header, of type size_t, containing the number of bytes to follow
    // followed by a write of the message.
    // If no callback is provided it's assumed the buffer is populated with the full message
    template<typename BufferSequence>
    void write(tcp::socket &socket, BufferSequence buffer, std::size_t message_size, std::function<void(std::size_t)> fill_buffer=[](std::size_t){}) {
        asio::write(socket, asio::buffer(&message_size, sizeof(std::size_t)));

        // Write message in buffer sized chunks
        std::size_t bytes_remaining = message_size;
        while (bytes_remaining) {
            auto bytes_to_write = std::min(asio::buffer_size(buffer), bytes_remaining);

            // Callback to fill buffer with bytes_to_write bytes
            fill_buffer(bytes_to_write);

            // Write the buffer
            asio::write(socket, asio::buffer(buffer, bytes_to_write), asio::transfer_exactly(bytes_to_write));

            bytes_remaining -= bytes_to_write;
        }
    }

    // Write an integer header, of type size_t, containing the number of bytes to follow
    // followed by a write of the message.
    // If no callback is provided it's assumed the buffer is populated with the full message
    template<typename BufferSequence>
    void async_write(tcp::socket &socket, BufferSequence buffer, std::size_t message_size, asio::yield_context yield, std::function<void(std::size_t)> fill_buffer=[](std::size_t){}) {
        asio::async_write(socket, asio::buffer(&message_size, sizeof(std::size_t)), yield);

        // Write message in buffer sized chunks
        std::size_t bytes_remaining = message_size;
        while (bytes_remaining) {
            auto bytes_to_write = std::min(asio::buffer_size(buffer), bytes_remaining);

            // Callback to fill buffer with bytes_to_write bytes
            fill_buffer(bytes_to_write);

            // Write the buffer
            asio::async_write(socket, asio::buffer(buffer, bytes_to_write), asio::transfer_exactly(bytes_to_write), yield);

            bytes_remaining -= bytes_to_write;
        }
    }

    // Write an integer header, of type size_t, containing the number of bytes to follow
    // followed by a write of the message.
    void async_write(tcp::socket &socket, asio::streambuf& buffer, asio::yield_context yield);
}