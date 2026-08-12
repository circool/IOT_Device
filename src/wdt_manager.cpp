/**
 * @file wdt_manager.cpp
 * @brief Управление сторожeвым таймером (Watchdog Timer)
 */

#include "wdt_manager.h"
#include "logger.h"
#include "settings.h"

#ifdef USE_WDT

// ============================================================================
// НАСТРОЙКИ
// ============================================================================

#ifndef WDT_TIMER_MS
// Время срабатывания WatchDog
#define WDT_TIMER_MS 5000
#endif

// ============================================================================
// ОПРЕДЕЛЕНИЕ API WDT
// ============================================================================

#if defined(ESP8266)

#define USE_HARDWARE_WDT 1
#define USE_NEW_TASK_WDT_API 0

#elif defined(ESP32)

#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
#define USE_HARDWARE_WDT 0
#define USE_NEW_TASK_WDT_API 1

#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#define USE_HARDWARE_WDT 0
#define USE_NEW_TASK_WDT_API 1
#else
#define USE_HARDWARE_WDT 0
#define USE_NEW_TASK_WDT_API 0
#endif

#else
// Использовать аппаратный WatchDog
#define USE_HARDWARE_WDT 0
#define USE_NEW_TASK_WDT_API 0
#endif

#else
// Не использовать аппаратный WatchDog
#define USE_HARDWARE_WDT 0
#define USE_NEW_TASK_WDT_API 0
#endif

// ============================================================================
// WDT ФУНКЦИИ
// ============================================================================

#if defined(ESP8266)

// ===== ESP8266 =====
static void wdt_impl_init() {
  wdt_enable(WDT_TIMER_MS / 1000);
  XLOG_DEBUG(CAT_WDT, "WDT init (ESP8266): timeout=%d ms", WDT_TIMER_MS);
}

static void wdt_impl_feed() {
  wdt_reset();
}

static void wdt_impl_stop() {
  wdt_disable();
  XLOG_DEBUG(CAT_WDT, "WDT stopped (ESP8266)");
}

#elif defined(ESP32) && USE_NEW_TASK_WDT_API == 1

// ===== ESP32-C6, ESP32-H2, ESP32-C3 (ESP-IDF 5.0+) =====
// Новый API: esp_task_wdt_init(&config)

#include "esp_task_wdt.h"

static bool _wdtInitialized = false;
static bool _wdtTaskAdded = false;

static void wdt_impl_init() {
  if (_wdtInitialized)
    return;

  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMER_MS,
      .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,
      .trigger_panic = true};

  esp_err_t err = esp_task_wdt_init(&wdt_config);

  // ESP_ERR_INVALID_STATE (259) = WDT уже инициализирован системой
  if (err == ESP_ERR_INVALID_STATE) {
    XLOG_DEBUG(CAT_WDT, "TWDT already initialized by system, using existing");
    _wdtInitialized = true;
  } else if (err != ESP_OK) {
    XLOG_ERROR(CAT_WDT, "esp_task_wdt_init failed: %d", err);
    return;
  } else {
    _wdtInitialized = true;
  }

  // ================================================================
  // ВАЖНО: даже если WDT уже инициализирован, нужно добавить задачу!
  // ================================================================
  err = esp_task_wdt_add(NULL);
  if (err == ESP_OK) {
    _wdtTaskAdded = true;
    XLOG_DEBUG(CAT_WDT, "Task added to TWDT");
  } else if (err == ESP_ERR_INVALID_STATE) {
    // WDT не инициализирован — это не должно произойти
    XLOG_WARN(CAT_WDT, "esp_task_wdt_add: TWDT not initialized");
  } else if (err == ESP_ERR_NOT_FOUND) {
    // Задача уже добавлена — это нормально
    XLOG_DEBUG(CAT_WDT, "Task already in TWDT");
    _wdtTaskAdded = true;
  } else {
    XLOG_ERROR(CAT_WDT, "esp_task_wdt_add failed: %d", err);
    return;
  }

  XLOG_DEBUG(CAT_WDT, "WDT init (new API): timeout=%d ms", WDT_TIMER_MS);
}

static void wdt_impl_feed() {
  if (!_wdtInitialized)
    return;
  if (!_wdtTaskAdded) {
    // Пытаемся добавить задачу, если ещё не добавлена
    esp_err_t err = esp_task_wdt_add(NULL);
    if (err == ESP_OK || err == ESP_ERR_NOT_FOUND) {
      _wdtTaskAdded = true;
    } else {
      return;
    }
  }

  esp_err_t err = esp_task_wdt_reset();
  if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
    XLOG_WARN(CAT_WDT, "esp_task_wdt_reset failed: %d", err);
  }
}

static void wdt_impl_stop() {
  if (!_wdtInitialized)
    return;

  if (_wdtTaskAdded) {
    esp_task_wdt_delete(NULL);
    _wdtTaskAdded = false;
  }

  esp_task_wdt_deinit();
  _wdtInitialized = false;
  XLOG_DEBUG(CAT_WDT, "WDT stopped (new API)");
}

#elif defined(ESP32)

// ===== ESP32, ESP32-S2, ESP32-S3, ESP32-C3 (ESP-IDF 4.x) =====
// Старый API: esp_task_wdt_init(timeout, panic)

#include "esp_task_wdt.h"

static bool _wdtInitialized = false;

static void wdt_impl_init() {
  if (_wdtInitialized)
    return;

  esp_task_wdt_init(WDT_TIMER_MS, true);
  esp_task_wdt_add(NULL);
  _wdtInitialized = true;
  XLOG_DEBUG(CAT_WDT, "WDT init (old API): timeout=%d ms", WDT_TIMER_MS);
}

static void wdt_impl_feed() {
  if (!_wdtInitialized)
    return;
  esp_task_wdt_reset();
}

static void wdt_impl_stop() {
  if (!_wdtInitialized)
    return;
  esp_task_wdt_delete(NULL);
  esp_task_wdt_deinit();
  _wdtInitialized = false;
  XLOG_DEBUG(CAT_WDT, "WDT stopped (old API)");
}

#else

// ===== Заглушка =====
static void wdt_impl_init() {
  XLOG_WARN(CAT_WDT, "WDT not supported on this platform");
}

static void wdt_impl_feed() {}
static void wdt_impl_stop() {}

#endif

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

void wdtInit() {
  wdt_impl_init();
  XLOG_INFO(CAT_WDT, "Watchdog initialized (timeout: %d ms)", WDT_TIMER_MS);
}

void wdtFeed() {
  wdt_impl_feed();
}

void wdtStop() {
  wdt_impl_stop();
}

void wdtStart() {
  wdt_impl_init();
}

#endif  // USE_WDT