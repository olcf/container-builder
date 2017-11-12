#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/buffer.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace message {
    template<typename H, typename BufferSequence>
    // Read an integer header, of type H, containing the number of bytes to follow
    // followed by a read of the message. Each time a buffer sized chunk is read the user provided process_read function is called
    H read(tcp::socket &socket, BufferSequence &buffer_sequence, std::function<void(std::size_t)> process_read) {
        auto buffer = asio::buffer(buffer_sequence);

        // Read the header containing the size of the message to be received
        H message_size;
        asio::read(socket, asio::buffer(&message_size, sizeof(H)));

        // Read message in buffer chunk_size chunks
        auto bytes_remaining = message_size;
        while (bytes_remaining > 0) {
            // Read the entire message if possible, otherwise read until buffer is full
            auto bytes_to_read = std::min<H>(asio::buffer_size(buffer), bytes_remaining);

            // Read into buffer
            auto bytes_read = asio::read(socket, buffer, asio::transfer_exactly(bytes_to_read));

            process_read(bytes_read);

            bytes_remaining -= bytes_read;
        }
    }
}