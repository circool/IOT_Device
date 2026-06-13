#ifndef WDT_H
#define WDT_H
// ======================== WATCHDOG FUNCTIONS ========================

#if WDT_ENABLED == 1

/**
 * @brief Инициализация сторожевого таймера (Watchdog Timer)
 */
void wdt_init();

/**
 * @brief Сброс сторожевого таймера (кормление WDT)
 */
void wdt_feed();

/**
 * @brief Остановка сторожевого таймера
 */
void wdt_stop();

/**
 * @brief Запуск сторожевого таймера
 */
void wdt_start();

#else

inline void wdt_init() {};
inline void wdt_feed() {};
inline void wdt_stop();
inline void wdt_start() {};

#endif  // WDT_ENABLED
#endif  // WDT_H