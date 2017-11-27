#include "Reservation.h"

void Reservation::async_wait(asio::yield_context yield) {
    // When entered into the queue the queue will tick and possibly expire call this->ready()
    // That is the timer will be potentially be fired before async_wait() is called
    // If the timer is expired async_wait will deadlock so we take care to only call it on a valid timer
    if (!active) {
        ready_timer.expires_at(boost::posix_time::pos_infin);
        // On timer cancel we will get an operation aborted error from async_wait
        boost::system::error_code ec;
        ready_timer.async_wait(yield[ec]);
        if (ec != asio::error::operation_aborted) {
            throw std::system_error(EBADMSG, std::system_category());
        }
    }
}

void Reservation::ready(Builder acquired_builder) {
    builder = acquired_builder;
    active = true;
    ready_timer.cancel();
}