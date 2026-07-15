/**
 * @file led.h
 * @brief Управление светодиодной индикацией
 * @details Читает глобальное состояние g_systemState и отображает его
 */

#ifndef LED_H
#define LED_H

#include "settings.h"

/**
 * @brief Номер пина светодиода индикации
 * @details 0 — индикация отключена
 */
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 0
#endif

/**
 * @brief Инверсия логики управления светодиодом
 * @details 0: HIGH = включён, 1: LOW = включён
 */
#ifndef LED_INVERTED
#ifdef ESP32
#define LED_INVERTED 0
#else
#define LED_INVERTED 1
#endif
#endif

#if FEATURE_LED_ENABLED == 1

/**
 * @brief Инициализация пина светодиода
 * @details Устанавливает режим пина и начальное состояние (выключен)
 */
void led_init();

/**
 * @brief Обновление состояния светодиода
 * @details Вызывается в loop(). Читает g_systemState и обновляет пина.
 *          Использует millis() для неблокирующего мигания.
 */
void led_update();

#else  // FEATURE_LED_ENABLED == 0

inline void led_init() {}
inline void led_update() {}

#endif  // FEATURE_LED_ENABLED

#endif  // LED_H