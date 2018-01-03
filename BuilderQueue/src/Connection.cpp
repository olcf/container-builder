#include "Connection.h"

using namespace std::placeholders;

void Connection::wait_for_close() {
    // stream.async_read()
}

void Connection::builder_ready(BuilderData builder) {
    // Persist this connection
    auto self(shared_from_this());

    // this->builder = builder

    // Create buffer from builder

    //stream.async_write(builder, wait_for_close())
}

void Connection::start() {
        // Persist this connection
        auto self(shared_from_this());

        // Read the initial request string
        stream.async_read(buffer, [this, self](beast::error_code error, std::size_t) {
            if(!error) {
                auto request = beast::buffers_to_string(buffer.data());
                buffer.consume(buffer.size());
                if(request == "checkout_builder_request") {
                    // Pass the provide_builder callback to the queue, that copy will keep this connection alive
                    // When a builder is available builder_read() will be called
                    queue.checkout_builder(std::bind(&Connection::builder_ready, this, _1));
                }
            }
        });
}