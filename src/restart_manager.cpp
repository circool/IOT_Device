/**
 * @file restart_manager.cpp
 * @brief Реализация менеджера перезагрузки
 * @version 0.12
 * @date 10.08.2026
 */
#include "settings.h"
#include "restart_manager.h"
#include "logger.h"


#ifdef USE_RESTART

// ============ Публичные методы ============

void RestartManager::request(unsigned long delayMs) {
  if (!_pending) {
    _pending = true;
    _requestTime = millis();
    _delayMs = delayMs ? delayMs : 500;
    XLOG_INFO(CAT_RESTART, "Restart requested in %lu ms", _delayMs);
  } else {
    XLOG_DEBUG(CAT_RESTART, "Restart already pending, ignoring duplicate");
  }
}

void RestartManager::update() {
  if (_pending && (millis() - _requestTime >= _delayMs)) {
    XLOG_INFO(CAT_RESTART, "Executing restart...");
    delay(100);
    ESP.restart();
  }
}

bool RestartManager::isPending() const {
  return _pending;
}

void RestartManager::cancel() {
  if (_pending) {
    _pending = false;
    XLOG_INFO(CAT_RESTART, "Restart cancelled");
  }
}

#endif  // USE_RESTART