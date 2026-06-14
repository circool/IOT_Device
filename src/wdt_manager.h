#ifndef WDT_H
#define WDT_H

#include "config.h"

// ============================================================================
// WATCHDOG FUNCTIONS
// ============================================================================

#if WDT_ENABLED == 1

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

#else  // WDT_ENABLED == 0

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

#endif  // WDT_ENABLED == 1

#endif  // WDT_H