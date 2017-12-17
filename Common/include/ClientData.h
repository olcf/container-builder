#pragma once

#include <string>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/string.hpp>

enum class Architecture {
    x86_64,
    ppc64le
};

class ClientData {
public:
    std::string user_id;
    bool tty;
    Architecture arch;

    void set_arch(std::string arch_string) {
            if(arch_string == "x86_64")
                arch = Architecture::x86_64;
            else if (arch_string == "ppc64le")
                arch = Architecture::ppc64le;
    }
};

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive &ar, ClientData &client_data, const unsigned int version) {
            ar & client_data.user_id;
            ar & client_data.tty;
            ar & client_data.arch;
        }
    } // namespace serialization
} // namespace boost