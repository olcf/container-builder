#include "Connection.h"
#include "ReservationRequest.h"
#include "Messenger.h"

// Handle the initial request
void Connection::begin() {
    try {
        auto self(shared_from_this());
        asio::spawn(client.socket.get_io_service(),
                    [this, self](asio::yield_context yield) {

                        boost::system::error_code error;
                        auto request = client.async_receive(yield[error], MessageType::string);
                        if (error) {
                            logger::write(client.socket, "Request failure" + error.message());
                        } else if (request == "checkout_builder_request") {
                            checkout_builder(yield[error]);
                        } else {
                            logger::write(client.socket, "Invalid request message received: " + request);
                        }
                    });
    } catch (...) {
        logger::write(client.socket, "Unknown connection exception caught");
    }
}

// Handle a client builder request
void Connection::checkout_builder(asio::yield_context yield) {
    logger::write(client.socket, "Checkout resource request");
    boost::system::error_code error;

    // Request a builder
    logger::write(client.socket, "Requesting builder from the queue");
    ReservationRequest reservation(queue);
    BuilderData builder = reservation.async_wait(yield[error]);
    if (error) {
        logger::write("reservation builder request failed: " + error.message());
        return;
    }

    // Send the fulfilled builder
    client.async_send(builder, yield[error]);
    if (error) {
        logger::write(client.socket, "Error sending builder: " + builder.id + "(" + builder.host + ")");
        return;
    }

    // Wait on connection to finish
    logger::write(client.socket, "Sent builder: " + builder.id + "(" + builder.host + ")");
    std::string complete = client.async_receive(yield[error], MessageType::string);
    if (error || complete != "checkout_builder_complete") {
        logger::write(client.socket, "Failed to receive completion message from client" + error.message());
        return;
    }

    logger::write(client.socket, "Connection completed successfully");
}