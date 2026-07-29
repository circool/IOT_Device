#include "restart_manager.h"
#include "logger.h"

static bool _pending = false;
static unsigned long _time = 0;

void restart_request(unsigned long delayMs) {
  if (_pending)
    return;
  _pending = true;
  _time = millis() + delayMs;
  XLOG_INFO(CAT_RESTART, "Restart requested (delay: %lu ms)",delayMs);  
}

void restart_update() {
  if (!_pending)
    return;
  if (millis() >= _time) {
    XLOG_INFO(CAT_RESTART, "Restarting...");
    Serial.flush();
    delay(100);
    ESP.restart();
  }
}