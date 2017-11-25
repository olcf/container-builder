#pragma once

#include "Resource.h"
#include "Reservation.h"
#include <list>
#include <deque>

// Execute queued callback functions as resources allow
class ResourceQueue {
public:
    // Create a new queue reservation and return it to the requester
    void enter(Reservation *reservation);

    // Add a resource to the queue
    void add_resource(Resource resource);

    // Remove a resource from the list of available resources
    void remove_resource(Resource resource);

    // On exit release remove any outstanding requests from pending queue
    void exit(Reservation *reservation) noexcept;

    // Return true if any resources are available
    bool resource_available() {
        return available_resources.size() > 0;
    }

    // Return true if any requests are pending
    bool pending_requests() {
        return pending_queue.size() > 0;
    }
private:
    // TODO : Review these types to see which is better
    std::list<Resource> available_resources;
    std::deque<Reservation *> pending_queue;

    // Advance the queue to see if a reservation can run
    // Return true if a job was started
    void tick();
};