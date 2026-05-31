#include "led.h"



#if STATUS_LED_PIN > 0

#ifndef LED_INVERTED
    #define LED_INVERTED 1  // По умолчанию инвертирован
#endif

static bool ledState = false;
static LedMode currentLedMode = LED_MODE_OFF;
static unsigned long lastBlinkTime = 0;
static int blinkStep = 0;

void led_init() {
    pinMode(STATUS_LED_PIN, OUTPUT);
    #if LED_INVERTED == 1
        digitalWrite(STATUS_LED_PIN, HIGH);  
    #else
        digitalWrite(STATUS_LED_PIN, LOW);
    #endif
    #if LOG_LED == 1
        Serial.printf("[LED] Initialized on pin %d (inverted=%d)\n", STATUS_LED_PIN, LED_INVERTED);
    #endif
}

void led_update() {
    unsigned long now = millis();
    bool shouldBeOn = false;
    
    switch (currentLedMode) {
        case LED_MODE_OFF:
            shouldBeOn = false;
            break;
            
        case LED_MODE_ON:
            shouldBeOn = true;
            break;
            
        case LED_MODE_SLOW_BLINK:  // Одиночное мигание (потеря WiFi) = "."
            shouldBeOn = (now % 1000) < 200;
            break;
            
        case LED_MODE_FAST_BLINK:  // Двойные вспышки (потеря MQTT) = ".."
            if (blinkStep == 0) {
                lastBlinkTime = now;
                blinkStep = 1;
                shouldBeOn = true;
            } 
            else if (blinkStep == 1) {
                if (now - lastBlinkTime >= 100) {
                    lastBlinkTime = now;
                    blinkStep = 2;
                    shouldBeOn = false;
                } else {
                    shouldBeOn = true;
                }
            }
            else if (blinkStep == 2) {
                if (now - lastBlinkTime >= 100) {
                    lastBlinkTime = now;
                    blinkStep = 3;
                    shouldBeOn = true;
                } else {
                    shouldBeOn = false;
                }
            }
            else if (blinkStep == 3) {
                if (now - lastBlinkTime >= 100) {
                    lastBlinkTime = now;
                    blinkStep = 0;
                    shouldBeOn = false;
                } else {
                    shouldBeOn = true;
                }
            }
            break;
    }
    
    if (shouldBeOn != ledState) {
        ledState = shouldBeOn;
        #if LED_INVERTED == 1
            digitalWrite(STATUS_LED_PIN, ledState ? LOW : HIGH);  
        #else
            digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        #endif
    }
}

void led_setMode(LedMode mode) {
    if (currentLedMode != mode) {
        currentLedMode = mode;
        blinkStep = 0;
        #if LOG_LED == 1
            const char* modeName = "UNKNOWN";
            switch (mode) {
                case LED_MODE_OFF: modeName = "OFF"; break;
                case LED_MODE_ON: modeName = "ON"; break;
                case LED_MODE_SLOW_BLINK: modeName = "SLOW_BLINK (WiFi lost)"; break;
                case LED_MODE_FAST_BLINK: modeName = "FAST_BLINK (MQTT lost)"; break;
            }
            Serial.printf("[LED] Mode changed to: %s\n", modeName);
        #endif
    }
}

#endif