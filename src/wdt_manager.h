#ifndef WDT_H
#define WDT_H

#include "config_manager.h"

// ============================================================================
// WATCHDOG (WDT)
// ============================================================================

/** @brief Включить аппаратный сторожевой таймер по умолчанию*/
#ifndef FEATURE_WDT_ENABLED
#define FEATURE_WDT_ENABLED 1
#endif

#ifndef WDT_TIMER_MS
#define WDT_TIMER_MS 5000
#endif

#if FEATURE_WDT_ENABLED == 1

/** @brief Таймаут WDT в миллисекундах */
#ifndef WDT_TIMER_MS
#define WDT_TIMER_MS 5000
#endif

/** @brief Множитель для расчёта WDT в loop (не используется в текущей версии)
 */
#ifndef LOOP_WATCHDOG_MULTIPLIER
#define LOOP_WATCHDOG_MULTIPLIER 3
#endif

void wdt_init();
void wdt_feed();
void wdt_start();
void wdt_stop();

#else

/** @brief Софт-WDT (заглушка, не реализован) */
#ifndef SOFT_WDT_ENABLED
#define SOFT_WDT_ENABLED 1
#endif

#endif

// ============================================================================
// WATCHDOG FUNCTIONS
// ============================================================================

#if FEATURE_WDT_ENABLED == 1

    /**
     * @brief Инициализация сторожевого таймера (Watchdog Timer)
     * @note Вызывается один раз в setup()
     */
    void wdt_init();

/**
 * @brief Сброс сторожевого таймера (кормление WDT)
 * @note Вызывается в loop() чтобы предотвратить перезагрузку
 */
void wdt_feed();

/**
 * @brief Остановка сторожевого таймера
 * @note Используется перед длительными операциями (например, запись EEPROM)
 */
void wdt_stop();

/**
 * @brief Запуск сторожевого таймера
 * @note Возобновляет работу WDT после остановки
 */
void wdt_start();

#else  // FEATURE_WDT_ENABLED == 0

/**
 * @brief Заглушка: инициализация WDT (отключена)
 */
inline void wdt_init() {}

/**
 * @brief Заглушка: сброс WDT (отключён)
 */
inline void wdt_feed() {}

/**
 * @brief Заглушка: остановка WDT (отключена)
 */
inline void wdt_stop() {}

/**
 * @brief Заглушка: запуск WDT (отключён)
 */
inline void wdt_start() {}

#endif  // FEATURE_WDT_ENABLED == 1

#endif  // WDT_H