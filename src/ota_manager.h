/**
 * @file ota_manager.cpp
 * @brief Обновления прошивки по воздухуъ
 */
#ifndef OTA_NAMAGER_H
#define OTA_NAMAGER_H

#include <Arduino.h>
#include "web_manager.h"

#ifndef FEATURE_OTA_ENABLED
#define FEATURE_OTA_ENABLED 1
#endif

#if FEATURE_OTA_ENABLED == 1

void ota_manager_init(WebServerClass* server);
bool ota_is_available();

/**
 * @brief Получить HTML-код кнопки OTA
 * @return HTML-строка с кнопкой или сообщением о недоступности
 */
String ota_getButtonHtml();

void ota_manager_update();

#else

// Заглушка - OTA слой отключён (FEATURE_OTA_ENABLED=0)
inline void ota_manager_init(WebServerClass* server) {
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
inline void ota_manager_update() {}

#endif

#endif  // OTA_NAMAGER_H