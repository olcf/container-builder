#pragma once

#include "BuilderQueue.h"
#include "Logger.h"
#include "Messenger.h"
#include <boost/asio/spawn.hpp>

namespace asio = boost::asio;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(BuilderQueue &queue, Messenger messenger) : queue(queue),
                                                                    messenger(std::move(messenger)) {}

    ~Connection() {
        logger::write("Ending connection");
    }

    void start(asio::io_context &io_context);

private:
    BuilderQueue &queue;
    Messenger messenger;

    void checkout_builder(asio::yield_context yield, std::error_code &error);
};