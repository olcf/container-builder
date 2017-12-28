#include "Logger.h"

// By default we log to ContainerBuilder.log but if CONSOLE_LOG is defined output it to stderr
BOOST_LOG_GLOBAL_LOGGER_INIT(logger::global_log, src::severity_logger<logger::severity_level>) {
    // Global logger object
    src::severity_logger<logger::severity_level> lg;
    // Register the severity attribute
    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
#ifdef LOG_TO_CONSOLE
    // Define a sink to std::cerr and format it with color
    auto console_sink = logging::add_console_log(std::cerr);
    console_sink->set_formatter(&color_log_severity);
#else
    // Log to a file, we don't want the log full of color sequence characters so we use a simple formatting
    logging::add_file_log("ContainerBuilder.log",
                          keywords::auto_flush = true,
                          keywords::open_mode = (std::ios::out | std::ios::app),
                          keywords::format = "%TimeStamp% [%Severity%]: %Message%");
#endif
    logging::add_common_attributes();
    return lg;
}

void logger::write(const std::string &message, severity_level severity) {
    try {
        auto &lg = global_log::get();
        BOOST_LOG_SEV(lg, severity) << message;
    } catch (...) {
        std::cerr << "Error writing message: " << message << std::endl;
    }
}

void logger::write(const tcp::socket &socket, const std::string &message, severity_level severity) {
    try {
        auto &lg = global_log::get();
        std::string connection_string;
        if (socket.is_open()) {
            connection_string = boost::lexical_cast<std::string>(socket.remote_endpoint());
        } else {
            connection_string = "socket-closed";
        }
        BOOST_LOG_SEV(lg, severity) << " [" << connection_string << "] : " << message;
    } catch (...) {
        std::cerr << "Error writing message: " << message << std::endl;
    }
}

// Custom formatter to color text output to console based on severity level
void logger::color_log_severity(logging::record_view const &rec, logging::formatting_ostream &strm) {
    // Add color based on severity
    auto severity = logging::extract<logger::severity_level>("Severity", rec);
    if (severity) {
        switch (severity.get()) {
            case logger::severity_level::info:
                strm << "\033[22m";
                break;
            case logger::severity_level::success:
                strm << "\033[32m";
                break;
            case logger::severity_level::warning:
                strm << "\033[33m";
                break;
            case logger::severity_level::error:
                strm << "\033[31m";
                break;
            case logger::severity_level::fatal:
                strm << "\033[31m";
                break;
            default:
                break;
        }
    }
    // Print formatted log message
    auto *facet = new boost::posix_time::time_facet;
    facet->format("%Y-%b-%d %H:%M:%S");
    strm.imbue(std::locale(strm.getloc(), facet));

    strm << logging::extract<boost::posix_time::ptime>("TimeStamp", rec) << " ["
         << logging::extract<logger::severity_level>("Severity", rec) << "] "
         << rec[logging::expressions::smessage];

    // Remove coloring
    if (severity) {
        strm << "\033[0m";
    }
}

std::ostream &logger::operator<<(std::ostream &strm, logger::severity_level &level) {
    static const char *strings[] = {
            "INFO",
            "SUCCESS",
            "WARNING",
            "ERROR",
            "FATAL"};

    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}