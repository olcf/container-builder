#include "OpenStackBuilder.h"
#include "boost/process"

namespace bp = boost::process;

namespace OpenStackBuilder {
    boost::optional<Resource> request_create(asio::yield_context) {
      // Create builder
        // Read async output untill finished
        // finish thread
    }

    void destroy(Resource builder, asio::yield_context) {
      // Destroy builder
        // read until output finished
    }
}