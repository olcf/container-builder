#include "BuilderQueue.h"
#include <iostream>
#include "Logger.h"
#include "OpenStackBuilder.h"

void BuilderQueue::enter(Reservation *reservation) {
    logger::write(reservation->socket, "Entering queue");
    pending_reservations.push_back(reservation);
}

// TODO make this actually noexcept
void BuilderQueue::exit(Reservation *reservation) noexcept {
    auto pending_position = std::find(pending_reservations.begin(), pending_reservations.end(), reservation);
    if (pending_position != pending_reservations.end()) {
        logger::write(reservation->socket, "Exiting queue without fulfilling request");
        pending_reservations.erase(pending_position);
    } else {
        logger::write(reservation->socket, "Exiting queue");
        completed_builders.push_back(reservation->builder);
    }
}

// Attempt to process the queue after an event that adds/removes builders or requests
void BuilderQueue::tick(asio::yield_context yield) {

    // Destroy any builders that are completed
    auto opt_completed_builder = get_next_completed_builder();
    if(opt_completed_builder) {
        auto completed_builder = opt_completed_builder.get();
        OpenStackBuilder::destroy(completed_builder, io_service, yield);
    }

    // If there is currently an outstanding request attempt to create a new builder
    if(pending_reservations_available()) {
        // Attempt to create builder
        auto opt_builder = OpenStackBuilder::request_create(io_service, yield);

        // If a builder was created attempt to assign it to an outstanding reservation
        if(opt_builder) {
            auto builder = opt_builder.get();
            auto opt_reservation = get_next_reservation();
            // If a reservation is still available provide it the resource
            if (opt_reservation) {
                auto next_reservation = opt_reservation.get();
                // Assign the builder to the reservation
                next_reservation->ready(builder);
            } else { // Mark the builder as completed if it wasn't assigned to a builder
                completed_builders.push_back(builder);
            }
        }
    }
}

bool BuilderQueue::pending_reservations_available() {
    return !pending_reservations.empty();
}

boost::optional<Reservation *> BuilderQueue::get_next_reservation() {
    boost::optional<Reservation *> opt_reservation;

    if (!pending_reservations.empty()) {
        opt_reservation = pending_reservations.front();
        pending_reservations.pop_front();
    }

    return opt_reservation;
}

boost::optional<Builder> BuilderQueue::get_next_completed_builder() {
    boost::optional<Builder> opt_reservation;

    if (!completed_builders.empty()) {
        opt_reservation = completed_builders.front();
        completed_builders.pop_front();
    }

    return opt_reservation;
}