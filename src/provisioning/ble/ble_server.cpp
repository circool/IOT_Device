/**
 * @file ble_server.cpp
 * @brief Реализация BLE-сервера провизионинга для ESP32
 *
 * @details Реализует BLE-сервер для ESP BLE Provisioning протокола.
 *          Использует библиотеку WiFiProv от Espressif.
 *
 * @see
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiProv
 */

#include "ble_server.h"
#include "logger.h"
#include "settings.h"

#if defined(ESP32) && !defined(ESP8266)

#include <WiFi.h>
#include <WiFiProv.h>

// ============================================================================
// СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ
// ============================================================================

static ProvConfigCallback g_configCallback = nullptr;
static BleWifiConfig g_receivedConfig;
static bool g_credentialsReceived = false;

/**
 * @brief UUID сервиса для ESP BLE Provisioning
 * @details Стандартный UUID, используемый приложением ESP BLE Provisioning
 */
static const uint8_t PROV_UUID[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b,
                                      0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03,
                                      0x04, 0x90, 0x1a, 0x02};

// ============================================================================
// ОБРАБОТЧИК СОБЫТИЙ
// ============================================================================

/**
 * @brief Обработчик системных событий Arduino
 *
 * @details Обрабатывает события провизионинга:
 *          - PROV_CRED_RECV — получены WiFi креденшелы
 *          - PROV_CRED_FAIL — ошибка авторизации
 *          - PROV_CRED_SUCCESS — успешное завершение
 *
 * @param sys_event Указатель на событие
 */
void SysProvEvent(arduino_event_t* sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_CRED_RECV: {
      if (sys_event->event_info.prov_cred_recv.ssid) {
        memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
        strncpy(g_receivedConfig.wifiSsid,
                (char*)sys_event->event_info.prov_cred_recv.ssid,
                sizeof(g_receivedConfig.wifiSsid) - 1);
        strncpy(g_receivedConfig.wifiPassword,
                (char*)sys_event->event_info.prov_cred_recv.password,
                sizeof(g_receivedConfig.wifiPassword) - 1);
        g_credentialsReceived = true;
      }
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_FAIL:
      if (g_configCallback) {
        g_configCallback(nullptr);  // Передаём nullptr при ошибке
      }
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      if (g_credentialsReceived && g_configCallback) {
        g_configCallback(&g_receivedConfig);
        g_credentialsReceived = false;
      }
      break;

    default:
      break;
  }
}

// ============================================================================
// РЕАЛИЗАЦИЯ КЛАССА
// ============================================================================

BleProvisioningServer::BleProvisioningServer(const char* deviceName) {
  if (deviceName && strlen(deviceName) < sizeof(_deviceName)) {
    strncpy(_deviceName, deviceName, sizeof(_deviceName) - 1);
    _deviceName[sizeof(_deviceName) - 1] = '\0';
  } else {
    strncpy(_deviceName, "PROV_123", sizeof(_deviceName) - 1);
  }
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
}

BleProvisioningServer::~BleProvisioningServer() {
  stop();
}

bool BleProvisioningServer::begin(ProvConfigCallback configCallback,
                                  ProvStatusCallback statusCallback) {
  if (_active)
    return true;

  g_configCallback = configCallback;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));

  (void)statusCallback;  // Пока не используется, но оставлен для совместимости

  LOG_DEBUG(CAT_PROVISIONING, "Starting BLE Provisioning server");
  LOG_DEBUG(CAT_PROVISIONING, "Device name: %s", _deviceName);
  LOG_DEBUG(CAT_PROVISIONING, "PIN: %s", BLE_PROVISIONING_PIN);
  LOG_INFO(CAT_PROVISIONING, "Use ESP BLE Prov app");

  // Регистрируем обработчик событий WiFi
  WiFi.onEvent(SysProvEvent);

  // Запускаем провизионинг
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE,
                          WIFI_PROV_SCHEME_HANDLER_FREE_BLE,
                          WIFI_PROV_SECURITY_1, BLE_PROVISIONING_PIN,
                          _deviceName, NULL, (uint8_t*)PROV_UUID,
                          true  // reset_provisioned
  );

  _active = true;
  return true;
}

void BleProvisioningServer::stop() {
  if (!_active)
    return;

  _active = false;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));

  WiFi.removeEvent(SysProvEvent);
}

bool BleProvisioningServer::isActive() const {
  return _active;
}

const char* BleProvisioningServer::getDeviceName() const {
  return _deviceName;
}

void BleProvisioningServer::setDeviceName(const char* name) {
  if (name && strlen(name) < sizeof(_deviceName)) {
    strncpy(_deviceName, name, sizeof(_deviceName) - 1);
    _deviceName[sizeof(_deviceName) - 1] = '\0';
  }
}

#endif  // ESP32