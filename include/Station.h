#ifndef STATION_H
#define STATION_H

#include <string>
#include <vector>
#include "Train.h"

class Station {
public:
    Station();
    Station(const std::string& id, const std::string& name);

    std::string id;
    std::string name;
    std::vector<Train> trains;
};

#endif // STATION_H