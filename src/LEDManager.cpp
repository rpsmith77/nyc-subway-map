#include "LEDManager.h"

CRGB LEDManager::leds[NUM_LEDS_SUBWAY];
CRGB LEDManager::errorLeds[NUM_LEDS_ERROR];

void LEDManager::initializeLEDs() {
    FastLED.addLeds<LED_TYPE, DATA_PIN_SUBWAY, COLOR_ORDER>(leds, NUM_LEDS_SUBWAY);
    FastLED.setBrightness(5);
}

void LEDManager::show() {
    FastLED.show();
}

void LEDManager::awaitingDataSequence() {
    // Alternate every other LED on/off, then swap every second
    const uint32_t period = 2000; // 2 seconds for a full cycle
    const uint32_t now = millis();
    bool even_on = ((now % period) < (period / 2));

    CRGB warmWhite = CRGB(255, 180, 80); // Warm white color

    for (int i = 0; i < NUM_LEDS_SUBWAY; ++i) {
        bool is_even = (i % 2 == 0);
        leds[i] = (is_even == even_on) ? warmWhite : CRGB::Black;
    }
}