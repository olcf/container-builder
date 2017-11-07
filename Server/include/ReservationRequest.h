#pragma once

#include "ResourceQueue.h"
#include "Resource.h"


// Handle RAII access to the ResourceQueue
class ReservationRequest {
public:
    ReservationRequest(asio::io_service &io_service, ResourceQueue &queue) : io_service(io_service),
                                                                             queue(queue),
                                                                             reservation(io_service) {
        queue.enter(&reservation);
    }

    ~ReservationRequest() {
        queue.exit(&reservation);
    }

    Resource async_wait(asio::yield_context yield);

private:
    asio::io_service &io_service;
    Reservation reservation;
    ResourceQueue &queue;
};