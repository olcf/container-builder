#include "Logger.h"

// By default we log to ContainerBuilder.log but if CONSOLE_LOG is defined output it to stderr
BOOST_LOG_GLOBAL_LOGGER_INIT(global_log, src::logger) {
    src::logger lg;

#ifdef LOG_TO_CONSOLE
    logging::add_console_log(std::cerr, boost::log::keywords::format = ">> %TimeStamp% (%LineID%) %Message%");
#else
    logging::add_file_log("ContainerBuilder.log",
                          keywords::auto_flush = true,
                          keywords::open_mode = (std::ios::out | std::ios::app),
                          keywords::format = "%TimeStamp% (%LineID%) %Message%");
#endif

    logging::add_common_attributes();

    return lg;
}

namespace logger {
    void write(const std::string &message) {
        try {
            src::logger &lg = global_log::get();
            BOOST_LOG(lg) << message;
        } catch(...) {
            std::cerr<<"Error writing message: "<< message << std::endl;
        }
    }

    void write(const tcp::socket &socket, const std::string &message) {
        try {
            src::logger &lg = global_log::get();
            std::string connection_string;
            if (socket.is_open()) {
                connection_string = boost::lexical_cast<std::string>(socket.remote_endpoint());
            } else {
                connection_string = "socket-closed";
            }
            BOOST_LOG(lg) << " [" << connection_string << "] : " << message;
        } catch(...) {
            std::cerr<<"Error writing message: "<< message << std::endl;
        }
    }
}