#pragma once

#include <iostream>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "Logger.h"

namespace asio = boost::asio;

// Print animated ellipses, used to indicate to the user we're waiting on an async routine
// Great care must be taken to ensure this class doesn't destruct before the timer is completely finished
// https://stackoverflow.com/questions/13070754/how-can-i-prevent-a-deadline-timer-from-calling-a-function-in-a-deleted-class
class WaitingAnimation {
public:
    WaitingAnimation(asio::io_service &io_service) : io_service(io_service),
                                                     timer(io_service),
                                                     frame_interval(
                                                             boost::posix_time::milliseconds(500)),
                                                     active(false) {
        asio::spawn(io_service,
                    [this](asio::yield_context yield) {
                        boost::system::error_code error;

                        if (active) {
                            std::cout << "\r" << message << ".  ";
                            timer.expires_from_now(frame_interval);
                            timer.async_wait(yield[error]);

                            std::cout << "\b\b.";
                            timer.expires_from_now(frame_interval);
                            timer.async_wait(yield[error]);

                            std::cout << ".";
                            timer.expires_from_now(frame_interval);
                            timer.async_wait(yield[error]);
                        }
                    });
    }

    void start(std::string &message) {
        this->message = message;
        timer.expires_from_now(frame_interval);
    }

    // Cancel any outstanding timers and set the expire time to not a date, stopping the animation
    void stop(const std::string &message, logger::severity_level level) {
        boost::system::error_code error;
        std::cout << "\r" << std::flush;
        logger::write(message, level);
    }

private:
    asio::io_service &io_service;
    boost::asio::deadline_timer timer;
    boost::posix_time::time_duration frame_interval;
    std::string message;
    bool active;
};
