#pragma once

#include "boost/asio/spawn.hpp"
#include "boost/optional.hpp"
#include "Resource.h"

namespace asio = boost::asio;

namespace OpenStackBuilder {
    boost::optional<Resource> request_create(asio::yield_context);
    void destroy(Resource builder, asio::yield_context);
};