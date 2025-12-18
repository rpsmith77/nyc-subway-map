#ifndef MTAMANAGER_H
#define MTAMANAGER_H

#include <string>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "Station.h"
#include "SubwayColors.h"

class MtaManager {
public:
    static void parseData(websockets::WebsocketsMessage msg);
    static void checkArrivals();
    static Station* findStationById(const std::string& id);
    static void purgeExpiredTrains();
    static void addNewTrains(Station& station, JsonArray arr);
    static void handleStationUpdate(JsonObject stationObj, time_t now);
    static bool hasAnyTrainData();
private:
    static SubwayColorMap colorMap;
    static constexpr size_t jsonSize = 100U * 1024U;
    static inline StaticJsonDocument<jsonSize> doc;
};

#endif // MTAMANAGER_H