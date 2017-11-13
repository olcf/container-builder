#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace message {
    // Write an integer header, of type H, containing the number of bytes to follow
    // followed by a write of the message.
    template<typename H, typename BufferSequence>
    void write(tcp::socket &socket, BufferSequence buffer, H message_size, std::function<void(H)> fill_buffer=[](H){}) {
        asio::write(socket, asio::buffer(&message_size, sizeof(H)));

        // Write message in buffer sized chunks
        H bytes_remaining = message_size;
        while (bytes_remaining) {
            auto bytes_to_write = std::min<H>(asio::buffer_size(buffer), bytes_remaining);

            // Callback to fill buffer with bytes_to_write bytes
            fill_buffer(bytes_to_write);

            // Write the buffer
            asio::write(socket, asio::buffer(buffer, bytes_to_write));

            bytes_remaining -= bytes_to_write;
        }
    }

    // Write an integer header, of type H, containing the number of bytes to follow
    // followed by a write of the message.
    template<typename H, typename BufferSequence>
    void async_write(tcp::socket &socket, BufferSequence buffer, H message_size, asio::yield_context yield, std::function<void(H)> fill_buffer=[](H){}) {
        asio::async_write(socket, asio::buffer(&message_size, sizeof(H)), yield);

        // Write message in buffer sized chunks
        H bytes_remaining = message_size;
        while (bytes_remaining) {
            auto bytes_to_write = std::min<H>(asio::buffer_size(buffer), bytes_remaining);

            // Callback to fill buffer with bytes_to_write bytes
            fill_buffer(bytes_to_write);

            // Write the buffer
            asio::async_write(socket, asio::buffer(buffer, bytes_to_write), yield);

            bytes_remaining -= bytes_to_write;
        }
    }
}