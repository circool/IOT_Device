/**
 * @file transport_wifi.h
 * @version 0.12
 * @brief WiFi-транспорт — реализация Transport для среды WiFi
 */

#ifndef TRANSPORT_WIFI_H
#define TRANSPORT_WIFI_H

#include "settings.h"
#include "transport.h"
#include "transport_wifi_mqtt.h"
#include "transport_manager_provisioning.h"
#include "transport_wifi_web.h"
#include "transport_types.h"

// ============================================================================
// WiFi-ТРАНСПОРТ
// ============================================================================

#ifdef USE_WIFI

// ============================================================================
// СОСТОЯНИЯ WiFi
// ============================================================================

/**
 * @brief Состояния подключения WiFi
 */
typedef enum {
  WIFI_STATE_DISCONNECTED,  ///< WiFi отключён
  WIFI_STATE_CONNECTING,    ///< Идёт подключение к WiFi
  WIFI_STATE_CONNECTED,     ///< WiFi подключён
  WIFI_STATE_PROVISIONING   ///< Режим настройки (AP)
} WiFiState;

// ============================================================================
// WiFi-ТРАНСПОРТ
// ============================================================================

/**
 * @brief WiFi-транспорт — реализация Transport для среды WiFi
 * @details Управляет подключением к WiFi, HTTP-сервером, MQTT-клиентом
 *          и режимом настройки (AP).
 */
typedef struct WiFiTransport {
  Transport base;  ///< Базовый интерфейс транспорта

  // ===== СРЕДА =====
  ProvisioningManager provisioning;  ///< Менеджер провизионинга (AP/BLE)

  // ===== ПРОТОКОЛЫ =====
  WebManager web;    ///< HTTP-сервер (читает данные по указателям)
  MQTTManager mqtt;  ///< MQTT-клиент (публикует по команде)

  // ===== СОСТОЯНИЕ =====
  bool initialized;     ///< Транспорт инициализирован
  bool connected;       ///< WiFi подключён
  WiFiState wifiState;  ///< Текущее состояние WiFi
  int connectAttempts;  ///< Счётчик попыток подключения

  // ===== УКАЗАТЕЛИ ДЛЯ КОМПОНЕНТОВ =====
  const TransportConfig* config;         ///< Конфигурация транспорта
  const DeviceConfig* deviceConfig;      ///< Конфигурация устройства (для Web)
  const DeviceState* deviceState;        ///< Состояние устройства (для Web)
  const TransportState* transportState;  ///< Состояние транспорта (для Web)

  // ===== ВНУТРЕННЕЕ СОСТОЯНИЕ ТРАНСПОРТА =====
  TransportState _state;  ///< Состояние (link_ok/gateway_ok/setup_mode)
  TransportEventCallback _eventCallback;  ///< Колбэк для событий
  void* _eventContext;                    ///< Контекст колбэка

} WiFiTransport;

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Получить глобальный экземпляр WiFi-транспорта
 * @return Указатель на структуру Transport
 * @note Транспорт создаётся статически, функция возвращает указатель на него
 */
Transport* getWiFiTransport();

/**
 * @brief Опубликовать RSSI (уровень сигнала WiFi)
 * @param rssi Уровень сигнала в dBm (отрицательное значение)
 * @note Вызывается из transport_update() с заданным интервалом
 */
void transport_publishRSSI(int rssi);

#else  // USE_WIFI == 0

// ============================================================================
// ЗАГЛУШКИ (USE_WIFI == 0)
// ============================================================================

/**
 * @brief Заглушка WiFiTransport — WiFi-транспорт отключён
 * @details Используется при TRANSPORT_TYPE != WIFI
 *          Структура существует только для совместимости типов.
 */
typedef struct WiFiTransport {
  Transport base;
} WiFiTransport;

/**
 * @brief Заглушка — WiFi-транспорт отключён
 * @return Всегда возвращает nullptr
 */
inline Transport* getWiFiTransport() {
  return nullptr;
}

/**
 * @brief Заглушка — WiFi-транспорт отключён
 */
inline void transport_publishRSSI(int rssi) {
  (void)rssi;
}

#endif  // USE_WIFI

#endif  // TRANSPORT_WIFI_H