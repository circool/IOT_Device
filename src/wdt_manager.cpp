#include "wdt_manager.h"
#include "config_manager.h"
#include "logger.h"

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <esp_task_wdt.h>
#endif

#if WDT_ENABLED == 1
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
// Определяем тип чипа
#if defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C6)
  // ===== НОВЫЙ API для ESP32-C3/S3/C6 =====
  esp_task_wdt_deinit();

  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = WDT_TIMER_MS,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&twdt_config);

  LOG_DEBUG(CAT_WDT, "ESP32-C3/S3/C6 task WDT initialized, timeout=%d ms",
            WDT_TIMER_MS);
#else
  // ===== СТАРЫЙ API для классического ESP32 =====
  esp_task_wdt_init(WDT_TIMER_MS / 1000, true);
  LOG_DEBUG(CAT_WDT, "ESP32 task WDT initialized, timeout=%d ms", WDT_TIMER_MS);
#endif

  esp_task_wdt_add(NULL);
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
  wdt_enable(WDT_TIMER_MS / 1000);
#elif defined(ESP32)
  if (!wdt_initialized)
    return;
  esp_task_wdt_add(NULL);
#endif
  wdt_stopped = false;
  LOG_INFO(CAT_WDT, "Started");
}

#endif