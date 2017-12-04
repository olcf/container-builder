#pragma once

#include "Builder.h"
#include "Reservation.h"
#include "boost/optional.hpp"
#include <list>
#include <queue>

class BuilderQueue {
public:
    explicit BuilderQueue(asio::io_service &io_service) : io_service(io_service),
                                                          max_available_builders(1) {}

    // Create a new queue reservation and return it to the requester
    Reservation& enter();

    // Attempt to process the queue after an event that adds/removes builders or requests
    void tick(asio::yield_context yield);
private:
    asio::io_service &io_service;

    // Hold reservations that are to be fulfilled
    std::list<Reservation> reservations;

    // Queue of available builders
    std::queue<Builder> available_builders;
    const std::queue<Builder>::size_type max_available_builders = 5;
};