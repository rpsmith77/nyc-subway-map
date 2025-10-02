#ifndef HEAPDEBUG_H
#define HEAPDEBUG_H

#include <Arduino.h>

class HeapDebug {
public:
  static void printHeapUsage() {
    constexpr size_t kTotalRamBytes = 512U * 1024U;
    const size_t heapSize = ESP.getHeapSize();
    const size_t freeHeap = ESP.getFreeHeap();
    const size_t usedHeap = (heapSize >= freeHeap) ? (heapSize - freeHeap) : 0;
    const float heapPercentOfRam =
        (kTotalRamBytes > 0) ? (static_cast<float>(heapSize) * 100.0f / kTotalRamBytes) : 0.0f;
    const float usedPercentOfHeap =
        (heapSize > 0) ? (static_cast<float>(usedHeap) * 100.0f / heapSize) : 0.0f;

    Serial.printf(
        "[Heap] Total RAM: %lu KB, Heap Region: %lu KB (%.1f%% of RAM), Used: %lu KB (%.1f%% of heap), Free: %lu KB\n",
        static_cast<unsigned long>(kTotalRamBytes / 1024),
        static_cast<unsigned long>(heapSize / 1024),
        heapPercentOfRam,
        static_cast<unsigned long>(usedHeap / 1024),
        usedPercentOfHeap,
        static_cast<unsigned long>(freeHeap / 1024));
}
};

#endif // HEAPDEBUG_H