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
    auto builder = reservation.async_wait(yield);

    // Send the acquired builder when ready
    messenger.async_send(builder, yield);

    // Wait for the client connection to end
    try {
        auto complete = messenger.async_receive(yield);
        if (complete == "checkout_builder_complete")
            logger::write(socket, "Client completed");
        else
            logger::write(socket, "Client complete error: " + complete);
    } catch(std::exception& e) {
        logger::write(socket, "Client disconnect");
    }
}