// ===== ФАЙЛ: src/provisioning/ble/ble_server.cpp =====
#include "ble_server.h"
#include "config_manager.h"
#include "logger.h"
#include "settings.h"
#include "wifi_manager.h"

#if defined(ESP32) && !defined(ESP8266)

#include <WiFi.h>
#include <WiFiProv.h>

static BleProvisioningServer* g_provServer = nullptr;
static ProvConfigCallback g_configCallback = nullptr;
static ProvStatusCallback g_statusCallback = nullptr;
static BleConfigData g_receivedConfig;
static bool g_credentialsReceived = false;
static bool g_credentialsSaved = false;

static const uint8_t PROV_UUID[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b,
                                      0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03,
                                      0x04, 0x90, 0x1a, 0x02};

void sysProvEvent(arduino_event_t* sys_event) {
  if (!g_provServer)
    return;

  switch (sys_event->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      LOG_INFO(CAT_PROVISIONING, "WiFi connected! IP: %s",
               IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr)
                   .toString()
                   .c_str());

      if (g_credentialsReceived && !g_credentialsSaved) {
        String ssid = WiFi.SSID();
        String password = WiFi.psk();

        LOG_INFO(CAT_PROVISIONING,
                 "Saving credentials (WiFi connected successfully)");
        LOG_INFO(CAT_PROVISIONING, "SSID: %s", ssid.c_str());

        memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
        strncpy(g_receivedConfig.wifiSsid, ssid.c_str(),
                sizeof(g_receivedConfig.wifiSsid) - 1);
        strncpy(g_receivedConfig.wifiPassword, password.c_str(),
                sizeof(g_receivedConfig.wifiPassword) - 1);

        if (g_configCallback) {
          g_configCallback(&g_receivedConfig);
          g_credentialsSaved = true;
        }
      }
      break;
    }

    case ARDUINO_EVENT_PROV_START:
      LOG_INFO(CAT_PROVISIONING, "Provisioning started");
      g_credentialsReceived = false;
      g_credentialsSaved = false;
      memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
      if (g_statusCallback)
        g_statusCallback(0x01);
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV: {
      LOG_INFO(CAT_PROVISIONING, "Credentials received from BLE");
      g_credentialsReceived = true;
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_FAIL: {
      LOG_ERROR(CAT_PROVISIONING,
                "Provisioning failed - WiFi authentication error");
      g_credentialsReceived = false;
      memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
      if (g_statusCallback)
        g_statusCallback(0x04);
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      LOG_INFO(CAT_PROVISIONING,
               "Provisioning successful - WiFi credentials applied");
      if (g_statusCallback)
        g_statusCallback(0x02);
      break;

    case ARDUINO_EVENT_PROV_END:
      LOG_INFO(CAT_PROVISIONING, "Provisioning ended");
      if (g_credentialsReceived && !g_credentialsSaved) {
        LOG_WARN(CAT_PROVISIONING,
                 "Provisioning ended without successful WiFi connection");
        g_credentialsReceived = false;
        memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
      }
      if (g_statusCallback)
        g_statusCallback(0x03);
      break;

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
  g_provServer = this;
  g_credentialsReceived = false;
  g_credentialsSaved = false;
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
  g_statusCallback = statusCallback;
  g_credentialsReceived = false;
  g_credentialsSaved = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
  (void)connCallback;
  (void)ipCallback;

  LOG_INFO(CAT_PROVISIONING, "========================================");
  LOG_INFO(CAT_PROVISIONING, "Starting BLE Provisioning via WiFiProv");
  LOG_INFO(CAT_PROVISIONING, "Device name: %s", _deviceName);
  LOG_INFO(CAT_PROVISIONING, "========================================");

  // Регистрируем обработчик событий WiFi
  WiFi.onEvent(sysProvEvent);

  const char* pop = BLE_PROVISIONING_PIN;

  WiFiProv.beginProvision(
      WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BLE,
      WIFI_PROV_SECURITY_1, pop, _deviceName, NULL, (uint8_t*)PROV_UUID, true);

  _active = true;
  LOG_INFO(CAT_PROVISIONING, "BLE provisioning started successfully!");
  LOG_INFO(CAT_PROVISIONING, "Device name: %s", _deviceName);
  LOG_INFO(CAT_PROVISIONING, "PIN: %s", pop);
  LOG_INFO(CAT_PROVISIONING, "Use ESP BLE Provisioning app");

  return true;
}

void BleProvisioningServer::stop() {
  if (!_active)
    return;
  _active = false;
  g_credentialsReceived = false;
  g_credentialsSaved = false;
  memset(&g_receivedConfig, 0, sizeof(g_receivedConfig));
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