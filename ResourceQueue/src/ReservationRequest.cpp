#include "ReservationRequest.h"

Resource ReservationRequest::async_wait(asio::yield_context yield) {
    reservation.async_wait(yield);
    return reservation.resource;
}