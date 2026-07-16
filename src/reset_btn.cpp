/**
 * @file reset_btn.cpp
 * @brief Реализация кнопки сброса
 */

#include "reset_btn.h"
#include "logger.h"
#include "system_state.h"

#if FEATURE_RESET_BUTTON_ENABLED == 1

static unsigned long _pressStartTime = 0;
static bool _isPressed = false;
static ResetButtonStage _stage = RELEASED;

void resetBtn_init() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  _isPressed = false;
  _pressStartTime = 0;
  _stage = RELEASED;
  XLOG_INFO(CAT_RESET_BTN, "Reset button initialized on pin %d", RESET_PIN);
}

void resetBtn_update() {
  bool isPressed = (digitalRead(RESET_PIN) == LOW);

  if (isPressed && !_isPressed) {
    _isPressed = true;
    _pressStartTime = millis();
    _stage = PRESSED;
    system_state_set_bit(STATE_BUTTON_PRESSED);
    XLOG_DEBUG(CAT_RESET_BTN, "Button PRESSED");
    return;
  }

  if (isPressed && _isPressed) {
    unsigned long duration = millis() - _pressStartTime;
    ResetButtonStage newStage;

    if (duration >= 3000) {
      newStage = STAGE_3S;
    } else if (duration >= 2000) {
      newStage = STAGE_2S;
    } else if (duration >= 1000) {
      newStage = STAGE_1S;
    } else {
      newStage = PRESSED;
    }

    if (newStage != _stage) {
      _stage = newStage;
      XLOG_DEBUG(CAT_RESET_BTN, "Button stage: %d (%lu ms)", _stage, duration);
    }
    return;
  }

  if (!isPressed && _isPressed) {
    _isPressed = false;
    _pressStartTime = 0;
    _stage = RELEASED;
    system_state_clear_bit(STATE_BUTTON_PRESSED);
    XLOG_DEBUG(CAT_RESET_BTN, "Button RELEASED");
  }
}

ResetButtonStage resetBtn_get_stage() {
  return _stage;
}

#endif  // FEATURE_RESET_BUTTON_ENABLED