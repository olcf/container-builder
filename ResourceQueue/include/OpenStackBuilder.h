#pragma once

#include "boost/asio/spawn.hpp"
#include "boost/optional.hpp"
#include "Builder.h"

namespace asio = boost::asio;

namespace OpenStackBuilder {
    boost::optional<Builder> request_create(asio::io_service& io_service, asio::yield_context yield);
    void destroy(Builder builder, asio::io_service& io_service, asio::yield_context yield );
};