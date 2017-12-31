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
    WaitingAnimation(const std::string &message) : active(true) {
        animation = std::thread([this, message]() {
            for (;;) {
                if (!active)
                    break;
                std::cout << "\r" << message << ".  ";
                std::this_thread::sleep_for(300ms);

                if (!active)
                    break;
                std::cout << "\b\b.";
                std::this_thread::sleep_for(300ms);

                if (!active)
                    break;
                std::cout << ".";
                std::this_thread::sleep_for(300ms);
            }
        });
    }

    ~WaitingAnimation() {
        if(active) {
            stop("Failed",logger::severity_level::error);
        }
    }

    // Stop and join the thread
    void stop(const std::string &message, logger::severity_level level) {
        active = false;
        animation.join();

        // Erase current line and set cursor at begining, overwriting current line
        std::cout << "\33[2K\r" << std::flush;
        logger::write(message, level);
    }

private:
    std::thread animation;
    std::atomic<bool> active;
};
