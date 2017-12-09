#include "BuilderQueue.h"
#include "Logger.h"
#include "OpenStackBuilder.h"

Reservation &BuilderQueue::enter() {
    reservations.emplace_back(io_service);
    return reservations.back();
}

void BuilderQueue::tick() {
    // Process pending reservations
    for (auto &reservation : reservations) {
        if (reservation.pending() && !available_builders.empty()) {
            reservation.ready(*available_builders.begin());
            available_builders.erase(available_builders.begin());
        }
    }



    // Update OpenStack by creating and removing builders as required
    // This section can take quite some time so is done in a coroutine
    if (!update_in_progress) {
        update_in_progress = true;
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        auto all_builders = OpenStackBuilder::get_builders(io_service, yield);
                        std::set<Builder> unavailable_builders;

                        // Update reservation information
                        for (auto &reservation : reservations) {
                            if (reservation.builder) {
                                unavailable_builders.insert(reservation.builder.get());
                            }
                            if (reservation.complete()) {
                                OpenStackBuilder::destroy(reservation.builder.get(), io_service, yield);
                            };
                        }

                        // TODO handle error state builders - potentially in destroy script?

                        // Remove all completed reservations
                        reservations.remove_if([](const auto &res) {
                            return res.complete();
                        });

                        // Available_builders = all_builders - unavailable_builders
                        std::set_difference(all_builders.begin(), all_builders.end(),
                                            unavailable_builders.begin(), unavailable_builders.end(),
                                            std::inserter(available_builders, available_builders.begin()));

                        // Spin up builders so we have the requested available
                        auto open_slots = max_builders - all_builders.size();
                        auto open_available_slots = max_available_builders - available_builders.size();
                        auto request_count = std::min(open_slots, open_available_slots);
                        for (auto i = 0; i < request_count; i++) {
                            OpenStackBuilder::request_create(io_service, yield);
                        }
                        update_in_progress = false;
                    });
    }

}