#include "ReservationRequest.h"

Resource ReservationRequest::async_wait() {
    reservation.async_wait(yield);
    return reservation.resource;
}