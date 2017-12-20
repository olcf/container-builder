#pragma once

#include "boost/asio/spawn.hpp"
#include "Builder.h"
#include <set>

namespace asio = boost::asio;

namespace OpenStackBuilder {
    std::set<BuilderData> get_builders(asio::io_service &io_service, asio::yield_context yield);
    void request_create(asio::io_service& io_service, asio::yield_context yield);
    void destroy(BuilderData builder, asio::io_service& io_service, asio::yield_context yield);
};