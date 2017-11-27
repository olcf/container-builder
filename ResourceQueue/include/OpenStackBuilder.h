#pragma once

#include "boost/asio/spawn.hpp"
#include "boost/optional.hpp"
#include "Builder.h"

namespace asio = boost::asio;

namespace OpenStackBuilder {
    boost::optional<Builder> request_create(asio::yield_context);
    void destroy(Builder builder, asio::yield_context);
};