#include "Builder.h"

Messenger Builder::connect_to_client(asio::yield_context yield) {
    Messenger client(io_service, "8080", yield);
    logger::write(client.socket, "Client connected");
}

void Builder::run() {
    io_service.run();
}
