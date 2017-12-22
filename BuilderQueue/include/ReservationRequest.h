#pragma once

#include "BuilderQueue.h"
#include "Builder.h"

// ReservationRequest breaks circular dependence between Reservation and BuilderQueue
// We must remove a reservation from the queue if it is destructed but we don't want
// The reservation to have a handle to the queue, else we have a weird circular dependency: reservation<-->queue
class ReservationRequest {

public:
    explicit ReservationRequest(BuilderQueue &queue, Messenger& client) :
                                               queue(queue),
                                               client(client),
                                               reservation(queue.enter())
    {}

    ~ReservationRequest() {
        reservation.set_request_complete();
    }

    BuilderData async_wait();

private:
    BuilderQueue &queue;
    Messenger& client;
    Reservation &reservation;
};