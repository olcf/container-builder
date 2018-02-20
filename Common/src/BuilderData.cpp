#include "BuilderData.h"

bool operator<(const BuilderData &lhs, const BuilderData &rhs) {
    return lhs.id < rhs.id;
}

bool operator==(const BuilderData &lhs, const BuilderData &rhs) {
    return lhs.id == rhs.id;
}