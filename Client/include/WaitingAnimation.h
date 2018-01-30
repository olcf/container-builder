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
    explicit WaitingAnimation(const std::string &message) : active(true) {
        const auto prefix = message + ": ";

        // If an error occurs before the thread is created still print a message
        std::cout << prefix << ".  ";

        // If debug logging is enabled don't do any animation as it might interfere with debug printing
        if (Logger::get_priority() > LogPriority::info) {
            return;
        }

        // Start a thread to run the animation
        animation = std::thread([this, prefix]() {
            for (;;) {
                if (!active)
                    break;
                std::cout << "\r" << prefix << ".  ";
                std::this_thread::sleep_for(500ms);

                if (!active)
                    break;
                std::cout << "\b\b. ";
                std::this_thread::sleep_for(500ms);

                if (!active)
                    break;
                std::cout << "\b.";
                std::this_thread::sleep_for(500ms);
            }
        });
    }

    ~WaitingAnimation() {
        if (active) {
            stop_error("Error encountered");
        }
    }

    // Stop the animation after success
    void stop_success(const std::string &message) {
        // Stop animation and join thread
        active = false;
        if (animation.joinable())
            animation.join();

        // Move cursor to start of initial animation message
        std::cout << "\b\b\b";

        std::cout << message << std::endl;
    }

    // Stop the animation after failure
    void stop_error(const std::string &message) {
        // Stop animation and join animation thread
        active = false;
        if (animation.joinable())
            animation.join();

        // Move cursor to start of initial animation message
        std::cout << "\b\b\b";

        std::cout << message << std::endl;
    }

private:
    std::thread animation;
    std::atomic<bool> active;
};
