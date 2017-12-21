#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/optional.hpp>
#include "Builder.h"

namespace asio = boost::asio;

enum class ReservationStatus {
    pending,             // Waiting on a builder to be provided by the queue
    active,              // Provided a builder
    request_complete,    // The queue request is complete and it's safe to cleanup any resources associated with reservation
    cleanup,             // post request complete cleanup has begun and the resources allocated to it are being cleaned up
    finalized            // post complete cleanup is complete and the reservation can be destroyed
};

// Reservations are handled by the queue and assigned builders as available
class Reservation {
public:
    explicit Reservation(asio::io_service& io_service) :
            status(ReservationStatus::pending),
            ready_timer(io_service) {}

    ~Reservation() {
        status = ReservationStatus::request_complete;
    }

    // Create an infinite timer that will be cancelled by the queue when the job is ready
    void async_wait(Messenger& client);

    // Callback used by BuilderQueue to cancel the timer which signals our reservation is ready
    void ready(BuilderData acquired_builder);

    bool pending() const {
        return status == ReservationStatus::pending;
    }

    bool request_complete() const {
        return status == ReservationStatus::request_complete;
    }

    bool active() const {
        return status == ReservationStatus::active;
    }


    bool finalized() const {
        return status == ReservationStatus::finalized;
    }

    void set_request_complete() {
        status = ReservationStatus::request_complete;
    }

    void set_enter_cleanup() {
        status = ReservationStatus::cleanup;
    }

    void set_finalize() {
        status = ReservationStatus::finalized;
    }

    boost::optional<BuilderData> builder;
    ReservationStatus status;
    boost::system::error_code error;
private:
    asio::deadline_timer ready_timer;
};