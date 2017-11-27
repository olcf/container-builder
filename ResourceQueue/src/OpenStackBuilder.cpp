#include "OpenStackBuilder.h"
#include "boost/process.hpp"

namespace bp = boost::process;

namespace OpenStackBuilder {
    boost::optional<Builder> request_create(asio::yield_context) {
      // Create builder
        // Read async output untill finished
        // finish thread
    }

    void destroy(Builder builder, asio::yield_context) {
      // Destroy builder
        // read until output finished
    }
}