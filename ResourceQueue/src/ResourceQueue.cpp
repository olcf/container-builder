#include "ResourceQueue.h"
#include <iostream>
#include "Logger.h"
#include "OpenStackBuilder.h"

// Attempt to process the queue after an event that adds/removes builders or requests
void ResourceQueue::tick(asio::yield_context yield) {

    // If there are no outstanding requests there is nothing to do
    if(pending_queue.empty()) {
        return;
    }

    // Attempt to create a new builder
    auto opt_builder = OpenStackBuilder::request_create(yield);

    // If a builder was created attempt to assign it to an outstanding reservation
    if(opt_builder) {
        auto builder = opt_builder.get();
        auto opt_next_reservation = get_next_reservation();
        // and a reservation is pending
        if (opt_next_reservation) {
            auto next_reservation = opt_next_reservation.get();
            // Assign the builder to the reservation
            next_reservation->ready(builder);
        } else {
            // Destroy the builder
            OpenStackBuilder::destroy(builder, yield);
            // Tick incase anything changed during the destruction process
            tick(yield);
        }
    }
}

void ResourceQueue::enter(Reservation *reservation, asio::yield_context yield) {
    logger::write(reservation->socket, "Entering queue");

    pending_queue.push_back(reservation);
    tick(yield);
}

// TODO make this actually noexcept
void ResourceQueue::exit(Reservation *reservation, asio::yield_context yield) noexcept {
    logger::write(reservation->socket, "Exiting queue");

    // If the reservation hasn't been fulfilled remove it from the queue
    auto pending_position = std::find(pending_queue.begin(), pending_queue.end(), reservation);
    if (pending_position != pending_queue.end()) {
        pending_queue.erase(pending_position);
    } else {     // If the reservation has been fulfilled destroy the builder
        OpenStackBuilder::destroy(reservation->resource, yield);
        tick(yield);
    }
}

boost::optional<Reservation *> ResourceQueue::get_next_reservation() {
    boost::optional<Reservation *> next_reservation;

    if (!pending_queue.empty()) {
        next_reservation = pending_queue.front();
        pending_queue.pop_front();
    }

    return next_reservation;
}