/**
 * @file restart_manager.cpp
 * @brief Реализация менеджера перезагрузок
 */

#include "restart_manager.h"
#include "logger.h"
#include "state_provider.h"

static bool _pending = false;
static unsigned long _time = 0;

void restartRequest(unsigned long delayMs) {
  if (_pending) {
    XLOG_DEBUG(CAT_RESTART, "Restart already pending, ignoring duplicate");
    return;
  }

  _pending = true;
  _time = millis() + delayMs;
  XLOG_INFO(CAT_RESTART, "Restart requested (delay: %lu ms)", delayMs);
}

void restartUpdate() {
  if (!_pending) {
    return;
  }

  if (millis() >= _time) {
    XLOG_INFO(CAT_RESTART, "Restarting...");

    Serial.flush();
    delay(100);
    ESP.restart();
  }
}