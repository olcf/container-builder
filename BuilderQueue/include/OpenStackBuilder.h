#pragma once

#include "boost/asio/spawn.hpp"
#include "Builder.h"
#include <set>

namespace asio = boost::asio;

namespace OpenStackBuilder {
    std::set<Builder> get_builders(asio::io_service &io_service, asio::yield_context yield);
    void request_create(asio::io_service& io_service, asio::yield_context yield);
    void destroy(Builder builder, asio::io_service& io_service, asio::yield_context yield);
};