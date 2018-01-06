#pragma once

#include <string>
#include <unistd.h>

enum class LogPriority {
        error,
        warning,
        success,
        info,
        debug
};

// Logger singleton, not designed to be thread safe
class Logger {
public:
    Logger() : has_tty(isatty(STDOUT_FILENO)) {}

    // Return the instance of the logger object
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    // Print a message to the logger
    void print(const std::string& message, LogPriority priority);

    // Methods to be used to print
    static void error(const std::string& message) {
        instance().print(message, LogPriority::error);
    }
    static void warning(const std::string& message) {
        instance().print(message, LogPriority::warning);
    }
    static void success(const std::string& message) {
        instance().print(message, LogPriority::error);
    }
    static void info(const std::string& message) {
        instance().print(message, LogPriority::info);
    }
    static void debug(const std::string& message) {
        instance().print(message, LogPriority::debug);
    }

private:
    const bool has_tty;
    void set_color(LogPriority priority);
    void unset_color();
    void print_prefix(LogPriority priority);
    void print_suffix();
};