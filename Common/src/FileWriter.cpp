#include "FileWriter.h"
#include <iostream>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>

// Write the specified file over the provided socket
// The format is the std::streampos filesize followed by the files binary contents
void FileWriter::write(tcp::socket &socket) {
    // Set cursor back to beginning of file
    file.seekg(0, std::ios::beg);

    // Send size of file
    asio::write(socket, asio::buffer(&file_size, sizeof(std::streampos)));

    // Send buffered file
    auto bytes_remaining = file_size;
    while (bytes_remaining) {
        auto bytes_to_send = std::min<std::streampos>(buffer.size(), bytes_remaining);

        // Read from file into buffer
        file.read(buffer.data(), bytes_to_send);

        // Async write the buffer
        asio::write(socket, asio::buffer(buffer, bytes_to_send));

        bytes_remaining -= bytes_to_send;
    }
}

// Write the specified file over the provided socket
// The format is the std::streampos filesize followed by the files binary contents
void FileWriter::async_write(tcp::socket &socket, asio::yield_context yield) {
    // Set cursor back to beginning of file
    file.seekg(0, std::ios::beg);

    // Send size of file
    asio::async_write(socket, asio::buffer(&file_size, sizeof(std::streampos)), yield);

    // Send buffered file
    auto bytes_remaining = file_size;
    while (bytes_remaining) {
        // Calculate number of bytes to read, up to buffer_size
        auto bytes_to_send = std::min<std::streampos>(buffer.size(), bytes_remaining);

        // Read from file into buffer
        file.read(buffer.data(), bytes_to_send);

        // Async write the buffer
        asio::async_write(socket, asio::buffer(buffer, bytes_to_send), yield);

        bytes_remaining -= bytes_to_send;
    }
}