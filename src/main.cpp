#include <Arduino.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoWebsockets.h>
#include "MtaHelper.h"
#include "WifiCredentials.h"
#include "station_map.h"
#include <set>

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define NUM_LEDS_SUBWAY 500
#define DATA_PIN_SUBWAY 10  // D7

#define NUM_LEDS_ERROR 2
#define DATA_PIN_ERROR 5  // D2

CRGB leds[NUM_LEDS_SUBWAY];
CRGB error_leds[NUM_LEDS_ERROR];
bool wifi_connection(false);
websockets::WebsocketsClient wsClient;
unsigned long ws_last_attempt = 0;
unsigned long ws_backoff = 1000; // Start with 1 second
const unsigned long WS_MAX_BACKOFF = 32000; // Max 32 seconds
const int WS_MAX_ATTEMPTS = 10;
int ws_attempts = 0;

const int json_doc_size = 200 * 1024;

// function declarations
void WifiConnection();
void InitializeTime();
void CheckStationArrivals();
Station* FindStationById(const std::string& id);
void RemoveExpiredTrains(Station& station, time_t now);
void AddNewTrains(Station& station, JsonArray arr);
void HandleStationUpdate(JsonObject stationObj, time_t now);
void OnWebSocketMessage(websockets::WebsocketsMessage msg);
void CheckWebsocketConnection();

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  FastLED.addLeds<LED_TYPE, DATA_PIN_SUBWAY, COLOR_ORDER>(leds, NUM_LEDS_SUBWAY);
  FastLED.addLeds<LED_TYPE, DATA_PIN_ERROR, COLOR_ORDER>(error_leds, NUM_LEDS_ERROR);
  FastLED.setBrightness(5);

  InitializeTime();

  delay(3000);


  wsClient.onMessage(OnWebSocketMessage);
  wsClient.onEvent([](websockets::WebsocketsEvent e, String data){
#ifdef DEBUG
    if (e == websockets::WebsocketsEvent::ConnectionOpened) Serial.println("WS opened");
    if (e == websockets::WebsocketsEvent::ConnectionClosed) Serial.println("WS closed");
    if (e == websockets::WebsocketsEvent::GotPing) Serial.println("WS ping");
    if (e == websockets::WebsocketsEvent::GotPong) Serial.println("WS pong");
#endif
  });
  String wsUrl = "ws://" + String(SERVER_HOST.c_str()) + ":" + String(SERVER_PORT.c_str()) + "/ws";
  wsClient.connect(wsUrl); 
}

void loop() {
  WifiConnection();
  CheckWebsocketConnection();
  CheckStationArrivals();
  FastLED.show();
  wsClient.poll(); 
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

void CheckStationArrivals() {
#ifdef DEBUG
  static std::map<int, std::set<std::string>> trains_at_station_last;
#endif  
  time_t current_time;
  time(&current_time);

  for (auto &pair : stationMap) {
    leds[pair.first] = CRGB::Black;
    Station &station = pair.second;
    std::set<std::string> trains_now;

    for (Train &train : station.trains) {
      if (train.AtStation(current_time)) {
        leds[pair.first] = GetTrainColor(train);
        trains_now.insert(train.route_id);

#ifdef DEBUG
        // Print only if train was not at station in previous check
        if (trains_at_station_last[pair.first].count(train.route_id) == 0) {
          Serial.printf(
            "Train %s ENTERED station %s (ID: %s) at %s",
            train.route_id.c_str(),
            station.name.c_str(),
            station.id.c_str(),
            ctime(&train.arrival_time)
          );
        }
#endif
      }
    }
#ifdef DEBUG
    // Update the set for next check
    trains_at_station_last[pair.first] = trains_now;
#endif
  }
}

Station* FindStationById(const std::string& id) {
  for (auto& pair : stationMap) {
    if (pair.second.id == id) return &pair.second;
  }
  return nullptr;
}

void RemoveExpiredTrains(Station& station, time_t now) {
  station.trains.erase(
    std::remove_if(
      station.trains.begin(),
      station.trains.end(),
      [now](const Train& t) { return t.arrival_time < now - 60; }
    ),
    station.trains.end()
  );
}

void AddNewTrains(Station& station, JsonArray arr) {
  for (JsonObject train : arr) {
    Train t;
    t.route_id = train["route"].as<std::string>();
    struct tm tm;
    strptime(train["time"].as<const char*>(), "%Y-%m-%dT%H:%M:%S%z", &tm);
    t.arrival_time = mktime(&tm);

    bool exists = false;
    for (const auto& existing : station.trains) {
      if (existing.route_id == t.route_id && existing.arrival_time == t.arrival_time) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      station.trains.push_back(t);
    }
  }
}

void HandleStationUpdate(JsonObject stationObj, time_t now) {
  std::string json_id = stationObj["id"].as<std::string>();
  Station* station = FindStationById(json_id);
  if (station) {
    RemoveExpiredTrains(*station, now);
    if (stationObj.containsKey("N")) AddNewTrains(*station, stationObj["N"].as<JsonArray>());
    if (stationObj.containsKey("S")) AddNewTrains(*station, stationObj["S"].as<JsonArray>());
  }
}

void OnWebSocketMessage(websockets::WebsocketsMessage msg) {
#ifdef DEBUG
  Serial.print("WebSocket message: ");
  Serial.println(msg.data());
#endif

  DynamicJsonDocument doc(json_doc_size);
  DeserializationError error = deserializeJson(doc, msg.data());
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray stations = doc["data"].as<JsonArray>();
  time_t now;
  time(&now);

  for (JsonObject stationObj : stations) {
    HandleStationUpdate(stationObj, now);
  }
}

void CheckWebsocketConnection() {
  if (!wsClient.available()) {
    unsigned long now = millis();
    if (now - ws_last_attempt >= ws_backoff && ws_attempts < WS_MAX_ATTEMPTS) {
      Serial.printf("WebSocket disconnected, attempt #%d, reconnecting in %lu ms...\n", ws_attempts + 1, ws_backoff);
      String wsUrl = "ws://" + String(SERVER_HOST.c_str()) + ":" + String(SERVER_PORT.c_str()) + "/ws";
      wsClient.connect(wsUrl);
      ws_last_attempt = now;
      ws_attempts++;
      ws_backoff = min(ws_backoff * 2, WS_MAX_BACKOFF);
    }
  } else {
    // Reset backoff and attempts on successful connection
    ws_attempts = 0;
    ws_backoff = 1000;
    ws_last_attempt = millis();
  }
}