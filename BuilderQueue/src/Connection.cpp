#include "Connection.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "Docker.h"

using namespace std::placeholders;

void Connection::wait_for_close() {
    // Persist this connection
    auto self(shared_from_this());

    Logger::info("Waiting for connection to close");
    stream.async_read(buffer, [this, self](beast::error_code error, std::size_t bytes) {
        boost::ignore_unused(bytes);

        if (error == websocket::error::closed) {
            Logger::info("Cleaning closing connection");
        } else {
            Logger::error("Error closing connection: " + error.message());
        }
    });
}

void Connection::builder_ready(BuilderData builder) {
    // Persist this connection
    auto self(shared_from_this());

    Logger::info("Serializing builder: " + builder.id);
    this->builder = builder;
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << builder;
    auto serialized_builder = archive_stream.str();
    std::string request_string(serialized_builder);

    Logger::info("Writing builder: " + builder.id);
    stream.async_write(asio::buffer(request_string), [this, self, request_string](beast::error_code error, std::size_t bytes) {
        boost::ignore_unused(bytes);
        if (!error) {
            wait_for_close();
        } else {
            Logger::error("Error writing builder: " + request_string);
        }
    });
}

void Connection::request_builder() {
    // Persist this connection
    auto self(shared_from_this());

    Logger::info("Request to checkout builder made");
    // Pass the provide_builder callback to the queue
    // 'self' is passed to keep the connection alive as long as it's waiting for a builder in the queue
    queue.checkout_builder([this, self](BuilderData builder) {
        builder_ready(builder);
    });
}

void Connection::request_queue_stats() {
    // Persist this connection
    auto self(shared_from_this());

    Logger::info("Request for queue stats made");
    auto status = queue.status_json();
    Logger::info("Writing queue stats");

    stream.async_write(asio::buffer(status), [this, self, status](beast::error_code error, std::size_t bytes) {
        boost::ignore_unused(bytes);
        if (error) {
            Logger::error("Wrote job stats");
        }
    });
}

void Connection::request_image_list() {
    // Persist this connection
    auto self(shared_from_this());

    // Get list of docker images
    Logger::info("Request for image list");
    std::make_shared<Docker>(stream.get_executor().context())->request_image_list([this, self](std::error_code ec, std::string image_list){
        Logger::info("Writing image list");
        stream.async_write(asio::buffer(image_list), [this, self, image_list](beast::error_code error, std::size_t bytes) {
            boost::ignore_unused(bytes);
            if (error) {
                Logger::error("Wrote image list");
            }
        });
    });
}

void Connection::read_request_string() {
    // Persist this connection
    auto self(shared_from_this());

    Logger::info("Reading initial request");
    stream.async_read(buffer, [this, self](beast::error_code error, std::size_t bytes) {
        boost::ignore_unused(bytes);
        if (!error) {
            auto request = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());
            if (request == "checkout_builder_request") {
                request_builder();
            } else if(request == "queue_status_request") {
                request_queue_stats();
            } else if(request == "image_list_request") {
                request_image_list();
            } else {
                Logger::error("Bad initial request string: " + request);
            }
        } else {
            Logger::error("Failed to read initial request string from client: " + error.message());
        }
    });
}

void Connection::start() {
    // Persist this connection
    auto self(shared_from_this());

    // Accept websocket handshake
    Logger::info("waiting for client WebSocket handshake");
    stream.async_accept([this, self](beast::error_code error) {
        if (!error) {
            Logger::info("Setting stream to binary mode");
            stream.binary(true);
            read_request_string();
        } else {
            Logger::error("WebSocket handshake error: " + error.message());
        }
    });
}