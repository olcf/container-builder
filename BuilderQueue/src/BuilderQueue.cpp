#include "BuilderQueue.h"
#include "Logger.h"
#include "OpenStackBuilder.h"

Reservation &BuilderQueue::enter() {
    reservations.emplace_back(io_service);
    return reservations.back();
}

void BuilderQueue::tick(asio::yield_context yield) {
    // Go through reservations and check for any that are completed
    // Spin down any completed VM's and remove them from the list of active builders
    for (auto &reservation : reservations) {
        if (reservation.complete() && reservation.builder) {
            OpenStackBuilder::destroy(reservation.builder.get(), io_service, yield);
            active_builders.remove_if([&](const auto &builder) {
                return builder.id == reservation.builder.get().id;
            });
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
            auto builder = available_builders.front();
            reservation.ready(builder);
            active_builders.push_back(builder);
            available_builders.pop();
        }
    }

    // Attempt to create a new builder if there is space
    // TODO make request_create spawn so it can go do its thing
    if (builder_count() < max_builders && available_builders.size() < max_available_builders) {
        auto opt_builder = OpenStackBuilder::request_create(io_service, yield);
        if (opt_builder)
            available_builders.push(opt_builder.get());
    }
}