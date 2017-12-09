#include "Builder.h"

bool operator <(const Builder &lhs, const Builder &rhs) {
    return lhs.id < rhs.id;
}

bool operator ==(const Builder &lhs, const Builder &rhs) {
    return lhs.id < rhs.id;
}