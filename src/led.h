#ifndef LED_H
#define LED_H

#include "config.h"

enum LedMode {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_SLOW_BLINK,      // Одиночное мигание (потеря WiFi)
    LED_MODE_FAST_BLINK,      // Двойные вспышки (потеря MQTT)
    LED_MODE_AP_BLINK         // Тройные вспышки (режим AP) — НОВЫЙ РЕЖИМ
};

// Функции всегда объявлены, но их реализация может быть пустой если STATUS_LED_PIN == 0
void led_init();
void led_update();
void led_setMode(LedMode mode);

#endif // LED_H