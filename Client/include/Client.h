#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include "Messenger.h"
#include "ClientData.h"

namespace asio = boost::asio;

class Client {
public:
    explicit Client(int argc, char **argv);

    ~Client() {
        // Restore the cursor
        std::cout << "\e[?25h" << std::flush;
    }

    Messenger connect_to_queue(asio::yield_context yield);

    Messenger connect_to_builder(Messenger &queue_messenger, asio::yield_context yield);

    ClientData client_data();

    // Start the IO service
    void run();

private:
    asio::io_context io_context;
    std::string definition_path;
    std::string container_path;
    std::string user_id;
    std::string queue_host;
    std::string queue_port;
    bool tty;
    Architecture arch;

    void parse_environment();

    void parse_arguments(int argc, char **argv);
};