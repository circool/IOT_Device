#ifndef RESET_BTN_H
#define RESET_BTN_H

#include <Arduino.h>
#include "settings.h"

#ifndef FEATURE_RESET_BUTTON_ENABLED
#define FEATURE_RESET_BUTTON_ENABLED 1
#endif

#ifndef RESET_PIN
#define RESET_PIN 0
#endif

enum ResetButtonStage : uint8_t {
  RELEASED = 0,
  PRESSED = 1,
  STAGE_1S = 2,
  STAGE_2S = 3,
  STAGE_3S = 4
};

#if FEATURE_RESET_BUTTON_ENABLED == 1

void resetBtn_init();
ResetButtonStage resetBtn_getState();

#else

inline void resetBtn_init() {}
inline ResetButtonStage resetBtn_getState() {
  return RELEASED;
}

#endif

#endif