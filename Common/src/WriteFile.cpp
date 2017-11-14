#include "WriteFile.h"
#include <iostream>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>
#include "WriteMessage.h"

// Write the specified file over the provided socket
// The format is the std::streampos filesize followed by the files binary contents
void WriteFile::write(tcp::socket &socket) {
    // Set cursor back to beginning of file
    file.seekg(0, std::ios::beg);

    message::write(socket, asio::buffer(buffer), file_size, [&](auto bytes_to_fill) {
        file.read(buffer.data(), bytes_to_fill);
    });
}

// Write the specified file over the provided socket
// The format is the std::streampos filesize followed by the files binary contents
void WriteFile::async_write(tcp::socket &socket, asio::yield_context yield) {
    // Set cursor back to beginning of file
    file.seekg(0, std::ios::beg);

    message::async_write(socket, asio::buffer(buffer), file_size, yield, [&](auto bytes_to_fill) {
        file.read(buffer.data(), bytes_to_fill);
    });
}