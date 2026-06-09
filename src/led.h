#ifndef LED_H
#define LED_H

#include "config.h"

/**
 * @brief Режимы работы светодиода
 */
enum LedMode {
    LED_MODE_OFF,               // Постоянно выключен
    LED_MODE_ON,                // Постоянно включён (всё OK)
    LED_MODE_SLOW_BLINK,        // Медленное мигание (нет WiFi)
    LED_MODE_FAST_BLINK,        // Частое мигание (нет MQTT)
    LED_MODE_AP_BLINK,          // Тройные вспышки (режим AP)
    LED_MODE_EMERGENCY_STOP,    // Аварийное отключение: длинная — пауза — две коротких
};

#if STATUS_LED_PIN > 0
/**
 * @brief Инициализация пина светодиода
 * Вызывается один раз в setup()
 */
void led_init();

/**
 * @brief Периодическое обновление состояния светодиода
 * Вызывается в loop()
 */
void led_update();

/**
 * @brief Установить режим работы светодиода
 * @param mode Режим из перечисления LedMode
 */
void led_setMode(LedMode mode);

#else
inline void led_init(){};
inline void led_update(){};
inline void led_setMode(LedMode mode){};
#endif // STATUS_LED_PIN
#endif // LED_H