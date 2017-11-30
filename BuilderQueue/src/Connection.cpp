#include "Connection.h"
#include "ReservationRequest.h"
#include "Messenger.h"

// Handle the initial request
void Connection::begin() {
    auto self(shared_from_this());
    asio::spawn(socket.get_io_service(),
                [this, self](asio::yield_context yield) {
                    try {
                        Messenger messenger(socket);
                        auto request = messenger.async_receive(yield);

                        if (request == "checkout_builder_request")
                            checkout_builder(yield);
                        else
                            throw std::system_error(EPERM, std::system_category(), request + " not supported");
                    }
                    catch (std::exception &e) {
                        logger::write(socket, std::string() + "Connection initial request error " + e.what());
                    }
                });
}

// Handle a client builder request
void Connection::checkout_builder(asio::yield_context yield) {
    try {
        logger::write(socket, "Checkout resource request");

        Messenger messenger(socket);

        logger::write(socket, "Requesting resource from the queue");
        ReservationRequest reservation(queue);
        auto builder = reservation.async_wait(yield);

        messenger.async_send(builder, yield);
        logger::write(socket, "sent builder: " + builder.id + "(" + builder.host + ")");


        auto complete = messenger.async_receive(yield);
        if (complete == "checkout_builder_complete")
            logger::write(socket, "Client completed");
        else
            logger::write(socket, "Client complete error: " + complete);
    } catch (std::exception &e) {
        logger::write(socket, "checkout_builder disconnect");
    }
}