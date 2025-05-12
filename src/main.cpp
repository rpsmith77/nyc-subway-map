#include <Arduino.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

#include "MtaHelper.h"
#include "WifiCredentials.h"
#include "station_map.h"

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define NUM_LEDS_SUBWAY 500
#define DATA_PIN_SUBWAY 10  // D7

#define NUM_LEDS_ERROR 2
#define DATA_PIN_ERROR 5  // D2

CRGB leds[NUM_LEDS_SUBWAY];
CRGB error_leds[NUM_LEDS_ERROR];
bool wifi_connection(false);
HTTPClient http;

// Batch HTTP requests to avoid halting the program for more than .5 seconds per
// fetch
int current_station(0);
int current_group(0);
const int num_groups = 45;
int group_size = stationMap.size() / num_groups;
// ensure all groups are fetched within 1 minute
const float fetch_interval = 60 / num_groups;
const int json_doc_size = 5 * 1024;

#ifdef DEBUG
size_t largest_memory_usage = 0;
#endif

// Global counter for HTTP errors, fail if 10% of requests fail in a row.
int http_error_count = 0;
const int http_error_threshold = stationMap.size() * 0.1;

// function declarations
void WifiConnection();
void InitializeTime();
void FetchSubwayData(Station &station);
void FetchStationGroup();
void CheckStationArrivals();
void ParseTrains(JsonArray trains_array, std::vector<Train> &trains);
void CheckHeap();
void CheckHttpErrors();
bool IsServerReachable(const char *host, uint16_t port);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");

  FastLED.addLeds<LED_TYPE, DATA_PIN_SUBWAY, COLOR_ORDER>(leds,
                                                          NUM_LEDS_SUBWAY);
  FastLED.addLeds<LED_TYPE, DATA_PIN_ERROR, COLOR_ORDER>(error_leds,
                                                         NUM_LEDS_ERROR);
  FastLED.setBrightness(5);

  InitializeTime();

  delay(3000);

  Serial.println("Initializing Stations, should take about 15-20 seconds...");
  for (auto &pair : stationMap) {
    Serial.printf("Fetching data for station %s\n", pair.second.name.c_str());
    FetchSubwayData(pair.second);
  }
}

void loop() {
  WifiConnection();

  EVERY_N_BSECONDS(fetch_interval) { FetchStationGroup(); }

  CheckStationArrivals();

  FastLED.show();

  EVERY_N_BSECONDS(1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); }
}

// put function definitions here:
void WifiConnection() {
  if (!wifi_connection && WiFi.status() == WL_CONNECTED) {
    wifi_connection = true;
    error_leds[0] = CRGB::White;
    Serial.println("Connected");
  }
  if (WiFi.status() != WL_CONNECTED) {
    wifi_connection = false;
    error_leds[0] = error_leds[0] == CRGB::Red ? CRGB::Black : CRGB::Red;
    delay(500);
  }
}

void InitializeTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Set the POSIX timezone string for US Eastern Time.
  // EST5EDT: Standard time is EST (UTC-5), Daylight time is EDT.
  // M3.2.0/2: DST starts on the second Sunday of March at 2 AM.
  // M11.1.0/2: DST ends on the first Sunday of November at 2 AM.
  setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();

  Serial.println("Attempting to initialize time with TZ string for automatic DST...");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time after setting TZ environment variable.");
    return;
  }
  Serial.println(&timeinfo, "Time initialized: %A, %B %d %Y %H:%M:%S %Z (%z)");
}

void ParseTrains(JsonArray trains_array, std::vector<Train> &trains) {
  for (JsonObject train : trains_array) {
    Train t;
    t.route_id = train["route"].as<std::string>();
    struct tm tm;
    strptime(train["time"].as<const char *>(), "%Y-%m-%dT%H:%M:%S%z", &tm);
    t.arrival_time = mktime(&tm);
    trains.push_back(t);
  }
}

void FetchSubwayData(Station &station) {
  std::string url = API_BY_ID + station.id;
  http.begin(url.c_str());
  int http_code = http.GET();
  if (http_code > 0) {
    String payload = http.getString();

    DynamicJsonDocument doc(json_doc_size);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      doc.shrinkToFit();

#ifdef DEBUG
      size_t memory_usage = doc.memoryUsage();
      if (memory_usage > largest_memory_usage) {
        largest_memory_usage = memory_usage;
      }
#endif

      station.trains.clear();
      ParseTrains(doc["data"][0]["N"], station.trains);
      ParseTrains(doc["data"][0]["S"], station.trains);
    } else {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
  } else {
    // Get current time for error logging
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Print timestamp with error information
    Serial.print("[");
    Serial.print(&timeinfo, "%Y-%m-%d %H:%M:%S");
    Serial.print("]\nError on HTTP request, code: ");
    Serial.println(http_code);
    Serial.print("Error message: ");
    Serial.println(http.errorToString(http_code).c_str());
    http_error_count++;
  }
  http.end();
}

void CheckStationArrivals() {
  time_t current_time;
  time(&current_time);
  for (auto &pair : stationMap) {
    leds[pair.first] = CRGB::Black;
    Station &station = pair.second;
    for (Train &train : station.trains) {
      if (train.AtStation(current_time)) {
        leds[pair.first] = GetTrainColor(train);
      }
    }
  }
}

void FetchStationGroup() {
  int start_index = current_group * group_size;
  int end_index = (current_group + 1) * group_size;
  // Ensure the last group includes any remaining stations
  if (current_group == num_groups - 1) {
    end_index = stationMap.size();
  }

  auto it = stationMap.begin();
  std::advance(it, start_index);
  for (int i = start_index; i < end_index && it != stationMap.end();
       ++i, ++it) {
    FetchSubwayData(it->second);
  }

  current_group = (current_group + 1) % num_groups;

  CheckHeap();
  CheckHttpErrors();
}

void CheckHeap() {
  size_t free_heap = ESP.getFreeHeap();
#ifdef DEBUG
  Serial.printf("Largest memory usage: %d\n", largest_memory_usage);
  Serial.printf("Free heap: %d\n", free_heap);
#endif
  if (free_heap < 1.25 * json_doc_size) {
    Serial.println(
        "Heap size is below 1.25 times the JSON document size, rebooting...");
    ESP.restart();
  }
}

void CheckHttpErrors() {
  if (http_error_count == 0) {
    return;
  }
  if (http_error_count > http_error_threshold && wifi_connection &&
      IsServerReachable(SERVER_HOST.c_str(), std::stoi(SERVER_PORT))) {
    Serial.println("HTTP error threshold exceeded, taking action...");
    ESP.restart();
  }
  http_error_count = 0;
}

bool IsServerReachable(const char *host, uint16_t port) {
  WiFiClient client;
  if (client.connect(host, port)) {
    client.stop();
    return true;
  } else {
    return false;
  }
}
