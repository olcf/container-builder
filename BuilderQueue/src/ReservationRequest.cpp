#include "ReservationRequest.h"

BuilderData ReservationRequest::async_wait() {
    reservation.async_wait(client);
    return reservation.builder.get();
}