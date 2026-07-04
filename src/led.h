#ifndef LED_H
#define LED_H

#include "config_manager.h"
#include "settings.h"

/** @brief Пин светодиода индикации (0 = отключён) */
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 0
#endif

enum LedMode {
  LED_MODE_OFF,      // Постоянно выключен
  LED_MODE_ON,       // Постоянно включён
  LED_MODE_MORZE_E,  // 1 точка/сек: █_______ (нет WiFi / 0-1 сек сброса)
  LED_MODE_MORZE_I,  // 2 точки/сек: █_█_____ (нет MQTT / 1-2 сек сброса)
  LED_MODE_MORZE_S,  // 3 точки/сек: █_█_█___ (AP/BLE mode / 2-3 сек сброса)
  LED_SLOW_BLINK,    // ███_█_█_ (аварийное отключение)
};

#if STATUS_LED_PIN > 0
void led_init();
void led_update();
void led_setMode(LedMode mode);
LedMode led_getMode();
#else
inline void led_init() {}
inline void led_update() {}
inline void led_setMode(LedMode mode) {}
inline LedMode led_getMode() {
  return LED_MODE_OFF;
}
#endif

#endif