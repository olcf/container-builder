#pragma once

#include <iostream>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "Logger.h"

namespace asio = boost::asio;

// Print animated ellipses, used to indicate to the user we're waiting on an async routine
class WaitingAnimation {
public:
    WaitingAnimation(asio::io_service &io_service, const std::string& prefix) : io_service(io_service),
                                                                         timer(io_service),
                                                                         expire_time(
                                                                                 boost::posix_time::milliseconds(500)),
                                                                         prefix(prefix) {
        asio::spawn(io_service,
                    [this, prefix](asio::yield_context yield) {
                        boost::system::error_code error;

                        for (;;) {
                            if (expire_time.is_not_a_date_time())
                                break;
                            std::cout << "\r" << prefix << ".  ";
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);

                            if (expire_time.is_not_a_date_time())
                                break;
                            std::cout << "\b\b.";
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);

                            if (expire_time.is_not_a_date_time())
                                break;
                            std::cout << ".";
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);
                        }
                    });
    }

    // Cancel any outstanding timers and set the expire time to not a date, stopping the animation
    // The suffix string will be printed in place of the animated ellipses
    void stop(const std::string& suffix, logger::severity_level level) {
        boost::system::error_code error;
        expire_time = boost::posix_time::not_a_date_time;
        timer.cancel(error);
        std::cout << "\r" << std::flush;
        logger::write(prefix + suffix, level);
    }

private:
    asio::io_service &io_service;
    boost::asio::deadline_timer timer;
    boost::posix_time::time_duration expire_time;
    std::string prefix;
};
