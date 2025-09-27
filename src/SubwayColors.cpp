#include "SubwayColors.h"

SubwayColorMap::SubwayColorMap() {
    routeColors = {
        {"1", Red},    {"2", Red},
        {"3", Red},    {"4", Green},
        {"5", Green},  {"6", Green},
        {"7", Purple}, {"7X", Purple},
        {"A", Blue},   {"B", Orange},
        {"C", Blue},   {"D", Orange},
        {"E", Blue},   {"F", Orange},
        {"FS", Gray},  {"G", Lime},
        {"H", Gray},   {"J", Brown},
        {"L", Gray},   {"M", Orange},
        {"N", Yellow}, {"Q", Yellow},
        {"R", Yellow}, {"S", Turquoise},
        {"SI", Gray},  {"W", Turquoise}
    };
}

SubwayColorMap::SubwayColor SubwayColorMap::getColor(const std::string& routeId) const {
    auto it = routeColors.find(routeId);
    if (it != routeColors.end()) {
        return it->second;
    }
    return Default;
}