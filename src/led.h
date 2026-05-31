#ifndef LED_H
#define LED_H

#include "config.h"

// Объявляем enum ДО условной компиляции
enum LedMode {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_SLOW_BLINK,
    LED_MODE_FAST_BLINK
};

// Функции всегда объявлены, но их реализация может быть пустой если STATUS_LED_PIN == 0
void led_init();
void led_update();
void led_setMode(LedMode mode);

#endif // LED_H