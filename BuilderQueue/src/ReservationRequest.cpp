#include "ReservationRequest.h"

BuilderData ReservationRequest::async_wait(asio::yield_context yield, std::error_code &error) {
    reservation.async_wait(yield, error);
    return reservation.builder.get();
}