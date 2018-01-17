#pragma once

#include <string>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/core/ignore_unused.hpp>

class BuilderData {
public:
    std::string host;
    std::string port;
    std::string id;
};

bool operator<(const BuilderData &lhs, const BuilderData &rhs);

bool operator==(const BuilderData &lhs, const BuilderData &rhs);

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive &ar, BuilderData &builder, const unsigned int version) {
            boost::ignore_unused(version);
            ar & builder.host;
            ar & builder.port;
            ar & builder.id;
        }
    } // namespace serialization
} // namespace boost