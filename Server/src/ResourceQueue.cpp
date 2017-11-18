#include "ResourceQueue.h"
#include <iostream>
#include "Logger.h"

// Create a new queue reservation and return it to the requester
void ResourceQueue::enter(Reservation *reservation) {
    logger::write("Entering queue");
    pending_queue.push_back(reservation);
    tick();
}

void ResourceQueue::add_resource(Resource resource) {
    available_resources.push_back(resource);
}

void ResourceQueue::exit(Reservation *reservation) noexcept {
    logger::write("Exiting queue");
    try {
        if (reservation->active) {
            add_resource(reservation->resource);
            tick();
        } else {
            auto pending_position = std::find(pending_queue.begin(), pending_queue.end(), reservation);
            if (pending_position != pending_queue.end())
                pending_queue.erase(pending_position);
            else {
                throw std::system_error(EADDRNOTAVAIL, std::generic_category(),
                                        "reservation not found in pending or active queues");
            }
        }
    } catch (std::exception const &e) {
        std::string except("Exception: ");
        except += e.what();
        logger::write(except);
    }
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