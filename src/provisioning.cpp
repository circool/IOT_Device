/**
 * @file provisioning_manager.cpp
 * @brief Реализация менеджера провизионинга
 */

#include "config_manager.h"
#include "logger.h"
#include "provisioning_manager.h"
#include "system_state.h"
#include "wifi_manager.h"  //@deprecated - вынести wifi в wifi_manager

#if FEATURE_PROVISIONING_ENABLED == 1

#if USE_BLE_PROVISIONING == 1
#include "ble_server.h"
#endif

#if USE_AP_PROVISIONING == 1
#include "web_common.h"

#if defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif

static WebServerClass* _apServer = nullptr;
static bool _apServerStarted = false;

static void startApServer() {
  if (_apServerStarted)
    XLOG_WARN(CAT_PROVISIONING, "AP server already running");
    return;

  XLOG_DEBUG(CAT_PROVISIONING, "Creating AP HTTP server on port 80");

  _apServer = new WebServerClass(80);
  if (!_apServer) {
    XLOG_ERROR(CAT_PROVISIONING, "Failed to create AP server");
    return;
  }

  _apServer->on("/", []() {
    XLOG_DEBUG(CAT_PROVISIONING, "GET / - serving AP provisioning page");
    web_sendApProvisioningPage(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk)
            srv->sendContent(chunk);
        },
        _apServer);
  });

  _apServer->on("/savewifi", HTTP_POST, []() {
    XLOG_DEBUG(CAT_PROVISIONING, "POST /savewifi - handling AP save");
    provisioning_handle_ap_save(_apServer);
  });

  _apServer->on("/config", []() {
    XLOG_DEBUG(CAT_PROVISIONING, "GET /config - redirect to /");
    _apServer->sendHeader("Location", "/");
    _apServer->send(302, "text/plain", "Redirecting...");
  });

  _apServer->on("/favicon.ico", []() { _apServer->send(404); });

  _apServer->begin();
  _apServerStarted = true;

  XLOG_INFO(CAT_PROVISIONING, "AP HTTP server started on port 80");
}

static void updateApServer() {
  if (_apServerStarted && _apServer) {
    _apServer->handleClient();
  }
}

#endif  // USE_AP_PROVISIONING

#if USE_BLE_PROVISIONING == 1
static BleProvisioningServer* g_bleServer = nullptr;
#endif

// ============================================================================
// BLE КОЛБЭКИ
// ============================================================================

#if USE_BLE_PROVISIONING == 1
static void onBleConfigReceived(const BleWifiConfig* bleConfig, void* context) {
  (void)context;
  auto& prov = ProvisioningManager::getInstance();

  if (!bleConfig) {
    XLOG_ERROR(CAT_PROVISIONING, "BLE config is null!");
    prov.onBleStatus(ARDUINO_EVENT_PROV_CRED_FAIL);
    return;
  }

  XLOG_INFO(CAT_PROVISIONING, "WiFi config received via BLE: %s",
            bleConfig->wifiSsid);

  ProvisioningData data;
  memset(&data, 0, sizeof(data));
  data.type = 0;

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  strncpy(data.wifiSsid, bleConfig->wifiSsid, sizeof(data.wifiSsid) - 1);
  strncpy(data.wifiPassword, bleConfig->wifiPassword,
          sizeof(data.wifiPassword) - 1);
#endif

  prov.onDataReceived(data);
}
#endif

// ============================================================================
// PROVISIONING MANAGER — РЕАЛИЗАЦИЯ
// ============================================================================

ProvisioningManager& ProvisioningManager::getInstance() {
  static ProvisioningManager instance;
  return instance;
}

bool ProvisioningManager::begin(ProvisioningCallback callback, void* context) {
  if (_started) {
    XLOG_WARN(CAT_PROVISIONING, "Provisioning already started");
    return false;
  }

  XLOG_DEBUG(CAT_PROVISIONING, "Provisioning begin() called");

  _callback = callback;
  _context = context;
  _started = true;
  _retryCount = 0;
  memset(&_data, 0, sizeof(_data));

#if USE_AP_PROVISIONING == 1
  _apStarted = false;
#endif

  system_state_set_bit(STATE_PROVISIONING);
  selectProvisioningMethod();
  return true;
}

void ProvisioningManager::update() {
  
#if USE_AP_PROVISIONING == 1
  updateApServer();
#endif
  // BLE: НЕ ТРЕБУЕТ update() — работает через события
}

const ProvisioningData* ProvisioningManager::getProvisioningData() {
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  if (strlen(_data.wifiSsid) > 0) {
    XLOG_DEBUG(CAT_PROVISIONING, "Provisioning data retrieved");
    _started = false;
    return &_data;
  }
#endif
  return nullptr;
}

void ProvisioningManager::onDataReceived(const ProvisioningData& data) {
  XLOG_INFO(CAT_PROVISIONING, "Provisioning data received");

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  XLOG_DEBUG(CAT_PROVISIONING, "SSID: %s, Password: %s", data.wifiSsid,
             data.wifiPassword[0] ? "***" : "(empty)");
#endif

  memcpy(&_data, &data, sizeof(ProvisioningData));

#if USE_BLE_PROVISIONING == 1
  if (g_bleServer) {
    g_bleServer->stop();
    delete g_bleServer;
    g_bleServer = nullptr;
    XLOG_DEBUG(CAT_BLE, "BLE stopped");
  }
#endif

// TODO: AP тоже останавливаем

  if (_callback) {
    _callback(true, _context);
  }
}

void ProvisioningManager::onBleStatus(uint8_t status) {
#if USE_BLE_PROVISIONING == 1
  if (status == ARDUINO_EVENT_PROV_CRED_FAIL) {
    XLOG_WARN(CAT_PROVISIONING, "BLE credentials failed (attempt %d/%d)",
              _retryCount + 1, MAX_RETRIES);
    _retryCount++;
    if (_retryCount >= MAX_RETRIES) {
      XLOG_ERROR(CAT_PROVISIONING, "BLE max retries exceeded");
      if (_callback) {
        _callback(false, _context);
      }
    }
  }
#else
  (void)status;
#endif
}

void ProvisioningManager::selectProvisioningMethod() {
#if USE_BLE_PROVISIONING == 0 && USE_AP_PROVISIONING == 0
  XLOG_WARN(CAT_PROVISIONING, "Provisioning disabled.");
  return;
#endif

#if USE_BLE_PROVISIONING == 1 && USE_AP_PROVISIONING == 1
  XLOG_INFO(CAT_PROVISIONING, "Starting AP + BLE provisioning");
#elif USE_BLE_PROVISIONING == 1
  XLOG_INFO(CAT_PROVISIONING, "Starting BLE provisioning");
#elif USE_AP_PROVISIONING == 1
  XLOG_INFO(CAT_PROVISIONING, "Starting AP provisioning");
#endif

#if USE_BLE_PROVISIONING == 1
#ifdef ESP32
  startBleProvisioning();
#endif
#endif

#if USE_AP_PROVISIONING == 1
  startApProvisioning();
#endif
}

// ============================================================================
// BLE-ПРОВИЗИОНИНГ
// ============================================================================

#if USE_BLE_PROVISIONING == 1
void ProvisioningManager::startBleProvisioning() {
#if defined(ESP32) && !defined(ESP8266)
  const char* deviceId = ConfigManager::getInstance().getDeviceId();
  g_bleServer = new BleProvisioningServer(deviceId);
  if (g_bleServer->begin()) {
    XLOG_INFO(CAT_PROVISIONING, "BLE provisioning started");
  } else {
    XLOG_ERROR(CAT_PROVISIONING, "Failed to start BLE provisioning");
    delete g_bleServer;
    g_bleServer = nullptr;
#if USE_AP_PROVISIONING == 1
    if (!_apStarted) {
      XLOG_WARN(CAT_PROVISIONING, "BLE failed, falling back to AP");
      startApProvisioning();
    }
#endif
  }
#endif
}
#endif

// ============================================================================
// AP-ПРОВИЗИОНИНГ
// ============================================================================

#if USE_AP_PROVISIONING == 1

void ProvisioningManager::startApProvisioning() {
  _apStarted = true;
  const char* deviceId = ConfigManager::getInstance().getDeviceId();
  wifi_start_ap(deviceId);
  startApServer();

  XLOG_INFO(CAT_PROVISIONING,
            "AP provisioning started, connect to SSID: " ANSI_BOLD
            "%s" ANSI_BOLD_RESET " and visit " ANSI_BOLD "%s" ANSI_BOLD_RESET
            " to set wifi settings.",
            deviceId, AP_IP_ADDRESS);
}

bool ProvisioningManager::isApComplete() const {
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  return strlen(_data.wifiSsid) > 0;
#else
  return false;
#endif
}

#endif

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ (обёртки для оркестратора)
// ============================================================================

static void onProvisioningComplete(bool success, void* context) {
  (void)context;
  if (!success) {
    XLOG_ERROR(CAT_PROVISIONING, "Provisioning FAILED!");
  }
}

void startProvisioning() {
  ProvisioningManager::getInstance().begin(onProvisioningComplete, nullptr);
}

void provisioning_update() {
  ProvisioningManager::getInstance().update();
}

const ProvisioningData* getProvisioningData() {
  return ProvisioningManager::getInstance().getProvisioningData();
}

// ============================================================================
// AP-ФУНКЦИИ (HTTP-обработчики)
// ============================================================================

#if USE_AP_PROVISIONING == 1

void provisioning_send_ap_page(WebServerClass* server) {
  if (!server)
    return;

  XLOG_DEBUG(CAT_PROVISIONING, "Sending AP provisioning page");

  web_sendApProvisioningPage(
      [](const char* chunk, void* context) {
        WebServerClass* srv = static_cast<WebServerClass*>(context);
        if (srv && chunk)
          srv->sendContent(chunk);
      },
      server);
}

void provisioning_handle_ap_save(WebServerClass* server) {
  if (!server)
    return;

  XLOG_INFO(CAT_PROVISIONING, "Handling AP save request");

  if (server->method() != HTTP_POST) {
    server->send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String ssid = server->arg("wifiSsid");
  String password = server->arg("wifiPassword");

  XLOG_DEBUG(CAT_PROVISIONING, "AP save: SSID='%s', Password=%s", ssid.c_str(),
             password.length() > 0 ? "***" : "(empty)");

  if (ssid.length() >= 32) {
    XLOG_WARN(CAT_PROVISIONING, "AP provisioning: SSID too long (%d chars)",
              ssid.length());
    web_sendResultPage(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk)
            srv->sendContent(chunk);
        },
        server, "SSID too long (max 31 chars)", false);
    return;
  }

  if (password.length() >= 64) {
    XLOG_WARN(CAT_PROVISIONING, "AP provisioning: password too long (%d chars)",
              password.length());
    web_sendResultPage(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk)
            srv->sendContent(chunk);
        },
        server, "Password too long (max 63 chars)", false);
    return;
  }

  ProvisioningData data;
  memset(&data, 0, sizeof(data));
  data.type = 0;

  strncpy(data.wifiSsid, ssid.c_str(), sizeof(data.wifiSsid) - 1);
  data.wifiSsid[sizeof(data.wifiSsid) - 1] = '\0';

  if (password.length() > 0) {
    strncpy(data.wifiPassword, password.c_str(), sizeof(data.wifiPassword) - 1);
    data.wifiPassword[sizeof(data.wifiPassword) - 1] = '\0';
  }

  ProvisioningManager::getInstance().onDataReceived(data);

  XLOG_INFO(CAT_PROVISIONING, "AP provisioning: WiFi credentials saved");

  web_sendResultPage(
      [](const char* chunk, void* context) {
        WebServerClass* srv = static_cast<WebServerClass*>(context);
        if (srv && chunk)
          srv->sendContent(chunk);
      },
      server, "WiFi credentials saved. Device will reboot and try to connect.",
      true);
}

#endif  // USE_AP_PROVISIONING

#endif  // FEATURE_PROVISIONING_ENABLED == 1