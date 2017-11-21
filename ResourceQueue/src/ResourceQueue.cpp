#include "ResourceQueue.h"
#include <iostream>
#include "Logger.h"

void ResourceQueue::enter(Reservation *reservation) {
    logger::write(reservation->socket, "Entering queue");
    pending_queue.push_back(reservation);
    tick();
}

void ResourceQueue::add_resource(Resource resource) {
    available_resources.push_back(resource);
    tick();
}

void ResourceQueue::exit(Reservation *reservation) noexcept {
    logger::write(reservation->socket, "Exiting queue");

    // If exit is called on an outstanding reservation remove it from the queue
    auto pending_position = std::find(pending_queue.begin(), pending_queue.end(), reservation);
    if (pending_position != pending_queue.end())
        pending_queue.erase(pending_position);
}

void ResourceQueue::tick() {
    if (!pending_queue.empty() && !available_resources.empty()) {
        // Grab next pending reservation and remove it from queue
        auto reservation = pending_queue.front();
        pending_queue.pop_front();
        // Grab an available resource and remove it from available
        auto resource = available_resources.front();
        available_resources.pop_front();
        // Invoke the reservation callback to inform the request it's ready to run
        reservation->ready(resource);
    }
}