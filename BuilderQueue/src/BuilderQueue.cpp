#include "BuilderQueue.h"
#include <iostream>
#include "Logger.h"
#include "OpenStackBuilder.h"

Reservation& BuilderQueue::enter() {
    reservations.emplace_back(io_service, ReservationStatus::pending);
    return reservations.back();
}

void BuilderQueue::exit(Reservation& reservation) {
    reservation.status = ReservationStatus::complete;
}

void BuilderQueue::tick(asio::yield_context yield) {
    // Destroy all completed builder OpenStack instances
    for (auto &reservation : reservations) {
        if (reservation.status == ReservationStatus::complete && reservation.builder) {
            OpenStackBuilder::destroy(reservation.builder.get(), io_service, yield);
            reservation.status = ReservationStatus::complete;
        }
    }

    // Remove all completed reservations
   reservations.remove_if([](const auto& res){
       return res.status == ReservationStatus::complete;
   });

    // Assign any available builders to outstanding reservations
    for (auto &reservation : reservations) {
        // When we're out of available builders break
        if(available_builders.empty()) {
            break;
        }
        else if (reservation.status == ReservationStatus::pending) {
            reservation.ready(available_builders.front());
            available_builders.pop();
        }
    }

    // Attempt to create a new builder if there is space
    if(available_builders.size() < max_available_builders) {
        auto opt_builder = OpenStackBuilder::request_create(io_service, yield);
        if(opt_builder)
            available_builders.push(opt_builder.get());
    }
}