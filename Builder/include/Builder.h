#pragma once

#include <boost/asio/io_context.hpp>

namespace asio = boost::asio;

class Builder {
public:
    explicit Builder();

    void run();

private:
    asio::io_context io_context;
};
