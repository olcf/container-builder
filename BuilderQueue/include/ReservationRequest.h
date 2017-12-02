#pragma once

#include "BuilderQueue.h"
#include "Builder.h"

// ReservationRequest breaks circular dependence between Reservation and BuilderQueue
// We must remove a reservation from the queue if it is destructed but we don't want
// The reservation to have a handle to the queue, else we have a weird circular dependency: reservation<-->queue
class ReservationRequest {
public:
    ReservationRequest(BuilderQueue &queue) :  queue(queue),
                                               reservation(queue.enter()){}

    Builder async_wait(asio::yield_context yield);
private:
    BuilderQueue &queue;
    Reservation& reservation;
};