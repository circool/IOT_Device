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
#include "config_manager.h"
#include "logger.h"
#include "settings.h"
#include "wifi_manager.h"

#if defined(ESP32) && !defined(ESP8266)

#include <WiFi.h>
#include <WiFiProv.h>

// ============================================================================
// СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ
// ============================================================================

static BleProvisioningServer* g_provServer =
    nullptr;  //!< Глобальный указатель на сервер
static ProvConfigCallback g_configCallback =
    nullptr;                                //!< Глобальный колбэк конфигурации
static BleConfigData g_receivedConfig;      //!< Буфер полученной конфигурации
static bool g_credentialsReceived = false;  //!< Флаг получения креденшелов

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
 *          - PROV_START — начало процесса
 *          - PROV_CRED_RECV — получены креденшелы
 *          - PROV_CRED_FAIL — ошибка авторизации
 *          - PROV_CRED_SUCCESS — успешное завершение
 *          - PROV_END — окончание провизионинга
 *
 * @param sys_event Указатель на событие
 *
 * @note Реализован по официальной документации ESP32 WiFiProv
 */
void SysProvEvent(arduino_event_t* sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      // Serial.printf("\nProvisioning Started\n");
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV: {
      // Serial.printf("\nReceived Wi-Fi credentials\n");
      // Serial.printf("\tSSID : %s\n",
      //               (const char*)sys_event->event_info.prov_cred_recv.ssid);
      // Serial.printf("\tPassword : %s\n",
                    // (const char*)sys_event->event_info.prov_cred_recv.password);

      // Сохраняем полученные данные
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
      Serial.printf("\nProvisioning failed!\n");
      if (g_configCallback) {
        g_configCallback(nullptr);  // Передаём nullptr при ошибке
      }
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.printf("\nProvisioning Successful\n");
      if (g_credentialsReceived && g_configCallback) {
        g_configCallback(&g_receivedConfig);
        g_credentialsReceived = false;
      }
      break;

    case ARDUINO_EVENT_PROV_END:
      Serial.printf("\nProvisioning Ends\n");
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
  g_provServer = this;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
}

BleProvisioningServer::~BleProvisioningServer() {
  stop();
  g_provServer = nullptr;
}

bool BleProvisioningServer::begin(ProvConfigCallback configCallback,
                                  ProvStatusCallback statusCallback,
                                  ProvConnCallback connCallback,
                                  ProvIpCallback ipCallback) {
  if (_active)
    return true;

  g_configCallback = configCallback;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));

  (void)statusCallback;
  (void)connCallback;
  (void)ipCallback;

  LOG_INFO(CAT_PROVISIONING, "========================================");
  LOG_INFO(CAT_PROVISIONING, "Starting BLE Provisioning");
  LOG_INFO(CAT_PROVISIONING, "Device name: %s", _deviceName);
  LOG_INFO(CAT_PROVISIONING, "PIN: %s", BLE_PROVISIONING_PIN);
  LOG_INFO(CAT_PROVISIONING, "========================================");

  // Регистрируем обработчик событий WiFi
  WiFi.onEvent(SysProvEvent);

  // Запускаем провизионинг по документации
  // Параметры:
  //   WIFI_PROV_SCHEME_BLE - используем BLE транспорт
  //   WIFI_PROV_SCHEME_HANDLER_FREE_BLE - стандартный обработчик
  //   WIFI_PROV_SECURITY_1 - уровень безопасности 1
  //   BLE_PROVISIONING_PIN - PIN для сопряжения (PoP)
  //   _deviceName - имя устройства в BLE
  //   NULL - пользовательские данные (не используются)
  //   (uint8_t*)PROV_UUID - UUID сервиса
  //   true - сбросить предыдущие данные провизионинга
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE,
                          WIFI_PROV_SCHEME_HANDLER_FREE_BLE,
                          WIFI_PROV_SECURITY_1, BLE_PROVISIONING_PIN,
                          _deviceName, NULL, (uint8_t*)PROV_UUID,
                          true  // reset_provisioned
  );

  _active = true;
  LOG_INFO(CAT_PROVISIONING, "BLE provisioning started successfully!");
  LOG_INFO(CAT_PROVISIONING, "Use ESP BLE Provisioning app");

  return true;
}

void BleProvisioningServer::stop() {
  if (!_active)
    return;

  _active = false;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));

  WiFi.removeEvent(SysProvEvent);
  LOG_INFO(CAT_PROVISIONING, "BLE provisioning stopped");
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

void BleProvisioningServer::sendStatus(uint8_t status) {
  (void)status;
}

void BleProvisioningServer::sendConnectionStatus(bool connected,
                                                 const char* ip) {
  (void)connected;
  (void)ip;
}

#endif  // ESP32