#pragma once

#include <string>
#include <unistd.h>

enum class LogPriority {
    error = 0,
    success = 1,
    warning = 2,
    info = 3,
    debug = 4
};

// Logger singleton, not designed to be thread safe
class Logger {
public:
    Logger() : has_tty(isatty(STDOUT_FILENO)),
               max_priority(LogPriority::info) {}

    static void set_priority(LogPriority priority) {
        instance().max_priority = priority;
    }

    static LogPriority get_priority() {
        return instance().max_priority;
    }

    // Return the instance of the logger object
    static Logger &instance() {
        static Logger instance;
        return instance;
    }

    // Print a message to the logger
    void print(const std::string &message, LogPriority priority) const;

    // Methods to be used to print
    static void error(const std::string &message) {
        const auto &logger = instance();
        if (logger.max_priority >= LogPriority::error)
            logger.print(message, LogPriority::error);
    }

    static void warning(const std::string &message) {
        const auto &logger = instance();
        if (logger.max_priority >= LogPriority::warning)
            logger.print(message, LogPriority::warning);
    }

    static void success(const std::string &message) {
        const auto &logger = instance();
        if (logger.max_priority >= LogPriority::success)
            logger.print(message, LogPriority::success);
    }

    static void info(const std::string &message) {
        const auto &logger = instance();
        if (logger.max_priority >= LogPriority::info)
            logger.print(message, LogPriority::info);
    }

    static void debug(const std::string &message) {
        const auto &logger = instance();
        if (logger.max_priority >= LogPriority::debug)
            logger.print(message, LogPriority::debug);
    }

private:
    void set_color(LogPriority priority) const;

    void unset_color() const;

    void print_prefix(LogPriority priority) const;

    void print_suffix() const;

    const bool has_tty;
    LogPriority max_priority; // The maximum priority to print
};