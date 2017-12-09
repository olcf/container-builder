#pragma once

#include <string>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/asio/spawn.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class Builder {
public:
    std::string host;
    std::string port;
    std::string id;
};

bool operator <(const Builder &lhs, const Builder &rhs);

bool operator ==(const Builder &lhs, const Builder &rhs);

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive &ar, Builder &builder, const unsigned int version) {
            ar & builder.host;
            ar & builder.port;
            ar & builder.id;
        }
    } // namespace serialization
} // namespace boost