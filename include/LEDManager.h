#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include <FastLED.h>

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define NUM_LEDS_SUBWAY 500
#define DATA_PIN_SUBWAY 10  // D7

#define NUM_LEDS_ERROR 2
#define DATA_PIN_ERROR 5  // D2

class LEDManager {
public:
    static CRGB leds[NUM_LEDS_SUBWAY];
    static CRGB errorLeds[NUM_LEDS_ERROR];
    static void initializeLEDs();
    static void show();
    static void awaitingDataSequence();
};
#endif // LEDMANAGER_H