/**
 * @file button_manager.cpp
 * @brief Реализация кнопки управления
 * @version 0.12
 * @date 10.08.2026
 */

#include "button_manager.h"
#include "logger.h"

#ifdef USE_BUTTON

// ============ Константы ============

#define BUTTON_SHORT_MAX 500  // 0.5 секунды (короткое нажатие)
#define BUTTON_MID_MIN 1000   // 1 секунда
#define BUTTON_LONG_MIN 2000  // 2 секунды
#define BUTTON_WARN_MIN 3000  // 3 секунды (предупреждение)
#define BUTTON_HOLD_MIN 4000  // 4 секунды (минимальная длительность для HOLD)
#define BUTTON_HOLD_MAX 5000  // 5 секунд (защита от залипания)

// ============ Публичные методы ============

void ButtonManager::init() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  _isPressed = false;
  _pressStartTime = 0;
  _currentStage = BUTTON_IDLE;
  _lastState = false;

  XLOG_DEBUG(CAT_BUTTON, "Button initialized: pin=%d, inverted=%d", BUTTON_PIN,
            BUTTON_INVERTED);
}

void ButtonManager::update() {
  // Чтение пина с учётом инверсии
  bool rawState = digitalRead(BUTTON_PIN);
  bool isPressedNow = BUTTON_INVERTED ? !rawState : rawState;

  // Детектирование фронтов
  if (isPressedNow && !_lastState) {
    // ========== НАЖАТИЕ ==========
    _isPressed = true;
    _pressStartTime = millis();
    _currentStage = BUTTON_IDLE;
    XLOG_DEBUG(CAT_BUTTON, "Button PRESSED");
  } else if (isPressedNow && _lastState) {
    // ========== УДЕРЖАНИЕ ==========
    unsigned long duration = millis() - _pressStartTime;

    // Защита от залипания: > 5 секунд → IDLE
    if (duration > BUTTON_HOLD_MAX) {
      if (_currentStage != BUTTON_IDLE) {
        _currentStage = BUTTON_IDLE;
        XLOG_DEBUG(CAT_BUTTON, "Button timeout (>5s), reset to IDLE");
      }
    } else {
      // Обновляем стадию удержания
      ButtonStage newStage = _calculateHoldStage(duration);
      if (newStage != _currentStage) {
        _currentStage = newStage;
        XLOG_DEBUG(CAT_BUTTON, "Button stage: %d (%lu ms)", _currentStage,
                   duration);
      }
    }
  } else if (!isPressedNow && _lastState) {
    // ========== ОТПУСКАНИЕ ==========
    unsigned long duration = millis() - _pressStartTime;

    // Определяем финальную стадию на основе длительности
    ButtonStage finalStage = _calculateReleaseStage(duration);

    if (finalStage != BUTTON_IDLE) {
      _currentStage = finalStage;
      XLOG_DEBUG(CAT_BUTTON, "Button RELEASED (duration=%lu ms, stage=%d)",
                 duration, finalStage);
    } else {
      _currentStage = BUTTON_IDLE;
      XLOG_DEBUG(CAT_BUTTON, "Button RELEASED (duration=%lu ms, ignored)",
                 duration);
    }

    _isPressed = false;
    _pressStartTime = 0;
  }

  _lastState = isPressedNow;
}

ButtonStage ButtonManager::getStage() const {
  return _currentStage;
}

void ButtonManager::clearEvent() {
  if (_currentStage == BUTTON_SHORT || _currentStage == BUTTON_HOLD) {
    _currentStage = BUTTON_IDLE;
    XLOG_DEBUG(CAT_BUTTON, "Event cleared: %d", _currentStage);
  }
}

// ============ Приватные методы ============

ButtonStage ButtonManager::_calculateHoldStage(unsigned long duration) const {
  if (duration < BUTTON_MID_MIN)
    return BUTTON_IDLE;
  if (duration < BUTTON_LONG_MIN)
    return BUTTON_MID;
  if (duration < BUTTON_WARN_MIN)
    return BUTTON_LONG;
  // Дальше — только WARN, даже на 4-5 секундах!
  // HOLD устанавливается ТОЛЬКО при отпускании
  return BUTTON_WARN;
}

ButtonStage ButtonManager::_calculateReleaseStage(
    unsigned long duration) const {
  // Короткое нажатие (< 0.5с)
  if (duration < BUTTON_SHORT_MAX) {
    return BUTTON_SHORT;
  }

  // HOLD: удержание от 4 до 5 секунд И отпущена
  if (duration >= BUTTON_HOLD_MIN && duration <= BUTTON_HOLD_MAX) {
    return BUTTON_HOLD;
  }

  // Остальные случаи (0.5-4с или >5с) → игнорируем
  return BUTTON_IDLE;
}

#endif  // USE_BUTTON