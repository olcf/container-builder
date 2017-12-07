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
            asio::spawn(io_service,
                        [&](asio::yield_context yield) {
                            OpenStackBuilder::destroy(reservation.builder.get(), io_service, yield);
                            //TODO check if the builder was actually destroyed
                            active_builders--;
                        });
        }
    }

    // Remove all completed reservations
    reservations.remove_if([](const auto &res) {
        return res.complete();
    });

    // Assign any unused builders to outstanding reservations
    for (auto &reservation : reservations) {
        if (cached_builders.empty()) {
            break;
        }
        if (reservation.pending()) {
            auto builder = cached_builders.front();
            reservation.ready(builder);
            cached_builders.pop();
            active_builders++;
        }
    }

    // Caclulate the total allowable builders
    auto all_builder_count = active_builders + cached_builders.size() + outstanding_builder_requests;
    auto open_slots = max_builders - all_builder_count;

    // Calculate if any cached slots are available
    auto open_cache_slots = max_cached_builders - cached_builders.size();

    // If slots are available attempt to fill them
    auto request_count = std::min(open_slots, open_cache_slots);

    for (int i=0; i < request_count; i++) {
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        outstanding_builder_requests++;
                        auto opt_builder = OpenStackBuilder::request_create(io_service, yield);
                        if (opt_builder) {
                            cached_builders.push(opt_builder.get());
                        }
                        outstanding_builder_requests--;
                    });
    }
}