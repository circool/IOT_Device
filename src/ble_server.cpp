/**
 * @file ble_server.cpp
 * @brief Реализация BLE-сервера провизионинга для ESP32
 */

#include "ble_server.h"
#include "logger.h"
#include "settings.h"

#if defined(ESP32) && !defined(ESP8266)

#include <WiFi.h>
#include <WiFiProv.h>

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

static const char* getProvEventName(arduino_event_id_t event_id) {
  switch (event_id) {
    case ARDUINO_EVENT_WIFI_READY:
      return "WIFI_READY";
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      return "WIFI_SCAN_DONE";
    case ARDUINO_EVENT_WIFI_STA_START:
      return "WIFI_STA_START";
    case ARDUINO_EVENT_WIFI_STA_STOP:
      return "WIFI_STA_STOP";
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      return "WIFI_STA_CONNECTED";
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      return "WIFI_STA_DISCONNECTED";
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      return "WIFI_STA_AUTHMODE_CHANGE";
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      return "WIFI_STA_GOT_IP";
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      return "WIFI_STA_GOT_IP6";
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      return "WIFI_STA_LOST_IP";
    case ARDUINO_EVENT_WIFI_AP_START:
      return "WIFI_AP_START";
    case ARDUINO_EVENT_WIFI_AP_STOP:
      return "WIFI_AP_STOP";
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      return "WIFI_AP_STACONNECTED";
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      return "WIFI_AP_STADISCONNECTED";
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      return "WIFI_AP_STAIPASSIGNED";
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      return "WIFI_AP_PROBEREQRECVED";
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      return "WIFI_AP_GOT_IP6";
    case ARDUINO_EVENT_WIFI_FTM_REPORT:
      return "WIFI_FTM_REPORT";
    case ARDUINO_EVENT_ETH_START:
      return "ETH_START";
    case ARDUINO_EVENT_ETH_STOP:
      return "ETH_STOP";
    case ARDUINO_EVENT_ETH_CONNECTED:
      return "ETH_CONNECTED";
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      return "ETH_DISCONNECTED";
    case ARDUINO_EVENT_ETH_GOT_IP:
      return "ETH_GOT_IP";
    case ARDUINO_EVENT_ETH_GOT_IP6:
      return "ETH_GOT_IP6";
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      return "WPS_ER_SUCCESS";
    case ARDUINO_EVENT_WPS_ER_FAILED:
      return "WPS_ER_FAILED";
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      return "WPS_ER_TIMEOUT";
    case ARDUINO_EVENT_WPS_ER_PIN:
      return "WPS_ER_PIN";
    case ARDUINO_EVENT_WPS_ER_PBC_OVERLAP:
      return "WPS_ER_PBC_OVERLAP";
    case ARDUINO_EVENT_SC_SCAN_DONE:
      return "SC_SCAN_DONE";
    case ARDUINO_EVENT_SC_FOUND_CHANNEL:
      return "SC_FOUND_CHANNEL";
    case ARDUINO_EVENT_SC_GOT_SSID_PSWD:
      return "SC_GOT_SSID_PSWD";
    case ARDUINO_EVENT_SC_SEND_ACK_DONE:
      return "SC_SEND_ACK_DONE";
    case ARDUINO_EVENT_PROV_INIT:
      return "PROV_INIT";
    case ARDUINO_EVENT_PROV_DEINIT:
      return "PROV_DEINIT";
    case ARDUINO_EVENT_PROV_START:
      return "PROV_START";
    case ARDUINO_EVENT_PROV_END:
      return "PROV_END";
    case ARDUINO_EVENT_PROV_CRED_RECV:
      return "PROV_CRED_RECV";
    case ARDUINO_EVENT_PROV_CRED_FAIL:
      return "PROV_CRED_FAIL";
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      return "PROV_CRED_SUCCESS";
    case ARDUINO_EVENT_MAX:
      return "EVENT_MAX";
    default:
      return "UNKNOWN";
  }
}

// ============================================================================
// UUID ДЛЯ BLE-СЕРВИСА
// ============================================================================

static const uint8_t PROV_UUID[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b,
                                      0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03,
                                      0x04, 0x90, 0x1a, 0x02};

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ (скрытые)
// ============================================================================

static ProvConfigCallback g_configCallback = nullptr;
static void* g_context = nullptr;
static BleWifiConfig g_receivedConfig;
static bool g_credentialsReceived = false;

// ============================================================================
// ОБРАБОТЧИК СОБЫТИЙ
// ============================================================================

static void SysProvEvent(arduino_event_t* sys_event) {
  const char* eventName = getProvEventName((arduino_event_id_t)sys_event->event_id);
  // XLOG_DEBUG(CAT_BLE, "BLE event: %s", eventName);
  // Serial.print("BLE event: ");
  // Serial.println(getProvEventName((arduino_event_id_t)sys_event->event_id));

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
        XLOG_DEBUG(CAT_BLE, "WiFi credentials received (SSID: %s)",
                  g_receivedConfig.wifiSsid);
        // Serial.print("SSID: ");
        // Serial.println(g_receivedConfig.wifiSsid);
        // Serial.print("Password: ");
        // Serial.println("***");
      }
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_FAIL: {
      XLOG_WARN(CAT_BLE, "Credentials failed");
      // Serial.print("BLE event: ");
      // Serial.println(getProvEventName((arduino_event_id_t)sys_event->event_id));
      if (g_configCallback) {
        g_configCallback(nullptr, g_context);
      }
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_SUCCESS: {
      XLOG_DEBUG(CAT_BLE, "Provisioning successful");
      // Serial.print("BLE event: ");
      // Serial.println(getProvEventName((arduino_event_id_t)sys_event->event_id));

      if (g_credentialsReceived && g_configCallback) {
        g_configCallback(&g_receivedConfig, g_context);
        g_credentialsReceived = false;
      }
      break;
    }

    case ARDUINO_EVENT_PROV_END: {
      XLOG_DEBUG(CAT_BLE, "Provisioning ended");
      // Serial.print("BLE event: ");
      // Serial.println(getProvEventName((arduino_event_id_t)sys_event->event_id));
      break;
    }

    case ARDUINO_EVENT_WIFI_SCAN_DONE: {
      uint16_t number = sys_event->event_info.wifi_scan_done.number;
      uint8_t status = sys_event->event_info.wifi_scan_done.status;
      // XLOG_DEBUG(CAT_BLE, "WiFi scan: %d networks (status=%d)", number,status);
      // Serial.printf("WiFi scan: %d networks (status=%d)\n", number, status);
      break;
    }

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
                                  void* context) {
  if (_active)
    return true;

  g_configCallback = configCallback;
  g_context = context;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));

  // XLOG_DEBUG(CAT_BLE, "Starting BLE Provisioning server");
  // XLOG_DEBUG(CAT_BLE, "Device name: %s", _deviceName);
  // XLOG_DEBUG(CAT_BLE, "PIN: %s", BLE_PROVISIONING_PIN);

  // Serial.println("Starting BLE Provisioning server");
  // Serial.printf("Device name: %s\n", _deviceName);
  // Serial.printf("PIN: %s\n", BLE_PROVISIONING_PIN);
  // Serial.println("Use ESP BLE Prov app");

  WiFi.onEvent(SysProvEvent);

  WiFiProv.beginProvision(
      WIFI_PROV_SCHEME_BLE,
      // WIFI_PROV_SCHEME_HANDLER_FREE_BLE,
      WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
       WIFI_PROV_SECURITY_1,
      BLE_PROVISIONING_PIN, _deviceName, NULL, (uint8_t*)PROV_UUID, true);

  _active = true;

  XLOG_DEBUG(CAT_BLE,
             "BLE provisioning started." ANSI_RESET
             " (Use ESP BLE Prov app, find device " ANSI_BOLD "%s " ANSI_RESET
             "and use PIN " ANSI_BOLD "%s" ANSI_RESET ").",
             _deviceName, BLE_PROVISIONING_PIN);
  return true;
}

void BleProvisioningServer::stop() {
  if (!_active)
    return;
  // Serial.println("Stopping BLE prov");
  _active = false;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));

  WiFi.removeEvent(SysProvEvent);
  XLOG_DEBUG(CAT_BLE, "BLE provisioning stopped");
}

// bool BleProvisioningServer::isActive() const {
//   return _active;
// }

// const char* BleProvisioningServer::getDeviceName() const {
//   return _deviceName;
// }

// void BleProvisioningServer::setDeviceName(const char* name) {
//   if (name && strlen(name) < sizeof(_deviceName)) {
//     strncpy(_deviceName, name, sizeof(_deviceName) - 1);
//     _deviceName[sizeof(_deviceName) - 1] = '\0';
//   }
// }

#endif  // ESP32