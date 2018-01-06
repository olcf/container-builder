#include "BuilderQueue.h"
#include "Server.h"
#include <boost/exception/all.hpp>

namespace asio = boost::asio;

int main(int argc, char *argv[]) {
    asio::io_context io_context;

    BuilderQueue queue(io_context);
    Server server(io_context, queue);

    // Keep running even in the event of an exception
    for (;;) {
        try {
            io_context.run();
        } catch (const boost::exception &ex) {
            auto diagnostics = diagnostic_information(ex);
            Logger::error(std::string() + "io_service exception encountered: " + diagnostics);
        } catch (const std::exception& ex) {
            Logger::error(std::string() + "io_service exception encountered: " + ex.what());
        } catch(...) {
            Logger::error("Unknown io_service exception encountered");
        }
    }
}