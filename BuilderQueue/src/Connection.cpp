#include "Connection.h"
#include "ReservationRequest.h"

// Handle a client builder request
void Connection::checkout_builder(asio::yield_context yield, boost::system::error_code& error) {
    logger::write("Checkout resource request");

    // Request a builder
    logger::write("Requesting builder from the queue");
    ReservationRequest reservation(queue);
    BuilderData builder = reservation.async_wait(yield, error);
    if (error) {
        logger::write("reservation builder request failed: " + error.message());
        return;
    }

    // Send the fulfilled builder to the client
    messenger.async_write_builder(builder, yield, error);
    if (error) {
        logger::write("Error sending builder: " + builder.id + "(" + builder.host + ")");
        return;
    }

    // Wait on connection to finish
    logger::write("Sent builder: " + builder.id + "(" + builder.host + ")");
    std::string complete = messenger.async_read_string(yield, error);
    if (error || complete != "checkout_builder_complete") {
        logger::write("Failed to receive completion message from client" + error.message());
        return;
    }

    logger::write("Connection completed successfully");
}