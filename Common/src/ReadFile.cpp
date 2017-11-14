#include "ReadFile.h"
#include "ReadMessage.h"

void ReadFile::read(tcp::socket &socket) {
    message::read(socket, asio::buffer(buffer), [this](std::size_t chunk_size) {
        file.write(this->buffer.data(), chunk_size);
    });
}

void ReadFile::async_read(tcp::socket &socket, asio::yield_context yield) {
    message::async_read(socket, asio::buffer(buffer), yield, [this](std::size_t chunk_size) {
        file.write(buffer.data(), chunk_size);
    });
}