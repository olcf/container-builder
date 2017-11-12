#pragma once

#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace message {
    template<typename H, typename BufferSequence>
    // Read an integer header, of type H, containing the number of bytes to follow
    // followed by a read of the message. Each time a buffer sized chunk is read the user provided process_read function is called
    void read(tcp::socket &socket, BufferSequence buffer, std::function<void(H)> process_read=[](H){}) {
        // Read the header containing the size of the message to be received
        H message_size;
        asio::read(socket, asio::buffer(&message_size, sizeof(H)));

        // Read message in buffer sized chunks
        H bytes_remaining = message_size;
        while (bytes_remaining > 0) {
            // Read the entire message if possible, otherwise read until buffer is full
            H bytes_to_read = std::min<H>(asio::buffer_size(buffer), bytes_remaining);

            // Read into buffer
            auto bytes_read = asio::read(socket, buffer, asio::transfer_exactly(bytes_to_read));
            process_read(bytes_read);

            bytes_remaining -= bytes_read;
        }
    }

    template<typename H, typename BufferSequence>
    // Read an integer header, of type H, containing the number of bytes to follow
    // followed by a read of the message. Each time a buffer sized chunk is read the user provided process_read function is called
    void async_read(tcp::socket &socket, BufferSequence buffer, asio::yield_context yield,
                 std::function<void(H)> process_read=[](H){}) {
   //     auto buffer = asio::buffer(buffer_sequence);

        // Read the header containing the size of the message to be received
        H message_size;
        asio::async_read(socket, asio::buffer(&message_size, sizeof(H)), yield);

        // Read message in buffer chunk_size chunks
        H bytes_remaining = message_size;
        while (bytes_remaining > 0) {
            // Read the entire message if possible, otherwise read until buffer is full
            H bytes_to_read = std::min<H>(asio::buffer_size(buffer), bytes_remaining);

            // Read into buffer
            auto bytes_read = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_read), yield);

            process_read(bytes_read);

            bytes_remaining -= bytes_read;
        }
    }
}

