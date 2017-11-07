#include "FileReader.h"
#include <iostream>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>

void FileReader::read(tcp::socket &socket) {
    // Set cursor back to beginning of file
    file.seekg(0, std::ios::beg);

    // Read size of file
    auto header_size = sizeof(std::streampos);
    asio::read(socket, asio::buffer(&file_size, header_size));

    // Read buffered file
    auto bytes_remaining = file_size;
    while (bytes_remaining > 0) {
        auto bytes_to_read = std::min<std::streampos>(buffer.size(), bytes_remaining);

        // Async read the buffer
        asio::read(socket, asio::buffer(buffer, bytes_to_read));

        // Read from file into buffer
        file.write(buffer.data(), bytes_to_read);

        bytes_remaining -= bytes_to_read;
    }
}

void FileReader::async_read(tcp::socket &socket, asio::yield_context yield) {
    // Set cursor back to beginning of file
    file.seekg(0, std::ios::beg);

    // Read size of file
    auto header_size = sizeof(std::streampos);
    asio::async_read(socket, asio::buffer(&file_size, header_size), yield);

    // Read buffered file
    auto bytes_remaining = file_size;
    while (bytes_remaining > 0) {
        auto bytes_to_read = std::min<std::streampos>(buffer.size(), bytes_remaining);

        // Async read the buffer
        asio::async_read(socket, asio::buffer(buffer, bytes_to_read), yield);

        // Read from file into buffer
        file.write(buffer.data(), bytes_to_read);

        bytes_remaining -= bytes_to_read;
    }
}