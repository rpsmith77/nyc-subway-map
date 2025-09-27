#include "NetworkManager.h"
#include <Arduino.h>
#include "MTAManager.h"

NetworkManager::NetworkManager(const char* ssid, const char* password, const char* host, const char* port)
    : ssid(ssid), password(password), host(host), port(port), wifiConnectionStatus(false), maxBackoffMs(30000){
}

void NetworkManager::initializeWifi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
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
    failedAttempts = 0; // Reset on success
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

  if (!wsClient.available()) {
    unsigned long now = millis();
    if (now - lastAttempt >= backoffMs) {
      Serial.println("WS disconnected, attempting reconnect...");
      String wsUrl = "ws://" + String(host) + ":" + String(port) + "/ws";
      wsClient.connect(wsUrl);
      lastAttempt = now;
      backoffMs = min(backoffMs * 2, maxBackoffMs);
    }
    return false;
  } else {
    backoffMs = 1000;
    static unsigned long lastPing = 0;
    if (millis() - lastPing > 30000) {
      wsClient.ping();
      lastPing = millis();
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

void NetworkManager::poll() { wsClient.poll(); }