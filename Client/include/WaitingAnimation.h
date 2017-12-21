#pragma once

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "Logger.h"

using namespace std::chrono_literals;

// Print animated ellipses, used to indicate to the user we're waiting on an async routine
class WaitingAnimation {
public:
    WaitingAnimation(const std::string &message) : active(false) {
         animation = std::thread([this, message]() {
             for(;;) {
                 if(!active)
                     break;
                 std::cout << "\r" << message << ".  ";
                 std::this_thread::sleep_for(300ms);

                 if(!active)
                     break;
                 std::cout << "\b\b.";
                 std::this_thread::sleep_for(300ms);

                 if(!active)
                     break;
                 std::cout << ".";
                 std::this_thread::sleep_for(300ms);
             }
        });
    }

    // Stop and join the thread
    void stop(const std::string &message, logger::severity_level level) {
        active = false;
        animation.join();

        std::cout << "\r" << std::flush;
        logger::write(message, level);
    }

private:
    std::thread animation;
    std::atomic<bool> active;
};
