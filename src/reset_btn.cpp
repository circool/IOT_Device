#include "reset_btn.h"
#include "logger.h"

#if FEATURE_RESET_BUTTON_ENABLED == 1

static unsigned long g_pressStartTime = 0;
static bool g_isPressed = false;
static ResetButtonStage g_currentStage = RELEASED;

static ResetButtonStage getStageForDuration(unsigned long duration) {
  if (duration < 1000)
    return PRESSED;
  if (duration < 2000)
    return STAGE_1S;
  if (duration < 3000)
    return STAGE_2S;
  return STAGE_3S;
}

void resetBtn_init() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  delay(10);
  g_isPressed = false;
  g_pressStartTime = 0;
  g_currentStage = RELEASED;
  XLOG_INFO(CAT_RESET_BTN, "Reset button initialized on pin %d", RESET_PIN);
}

ResetButtonStage resetBtn_getState() {
  bool isPressed = (digitalRead(RESET_PIN) == LOW);

  if (isPressed && !g_isPressed) {
    g_isPressed = true;
    g_pressStartTime = millis();
    g_currentStage = PRESSED;
    return g_currentStage;
  }

  if (isPressed && g_isPressed) {
    unsigned long duration = millis() - g_pressStartTime;
    ResetButtonStage newStage = getStageForDuration(duration);
    if (newStage != g_currentStage) {
      g_currentStage = newStage;
    }
    return g_currentStage;
  }

  if (!isPressed && g_isPressed) {
    g_isPressed = false;
    g_pressStartTime = 0;
    g_currentStage = RELEASED;
    return g_currentStage;
  }

  return g_currentStage;
}

#endif