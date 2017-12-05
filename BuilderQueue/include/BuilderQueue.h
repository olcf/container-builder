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
                                                          outstanding_builder_requests(0)
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

    // Queue of builders which are currently checked out
    std::list<Builder> active_builders;

    std::queue<Builder>::size_type builder_count() const {
        return unused_builders.size() + active_builders.size();
    }

    // Maximum number of active and unused builders
    const std::size_t max_builders;

    // Maximum number of builders to spin up and keep up and running in reserve
    const std::size_t max_unused_builders;

    std::size_t outstanding_builder_requests;
};