#include <boost/asio/spawn.hpp>
#include "BuilderQueue.h"
#include "OpenStack.h"
#include "Logger.h"

void BuilderQueue::return_builder(BuilderData builder) {
    std::make_shared<OpenStack>(io_context)->destroy(builder, [this, builder](std::error_code error) {
        if (!error) {
            active_builders.remove(builder);
            create_reserve_builders();
        }
    });
}

void BuilderQueue::process_pending_handler() {
    Logger::info("Processing pending handlers");

    if (!pending_handlers.empty() && !reserve_builders.empty()) {
        // Get the next builder and handler in the queue
        auto builder = reserve_builders.front();
        auto handler = pending_handlers.front();

        // Remove them from the queue
        reserve_builders.pop_front();
        pending_handlers.pop_front();

        // Invoke the handler to pass the builder to the connection
        io_context.post(std::bind(handler,builder));

        Logger::info("Providing builder to client: " + builder.id);

        // Attempt to create a new reserve builder if required
        create_reserve_builders();
    }
}

void BuilderQueue::create_reserve_builders() {
    // Attempt to create the required number of reserve builders while staying below the total allowed builder count
    auto potential_reserve_count = reserve_builders.size() + outstanding_create_count;

    if (potential_reserve_count < max_reserve_builder_count) {
        auto request_count = std::min(max_reserve_builder_count - potential_reserve_count, max_builder_count);

        outstanding_create_count += request_count;

        Logger::info("Attempting to create " + std::to_string(request_count) + " builders");

        for (std::size_t i = 0; i < request_count; i++) {
            Logger::info("Attempting to create builder " + std::to_string(i));

            std::make_shared<OpenStack>(io_context)->request_create([this, i](std::error_code error, BuilderData builder) {
                outstanding_create_count--;
                if (!error) {
                    Logger::info("Created builder " + std::to_string(i) + ": " + builder.id);
                    reserve_builders.push_back(builder);
                    process_pending_handler();
                } else {
                    Logger::error("Error creating builder " + std::to_string(i));
                }
            });
        }
    }
}