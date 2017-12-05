#pragma once

#include "Builder.h"
#include "Reservation.h"
#include "boost/optional.hpp"
#include <list>
#include <queue>

class BuilderQueue {
public:
    explicit BuilderQueue(asio::io_service &io_service) : io_service(io_service),
                                                          max_builders(10),
                                                          max_unused_builders(max_builders),
                                                          outstanding_builder_requests(0),
                                                          active_builders(0)
                                                          {}

    // Create a new queue reservation and return it to the requester
    Reservation& enter();

    // Attempt to process the queue after an event that adds/removes builders or requests
    void tick(asio::yield_context yield);
private:
    asio::io_service &io_service;

    // Hold reservations that are to be fulfilled
    std::list<Reservation> reservations;

    // Queue of unused builders
    std::queue<Builder> unused_builders;

    // Maximum number of active and unused builders
    const std::size_t max_builders;

    // Maximum number of builders to spin and keep up in reserve
    const std::size_t max_unused_builders;

    // Builders that have ben requested but not yet returned
    std::size_t outstanding_builder_requests;

    // Number of active, and in the process of shutting down after being active, builders
    std::size_t active_builders;
};