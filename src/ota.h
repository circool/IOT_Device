#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include "web.h"  // ← для WebServerClass

#if OTA_ENABLED == 1

/**
 * @brief Инициализация OTA
 * @param server Указатель на WebServer (инъекция зависимости)
 */
void ota_init(WebServerClass* server);

/**
 * @brief Установить флаг доступности OTA
 * @param available true — OTA доступен (достаточно flash)
 */
void ota_set_available(bool available);

/**
 * @brief Проверить доступность OTA
 * @return true — OTA доступен
 */
bool ota_is_available();

#else
// Пустые заглушки
inline void ota_init(WebServerClass* server) {
  (void)server;
}
inline void ota_set_available(bool available) {
  (void)available;
}
inline bool ota_is_available() {
  return false;
}
#endif

#endif  // OTA_H