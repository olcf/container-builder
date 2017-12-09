#pragma once

#include "Builder.h"
#include "Reservation.h"
#include "boost/optional.hpp"
#include <list>
#include <queue>
#include <set>

class BuilderQueue {
public:
    explicit BuilderQueue(asio::io_service &io_service) : io_service(io_service),
                                                          max_builders(1),
                                                          max_available_builders(max_builders),
                                                          update_in_progress(false)
                                                          {}

    // Create a new queue reservation and return it to the requester
    Reservation& enter();

    // Attempt to process the queue after an event that adds/removes builders or requests
    void tick();
private:
    asio::io_service &io_service;

    // Hold reservations that are to be fulfilled
    std::list<Reservation> reservations;

    // List of currently available builders
    std::set<Builder> available_builders;

    // Maximum number of active and cached builders
    const std::size_t max_builders;

    // Maximum number of builders to spin and keep up in reserve
    const std::size_t max_available_builders;

    // Flag to determine if an update is already in progress
    bool update_in_progress;
};