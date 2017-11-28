#pragma once

#include "BuilderQueue.h"
#include "Builder.h"

// ReservationRequest breaks circular dependence between Reservation and BuilderQueue
// We must remove a reservation from the queue if it is destructed but we don't want
// The reservation to have a handle to the queue, else we have a weird circular dependency: reservation<-->queue
class ReservationRequest {
public:
    ReservationRequest(tcp::socket &socket, BuilderQueue &queue) : socket(socket),
                                                                   reservation(socket),
                                                                   queue(queue) {
        queue.enter(&reservation);
    }

    ~ReservationRequest() {
        try {
            queue.exit(&reservation);
        } catch (...) {

        }
    }

    Builder async_wait(asio::yield_context yield);

    tcp::socket &socket;
private:
    Reservation reservation;
    BuilderQueue &queue;
};