/**
 * @file led_manager.cpp
 * @brief Реализация управления светодиодной индикацией
 * @version 0.12
 * @date 10.08.2026
 */

#include "led_manager.h"
#include "logger.h"

#ifdef USE_LED

// ============ Внутренние константы ============

#define PULSE_DURATION_MS 100    // Длительность одной вспышки (мс)
#define PULSE_GAP_MS 100         // Пауза между вспышками внутри серии (мс)
#define SERIES_DURATION_MS 1000  // Длительность серии (всегда 1 секунда)

// ============ Публичные методы ============

void LedManager::init() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  _currentMode = LED_OFF;
  _state = false;

  _setPinState(false);

  XLOG_DEBUG(CAT_LED, "LED initialized: pin=%d, inverted=%d, interval=%dms",
            STATUS_LED_PIN, LED_INVERTED, LED_SERIES_INTERVAL_MS);
}

void LedManager::setMode(LedMode mode) {
  if (_currentMode != mode) {
    _currentMode = mode;

    const char* names[] = {"OFF",
                           "ON",
                           "MORZE_E (1 pulse/series)",
                           "MORZE_I (2 pulses/series)",
                           "MORZE_S (3 pulses/series)",
                           "SLOW_BLINK (1s ON, 1s OFF)"};
    XLOG_DEBUG(CAT_LED, "Mode: %s", names[mode]);
  }
}

void LedManager::update() {
  if (_currentMode == LED_OFF) {
    _setPinState(false);
    return;
  }

  if (_currentMode == LED_ON) {
    _setPinState(true);
    return;
  }

  unsigned long now = millis();
  bool shouldBeOn = _isPulseActive(now);
  _setPinState(shouldBeOn);
}

// ============ Приватные методы ============

bool LedManager::_isPulseActive(unsigned long now) const {
  unsigned long period = LED_SERIES_INTERVAL_MS;
  unsigned long pos = now % period;

  // Если интервал меньше длительности серии — всё время горим
  if (period <= SERIES_DURATION_MS) {
    pos = now % SERIES_DURATION_MS;
  } else {
    // Есть пауза между сериями
    if (pos >= SERIES_DURATION_MS) {
      return false;  // Пауза
    }
  }

  // Логика вспышек внутри серии (pos в диапазоне 0..999)
  switch (_currentMode) {
    case LED_MORZE_E:
      // 1 вспышка: 100 ON, 900 OFF
      return (pos < PULSE_DURATION_MS);

    case LED_MORZE_I:
      // 2 вспышки: 100 ON, 100 OFF, 100 ON, 700 OFF
      return (pos < PULSE_DURATION_MS) ||
             (pos >= (PULSE_DURATION_MS + PULSE_GAP_MS) &&
              pos < (PULSE_DURATION_MS * 2 + PULSE_GAP_MS));

    case LED_MORZE_S:
      // 3 вспышки: 100 ON, 100 OFF, 100 ON, 100 OFF, 100 ON, 500 OFF
      return (pos < PULSE_DURATION_MS) ||
             (pos >= (PULSE_DURATION_MS + PULSE_GAP_MS) &&
              pos < (PULSE_DURATION_MS * 2 + PULSE_GAP_MS)) ||
             (pos >= (PULSE_DURATION_MS * 2 + PULSE_GAP_MS * 2) &&
              pos < (PULSE_DURATION_MS * 3 + PULSE_GAP_MS * 2));

    case LED_SLOW_BLINK:
      // 1с ON, 1с OFF (если period >= 2000)
      return (pos < 1000);

    default:
      return false;
  }
}

void LedManager::_setPinState(bool on) {
  if (on != _state) {
    _state = on;
    if (LED_INVERTED) {
      digitalWrite(STATUS_LED_PIN, on ? LOW : HIGH);
    } else {
      digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
    }
  }
}

#endif  // USE_LED