#include "ReadFile.h"
#include "ReadMessage.h"

void ReadFile::read(tcp::socket &socket) {
    message::read<std::streampos>(socket, asio::buffer(buffer), [this](auto chunk_size) {
        file.write(this->buffer.data(), chunk_size);
    });
}

void ReadFile::async_read(tcp::socket &socket, asio::yield_context yield) {
    message::async_read<std::streampos>(socket, asio::buffer(buffer), yield, [this](auto chunk_size) {
        file.write(buffer.data(), chunk_size);
    });
}