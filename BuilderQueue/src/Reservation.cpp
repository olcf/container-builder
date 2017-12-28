#include "Reservation.h"
#include "Logger.h"

void Reservation::async_wait(asio::yield_context yield, boost::system::error_code &error) {
    // The queue may tick and call ready() before async_wait() is called
    // That is the timer will be potentially be fired before async_wait() is called
    // If the timer is expired async_wait will deadlock so we take care to only call it on a valid timer
    if (status == ReservationStatus::pending) {
        ready_timer.expires_at(boost::posix_time::pos_infin);
        // On timer cancel we will get an operation aborted error from async_wait
        ready_timer.async_wait(yield[error]);
        if (error != asio::error::operation_aborted) {
            logger::write("Error in reservation async_wait" + error.message());
            return;
        }
    }
}

void Reservation::ready(BuilderData acquired_builder) {
    status = ReservationStatus::active;
    builder = acquired_builder;
    ready_timer.cancel();
}