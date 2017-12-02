#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/optional.hpp>
#include "Builder.h"

namespace asio = boost::asio;

enum class ReservationStatus {
    pending,
    active,
    complete
};

// Reservations are handled by the queue and assigned builders as available
class Reservation {
public:
    explicit Reservation(asio::io_service& io_service) : status(ReservationStatus::pending),
                                                         ready_timer(io_service) {}

    ~Reservation() {
        status = ReservationStatus::complete;
    }

    // Create an infinite timer that will be cancelled by the queue when the job is ready
    void async_wait(asio::yield_context yield);

    // Callback used by BuilderQueue to cancel the timer which signals our reservation is ready
    void ready(Builder acquired_builder);

    bool pending() const {
        return status == ReservationStatus::pending;
    }

    bool complete() const {
        return status == ReservationStatus::complete;
    }

    boost::optional<Builder> builder;
    ReservationStatus status;
private:
    asio::deadline_timer ready_timer;
};