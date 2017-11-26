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
    std::string host;
    std::string port;
    std::string id;
};
namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive &ar, Resource &res, const unsigned int version) {
            ar & res.host;
            ar & res.port;
            ar & res.id;
        }
    } // namespace serialization
} // namespace boost