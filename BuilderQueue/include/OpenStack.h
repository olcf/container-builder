#pragma once

#include "boost/asio/io_context.hpp"
#include "BuilderData.h"
#include <set>

namespace asio = boost::asio;
using CreateHandler = std::function<void (std::error_code error, BuilderData builder)>;
using DestroyHandler = std::function<void (std::error_code error)>;

namespace OpenStack {
    void request_create(asio::io_context &io_context, CreateHandler&& handler);

    void destroy(BuilderData builder, asio::io_context &io_context, DestroyHandler&& handler);
};