#include "MTAManager.h"
#include "GeneratedStationMap.h"
#include <ArduinoJson.h>
#include "LEDManager.h"
#include <set>
#include "SubwayColors.h"
#include <ArduinoWebsockets.h>
#include "Station.h"

SubwayColorMap MtaManager::colorMap;

static time_t parseIso8601(const char* str) {
    struct tm tm = {0};
    tm.tm_year = atoi(str) - 1900;
    tm.tm_mon = atoi(str + 5) - 1;
    tm.tm_mday = atoi(str + 8);
    tm.tm_hour = atoi(str + 11);
    tm.tm_min = atoi(str + 14);
    tm.tm_sec = atoi(str + 17);
    tm.tm_isdst = -1;
    return mktime(&tm);
}

void MtaManager::parseData(websockets::WebsocketsMessage msg) {
  StaticJsonDocument<200> filter;
  filter["data"][0]["id"] = true;
  filter["data"][0]["N"][0]["route"] = true;
  filter["data"][0]["N"][0]["time"] = true;
  filter["data"][0]["S"][0]["route"] = true;
  filter["data"][0]["S"][0]["time"] = true;

  doc.clear();
  
  DeserializationError error = deserializeJson(doc, msg.data(), DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  float memoryKB = doc.memoryUsage() / 1024.0;
  Serial.print("JSON Doc Memory Usage: ");
  Serial.print(memoryKB, 1);
  Serial.println(" KB");

  JsonArray stations = doc["data"].as<JsonArray>();
  time_t now;
  time(&now);

  for (JsonObject stationObj : stations) {
    handleStationUpdate(stationObj, now);
  }
}

void MtaManager::checkArrivals() {
#ifdef DEBUG
  static std::map<int, std::set<std::string>> trainsAtStationLast;
#endif  
  time_t currentTime;
  time(&currentTime);

  for (auto &pair : stationMap) {
    LEDManager::leds[pair.first] = CRGB::Black;
    Station &station = pair.second;
    std::set<std::string> trainsNow;

    for (Train &train : station.trains) {
      if (train.atStation(currentTime)) {
        LEDManager::leds[pair.first] = colorMap.getColor(train.routeId);
        trainsNow.insert(train.routeId);

#ifdef DEBUG
        if (trainsAtStationLast[pair.first].count(train.routeId) == 0) {
          Serial.printf(
            "Train %s ENTERED station %s (ID: %s) at %s",
            train.routeId.c_str(),
            station.name.c_str(),
            station.id.c_str(),
            ctime(&train.arrivalTime)
          );
        }
#endif
      }
    }
#ifdef DEBUG
    trainsAtStationLast[pair.first] = trainsNow;
#endif
  }
}

Station* MtaManager::findStationById(const std::string& id) {
    for (auto& pair : stationMap) {
        if (pair.second.id == id) {
            return &pair.second;
        }
    }
    return nullptr;
}

void MtaManager::purgeExpiredTrains() {
  time_t now;
  time(&now);
  for (auto &pair : stationMap) {
    Station &station = pair.second;
    station.trains.erase(
      std::remove_if(
        station.trains.begin(),
        station.trains.end(),
        [now](const Train& t){ return t.arrivalTime < now - 30.1; }
      ),
      station.trains.end()
    );
  }
}

void MtaManager::addNewTrains(Station& station, JsonArray arr, time_t now) {
  for (JsonObject train : arr) {
    const char* timeStr = train["time"].as<const char*>();
    if (!timeStr) continue;

    Train t;
    t.routeId = train["route"].as<std::string>();
    t.arrivalTime = parseIso8601(timeStr);

    // Reject trains more than 30s old or over 5 minutes ahead.
    double timeDiff = difftime(t.arrivalTime, now);
    if (timeDiff > 300 || timeDiff < -30.1) {
#ifdef DEBUG
      Serial.printf("Skipping train station=%s route=%s diff=%.1fs arrival=%ld now=%ld\n",
                    station.id.c_str(), t.routeId.c_str(), timeDiff,
                    static_cast<long>(t.arrivalTime), static_cast<long>(now));
#endif
      continue;
    }


    bool exists = false;
    for (const auto& existing : station.trains) {
      if (existing.routeId == t.routeId && existing.arrivalTime == t.arrivalTime) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      station.trains.push_back(t);
    }
  }
}

void MtaManager::handleStationUpdate(JsonObject stationObj, time_t now) {
  std::string jsonId = stationObj["id"].as<std::string>();
  Station* station = findStationById(jsonId);
  if (station) {
    if (stationObj.containsKey("N")) addNewTrains(*station, stationObj["N"].as<JsonArray>(), now);
    if (stationObj.containsKey("S")) addNewTrains(*station, stationObj["S"].as<JsonArray>(), now);
  }
}

bool MtaManager::hasAnyTrainData() {
  for (const auto& pair : stationMap) {
    const Station& station = pair.second;
    if (!station.trains.empty()) {
      return true;
    }
  }
  return false;
}