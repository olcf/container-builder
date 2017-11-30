#include "BuilderQueue.h"
#include <iostream>
#include "Logger.h"
#include "OpenStackBuilder.h"

Reservation& BuilderQueue::enter() {
    reservations.emplace_back(io_service, ReservationStatus::pending);
    return reservations.back();
}

void BuilderQueue::exit(Reservation& reservation) {
    reservation.status = ReservationStatus::cleanup;
}

void BuilderQueue::tick(asio::yield_context yield) {
    // Attempt to destroy all completed builders OpenStack instances
    for (auto &reservation : reservations) {
        if (reservation.status == ReservationStatus::cleanup && reservation.builder) {
            OpenStackBuilder::destroy(reservation.builder.get(), io_service, yield);
            reservation.status = ReservationStatus::complete;
        }
    }

    // If the first element is complete pop it off
    // We only pop the first element as to not invalidate the reservation references
    if(!reservations.empty() && reservations.front().status == ReservationStatus::complete) {
        reservations.pop_front();
    }

    // Attempt to spin up a VM for the next pending reservation
    // Care must be taken here as during request_create the reservation could be exited
    for (auto &reservation : reservations) {
        if (reservation.status == ReservationStatus::pending) {
            auto opt_builder = OpenStackBuilder::request_create(io_service, yield);
            if (opt_builder) {
                reservation.ready(opt_builder.get());
                if(reservation.status == ReservationStatus::pending) {
                    reservation.status = ReservationStatus::active;
                }
            }
            break;
        }
    }
}