#define BOOST_LOG_DYN_LINK 1

#include "Logger.h"

BOOST_LOG_GLOBAL_LOGGER_INIT(global_log, src::logger) {
    src::logger lg;
    logging::add_file_log("ContainerBuilder.log",
                          keywords::auto_flush = true,
                          keywords::open_mode = (std::ios::out | std::ios::app),
                          keywords::format = "%TimeStamp% (%LineID%) %Message%");
    logging::add_common_attributes();

    return lg;
}

namespace logger {
    void write(const std::string &message) {
        src::logger &lg = global_log::get();
        BOOST_LOG(lg) << message;
    }

    void write(const tcp::socket &socket, const std::string &message) {
        src::logger &lg = global_log::get();
        const std::string connection_string = boost::lexical_cast<std::string>(socket.remote_endpoint());
        BOOST_LOG(lg) << " [" << connection_string << "] : " << message;
    }
}