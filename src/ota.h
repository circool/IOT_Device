#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include "web.h"

#ifndef FEATURE_OTA_ENABLED
#define FEATURE_OTA_ENABLED 1
#endif

#if FEATURE_OTA_ENABLED == 1

void ota_init(WebServerClass* server);
bool ota_is_available();

/**
 * @brief Получить HTML-код кнопки OTA
 * @return HTML-строка с кнопкой или сообщением о недоступности
 */
String ota_getButtonHtml();

void ota_loop();

#else

// Заглушки
inline void ota_init(WebServerClass* server) {
  (void)server;
}

inline bool ota_is_available() {
  return false;
}

inline String ota_getButtonHtml() {
  return String();
}

inline void ota_loop() {}

#endif

#endif  // OTA_H