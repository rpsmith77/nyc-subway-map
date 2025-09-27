#ifndef TRAIN_H
#define TRAIN_H

#include <string>
#include <ctime>

class Train {
public:
    Train();
    Train(const std::string& routeId, time_t arrivalTime);

    bool atStation(time_t currentTime) const;

    std::string routeId;
    time_t arrivalTime;
private:
    static const uint8_t arrivalWindowSeconds = 30;
};

#endif // TRAIN_H