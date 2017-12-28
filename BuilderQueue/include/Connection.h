#pragma once

#include "BuilderQueue.h"
#include "Logger.h"
#include "Messenger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(BuilderQueue &queue, Messenger messenger) : queue(queue),
                                                                    messenger(std::move(messenger)) {}

    ~Connection() {
        logger::write("Ending connection");
    }

    void start(asio::io_context& io_context) {
        try {
            auto self(shared_from_this());

            logger::write("Established connection");

            asio::spawn(io_context,
                        [this, self](asio::yield_context yield) {
                            boost::system::error_code error;
                            auto request = messenger.async_read_string(yield, error);
                            if (error) {
                                logger::write("Request failure" + error.message());
                            } else if (request == "checkout_builder_request") {
                                checkout_builder(yield, error);
                            } else {
                                logger::write("Invalid request message received: " + request);
                            }

                        });
        } catch (std::exception& ex) {
            logger::write(std::string() + "Connection exception: " + ex.what());
        } catch(...) {
            logger::write("Unknown connection exception caught!");
        }
    }

private:
    BuilderQueue &queue;
    Messenger messenger;

    void checkout_builder(asio::yield_context yield, boost::system::error_code& error);
};