#include <boost/asio/spawn.hpp>
#include "BuilderQueue.h"
#include "OpenStack.h"


void BuilderQueue::fetch_builder(FetchHandler&& handler) {
    pending_handlers.emplace_back(handler);
    process_pending_handlers();
}

void BuilderQueue::return_builder(BuilderData builder) {
    OpenStack::destroy(builder, io_context, [this](std::error_code error){
        if(error) {
            // Bloop...not sure exactly what to do if we have a "hung" builder
        }
        active_builders.remove(builder);
        create_reserve_builders();
    });
}

void BuilderQueue::process_pending_handler() {
    if(!pending_handlers.empty() && !reserve_builders.empty()) {
        // Get the next builder and handler in the queue
        auto builder = reserve_builders.front();
        auto handler = pending_handlers.front();

        // Remove them from the queue
        reserve_builders.pop_front();
        pending_handlers.pop_front();

        // Invoke the handler to pass the builder to the connection
        io_context.post(handler(builder));

        // Attempt to create a new reserve builder if required
        create_reserve_builders();
    }
}

void BuilderQueue::create_reserve_builders() {
    // Attempt to create the required number of reserve builders while staying below the total allowed builder count
    auto potential_reserve_count = reserve_builders.size() + outstanding_create_count;

    if(potential_reserve_count < max_reserve_builder_count) {
        auto request_count = std::min(max_reserve_builder_count - potential_reserve_count, max_builder_count);

        outstanding_create_count += request_count;

        for(i=0; i<request_count; i++) {
            OpenStack::request_create(io_context, [this](std::error_code error, BuilderData builder) {
                outstanding_create_count--;
                if(!error) {
                    reserve_builders.push_back(builder);
                    process_next_handler();
                }
            });
        }
    }
}