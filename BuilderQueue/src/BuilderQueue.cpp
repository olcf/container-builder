#include "BuilderQueue.h"
#include "Logger.h"
#include "OpenStackBuilder.h"

Reservation &BuilderQueue::enter() {
    reservations.emplace_back(io_service);
    return reservations.back();
}

void BuilderQueue::tick(asio::yield_context yield) {
    int active_builder_count = 0;

    // Go through reservations and check for any that are completed
    // Spin down any completed VM's and remove them from the list of active builders
    for (auto &reservation : reservations) {
        if (reservation.complete() && reservation.builder) {
            asio::spawn(io_service,
                        [&](asio::yield_context yield) {
                            OpenStackBuilder::destroy(reservation.builder.get(), io_service, yield);
                        });
        } else if (reservation.active() && reservation.builder) {
            active_builder_count++;
        }
    }

    // Remove all completed reservations
    reservations.remove_if([](const auto &res) {
        return res.complete();
    });

    // Assign any unused builders to outstanding reservations
    for (auto &reservation : reservations) {
        if (unused_builders.empty()) {
            break;
        }
        if (reservation.pending()) {
            auto builder = unused_builders.front();
            reservation.ready(builder);
            unused_builders.pop();
        }
    }

    // Attempt to create a new builder if there is space
    // Requesting a new builder may take quite a bit of time so it's done in a coroutine
    auto used_builder_count = active_builder_count + unused_builders.size() + outstanding_builder_requests;
    auto total_builder_space_left = max_builders - used_builder_count;
    auto unused_builder_space = unused_builders.size() < max_unused_builders;
    if (total_builder_space_left > 0 && unused_builder_space > 0) {
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        outstanding_builder_requests++;
                        auto opt_builder = OpenStackBuilder::request_create(io_service, yield);
                        if (opt_builder) {
                            unused_builders.push(opt_builder.get());
                        }
                        outstanding_builder_requests--;
                    });
    }
}