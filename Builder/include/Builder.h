#pragma once

#include <boost/asio.hpp>

namespace asio = boost::asio;

class Builder {
public:
    explicit Builder();

    // Start the IO service
    void run();

private:
    asio::io_service io_service;
};
