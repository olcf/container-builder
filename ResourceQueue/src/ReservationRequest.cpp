#include "ReservationRequest.h"

Builder ReservationRequest::async_wait() {
    reservation.async_wait(yield);
    return reservation.builder;
}