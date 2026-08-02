/**
 * @file provisioning_ap_server.h
 * @brief AP-сервер для провизионинга
 */

#ifndef PROVISIONING_AP_SERVER_H
#define PROVISIONING_AP_SERVER_H

#include <Arduino.h>
#include "settings.h"

// ============================================================================
// НАСТРОЙКИ
// ============================================================================

#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"
#endif

// ============================================================================
// API
// ============================================================================

#if USE_AP_PROVISIONING == 1

/**
 * @brief Запустить AP-сервер провизионинга
 * @param deviceId Имя устройства (SSID точки доступа)
 * @return true при успешном запуске
 */
bool ap_server_start(const char* deviceId);

/**
 * @brief Периодическая обработка AP-сервера
 */
void ap_server_update();

/**
 * @brief Остановить AP-сервер
 */
void ap_server_stop();

/**
 * @brief Проверить, активен ли AP-сервер
 */
bool ap_server_is_running();

/**
 * @brief Получить SSID, полученный через AP
 */
const char* ap_server_get_ssid();

/**
 * @brief Получить пароль, полученный через AP
 */
const char* ap_server_get_password();

/**
 * @brief Проверить, получены ли данные через AP
 */
bool ap_server_has_data();

/**
 * @brief Очистить полученные данные
 */
void ap_server_clear_data();

#else

// ============================================================================
// ЗАГЛУШКИ
// ============================================================================

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline bool ap_server_start(const char* deviceId) {
  (void)deviceId;
  return false;
}

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline void ap_server_update() {}

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline void ap_server_stop() {}

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline bool ap_server_is_running() {
  return false;
}

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline const char* ap_server_get_ssid() {
  return nullptr;
}

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline const char* ap_server_get_password() {
  return nullptr;
}

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline bool ap_server_has_data() {
  return false;
}

// Заглушка - AP-провизионинг отключён (USE_AP_PROVISIONING=0)
inline void ap_server_clear_data() {}

#endif  // USE_AP_PROVISIONING

#endif  // PROVISIONING_AP_SERVER_H