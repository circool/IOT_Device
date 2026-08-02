/**
 * @file web_ota_manager.cpp
 * @brief Обновления прошивки по воздухуъ
 */
#ifndef WEB_OTA_MANAGER_H
#define WEB_OTA_MANAGER_H

#include <Arduino.h>
#include "web_manager.h"

#ifndef FEATURE_OTA_ENABLED
#define FEATURE_OTA_ENABLED 1
#endif

#if FEATURE_OTA_ENABLED == 1

void web_ota_manager_init(WebServerClass* server);
bool ota_is_available();

/**
 * @brief Получить HTML-код кнопки OTA
 * @return HTML-строка с кнопкой или сообщением о недоступности
 */
String ota_getButtonHtml();

void web_ota_manager_update();

#else

// Заглушка - OTA слой отключён (FEATURE_OTA_ENABLED=0)
inline void web_ota_manager_init(WebServerClass* server) {
  (void)server;
}

// Заглушка - OTA слой отключён (FEATURE_OTA_ENABLED=0)
inline bool ota_is_available() {
  return false;
}

// Заглушка - OTA слой отключён (FEATURE_OTA_ENABLED=0)
inline String ota_getButtonHtml() {
  return String();
}

// Заглушка - OTA слой отключён (FEATURE_OTA_ENABLED=0)
inline void web_ota_manager_update() {}

#endif

#endif  // WEB_OTA_MANAGER_H