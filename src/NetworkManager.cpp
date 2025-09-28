#include "NetworkManager.h"
#include <Arduino.h>
#include "MTAManager.h"
#include <WiFi.h>

NetworkManager::NetworkManager(const char* ssid, const char* password, const char* host, const char* port)
    : ssid(ssid), password(password), host(host), port(port), wifiConnectionStatus(false), maxBackoffMs(30000){
}

void NetworkManager::initializeWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);          // prevent modem sleep latency spikes
  WiFi.setAutoReconnect(true);   // retry automatically
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  unsigned long start = millis();
  // Wait for WiFi, but cap at 20s so setup doesn't block forever if AP is unreachable.
  // Exiting lets the main loop handle retries/backoff without stalling the device.
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    Serial.print(".");
    yield();
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
  } else {
    Serial.println("\nWiFi connect timeout, continuing and will retry in loop...");
  }
}

bool NetworkManager::checkWifiConnection() {
  static unsigned long lastAttempt = 0;
  static unsigned long backoffMs = 1000;
  static int failedAttempts = 0;

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnectionStatus) {
      wifiConnectionStatus = true;
      Serial.println("Connected");
    }
    backoffMs = 1000;
    failedAttempts = 0;
    return true;
  }

  // Disconnected
  if (wifiConnectionStatus) {
    Serial.println("WiFi lost");
  }
  wifiConnectionStatus = false;

  unsigned long now = millis();
  if (now - lastAttempt >= backoffMs) {
    Serial.println("Attempting WiFi reconnect...");
    WiFi.disconnect(true);
    delay(50);
    WiFi.begin(ssid, password);
    lastAttempt = now;
    backoffMs = min(backoffMs * 2, maxBackoffMs);

    if (backoffMs == maxBackoffMs) {
      failedAttempts++;
      if (failedAttempts >= 5) {
        Serial.println("Max WiFi reconnect attempts reached. Rebooting ESP32...");
        ESP.restart();
      }
    }
  }
  return false;
}

void NetworkManager::initializeWebsocket() {
  wsClient.onMessage([this](websockets::WebsocketsMessage msg) {
    this->onWebSocketMessage(msg);
  });
  wsClient.onEvent([](websockets::WebsocketsEvent e, String data){
    if (e == websockets::WebsocketsEvent::ConnectionOpened) Serial.println("WS opened");
    if (e == websockets::WebsocketsEvent::ConnectionClosed) Serial.println("WS closed");
#ifdef DEBUG
    if (e == websockets::WebsocketsEvent::GotPing) Serial.println("WS ping");
    if (e == websockets::WebsocketsEvent::GotPong) Serial.println("WS pong");
#endif
  });
  String wsUrl = "ws://" + String(host) + ":" + String(port) + "/ws";
  wsClient.connect(wsUrl);
}

bool NetworkManager::checkWebsocketConnection() {
  static unsigned long lastAttempt = 0;
  static unsigned long backoffMs = 1000;
  static unsigned long lastPing = 0;
  static bool wasConnected = false;
  static bool everConnected = false;

  wsClient.poll();

  if (!wsClient.available()) {
    wasConnected = false;

    unsigned long now = millis();
    if (now - lastAttempt >= backoffMs) {
      Serial.println("WS disconnected, attempting reconnect...");
      wsClient.close();
      delay(20);
      String wsUrl = "ws://" + String(host) + ":" + String(port) + "/ws";
      wsClient.connect(wsUrl);
      lastAttempt = now;
      backoffMs = min(backoffMs * 2, maxBackoffMs);
      lastPing = 0;
    }
    return false;
  } else {
    backoffMs = 1000;
    if (!wasConnected) {
      if (everConnected) {
        Serial.println("WS reconnected");
      } else {
        everConnected = true; // first connection, don't label as reconnect
      }
      wasConnected = true;
    }

    unsigned long now = millis();
    if (now - lastPing > 10000) { // ping every 10s
      wsClient.ping();
      lastPing = now;
    }
    return true;
  }
}

void NetworkManager::onWebSocketMessage(websockets::WebsocketsMessage msg) {
#ifdef DEBUG
  Serial.print("WebSocket message: ");
  Serial.println(msg.data());
#endif
  MtaManager::parseData(msg);
}

void NetworkManager::poll() {
  wsClient.poll();
  yield();
}