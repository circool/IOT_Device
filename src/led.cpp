/**
 * @file led.cpp
 * @brief Реализация управления светодиодной индикацией
 */

#include "led.h"
#include <Arduino.h>
#include "logger.h"
#include "system_state.h"

#if FEATURE_LED_ENABLED == 1

void led_init() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LED_INVERTED ? HIGH : LOW);
  XLOG_DEBUG(CAT_LED, "Init pin %d (inverted=%d)", STATUS_LED_PIN,
             LED_INVERTED);
}

void led_update() {
  static bool lastPhysicalState = false;
  unsigned long now = millis();
  bool shouldBeOn = false;

  // Читаем состояние напрямую из глобальной переменной
  switch (g_systemState) {
    case SystemState::INIT:
    case SystemState::RESTART_PENDING:
      shouldBeOn = false;
      break;

    case SystemState::NORMAL:
      shouldBeOn = true;
      break;

    case SystemState::MODE_1:
      // 1 точка: █_______ (100 ON, 900 OFF)
      shouldBeOn = (now % 1000) < 100;
      break;

    case SystemState::MODE_2: {
      // 2 точки: █_█_____ (100 ON, 100 OFF, 100 ON, 700 OFF)
      unsigned long phase = now % 1000;
      shouldBeOn = (phase < 100) || (phase >= 200 && phase < 300);
      break;
    }

    case SystemState::MODE_3: {
      // 3 точки: █_█_█___ (100 ON, 100 OFF, 100 ON, 100 OFF, 100 ON, 500 OFF)
      unsigned long phase = now % 1000;
      shouldBeOn = (phase < 100) || (phase >= 200 && phase < 300) ||
                   (phase >= 400 && phase < 500);
      break;
    }

    case SystemState::MODE_4:
      // Медленное мигание: 1 сек горит, 1 сек не горит
      shouldBeOn = (now % 2000) < 1000;
      break;

    default:
      shouldBeOn = false;
      break;
  }

  // Обновляем пин только при изменении
  if (shouldBeOn != lastPhysicalState) {
    lastPhysicalState = shouldBeOn;
    digitalWrite(STATUS_LED_PIN, shouldBeOn ? (LED_INVERTED ? LOW : HIGH)
                                            : (LED_INVERTED ? HIGH : LOW));
  }
}

#endif  // FEATURE_LED_ENABLED