#include "ReservationRequest.h"

Builder ReservationRequest::async_wait(asio::yield_context yield) {
    reservation.async_wait(yield);
    return reservation.builder.get();
}