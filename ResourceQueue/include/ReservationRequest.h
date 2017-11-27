#pragma once

#include "ResourceQueue.h"
#include "Resource.h"

// ReservationRequest breaks circular dependence between Reservation and ResourceQueue
// We must remove a reservation from the queue if it is destructed but we don't want
// The reservation to have a handle to the queue, else we have a weird circular dependency: reservation<-->queue
class ReservationRequest {
public:
    ReservationRequest(tcp::socket &socket, ResourceQueue &queue, asio::yield_context yield) : socket(socket),
                                                                                               queue(queue),
                                                                                               reservation(socket),
                                                                                               yield(yield){
        queue.enter(&reservation, yield);
    }

    ~ReservationRequest() {
        queue.exit(&reservation, yield);
    }

    Resource async_wait();

    tcp::socket &socket;
private:
    Reservation reservation;
    ResourceQueue &queue;
    asio::yield_context yield;
};