/**
 * @file provisioning_ble_server.cpp
 * @brief BLE-сервер для провизионинга
 */

#include "provisioning_ble_server.h"
#include "logger.h"
#include "settings.h"
#include "transport_manager_provisioning.h"

#ifdef USE_BLE

#ifdef ESP32
BleProvisioningServer* g_bleServer = nullptr;

#include <WiFi.h>
#include <WiFiProv.h>

// Определяем, какие макросы использовать
// Для ESP32-C6/H2 используем NETWORK_PROV_*, для остальных — WIFI_PROV_*
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
#define PROV_SCHEME_BLE NETWORK_PROV_SCHEME_BLE
#define PROV_SCHEME_HANDLER_FREE NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM
#define PROV_SECURITY NETWORK_PROV_SECURITY_1
#else
#define PROV_SCHEME_BLE WIFI_PROV_SCHEME_BLE
#define PROV_SCHEME_HANDLER_FREE WIFI_PROV_SCHEME_HANDLER_FREE_BTDM
#define PROV_SECURITY WIFI_PROV_SECURITY_1
#endif

static const uint8_t PROV_UUID[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b,
                                      0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03,
                                      0x04, 0x90, 0x1a, 0x02};

static BleWifiConfig g_receivedConfig;
static bool g_credentialsReceived = false;
static ProvisioningManager* g_provManager = nullptr;

static void SysProvEvent(arduino_event_t* sys_event) {
  if (!g_provManager) {
    XLOG_WARN(CAT_BLE, "ProvisioningManager not set");
    return;
  }

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
      }
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_FAIL: {
      XLOG_WARN(CAT_BLE, "Credentials failed");
      g_provManager->onBleError();
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_SUCCESS: {
      XLOG_DEBUG(CAT_BLE, "Provisioning successful");

      if (g_credentialsReceived) {
        g_provManager->onDataReceived(g_receivedConfig.wifiSsid,
                                      g_receivedConfig.wifiPassword);
        g_credentialsReceived = false;
      }
      break;
    }

    case ARDUINO_EVENT_PROV_END: {
      XLOG_DEBUG(CAT_BLE, "Provisioning ended");
      break;
    }

    default:
      break;
  }
}

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

bool BleProvisioningServer::begin(ProvisioningManager* manager) {
  if (_active)
    return true;

  if (!manager) {
    XLOG_ERROR(CAT_BLE, "ProvisioningManager is null");
    return false;
  }

  g_provManager = manager;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));

  WiFi.onEvent(SysProvEvent);

  // Используем макросы, определенные выше
  WiFiProv.beginProvision(PROV_SCHEME_BLE, PROV_SCHEME_HANDLER_FREE,
                          PROV_SECURITY, BLE_PROVISIONING_PIN, _deviceName,
                          NULL, (uint8_t*)PROV_UUID, true);

  _active = true;

  XLOG_DEBUG(CAT_BLE,
             "BLE provisioning started. "
             "Use ESP BLE Prov app, find device " ANSI_BOLD
             "%s " ANSI_BOLD_RESET "and use PIN " ANSI_BOLD "%s",
             _deviceName, BLE_PROVISIONING_PIN);
  return true;
}

void BleProvisioningServer::stop() {
  if (!_active)
    return;

  _active = false;
  g_credentialsReceived = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
  g_provManager = nullptr;

  WiFi.removeEvent(SysProvEvent);
  XLOG_DEBUG(CAT_BLE, "BLE provisioning stopped");
}

#endif
#endif // USE_BLE