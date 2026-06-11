#include <Arduino.h>
#include "led.h"
#include "logger.h"

#if STATUS_LED_PIN > 0

#ifndef LED_INVERTED
  #ifdef ESP32
      #define LED_INVERTED 0
  #else
      #define LED_INVERTED 1
  #endif
#endif

static LedMode currentMode = LED_MODE_OFF;

void led_init() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LED_INVERTED ? HIGH : LOW);
  LOG_INFO(CAT_LED, "Init pin %d (inverted=%d)", STATUS_LED_PIN, LED_INVERTED);
}

void led_setMode(LedMode mode) {
  if (currentMode != mode) {
    currentMode = mode;
    const char* names[] = {"OFF", "ON", "NO WIFI (1 blink)", "NO MQTT (2 blink)", "AP MODE (3 blink)", "EMERGENCY OFF (1 long and 2 short)"};
    LOG_INFO(CAT_LED, "Mode: %s", names[mode]);
  }
}

LedMode led_getMode() {
  return currentMode;
}

void led_update() {
  static bool lastState = false;
  unsigned long now = millis();
  bool shouldBeOn = false;   
  switch (currentMode) {
    case LED_MODE_OFF:
      shouldBeOn = false;
      break;
        
    case LED_MODE_ON:
      shouldBeOn = true;
      break;
        
    case LED_MODE_MORZE_E:
      // 1 точка: █_______ (100 ON, 900 OFF)
      shouldBeOn = (now % 1000) < 100;
      break;
        
    case LED_MODE_MORZE_I:
      // 2 точки: █_█_____
      // 100 ON, 100 OFF, 100 ON, 700 OFF
      {
          unsigned long phase = now % 1000;
          shouldBeOn = (phase < 100) || (phase >= 200 && phase < 300);
      }
      break;
        
    case LED_MODE_MORZE_S:
      // 3 точки: █_█_█___
      // 100 ON, 100 OFF, 100 ON, 100 OFF, 100 ON, 500 OFF
      {
          unsigned long phase = now % 1000;
          shouldBeOn = (phase < 100) || 
                        (phase >= 200 && phase < 300) || 
                        (phase >= 400 && phase < 500);
      }
      break;
        
    case LED_MODE_MORZE_D:
      // Паттерн: ███_█_█_
      // 300 ON, 100 OFF, 100 ON, 100 OFF, 100 ON, 100 OFF
      // Период: 800 мс
      {
          unsigned long phase = now % 800;
          shouldBeOn = (phase < 300) || 
                        (phase >= 400 && phase < 500) || 
                        (phase >= 600 && phase < 700);
      }
      break;
  }
    
  if (shouldBeOn != lastState) {
    lastState = shouldBeOn;
    digitalWrite(STATUS_LED_PIN, shouldBeOn ? (LED_INVERTED ? LOW : HIGH) : (LED_INVERTED ? HIGH : LOW));
  }
}

#endif