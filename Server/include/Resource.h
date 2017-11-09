#pragma once

#include <string>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/asio/spawn.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class Resource {
public:
    std::string loop_device;
    std::string host;

    // Async write of a Resource
    // Send header consisting of 4 byte size, in bytes, of archived Resource
    // followed by our serialized object
    void async_write(tcp::socket &socket, asio::yield_context yield);

    // Async read of a Resource
    // Read header consisting of 4 byte size, in bytes, of archived Resource
    // followed by our serialized object
    void read(tcp::socket &socket);
};
namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive &ar, Resource &res, const unsigned int version) {
            ar & res.loop_device;
            ar & res.host;
        }
    } // namespace serialization
} // namespace boost