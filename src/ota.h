#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include "web.h"  // Для WebServerClass (реальный или заглушка)

#if OTA_ENABLED == 1

/**
 * @brief Инициализация OTA
 * @param server Указатель на WebServer (инъекция зависимости)
 */
void ota_init(WebServerClass* server);


/**
 * @brief Проверить доступность OTA
 * @return true — OTA доступен
 */
bool ota_is_available();

#else

// Заглушки
inline void ota_init(WebServerClass* server) {
  (void)server;
}

inline bool ota_is_available() {
  return false;
}

#endif

#endif  // OTA_H