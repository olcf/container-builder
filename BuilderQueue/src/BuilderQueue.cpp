#include "BuilderQueue.h"
#include "OpenStack.h"
#include "boost/asio/deadline_timer.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

void BuilderQueue::return_builder(BuilderData builder) {
    Logger::info("Attempting to destroy builder: " + builder.id);
    std::make_shared<OpenStack>(io_context)->destroy(builder, [this, builder](std::error_code error) {
        if (!error) {
            Logger::info("Destroyed builder: " + builder.id);
            active_builders.remove(builder);
            create_reserve_builders();
        } else {
            Logger::info("Error destroying builder, retrying in 5 seconds: " + builder.id);
            asio::deadline_timer timer(io_context, boost::posix_time::seconds(5));
            timer.async_wait(std::bind(&BuilderQueue::return_builder, this, builder));
        }
    });
}

void BuilderQueue::process_pending_handler() {
    Logger::info("Processing " + std::to_string(pending_handlers.size()) +
                 " handlers with " + std::to_string(reserve_builders.size()) + " reserve builders");

    if (!pending_handlers.empty() && !reserve_builders.empty()) {
        Logger::info("Processing pending handler");

        // Get the next builder and handler in the queue
        auto builder = reserve_builders.front();
        auto handler = pending_handlers.front();

        // Add builder to active builder list
        active_builders.emplace_back(builder);

        // Remove the builder and handler from the queue
        reserve_builders.pop_front();
        pending_handlers.pop_front();

        Logger::info("Providing builder to client: " + builder.id);

        // Post the handler to pass the builder to the connection
        asio::post(io_context, std::bind(handler, builder));

        // Attempt to create a new reserve builder if required
        create_reserve_builders();
    }
}

void BuilderQueue::create_reserve_builders() {
    Logger::info(
            "Checking reserve builder count with " + std::to_string(reserve_builders.size()) + " reserve builders");

    // Attempt to create the required number of reserve builders while staying below the total allowed builder count
    auto potential_reserve_count = reserve_builders.size() + outstanding_create_count;
    auto potential_total_count = potential_reserve_count + active_builders.size();

    // If the reserve builder count isn't full request more
    if (potential_reserve_count < max_reserve_builder_count) {
        // Get the amount required to fill the reserve count
        auto reserve_request_count = max_reserve_builder_count - potential_reserve_count;

        // Make sure we stay under the maximum number of total builders
        auto request_count = std::min(reserve_request_count, max_builder_count - potential_total_count);

        outstanding_create_count += request_count;

        Logger::info("Attempting to create " + std::to_string(request_count) + " builders");

        for (std::size_t i = 0; i < request_count; i++) {
            Logger::info("Attempting to create builder " + std::to_string(i));

            std::make_shared<OpenStack>(io_context)->request_create(
                    [this, i](std::error_code error, BuilderData builder) {
                        outstanding_create_count--;
                        if (!error) {
                            Logger::info("Created builder " + std::to_string(i) + ": " + builder.id);
                            reserve_builders.push_back(builder);
                            process_pending_handler();
                        } else {
                            Logger::error("Error creating builder, retrying in five seconds: " + std::to_string(i));
                            asio::deadline_timer timer(io_context, boost::posix_time::seconds(5));
                            timer.async_wait(std::bind(&BuilderQueue::create_reserve_builders, this));
                        }
                    });
        }
    }
}