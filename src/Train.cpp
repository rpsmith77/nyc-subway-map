#include "Train.h"
#include <cmath>

Train::Train() : routeId(""), arrivalTime(0) {}

Train::Train(const std::string& routeId, time_t arrivalTime)
    : routeId(routeId), arrivalTime(arrivalTime) {}

bool Train::atStation(time_t currentTime) const {
    return std::difftime(currentTime, arrivalTime) >= 0 &&
           std::difftime(currentTime, arrivalTime) <= arrivalWindowSeconds;
}