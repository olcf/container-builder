#pragma once

#include "Builder.h"
#include "Reservation.h"
#include "boost/optional.hpp"
#include <list>
#include <deque>

// Execute queued callback functions as resources allow
class BuilderQueue {
public:
    explicit BuilderQueue(asio::io_service& io_service) : io_service(io_service) {};

    // Create a new queue reservation and return it to the requester
    void enter(Reservation *reservation, asio::yield_context yield);

    // On exit release remove any outstanding requests from pending queue
    void exit(Reservation *reservation, asio::yield_context yield) noexcept;

    // Return the next reservation, if one is available
    boost::optional<Reservation*> get_next_reservation(asio::yield_context yield);

    // Attempt to process the queue after an event that adds/removes builders or requests
    void tick(asio::yield_context yield);

    // Return the next reservation in the queue, if one is available
    boost::optional<Reservation *> get_next_reservation();

private:
    asio::io_service& io_service;
    std::deque<Reservation *> pending_queue;
};