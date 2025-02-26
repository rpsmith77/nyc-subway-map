#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include "WifiCredentials.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "MtaHelper.h"
#include <time.h>
#include "station_map.h"

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define NUM_LEDS_SUBWAY 500
#define DATA_PIN_SUBWAY 10 // D7

#define NUM_LEDS_ERROR 2
#define DATA_PIN_ERROR 5 // D2

CRGB leds[NUM_LEDS_SUBWAY];
CRGB error_leds[NUM_LEDS_ERROR];
bool wifiConnection(false);

// Batch HTTP requests to avoid halting the program for more than .5 seconds per fetch
int currentStation(0);
int currentGroup(0);
const int NUM_GROUPS = 45;
int groupSize = stationMap.size() / NUM_GROUPS;
const float FETCH_INTERVAL = 60 / NUM_GROUPS;
const int JSON_DOC_SIZE = 5 * 1024;

// Global variable to store the largest memory usage
size_t largestMemoryUsage = 0;

// function declarations
void wifi_connection();
void initialize_time();
void fetch_subway_data(Station& station);
void fetch_station_group();
void checkStationArrivals();
void parse_trains(JsonArray trainsArray, std::vector<Train>& trains);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");

  FastLED.addLeds<LED_TYPE, DATA_PIN_SUBWAY, COLOR_ORDER>(leds, NUM_LEDS_SUBWAY);
  FastLED.addLeds<LED_TYPE, DATA_PIN_ERROR, COLOR_ORDER>(error_leds, NUM_LEDS_ERROR);
  FastLED.setBrightness(5);

  initialize_time();

  delay(3000);

  Serial.println("Initializing Stations, should take about 15-20 seconds...");
  for (auto& pair : stationMap) {
    Serial.printf("Fetching data for station %s\n", pair.second.name.c_str());
    fetch_subway_data(pair.second);
  }
}

void loop() {
  wifi_connection();

  EVERY_N_BSECONDS(FETCH_INTERVAL) {
    fetch_station_group();
  }

  checkStationArrivals();

  FastLED.show();
  
  EVERY_N_BSECONDS(1) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

}

// put function definitions here:
void wifi_connection() {
  if (!wifiConnection && WiFi.status() == WL_CONNECTED) {
    wifiConnection = true;
    error_leds[0] = CRGB::White;
    Serial.println("Connected");
  }
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnection = false;
    error_leds[0] = error_leds[0] == CRGB::Red ? CRGB::Black : CRGB::Red;
    delay(500);
  }
}

void initialize_time() {
  // Set timezone to Eastern Standard Time (EST)
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // Wait for time to be set
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "Time initialized: %A, %B %d %Y %H:%M:%S");
}

void parse_trains(JsonArray trainsArray, std::vector<Train>& trains) {
  for (JsonObject train : trainsArray) {
    Train t;
    t.routeId = train["route"].as<std::string>();
    struct tm tm;
    strptime(train["time"].as<const char*>(), "%Y-%m-%dT%H:%M:%S%z", &tm);
    t.arrivalTime = mktime(&tm);
    trains.push_back(t);
  }
}

void fetch_subway_data(Station& station) {
  HTTPClient http;
  std::string url = API_BY_ID + station.id;
  http.begin(url.c_str());
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    // Update the largest memory usage
    size_t memoryUsage = doc.memoryUsage();
    if (memoryUsage > largestMemoryUsage) {
      largestMemoryUsage = memoryUsage;
    }

    station.trains.clear();
    parse_trains(doc["data"][0]["N"], station.trains);
    parse_trains(doc["data"][0]["S"], station.trains);
  } else {
    Serial.println("Error on HTTP request");
  }
  http.end();
}

void checkStationArrivals() {
  time_t currentTime;
  time(&currentTime);
  for (auto &pair : stationMap) {
    leds[pair.first] = CRGB::Black;
    Station &station = pair.second;
    for (Train &train : station.trains) {
      if (train.atStation(currentTime)) {
        leds[pair.first] = getTrainColor(train);
      }
    }
  }
}

void fetch_station_group() {
  int startIndex = currentGroup * groupSize;
  int endIndex = (currentGroup + 1) * groupSize;
  if (currentGroup == NUM_GROUPS - 1) {
    endIndex = stationMap.size(); // Ensure the last group includes any remaining stations
  }

  auto it = stationMap.begin();
  std::advance(it, startIndex);
  for (int i = startIndex; i < endIndex && it != stationMap.end(); ++i, ++it) {
    fetch_subway_data(it->second);
  }

  currentGroup = (currentGroup + 1) % NUM_GROUPS;

  // Print heap size after fetching data
  size_t freeHeap = ESP.getFreeHeap();
  Serial.print("Heap size after fetch: ");
  Serial.println(freeHeap);

  // Print the largest memory usage
  Serial.print("Largest memory usage: ");
  Serial.println(largestMemoryUsage);

  // Reboot the board if the heap size is below 1.25 times the JSON document size
  if (freeHeap < 1.25 * JSON_DOC_SIZE) {
    Serial.println("Heap size is below 1.25 times the JSON document size, rebooting...");
    ESP.restart();
  }
}
