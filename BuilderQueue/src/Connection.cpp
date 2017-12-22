#include "Connection.h"
#include "ReservationRequest.h"

// Handle a client builder request
void Connection::checkout_builder(Messenger& client) {
    logger::write(client.socket, "Checkout resource request");

    // Request a builder
    logger::write(client.socket, "Requesting builder from the queue");
    ReservationRequest reservation(queue, client);
    BuilderData builder = reservation.async_wait();
    if (client.error) {
        logger::write("reservation builder request failed: " + client.error.message());
        return;
    }

    // Send the fulfilled builder
    client.async_send(builder);
    if (client.error) {
        logger::write(client.socket, "Error sending builder: " + builder.id + "(" + builder.host + ")");
        return;
    }

    // Wait on connection to finish
    logger::write(client.socket, "Sent builder: " + builder.id + "(" + builder.host + ")");
    std::string complete = client.async_receive(MessageType::string);
    if (client.error || complete != "checkout_builder_complete") {
        logger::write(client.socket, "Failed to receive completion message from client" + client.error.message());
        return;
    }

    logger::write(client.socket, "Connection completed successfully");
}