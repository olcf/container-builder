#pragma once

#include "BuilderQueue.h"
#include "Builder.h"

// ReservationRequest breaks circular dependence between Reservation and BuilderQueue
// We must remove a reservation from the queue if it is destructed but we don't want
// The reservation to have a handle to the queue, else we have a weird circular dependency: reservation<-->queue
class ReservationRequest {
public:
    ReservationRequest(tcp::socket &socket, BuilderQueue &queue, asio::yield_context yield) : socket(socket),
                                                                                              reservation(socket),
                                                                                              queue(queue),
                                                                                               yield(yield){
        queue.enter(&reservation, yield);
    }

    ~ReservationRequest() {
        queue.exit(&reservation, yield);
    }

    Builder async_wait();

    tcp::socket &socket;
private:
    Reservation reservation;
    BuilderQueue &queue;
    asio::yield_context yield;
};