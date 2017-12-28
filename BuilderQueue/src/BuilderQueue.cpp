#include <boost/asio/spawn.hpp>
#include "BuilderQueue.h"
#include "OpenStackBuilder.h"

Reservation &BuilderQueue::enter() {
    reservations.emplace_back(io_context);
    return reservations.back();
}

void BuilderQueue::tick(asio::yield_context yield) {
    boost::system::error_code get_error;

    // Update list of "Active" OpenStack builders
    auto all_builders = OpenStackBuilder::get_builders(io_context, yield, get_error);
    if (get_error) {
        logger::write("Error calling get_builders" + get_error.message());
        return;
    }

    std::set<BuilderData> unavailable_builders;

    // Delete reservations that are completed
    reservations.remove_if([](const auto &reservation) {
        return reservation.finalized();
    });

    // Process existing reservations
    for (auto &reservation : reservations) {
        // Set unavailable builders, that is builders which are attached to a reservation
        if (reservation.builder) {
            unavailable_builders.insert(reservation.builder.get());

            // If the reservation has completed delete the builder
            if (reservation.request_complete()) {
                reservation.set_enter_cleanup();
                asio::spawn(io_context,
                            [this, &reservation](asio::yield_context destroy_yield) {
                                boost::system::error_code cleanup_error;
                                OpenStackBuilder::destroy(reservation.builder.get(), io_context, destroy_yield, cleanup_error);
                                if (cleanup_error) {
                                    logger::write("Error destryoing builder " + cleanup_error.message());
                                } else {
                                    reservation.set_finalize();
                                }
                            });
            }
        }
    }

    // Available_builders = all_builders - unavailable_builders
    std::set<BuilderData> available_builders;
    std::set_difference(all_builders.begin(), all_builders.end(),
                        unavailable_builders.begin(), unavailable_builders.end(),
                        std::inserter(available_builders, available_builders.begin()));

    // Assign any available builders to any pending reservations
    for (auto &reservation : reservations) {
        if (reservation.pending() && !available_builders.empty()) {
            reservation.ready(*available_builders.begin());
            available_builders.erase(available_builders.begin());
        }
    }

    // Request new builders if slots are open
    // Care must be taken as all_builders may include a builder from a pending request that hasn't completely returned yet
    // that is to say open_slots, open_available_slots may be negative
    int open_slots = max_builders - all_builders.size() - pending_requests;
    int open_available_slots = max_available_builders - available_builders.size() - pending_requests;
    int request_count = std::min(open_slots, open_available_slots);
    for (int i = 0; i < request_count; i++) {
        pending_requests++;
        asio::spawn(io_context,
                    [this](asio::yield_context request_yield) {
                        boost::system::error_code create_error;
                        OpenStackBuilder::request_create(io_context, request_yield, create_error);
                        if (create_error) {
                            logger::write("Error requesting builder create " + create_error.message());
                        } else {
                            pending_requests--;
                        }
                    });
    }
}