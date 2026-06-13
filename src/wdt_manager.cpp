#include "wdt_manager.h"
#include "config.h"
#include "logger.h"

#include <Arduino.h>  // ← для Serial, millis и т.д.

#if defined(ESP8266)
#include <ESP8266WiFi.h>  // ← для ESP (класс ESP)
// Встроенные макросы wdt_disable/wdt_enable доступны после включения Arduino.h
#elif defined(ESP32)
#include <esp_task_wdt.h>
#endif

static bool wdt_initialized = false;
static bool wdt_stopped = false;

void wdt_init() {
  if (wdt_initialized)
    return;

#if WDT_ENABLED == 0
  return;
#endif

#if defined(ESP8266)
  ESP.wdtEnable(WDT_TIMER_MS);
  LOG_DEBUG(CAT_WDT, "ESP8266 WDT enabled, timeout=%d ms", WDT_TIMER_MS);

#elif defined(ESP32)
  esp_task_wdt_init(WDT_TIMER_MS / 1000, true);
  esp_task_wdt_add(NULL);
  LOG_DEBUG(CAT_WDT, "ESP32 task WDT enabled, timeout=%d ms", WDT_TIMER_MS);
#endif

  wdt_initialized = true;
}

void wdt_feed() {
#if WDT_ENABLED == 0
  return;
#endif

#ifdef WDT_TEST
  return;
#endif

  if (wdt_stopped)
    return;

#if defined(ESP8266)
  ESP.wdtFeed();
#elif defined(ESP32)
  if (wdt_initialized)
    esp_task_wdt_reset();
#endif
}

void wdt_stop() {
#if WDT_ENABLED == 0
  return;
#endif

  if (wdt_stopped)
    return;

#if defined(ESP8266)
  // Встроенный макрос ESP8266 (объявлен в Arduino.h)
  wdt_disable();
#elif defined(ESP32)
  if (!wdt_initialized)
    return;
  esp_task_wdt_delete(NULL);
#endif

  wdt_stopped = true;
  LOG_INFO(CAT_WDT, "Stopped");
}

void wdt_start() {
#if WDT_ENABLED == 0
  return;
#endif

  if (!wdt_stopped)
    return;

#if defined(ESP8266)
  // Встроенный макрос ESP8266, время в секундах
  wdt_enable(WDT_TIMER_MS / 1000);
#elif defined(ESP32)
  if (!wdt_initialized)
    return;
  esp_task_wdt_add(NULL);
#endif
  wdt_stopped = false;
  LOG_INFO(CAT_WDT, "Started");
}