/**
 * @file led_manager.h
 * @brief Управление светодиодной индикацией (тупой исполнитель)
 * @details Получает готовый режим через led_set_mode() и применяет его.
 */

#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "settings.h"



/**
 * @brief Номер пина светодиода
 * @details 0 — индикация отключена
 */
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 0
#endif

/**
 * @brief Инверсия логики управления
 * @details 0: HIGH = включён, 1: LOW = включён
 */
#ifndef LED_INVERTED
#ifdef ESP32
#define LED_INVERTED 0
#else
#define LED_INVERTED 1
#endif
#endif

/**
 * @brief Режимы LED (готовые для применения)
 */
enum LedMode {
  LED_OFF,        /**< Выключен */
  LED_ON,         /**< Постоянно горит */
  LED_MORZE_E,    /**< 1 вспышка/сек */
  LED_MORZE_I,    /**< 2 вспышки/сек */
  LED_MORZE_S,    /**< 3 вспышки/сек */
  LED_SLOW_BLINK, /**< Медленное мигание (1с ON, 1с OFF) */
};

#if FEATURE_LED_ENABLED == 1

/**
 * @brief Инициализация пина светодиода
 */
void led_init();

/**
 * @brief Установить режим LED
 * @param mode Режим из LedMode
 */
void led_set_mode(LedMode mode);

/**
 * @brief Обновление физического состояния LED
 * @details Вызывается в loop(). Применяет текущий режим.
 *          Использует millis() для неблокирующего мигания.
 */
void led_update();

#else  // FEATURE_LED_ENABLED == 0

inline void led_init() {}
inline void led_set_mode(LedMode mode) {
  (void)mode;
}
inline void led_update() {}

#endif  // FEATURE_LED_ENABLED

#endif  // LED_MANAGER_H