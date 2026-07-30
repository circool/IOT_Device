/**
 * @file wdt_manager.h
 * @brief Управление сторожeвым таймером (Watchdog Timer)
 */

#ifndef WDT_H
#define WDT_H

#include <Arduino.h>
#include "settings.h"

// ============================================================================
// НАСТРОЙКИ
// ============================================================================

/** @brief Включить аппаратный сторожевой таймер по умолчанию */
#ifndef FEATURE_WDT_ENABLED
#define FEATURE_WDT_ENABLED 1
#endif

#ifndef WDT_TIMER_MS
#define WDT_TIMER_MS 5000
#endif

// ============================================================================
// API
// ============================================================================

#if FEATURE_WDT_ENABLED == 1

void wdt_init();
void wdt_feed();
void wdt_start();
void wdt_stop();

#else  // FEATURE_WDT_ENABLED == 0

// ============================================================================
// ЗАГЛУШКИ
// ============================================================================

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdt_init() {}

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdt_feed() {}

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdt_start() {}

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdt_stop() {}

#endif  // FEATURE_WDT_ENABLED

#endif  // WDT_H