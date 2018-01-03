#include "Connection.h"
#include <boost/archive/text_oarchive.hpp>

using namespace std::placeholders;

void Connection::wait_for_close() {
    // Persist this connection
    auto self(shared_from_this());

    // Wait for the connection to close
    stream.async_read(buffer, [this, self] (beast::error_code error, std::size_t bytes) {
        if(error == websocket::error::closed) {
          // closed cleanly
        } else {
            // log error
        }
    });
}

void Connection::builder_ready(BuilderData builder) {
    // Persist this connection
    auto self(shared_from_this());

    // Serialize the builder
    this->builder = builder;
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << builder;
    auto serialized_builder = archive_stream.str();
    std::string request_string(serialized_builder);

    // Write the builder to the client
    stream.async_write(asio::buffer(request_string), [this, self] (beast::error_code error, std::size_t bytes){
        if(!error) {
            wait_for_close();
        }
    });
}

void Connection::start() {
        // Persist this connection
        auto self(shared_from_this());

        // Read the initial request string
        stream.async_read(buffer, [this, self] (beast::error_code error, std::size_t bytes) {
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