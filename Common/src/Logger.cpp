#include <iostream>
#include "Logger.h"

// Set terminal color if we're outputing to a tty
void Logger::set_color(LogPriority priority) const {
    if(has_tty) {
        switch (priority) {
            case LogPriority::error:
                std::clog << "\033[31m";
                break;
            case LogPriority::warning:
                std::clog << "\033[33m";
                break;
            case LogPriority::success:
                std::clog << "\033[32m";
                break;
            case LogPriority::info:
                std::clog << "\033[22m";
                break;
            case LogPriority::debug:
                std::clog << "\033[31m";
                break;
            default:
                break;
        }
    }
}

// Print a prefix based upon the priority level
void Logger::print_prefix(LogPriority priority) const {
    switch (priority) {
        case LogPriority::error:
            std::clog << "[ERROR] ";
            break;
        case LogPriority::warning:
            std::clog << "[WARNING] ";
            break;
        case LogPriority::success:
            std::clog << "[SUCCESS] ";
            break;
        case LogPriority::info:
            std::clog << "[INFO] ";
            break;
        case LogPriority::debug:
            std::clog << "[DEBUG] ";
            break;
        default:
            break;
    }
}

// Print a suffix
void Logger::print_suffix() const {
    std::clog << std::endl;
}

void Logger::unset_color() const {
    std::clog << "\033[0m";
}

// Print a log message
void Logger::print(const std::string& message, LogPriority priority) const {
    set_color(priority);

    print_prefix(priority);

    std::clog << message;

    print_suffix();

    unset_color();
}

