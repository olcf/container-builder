#include "BuilderQueue.h"
#include "Logger.h"
#include "OpenStackBuilder.h"

Reservation &BuilderQueue::enter() {
    reservations.emplace_back(io_service);
    return reservations.back();
}

void BuilderQueue::tick(asio::yield_context yield) {
    // Update list of "Active" OpenStack builders
    auto all_builders = OpenStackBuilder::get_builders(io_service, yield);
    std::set<Builder> unavailable_builders;

    // Process reservations
    for (auto &reservation : reservations) {
        // Set unavailable builders, that is builders which are attached to a reservation
        if (reservation.builder) {
            unavailable_builders.insert(reservation.builder.get());

            // If the reservation is complete delete the builder
            if (reservation.complete()) {
                asio::spawn(io_service,
                            [&](asio::yield_context destroy_yield) {
                                OpenStackBuilder::destroy(reservation.builder.get(), io_service, destroy_yield);
                                reservation.builder = boost::none;
                            });
            }
        }
    }

    // Delete reservations that are completed and don't have an active builder associated with them
    reservations.remove_if([](const auto &reservation) {
        return (reservation.complete() && !reservation.builder);
    });

    // Available_builders = all_builders - unavailable_builders
    std::set<Builder> available_builders;
    std::set_difference(all_builders.begin(), all_builders.end(),
                        unavailable_builders.begin(), unavailable_builders.end(),
                        std::inserter(available_builders, available_builders.begin()));

    // Assign builders to any pending reservations
    for (auto &reservation : reservations) {
        if (reservation.pending() && !available_builders.empty()) {
            reservation.ready(*available_builders.begin());
            available_builders.erase(available_builders.begin());
        }
    }

    // Request new builders if slots are open
    // Care must be taken as all_builders may include the the pending request before it's returned(open_slots, open_available_slots may be negative)
    int open_slots = max_builders - all_builders.size() - pending_requests;
    int open_available_slots = max_available_builders - available_builders.size() - pending_requests;
    int request_count = std::min(open_slots, open_available_slots);
    for (int i=0; i < request_count; i++) {
        pending_requests++;
        asio::spawn(io_service,
                    [&](asio::yield_context request_yield) {
                        OpenStackBuilder::request_create(io_service, request_yield);
                        pending_requests--;
                    });
    }
}