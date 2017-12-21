#pragma once

#include "BuilderQueue.h"
#include "Logger.h"
#include "Messenger.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket, BuilderQueue &queue) : queue(queue)
    {
        logger::write(socket, "Established connection");

        try {
            auto self(shared_from_this());
            asio::spawn(socket.get_io_service(),
                        [this, self, socket=std::move(socket)](asio::yield_context yield) mutable {
                            Messenger client(std::move(socket), yield);

                            auto request = client.async_receive(MessageType::string);
                            if (client.error) {
                                logger::write(client.socket, "Request failure" + client.error.message());
                            } else if (request == "checkout_builder_request") {
                                checkout_builder(client);
                            } else {
                                logger::write(client.socket, "Invalid request message received: " + request);
                            }
                        });
        } catch (...) {
            logger::write("Unknown connection exception caught");
        }

    }

    ~Connection() {
        logger::write("Ending connection");
    }

private:
    BuilderQueue &queue;

    // Checkout a builder
    void checkout_builder(Messenger& client);
};