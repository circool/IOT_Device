/**
 * @file web_ota_manager.h
 * @brief OTA-подсистема Web-слоя
 */

#ifndef WEB_OTA_MANAGER_H
#define WEB_OTA_MANAGER_H

#include <Arduino.h>
#include "web_common.h"

#ifndef FEATURE_OTA_ENABLED
#define FEATURE_OTA_ENABLED 1
#endif

#ifdef USE_OTA

/**
 * @brief Инициализация OTA-подсистемы
 * @param server Указатель на WebServerClass (передаётся из web_manager)
 * @note Регистрирует маршрут /update через ElegantOTA
 * @note Если OTA недоступна (Flash < 2MB) — ничего не делает
 */
void web_ota_manager_init(WebServerClass* server);

/**
 * @brief Проверить доступность OTA
 * @return true если OTA доступна (Flash ≥ 2MB), false в противном случае
 * @note При FEATURE_OTA_ENABLED == 0 возвращает false (заглушка)
 */
bool ota_is_available();

/**
 * @brief Получить HTML-код кнопки OTA
 * @return HTML-строка с кнопкой или сообщением о недоступности
 * @note Используется Web-слоем для отображения кнопки на странице конфигурации
 */
String ota_getButtonHtml();

/**
 * @brief Периодическая обработка OTA
 * @note ElegantOTA 2.2.x не требует отдельного вызова в loop()
 * @note Функция оставлена для единообразия с другими слоями
 */
void web_ota_manager_update();
#else

// Заглушка - OTA отключён (FEATURE_OTA_ENABLED=0)
inline void web_ota_manager_init(WebServerClass* server) {
  (void)server;
}

// Заглушка - OTA отключён (FEATURE_OTA_ENABLED=0)
inline bool ota_is_available() {
  return false;
}

// Заглушка - OTA отключён (FEATURE_OTA_ENABLED=0)
inline String ota_getButtonHtml() {
  return String();
}

// Заглушка - OTA отключён (FEATURE_OTA_ENABLED=0)
inline void web_ota_manager_update() {}

#endif

#endif  // WEB_OTA_MANAGER_H