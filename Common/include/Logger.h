#pragma once

#include <string>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;
using boost::asio::ip::tcp;

namespace logger {
    enum severity_level {
        info,
        success,
        warning,
        error,
        fatal
    };

    // Print string representation of severity levels
    std::ostream &operator<<(std::ostream &strm, logger::severity_level &level);

    // Add color to console logger output
    void color_log_severity(logging::record_view const& rec, logging::formatting_ostream& strm);

    // Write a simple message to the log
    void write(const std::string &message, severity_level severity=severity_level::info);

    // Write a log mesage with socket information appended
//    void write(const tcp::socket &socket, const std::string &message, severity_level severity=severity_level::info);

    BOOST_LOG_GLOBAL_LOGGER(global_log, src::severity_logger<severity_level>)
}