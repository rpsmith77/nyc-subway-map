#ifndef MTAHELPER_H
#define MTAHELPER_H

#include <string>
#include <map>
#include <ctime>
#include <vector>

struct Train {
    std::string routeId;
    time_t arrivalTime;

    Train() {
        this->routeId = "";
        this->arrivalTime = 0;
    }

    bool atStation(time_t currentTime) const {
        return std::difftime(currentTime, arrivalTime) >= 0 && std::difftime(currentTime, arrivalTime) <= 30;
    }
};

struct Station {
    std::string id;
    std::string name;
    std::vector<Train> trains;

    Station(std::string id, std::string name) {
        this->id = id;
        this->name = name;
    }
    Station() : id(""), name("") {}
};

enum SubwayColor : uint32_t {
  Blue = 0x0039a6,
  Orange = 0xff6319,
  Lime = 0x6cbe45,
  Gray = 0xa7a9ac,
  Brown = 0x7B3F00,
  Yellow = 0xfccc0a,
  Red = 0xee352e,
  Green = 0x00933c,
  Purple = 0xb933ad,
  Turquoise = 0x00add0,
  Default = 0xffffff
};

static std::map<std::string, SubwayColor> RouteColors = {
    {"1", SubwayColor::Red},
    {"2", SubwayColor::Red},
    {"3", SubwayColor::Red},
    {"4", SubwayColor::Green},
    {"5", SubwayColor::Green},
    {"6", SubwayColor::Green},
    {"7", SubwayColor::Purple},
    {"7X", SubwayColor::Purple},
    {"A", SubwayColor::Blue},
    {"B", SubwayColor::Orange},
    {"C", SubwayColor::Blue},
    {"D", SubwayColor::Orange},
    {"E", SubwayColor::Blue},
    {"F", SubwayColor::Orange},
    {"FS", SubwayColor::Gray},
    {"G", SubwayColor::Lime},
    {"H", SubwayColor::Gray},
    {"J", SubwayColor::Brown},
    {"L", SubwayColor::Gray},
    {"M", SubwayColor::Orange},
    {"N", SubwayColor::Yellow},
    {"Q", SubwayColor::Yellow},
    {"R", SubwayColor::Yellow},
    {"S", SubwayColor::Turquoise},
    {"SI", SubwayColor::Gray},
    {"W", SubwayColor::Turquoise}
  };

SubwayColor getTrainColor(const Train& train) {
    auto it = RouteColors.find(train.routeId);
    if (it != RouteColors.end()) {
        return it->second;
    } else {
        return SubwayColor::Default;
    }
}

#endif // MTAHELPER_H