#include "NetworkManager.h"
#include <Arduino.h>
#include "MTAManager.h"
#include <WiFi.h>

NetworkManager::NetworkManager(const char* ssid, const char* password, const char* host, const char* port)
    : ssid(ssid),
      password(password),
      host(host),
      port(port),
      wifiConnectionStatus(false),
      wsClient(),
      maxBackoffMs(30000),
      wifiLastAttempt(0),
      wifiBackoffMs(1000),
      wifiFailedAttempts(0),
      websocketLastAttempt(0),
      websocketBackoffMs(1000),
      websocketFailedAttempts(0),
      websocketLastPing(0),
      websocketWasConnected(false),
      websocketEverConnected(false)
{
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
    if (WiFi.status() == WL_CONNECTED) {
        handleWifiConnected();
        return true;
    } else {
        handleWifiDisconnected();
        attemptWifiReconnect();
        return false;
    }
}

void NetworkManager::handleWifiConnected() {
    if (!wifiConnectionStatus) {
        wifiConnectionStatus = true;
        Serial.println("Connected");
    }
    wifiBackoffMs = 1000;
    wifiFailedAttempts = 0;
}

void NetworkManager::handleWifiDisconnected() {
    if (wifiConnectionStatus) {
        Serial.println("WiFi lost");
    }
    wifiConnectionStatus = false;
}

void NetworkManager::attemptWifiReconnect() {
    unsigned long now = millis();
    if (now - wifiLastAttempt >= wifiBackoffMs) {
        Serial.println("Attempting WiFi reconnect...");
        WiFi.disconnect(true);
        delay(50);
        WiFi.begin(ssid, password);
        wifiLastAttempt = now;
        wifiBackoffMs = min(wifiBackoffMs * 2, maxBackoffMs);

        if (wifiBackoffMs == maxBackoffMs) {
            wifiFailedAttempts++;
            handleMaxWifiFailures();
        }
    }
}

void NetworkManager::handleMaxWifiFailures() {
    if (wifiFailedAttempts >= 5) {
        Serial.println("Max WiFi reconnect attempts reached. Rebooting ESP32...");
        ESP.restart();
    }
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
  wsClient.poll();

  if (!wsClient.available()) {
    handleWebsocketDisconnect();
    return false;
  } else {
    handleWebsocketConnected();
    return true;
  }
}

void NetworkManager::handleWebsocketDisconnect() {
  // Use member variables for state
  websocketWasConnected = false;
  unsigned long now = millis();
  if (now - websocketLastAttempt >= websocketBackoffMs) {
    attemptWebsocketReconnect();
  }
}

void NetworkManager::attemptWebsocketReconnect() {
  Serial.println("WS disconnected, attempting reconnect...");
  wsClient.close();
  delay(20);
  String wsUrl = "ws://" + String(host) + ":" + String(port) + "/ws";
  wsClient.connect(wsUrl);
  websocketLastAttempt = millis();
  websocketBackoffMs = min(websocketBackoffMs * 2, maxBackoffMs);
  websocketLastPing = 0;

  if (websocketBackoffMs == maxBackoffMs) {
    websocketFailedAttempts++;
    if (websocketFailedAttempts >= 5) {
      if (isServerPingable()) {
        Serial.println("Server is pingable. Rebooting ESP32...");
        ESP.restart();
      } else {
        Serial.println("Server is not pingable. Not rebooting.");
        websocketFailedAttempts = 0;
      }
    }
  }
}

bool NetworkManager::isServerPingable() {
  WiFiClient pingClient;
  if (pingClient.connect(host, atoi(port))) {
    pingClient.stop();
    return true;
  }
  return false;
}

void NetworkManager::handleWebsocketConnected() {
  websocketBackoffMs = 1000;
  websocketFailedAttempts = 0;
  if (!websocketWasConnected) {
    if (websocketEverConnected) {
      Serial.println("WS reconnected");
    } else {
      websocketEverConnected = true;
    }
    websocketWasConnected = true;
  }
  unsigned long now = millis();
  if (now - websocketLastPing > 10000) {
    wsClient.ping();
    websocketLastPing = now;
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