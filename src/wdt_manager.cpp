#include "wdt_manager.h"
#include <Arduino.h>
#include "logger.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <esp_task_wdt.h>
#endif

#if FEATURE_WDT_ENABLED == 1
static bool wdt_initialized = false;
static bool wdt_stopped = false;

void wdt_init() {
#if FEATURE_WDT_ENABLED == 0
  return;
#endif

  if (wdt_initialized)
    return;

#if defined(ESP8266)
  ESP.wdtEnable(WDT_TIMER_MS);
  XLOG_DEBUG(CAT_WDT, "ESP8266 WDT enabled, timeout=%d ms", WDT_TIMER_MS);

#elif defined(ESP32)
// Проверяем, какой API доступен
#if defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C3)
  // Новый API (ESP-IDF 5.0+)
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMER_MS,
      .idle_core_mask = 1,  // Для одноядерных чипов (C3, C6, H2)
      .trigger_panic = true,
  };
  esp_task_wdt_init(&wdt_config);
#elif defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32)
  // Для ESP32 и ESP32-S3 (двухъядерные)
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMER_MS,
      .idle_core_mask = (1 << CONFIG_ESP_INT_WDT_CPU_NUM) - 1,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&wdt_config);
#else
  // Старый API (ESP-IDF 4.x) для ESP32, ESP32-S2
  esp_task_wdt_init(WDT_TIMER_MS / 1000, true);
#endif

  esp_task_wdt_add(NULL);
  XLOG_DEBUG(CAT_WDT, "ESP32 task WDT initialized, timeout=%d ms",
             WDT_TIMER_MS);

#else
#warning "Unsupported platform for WDT"
#endif

  wdt_initialized = true;
}

void wdt_feed() {
#if FEATURE_WDT_ENABLED == 0
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
#if FEATURE_WDT_ENABLED == 0
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
  XLOG_INFO(CAT_WDT, "Stopped");
}

void wdt_start() {
#if FEATURE_WDT_ENABLED == 0
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
  XLOG_INFO(CAT_WDT, "Started");
}

#endif  // FEATURE_WDT_ENABLED == 1