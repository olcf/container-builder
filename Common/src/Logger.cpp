#include "Logger.h"

// Custom formatter to color text output based on severity level
void color_log_severity(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // Add color based on severity
    auto severity = logging::extract<logger::severity_level>("Severity", rec);
    if (severity) {
        switch (severity.get()) {
            case logger::severity_level::normal:
                strm << "\033[32m";
                break;
            case logger::severity_level::warning:
                strm << "\033[33m";
                break;
            case logger::severity_level::error:
                strm << "\033[31m";
            case logger::severity_level::fatal:
                strm << "\033[31m";
                break;
            default:
                break;
        }
    }

    // Print formatted log message
    strm << logging::extract<std::string>("TimeStamp", rec) << '[' << logging::extract<std::string>("Severity", rec) << ']'
         << rec[logging::expressions::smessage];

    // Remove coloring
    if (severity) {
        strm << "\033[0m";
    }
}


// By default we log to ContainerBuilder.log but if CONSOLE_LOG is defined output it to stderr
BOOST_LOG_GLOBAL_LOGGER_INIT(logger::global_log, src::severity_logger<logger::severity_level>) {
    src::severity_logger<logger::severity_level> lg;

#ifdef LOG_TO_CONSOLE
    logging::add_console_log(std::cerr);
#else
    logging::add_file_log("ContainerBuilder.log",
                          keywords::auto_flush = true,
                          keywords::open_mode = (std::ios::out | std::ios::app),
                          keywords::format = "%TimeStamp% [%Severity%]: %Message%");
#endif

    typedef logging::sinks::synchronous_sink< logging::sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();
    sink->set_formatter(&color_log_severity);
    logging::core::get()->add_sink(sink);

    logging::add_common_attributes();

    return lg;
}

namespace logger {
    void write(const std::string &message, severity_level severity) {
        try {
            auto &lg = global_log::get();
            BOOST_LOG_SEV(lg, severity) << message;
        } catch(...) {
            std::cerr<<"Error writing message: "<< message << std::endl;
        }
    }

    void write(const tcp::socket &socket, const std::string &message, severity_level severity) {
        try {
            auto &lg = global_log::get();
            std::string connection_string;
            if (socket.is_open()) {
                connection_string = boost::lexical_cast<std::string>(socket.remote_endpoint());
            } else {
                connection_string = "socket-closed";
            }
            BOOST_LOG_SEV(lg, severity) << " [" << connection_string << "] : " << message;
        } catch(...) {
            std::cerr<<"Error writing message: "<< message << std::endl;
        }
    }
}