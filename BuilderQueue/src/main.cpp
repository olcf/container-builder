#include "BuilderQueue.h"
#include "Server.h"

namespace asio = boost::asio;

int main(int argc, char *argv[]) {
    asio::io_context io_context;

    BuilderQueue queue(io_context);
    Server server(io_context, queue);

    io_context.run();
}