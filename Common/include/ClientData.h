#pragma once

#include <string>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/core/ignore_unused.hpp>
#include "Logger.h"

enum class ArchType {
    x86_64,
    ppc64le
};

enum class BackendType {
    singularity,
    docker,
    unspecified
};

namespace Backend {
    static BackendType to_backend(const std::string &backend_string) {
        if (backend_string == "singularity")
            return BackendType::singularity;
        else if (backend_string == "docker")
            return BackendType::docker;
        else
            return BackendType::unspecified;
    }
}

namespace Arch {
    static ArchType to_arch(const std::string &arch_string) {
        if (arch_string == "x86_64")
            return ArchType::x86_64;
        else if (arch_string == "ppc64le")
            return ArchType::ppc64le;
        else
            throw std::runtime_error("Incorrect ArchType provided");
    }
};

class ClientData {
public:
    std::string user_id;
    bool tty;
    LogPriority log_priority;
    ArchType arch;
    BackendType backend;
    std::string container_path;
    std::string definition_path;
    std::string queue_host;
};

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive &ar, ClientData &client_data, const unsigned int version) {
            boost::ignore_unused(version);
            ar & client_data.user_id;
            ar & client_data.tty;
            ar & client_data.log_priority;
            ar & client_data.arch;
            ar & client_data.backend;
            ar & client_data.container_path;   // Client side container image path
            ar & client_data.definition_path;  // Client side definition path
            ar & client_data.queue_host;
        }
    } // namespace serialization
} // namespace boost