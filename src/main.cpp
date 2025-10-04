#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "WifiCredentials.h"
#include "GeneratedStationMap.h"
#include "TimeManager.h"
#include "NetworkManager.h"
#include "Station.h"
#include "Train.h"
#include "SubwayColors.h"
#include "LEDManager.h"
#include "MTAManager.h"

#ifdef HEAPDEBUG
#include "HeapDebug.h"
#endif

NetworkManager net(WIFI_SSID, WIFI_PASSWORD, SERVER_HOST, SERVER_PORT);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  net.initializeWifi();
  delay(200);
  net.initializeWebsocket();
  LEDManager::initializeLEDs();
  TimeManager::initializeTime();
  delay(3000);
}

void loop() {
  net.poll(); 
  
  if (net.checkWifiConnection())
    net.checkWebsocketConnection();

  MtaManager::purgeExpiredTrains();
  MtaManager::checkArrivals();

  if (!MtaManager::hasAnyTrainData())
    LEDManager::awaitingDataSequence();

  LEDManager::show();
  EVERY_N_SECONDS(1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); }
  EVERY_N_SECONDS(60) { TimeManager::printCurrentTime(); }

#ifdef HEAPDEBUG
  EVERY_N_SECONDS(60) { HeapDebug::printHeapUsage(); }
#endif

}