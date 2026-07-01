/**
 * @file ble_server.h
 * @brief BLE Provisioning Server для ESP32
 *
 * Реализует BLE-комиссионинг через протокол ESP BLE Provisioning.
 * Совместим с приложением ESP BLE Provisioning (доступно в Play Store).
 *
 * @note Для настройки используйте приложение "ESP BLE Provisioning" от
 * Espressif
 * @see https://play.google.com/store/apps/details?id=com.espressif.bleprov
 */

#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>
#include <functional>
#include "settings.h"

// Вместо forward declaration включаем правильный заголовок
// Но только если мы компилируем для ESP32
#if defined(ESP32) && !defined(ESP8266)
#include <WiFiGeneric.h>  // Содержит полное определение arduino_event_t
#endif

struct BleConfigData {
  char wifiSsid[32];
  char wifiPassword[64];
  char mqttBroker[64];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPassword[64];
  char mqttClientId[24];
  char zigbeeNetworkKey[32];
  uint16_t zigbeePanId;
  uint8_t zigbeeChannel;
  uint8_t protocolMode;
};

using ProvConfigCallback = std::function<void(const BleConfigData* config)>;
using ProvStatusCallback = std::function<void(uint8_t status)>;

enum class BleProvisioningState : uint8_t {
  IDLE = 0,
  WAITING_CREDENTIALS,
  CONNECTING_WIFI,
  WIFI_CONNECTED,
  WIFI_AUTH_ERROR,
  TIMEOUT,
  MAX_ATTEMPTS_REACHED,
  STOPPED
};

/**
 * @brief BLE Provisioning Server
 *
 * Устройство становится доступно как BLE-устройство с именем {prefix}_XXXX.
 * Для настройки используйте приложение ESP BLE Provisioning.
 *
 * @section provisioning_flow Процесс настройки:
 * 1. Устройство запускает BLE-сервер при первом включении
 * 2. Пользователь подключается через ESP BLE Provisioning
 * 3. Вводит SSID и пароль WiFi
 * 4. Устройство пытается подключиться (максимум 3 попытки)
 * 5. При успехе — сохраняет настройки и перезагружается
 * 6. При ошибке (неверный пароль/SSID) — перезапускает процесс
 *
 * @section app_settings Настройки в приложении:
 * - Security: Security 1 (PoP)
 * - PoP (Proof of Possession): 12345678
 * - Device Name: {prefix}_{XXXX} (например, fan_ABCD)
 *
 * @section supported_platforms Поддерживаемые платформы:
 * - ESP32 (классический)
 * - ESP32-C3
 * - ESP32-S3
 * - ESP32-C6 (с BLE)
 *
 * @note Для ESP8266 BLE не доступен — используется AP-режим
 */
class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr);
  ~BleProvisioningServer();

  bool begin(ProvConfigCallback configCallback,
             ProvStatusCallback statusCallback = nullptr,
             uint32_t timeoutMs = BLE_PROVISIONING_TIMEOUT_MS);

  void stop();
  void deinit();

  bool isActive() const;
  BleProvisioningState getState() const;
  void process();

  void onWiFiEvent(arduino_event_t* event);

 private:
  bool startWiFiProvisioning();

  bool _active = false;
  bool _provisioningStarted = false;
  char _deviceName[32];
  BleConfigData _config;
  BleProvisioningState _state = BleProvisioningState::IDLE;

  ProvConfigCallback _configCallback;
  ProvStatusCallback _statusCallback;

  static const uint8_t MAX_WIFI_ATTEMPTS = BLE_PROVISIONING_MAX_ATTEMPTS;
  uint8_t _wifiAttempts = 0;
  uint32_t _startTimeMs = 0;
  uint32_t _timeoutMs = 0;
  bool _credentialsReceived = false;
  bool _configSaved = false;
};

#endif  // BLE_SERVER_H