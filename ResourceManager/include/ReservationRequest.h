#pragma once

#include "ResourceQueue.h"
#include "Resource.h"


// Handle RAII access to the ResourceQueue
class ReservationRequest {
public:
    ReservationRequest(tcp::socket &socket, ResourceQueue &queue) : socket(socket),
                                                                    queue(queue),
                                                                    reservation(socket) {
        queue.enter(&reservation);
    }

    ~ReservationRequest() {
        queue.exit(&reservation);
    }

    Resource async_wait(asio::yield_context yield);

    tcp::socket &socket;
private:
    Reservation reservation;
    ResourceQueue &queue;
};