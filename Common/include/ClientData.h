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

namespace Arch {
    static Architecture to_arch(const std::string& arch_string) {
        if (arch_string == "x86_64")
            return Architecture::x86_64;
        else if (arch_string == "ppc64le")
            return Architecture::ppc64le;
        else
            throw std::runtime_error("Incorrect architecture supported");
    }
};

class ClientData {
public:
    std::string user_id;
    bool tty;
    Architecture arch;
    std::string container_path;
    std::string definition_path;
    std::string queue_host;
};

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive &ar, ClientData &client_data, const unsigned int version) {
            ar & client_data.user_id;
            ar & client_data.tty;
            ar & client_data.arch;
            ar & client_data.container_path;
            ar & client_data.definition_path;
            ar & client_data.queue_host;
        }
    } // namespace serialization
} // namespace boost