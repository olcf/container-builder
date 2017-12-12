#include "Connection.h"
#include "ReservationRequest.h"
#include "Messenger.h"

// Handle the initial request
void Connection::begin() {
    try {
        auto self(shared_from_this());
        asio::spawn(socket.get_io_service(),
                    [this, self](asio::yield_context yield) {
                        Messenger messenger(socket);
                        boost::system::error_code error;
                        auto request = messenger.async_receive(yield[error], MessageType::string);
                        if (error) {
                            logger::write(socket, "Request failure" + error.message());
                        } else if (request == "checkout_builder_request") {
                            checkout_builder(yield[ec]);
                        } else {
                            logger::write(socket, "Invalid request message received: " + request);
                        }
                    });
    } catch(...) {
        logger::write(socket, "Unknown connection exception caught");
    }
}

// Handle a client builder request
void Connection::checkout_builder(asio::yield_context yield) {
    logger::write(socket, "Checkout resource request");
    boost::system::error_code error;

    Messenger messenger(socket);

    // Request a builder
    logger::write(socket, "Requesting builder from the queue");
    ReservationRequest reservation(queue);
    Builder builder = reservation.async_wait(yield[error]);
    if (error) {
        logger::write("reservation builder request failed: " + error.message());
        return;
    }

    // Send the fulfilled builder
    messenger.async_send(builder, yield[error]);
    if (error) {
        logger::write(socket, "Error sending builder: " + builder.id + "(" + builder.host + ")");
        return;
    }

    // Wait on connection to finish
    logger::write(socket, "Sent builder: " + builder.id + "(" + builder.host + ")");
    std::string complete = messenger.async_receive(yield[error], MessageType::string);
    if (error || complete != "checkout_builder_complete") {
        logger::write(socket, "Failed to receive completion message from client" + error.message());
        return;
    }

    logger::write(socket, "Connection completed successfully");
}