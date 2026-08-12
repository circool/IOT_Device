/**
 * @file wdt_manager.h
 * @brief Управление сторожeвым таймером (Watchdog Timer)
 */

#ifndef WDT_H
#define WDT_H

#include <Arduino.h>
#include "settings.h"



// ============================================================================
// API
// ============================================================================

#ifdef USE_WDT

/*
 *
 * @brief Инициализация сторожевого таймера* @details Настраивает WDT с
        таймаутом WDT_TIMER_MS.* Вызывается один раз в
        setup().
*/
    void wdtInit();

/**
 * @brief Сброс сторожевого таймера
 * @details Вызывается в loop() для предотвращения срабатывания WDT.
 *          Если WDT не сбрасывать в течение WDT_TIMER_MS, устройство
 * перезагрузится.
 */
void wdtFeed();

/**
 * @brief Остановка сторожевого таймера
 * @details Используется перед перезагрузкой или при входе в Deep Sleep.
 */
void wdtStop();

/**
 * @brief Запуск сторожевого таймера
 * @details Повторная инициализация после остановки.
 */
void wdtStart();

#else  // USE_WDT

// ============================================================================
// ЗАГЛУШКИ
// ============================================================================

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdtInit() {}

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdtFeed() {}

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdtStart() {}

// Заглушка — WDT отключён (FEATURE_WDT_ENABLED == 0)
inline void wdtStop() {}

#endif  // FEATURE_WDT_ENABLED

#endif  // WDT_H