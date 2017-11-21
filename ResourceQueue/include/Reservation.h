#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "Resource.h"

namespace asio = boost::asio;

// Reservations are handled by the queue and assigned resources as available
class Reservation {
public:
    explicit Reservation(tcp::socket &socket) : socket(socket),
                                                active(false),
                                                ready_timer(socket.get_io_service())
    {}

    // Create an infinite timer that will be cancelled by the queue when the job is ready
    void async_wait(asio::yield_context yield);

    // Callback used by ResourceQueue to cancel the timer which signals our reservation is ready
    void ready(Resource acquired_resource);

    Resource resource;
    tcp::socket &socket;
    bool active;
private:
    asio::deadline_timer ready_timer;
};
