#define BOOST_LOG_DYN_LINK 1

#include "Logger.h"
#include <boost/lexical_cast.hpp>

namespace logger {
    BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(global_log, src::logger_mt)

    void init(std::string log_filename) {
        logging::add_file_log(log_filename,
                              keywords::auto_flush = true,
                              keywords::open_mode = (std::ios::out | std::ios::app),
                              keywords::format = "%TimeStamp% (%LineID%) %Messenger%");
        logging::add_common_attributes();
        logger::write("Initializing logger\n");
    }

    void write(const std::string& message) {
        src::logger_mt &lg = global_log::get();
        BOOST_LOG(lg) << message;
    }

    void write(tcp::socket& socket, const std::string& message) {
        src::logger_mt &lg = global_log::get();
        const std::string connection_string = boost::lexical_cast<std::string>(socket.remote_endpoint());
        BOOST_LOG(lg) <<" ["<< connection_string << "] : "<<message;
    }
}