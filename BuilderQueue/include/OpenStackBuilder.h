#pragma once

#include "boost/asio/spawn.hpp"
#include "boost/asio/io_context.hpp"
#include "BuilderData.h"
#include <set>

namespace asio = boost::asio;

namespace OpenStackBuilder {
    std::set<BuilderData> get_builders(asio::io_context &io_context, asio::yield_context yield, boost::system::error_code &error);
    void request_create(asio::io_context& io_context, asio::yield_context yield, boost::system::error_code &error);
    void destroy(BuilderData builder, asio::io_context& io_context, asio::yield_context yield, boost::system::error_code &error);
};