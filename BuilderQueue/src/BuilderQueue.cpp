#include "BuilderQueue.h"
#include "Logger.h"
#include "OpenStackBuilder.h"

Reservation &BuilderQueue::enter() {
    reservations.emplace_back(io_service);
    return reservations.back();
}

void BuilderQueue::tick(asio::yield_context yield) {
    logger::write("tick");

    // Destroy all completed builder OpenStack instances
    for (auto &reservation : reservations) {
        if (reservation.complete() && reservation.builder) {
            OpenStackBuilder::destroy(reservation.builder.get(), io_service, yield);
        }
    }

    // Remove all completed reservations
    reservations.remove_if([](const auto &res) {
        return res.complete();
    });

    // Assign any available builders to outstanding reservations
    for (auto &reservation : reservations) {
        if (available_builders.empty()) {
            break;
        }
        if (reservation.pending()) {
            reservation.ready(available_builders.front());
            available_builders.pop();
        }
    }

    // Attempt to create a new builder if there is space
    // Should we completely fill the builder reserve each tick?
    if (available_builders.size() < max_available_builders) {
        auto opt_builder = OpenStackBuilder::request_create(io_service, yield);
        if (opt_builder)
            available_builders.push(opt_builder.get());
    }
}