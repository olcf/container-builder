#include "Connection.h"
#include "ReservationRequest.h"
#include "Messenger.h"

void Connection::begin() {
    auto self(shared_from_this());
    asio::spawn(socket.get_io_service(),
                [this, self](asio::yield_context yield) {
                    try {
                        // Handle request type
                        Messenger messenger(socket);
                        auto request = messenger.async_receive(yield);

                        if (request == "checkout_builder_request")
                            checkout_builder(yield);
                        else if (request == "checkin_builder_request")
                            checkin_builder(yield);
                        else
                            throw std::system_error(EPERM, std::system_category(), request + " not supported");
                    }
                    catch (std::exception &e) {
                        std::string except("Exception: ");
                        except += e.what();
                        logger::write(socket, except);
                    }
                });
}

// Handle a client builder request
void Connection::checkout_builder(asio::yield_context yield) {
    logger::write(socket, "Checkout resource request");

    Messenger messenger(socket);

    // Request a builder from the queue
    ReservationRequest reservation(socket, queue);
    auto resource = reservation.async_wait(yield);

    // Send the acquired resource when ready
    messenger.async_send(resource, yield);
}

// Handle checking in a new builder, adding it to the pool of available builders
// When the build is finished the builder instance is shutdown
void Connection::checkin_builder(asio::yield_context yield) {

    // Read the builders information
    Messenger builder_messenger(socket);
    auto resource = builder_messenger.async_receive_resource(yield);

    queue.add_resource(resource);
    logger::write("Checked in new resource: " + resource.host + ":" + resource.port);

    // Wait for the builder to complete
    try {
        auto complete = builder_messenger.async_receive(yield);
        logger::write(socket, "Builder completed");
    } catch(std::exception& e) {
        logger::write(socket, "Builder failed to inform Queue of completion");
    }

    // Remove the builder from available builders
    queue.remove_resource(resource);
}