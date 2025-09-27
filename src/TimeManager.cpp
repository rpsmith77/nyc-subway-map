#include "TimeManager.h"
#include <time.h>
#include <Arduino.h>

void TimeManager::initializeTime() {
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

void TimeManager::printCurrentTime() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char buf[64];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &timeinfo);
  Serial.print("Current time: ");
  Serial.println(buf);
}