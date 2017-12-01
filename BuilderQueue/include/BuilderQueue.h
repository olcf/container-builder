#pragma once

#include "Builder.h"
#include "Reservation.h"
#include "boost/optional.hpp"
#include <list>
#include <queue>

class BuilderQueue {
public:
    explicit BuilderQueue(asio::io_service &io_service) : io_service(io_service) {}

    // Create a new queue reservation and return it to the requester
    Reservation& enter();

    // On exit release remove any outstanding requests from pending queue
    void exit(Reservation& reservation);

    // Attempt to process the queue after an event that adds/removes builders or requests
    void tick(asio::yield_context yield);
private:
    asio::io_service &io_service;

    // Hold reservations that are to be fulfilled
    std::list<Reservation> reservations;

    // Queue of available builders
    std::queue<Builder> available_builders;
    std::queue<Builder>::size_type max_available_builders;
};