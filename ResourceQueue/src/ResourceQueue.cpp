#include "ResourceQueue.h"
#include <iostream>
#include "Logger.h"
#include "boost/process.hpp"

// TODO : Refactor this so the request handling is more decoupled

namespace bp = boost::process;

// Request for a new builder to be brought up, if successful when the builder is ready add_resource() will be called and the queue will tick
void request_new_builder() {
    bp::ipstream out_stream;
    bp::system("/home/queue/BringUpBuilder", (bp::std_out & bp::std_err) > out_stream);
    std::string output_string;
    out_stream >> output_string;
};

void request_destroy_builder() {
    bp::ipstream out_stream;
    bp::system("openstack #builder_ID", (bp::std_out & bp::std_err) > out_stream);
    std::string output_string;
    out_stream >> output_string;

    // Tick as we may have had a request come in between requesting shutdown and actually being shutdown
    tick();
};

void ResourceQueue::enter(Reservation *reservation) {
    logger::write(reservation->socket, "Entering queue");
    pending_queue.push_back(reservation);

    // Tick as we may have available builders
    tick();
}

void ResourceQueue::exit(Reservation *reservation) noexcept {
    logger::write(reservation->socket, "Exiting queue");

    // If exit is called on an outstanding reservation remove it from the queue
    auto pending_position = std::find(pending_queue.begin(), pending_queue.end(), reservation);
    if (pending_position != pending_queue.end())
        pending_queue.erase(pending_position);
}

void ResourceQueue::add_resource(Resource resource) {
    available_resources.push_back(resource);

    // Tick to process pending requests
    tick();
}

void ResourceQueue::remove_resource(Resource resource) {
    // Remove the resource from the list of available resources
    auto resource_position = std::find(available_resources.begin(), available_resources.end(), resource);
    if (resource_position != available_resources.end())
        available_resources.erase(resource_position);

    // Shut down the builder
    request_destroy_builder(resource.id);
}

// If there is a pending build request either give it a resource or make a request for a new resource to be created
void ResourceQueue::tick() {
    if (!pending_queue.empty()) {
        // If r
        if(!available_resources.empty()) {
            // Grab next pending reservation and remove it from queue
            auto reservation = pending_queue.front();
            pending_queue.pop_front();

            // Grab an available resource and remove it from list of available resources
            auto resource = available_resources.front();
            remove_resource(resource);

            // Invoke the reservation callback to inform the request it's ready to run
            reservation->ready(resource);
        } else {
            // If requests are pending but no builders are available try to create a new builder
            request_new_builder();
        }
    }
}