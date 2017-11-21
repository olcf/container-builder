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

                        if (request == "checkout_resource_request")
                            checkout_resource(yield);
                        else if (request == "checkin_resource_request")
                            checkin_resource(yield);
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

void Connection::checkout_resource(asio::yield_context yield) {
    logger::write(socket, "Checkout resource request");

    Messenger messenger(socket);

    // Wait in the queue for a reservation to begin
    ReservationRequest reservation(socket, queue);
    auto resource = reservation.async_wait(yield);

    messenger.async_send(resource, yield);
}

void Connection::checkin_resource(asio::yield_context yield) {
    Messenger messenger(socket);
    auto resource = messenger.async_receive_resource(yield);
    queue.add_resource(resource);
    logger::write("Checked in new resource: " + resource.host + ":" + resource.port);
}