

/**
 * @file led_manager.cpp
 * @brief Реализация управления светодиодной индикацией
 */

#include "led_manager.h"
#include <Arduino.h>
#include "logger.h"

#if FEATURE_LED_ENABLED == 1

static LedMode _mode = LED_OFF;

void led_init() {
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LED_INVERTED ? HIGH : LOW);
    XLOG_DEBUG(CAT_LED, "Init pin %d (inverted=%d)", STATUS_LED_PIN, LED_INVERTED);
}

void led_set_mode(LedMode mode) {
    if (_mode != mode) {
        _mode = mode;
        const char* names[] = {
            "OFF",
            "ON (Pramanent)",
            "Morze E - No WiFi or Reset BTN pressed 1 sec (1 blink)",
            "Morze I - No MQTT or Reset BTN pressed 2 sec (2 blink)",
            "Morze S - Provisioning mode or Reset BTN pressed 3 sec (3 blink)",
            "SLOW blinks"};
        XLOG_DEBUG(CAT_LED, "Mode: %s", names[mode]);
    }
}

void led_update() {
    static bool lastState = false;
    unsigned long now = millis();
    bool shouldBeOn = false;

    switch (_mode) {
        case LED_OFF:
            shouldBeOn = false;
            break;

        case LED_ON:
            shouldBeOn = true;
            break;

        case LED_MORZE_E:
            shouldBeOn = (now % 1000) < 100;
            break;

        case LED_MORZE_I: {
            unsigned long phase = now % 1000;
            shouldBeOn = (phase < 100) || (phase >= 200 && phase < 300);
            break;
        }

        case LED_MORZE_S: {
            unsigned long phase = now % 1000;
            shouldBeOn = (phase < 100) || (phase >= 200 && phase < 300) ||
                         (phase >= 400 && phase < 500);
            break;
        }

        case LED_SLOW_BLINK:
            shouldBeOn = (now % 2000) < 1000;
            break;

        default:
            shouldBeOn = false;
            break;
    }

    if (shouldBeOn != lastState) {
        lastState = shouldBeOn;
        digitalWrite(STATUS_LED_PIN, shouldBeOn ? (LED_INVERTED ? LOW : HIGH)
                                                : (LED_INVERTED ? HIGH : LOW));
    }
}

#endif  // FEATURE_LED_ENABLED