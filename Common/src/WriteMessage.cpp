#include "WriteMessage.h"

namespace message {
    void async_write(tcp::socket &socket, asio::streambuf &buffer, asio::yield_context yield) {
        std::size_t message_size = buffer.size();
        asio::async_write(socket, asio::buffer(&message_size, sizeof(std::size_t)), yield);

        // Write the buffer
        asio::async_write(socket, buffer, asio::transfer_exactly(message_size), yield);
    }
}