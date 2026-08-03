/**
 * @file restart_manager.cpp
 * @brief Реализация менеджера перезагрузок
 */

#include "restart_manager.h"
#include "logger.h"
#include "state_provider.h"

static bool _pending = false;
static unsigned long _time = 0;

void restart_request(unsigned long delayMs) {
  if (_pending) {
    XLOG_DEBUG(CAT_RESTART, "Restart already pending, ignoring duplicate");
    return;
  }

  _pending = true;
  _time = millis() + delayMs;

  // @deprecated Будет удалён после перехода на StateProvider
  // system_state_set_bit(STATE_RESTART);

  // Пишем в StateProvider
  StateProvider::getInstance().update_restart(true);

  XLOG_INFO(CAT_RESTART, "Restart requested (delay: %lu ms)", delayMs);
}

void restart_update() {
  if (!_pending) {
    return;
  }

  if (millis() >= _time) {
    XLOG_INFO(CAT_RESTART, "Restarting...");

    // Сбрасываем флаг в StateProvider перед перезагрузкой
    // StateProvider::getInstance().update_restart(false);

    Serial.flush();
    delay(100);
    ESP.restart();
  }
}