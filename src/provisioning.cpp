/**
 * @file provisioning.cpp
 * @brief Реализация менеджера провизионинга
 */

#include "provisioning.h"
#include "config_manager.h"
#include "logger.h"
#include "system_state.h"
#include "wifi_manager.h"

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

// ============================================================================
// СТАТИЧЕСКИЕ ДАННЫЕ ДЛЯ HTTP-СЕРВЕРА AP-ПРОВИЗИОНИНГА
// ============================================================================

static WebServerClass* _apServer = nullptr;
static bool _apServerStarted = false;

// ============================================================================
// СТАТИЧЕСКИЕ ФУНКЦИИ ДЛЯ HTTP-СЕРВЕРА (НЕ МЕТОДЫ КЛАССА!)
// ============================================================================

static void startApServer() {
  if (_apServerStarted) {
    XLOG_DEBUG(CAT_PROVISIONING, "AP server already running");
    return;
  }

  XLOG_DEBUG(CAT_PROVISIONING, "Creating AP HTTP server on port 80");

  _apServer = new WebServerClass(80);

  if (!_apServer) {
    XLOG_ERROR(CAT_PROVISIONING, "Failed to create AP server");
    return;
  }

  // ===== МАРШРУТЫ =====
  _apServer->on("/", []() {
    XLOG_DEBUG(CAT_PROVISIONING, "GET / - serving AP provisioning page");
    web_sendApProvisioningPage(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk) {
            srv->sendContent(chunk);
          }
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

static bool isApServerRunning() {
  return _apServerStarted;
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

  if (prov.isCompleted()) {
    XLOG_DEBUG(CAT_PROVISIONING, "Already completed, ignoring BLE");
    return;
  }

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
  data.wifiSsid[sizeof(data.wifiSsid) - 1] = '\0';
  strncpy(data.wifiPassword, bleConfig->wifiPassword,
          sizeof(data.wifiPassword) - 1);
  data.wifiPassword[sizeof(data.wifiPassword) - 1] = '\0';
#endif

  prov.onDataReceived(data);
}
#endif  // USE_BLE_PROVISIONING

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
  _state = ProvisioningState::IDLE;
  _retryCount = 0;
  _completedBy = ProvisioningMethod::NONE;
  memset(&_data, 0, sizeof(_data));

#if USE_AP_PROVISIONING == 1
  _apStarted = false;
#endif

  selectProvisioningMethod();
  return true;
}

void ProvisioningManager::update() {
  if (_state == ProvisioningState::COMPLETED ||
      _state == ProvisioningState::FAILED) {
    return;
  }

  // ================================================================
  // ОБНОВЛЯЕМ AP-СЕРВЕР (ЕСЛИ ЗАПУЩЕН)
  // ================================================================
#if USE_AP_PROVISIONING == 1
  updateApServer();
#endif

  // ================================================================
  // ПРОВЕРЯЕМ ЗАВЕРШЕНИЕ AP-ПРОВИЗИОНИНГА
  // ================================================================
#if USE_AP_PROVISIONING == 1
  if (_apStarted && system_state_has_bit(STATE_PROVISIONING)) {
    if (isApComplete()) {
      XLOG_DEBUG(CAT_PROVISIONING, "AP provisioning complete detected");
      _state = ProvisioningState::COMPLETED;
      _completedBy = ProvisioningMethod::WIFI;
      if (_callback) {
        _callback(ProvisioningMethod::WIFI, _context);
      }
    }
  }
#endif
}

bool ProvisioningManager::isCompleted() const {
  return _state == ProvisioningState::COMPLETED ||
         _state == ProvisioningState::FAILED;
}

ProvisioningMethod ProvisioningManager::getCompletedBy() const {
  return _completedBy;
}

ProvisioningState ProvisioningManager::getState() const {
  return _state;
}

int ProvisioningManager::getRetryCount() const {
  return _retryCount;
}

const ProvisioningData* ProvisioningManager::getData() const {
  return &_data;
}

void ProvisioningManager::onDataReceived(const ProvisioningData& data) {
  if (_state == ProvisioningState::COMPLETED) {
    XLOG_DEBUG(CAT_PROVISIONING, "Already completed, ignoring new data");
    return;
  }

  XLOG_INFO(CAT_PROVISIONING, "Provisioning data received");

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  XLOG_DEBUG(CAT_PROVISIONING, "SSID: %s, Password: %s", data.wifiSsid,
             data.wifiPassword[0] ? "***" : "(empty)");
#endif

  memcpy(&_data, &data, sizeof(ProvisioningData));
  _state = ProvisioningState::COMPLETED;

#if USE_BLE_PROVISIONING == 1
  if (g_bleServer) {
    g_bleServer->stop();
    delete g_bleServer;
    g_bleServer = nullptr;
    XLOG_DEBUG(CAT_BLE, "BLE stopped");
  }
#endif

  if (_callback) {
    _callback(_completedBy, _context);
  }
}

void ProvisioningManager::onBleStatus(uint8_t status) {
  if (_state == ProvisioningState::COMPLETED ||
      _state == ProvisioningState::FAILED) {
    return;
  }

#if USE_BLE_PROVISIONING == 1
  if (status == ARDUINO_EVENT_PROV_CRED_FAIL) {
    XLOG_WARN(CAT_PROVISIONING, "BLE credentials failed (attempt %d/%d)",
              _retryCount + 1, MAX_RETRIES);
    _retryCount++;

    if (_retryCount < MAX_RETRIES) {
      XLOG_INFO(CAT_PROVISIONING, "Waiting for new BLE connection attempt");
      _state = ProvisioningState::ACTIVE;
    } else {
      XLOG_ERROR(CAT_PROVISIONING, "Max retries exceeded, provisioning FAILED");
      _state = ProvisioningState::FAILED;
      _completedBy = ProvisioningMethod::FAILED;
      if (_callback) {
        _callback(ProvisioningMethod::FAILED, _context);
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
#elif defined(ESP8266)
  XLOG_WARN(CAT_PROVISIONING, "ESP8266 does not support BLE");
#endif
#endif

#if USE_AP_PROVISIONING == 1
  startApProvisioning();
  _state = ProvisioningState::ACTIVE;
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
    _state = ProvisioningState::ACTIVE;
    XLOG_INFO(CAT_PROVISIONING, "BLE provisioning started");
  } else {
    XLOG_ERROR(CAT_PROVISIONING, "Failed to start BLE provisioning");
    delete g_bleServer;
    g_bleServer = nullptr;

#if USE_AP_PROVISIONING == 1
    if (!_apStarted) {
      XLOG_WARN(CAT_PROVISIONING, "BLE failed, falling back to AP");
      startApProvisioning();
    } else {
      _state = ProvisioningState::FAILED;
      _completedBy = ProvisioningMethod::FAILED;
      if (_callback) {
        _callback(ProvisioningMethod::FAILED, _context);
      }
    }
#else
    _state = ProvisioningState::FAILED;
    _completedBy = ProvisioningMethod::FAILED;
    if (_callback) {
      _callback(ProvisioningMethod::FAILED, _context);
    }
#endif
  }
#endif
}
#endif  // USE_BLE_PROVISIONING

// ============================================================================
// AP-ПРОВИЗИОНИНГ
// ============================================================================

#if USE_AP_PROVISIONING == 1

void ProvisioningManager::startApProvisioning() {
  _apStarted = true;
  const char* deviceId = ConfigManager::getInstance().getDeviceId();
  wifi_start_ap(deviceId);

  XLOG_DEBUG(CAT_PROVISIONING, "AP started, starting HTTP server");

  // ================================================================
  // ЗАПУСКАЕМ ЛЕГКОВЕСНЫЙ HTTP-СЕРВЕР ДЛЯ AP-ПРОВИЗИОНИНГА
  // ================================================================
  startApServer();

  _state = ProvisioningState::ACTIVE;
  XLOG_INFO(CAT_PROVISIONING,
            "AP provisioning started, connect to SSID: " ANSI_BOLD
            "%s" ANSI_BOLD_RESET " and visit " ANSI_BOLD "%s" ANSI_BOLD_RESET
            " to set wifi settings.",
            deviceId, AP_IP_ADDRESS);
}

bool ProvisioningManager::isApComplete() const {
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  bool complete = strlen(_data.wifiSsid) > 0;
  if (complete) {
    XLOG_DEBUG(CAT_PROVISIONING, "AP complete: SSID present");
  }
  return complete;
#else
  return false;
#endif
}

#endif  // USE_AP_PROVISIONING

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ
// ============================================================================

static void onProvisioningComplete(ProvisioningMethod method, void* context) {
  (void)context;
  auto& prov = ProvisioningManager::getInstance();

  if (method == ProvisioningMethod::FAILED) {
    XLOG_ERROR(CAT_PROVISIONING, "Provisioning FAILED!");
    return;
  }

  const auto* data = prov.getData();
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  if (data && data->type == 0 && strlen(data->wifiSsid) > 0) {
    XLOG_INFO(CAT_PROVISIONING, "Provisioning complete - SSID: %s",
              data->wifiSsid);
  }
#endif
}

void startProvisioning() {
  XLOG_DEBUG(CAT_PROVISIONING, "startProvisioning() called");
  ProvisioningManager::getInstance().begin(onProvisioningComplete, nullptr);
}

bool isProvisioningComplete() {
  return ProvisioningManager::getInstance().isCompleted();
}

ProvisioningMethod getProvisioningMethod() {
  return ProvisioningManager::getInstance().getCompletedBy();
}

ProvisioningState getProvisioningState() {
  return ProvisioningManager::getInstance().getState();
}

int getProvisioningRetryCount() {
  return ProvisioningManager::getInstance().getRetryCount();
}

const ProvisioningData* getProvisioningData() {
  return ProvisioningManager::getInstance().getData();
}

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ ДЛЯ AP-СЕРВЕРА (вызываются из оркестратора)
// ============================================================================

#if USE_AP_PROVISIONING == 1

void provisioning_ap_server_update() {
  updateApServer();
}

#endif  // USE_AP_PROVISIONING

// ============================================================================
// ФУНКЦИИ AP-ПРОВИЗИОНИНГА (вызываются из HTTP-сервера)
// ============================================================================

#if USE_AP_PROVISIONING == 1

void provisioning_send_ap_page(WebServerClass* server) {
  if (!server) {
    XLOG_ERROR(CAT_PROVISIONING, "provisioning_send_ap_page: server is null");
    return;
  }

  XLOG_DEBUG(CAT_PROVISIONING, "Sending AP provisioning page");

  web_sendApProvisioningPage(
      [](const char* chunk, void* context) {
        WebServerClass* srv = static_cast<WebServerClass*>(context);
        if (srv && chunk) {
          srv->sendContent(chunk);
        }
      },
      server);

  XLOG_DEBUG(CAT_PROVISIONING, "AP provisioning page sent");
}

void provisioning_handle_ap_save(WebServerClass* server) {
  if (!server) {
    XLOG_ERROR(CAT_PROVISIONING, "provisioning_handle_ap_save: server is null");
    return;
  }

  XLOG_INFO(CAT_PROVISIONING, "Handling AP save request");

  // Проверяем, что это POST-запрос
  if (server->method() != HTTP_POST) {
    XLOG_WARN(CAT_PROVISIONING, "AP save: method not POST (%d)",
              server->method());
    server->send(405, "text/plain", "Method Not Allowed");
    return;
  }

  // Получаем параметры
  String ssid = server->arg("wifiSsid");
  String password = server->arg("wifiPassword");

  XLOG_DEBUG(CAT_PROVISIONING, "AP save: SSID='%s', Password=%s", ssid.c_str(),
             password.length() > 0 ? "***" : "(empty)");

  // Валидация SSID
  if (ssid.length() >= 32) {
    XLOG_WARN(CAT_PROVISIONING, "AP provisioning: SSID too long (%d chars)",
              ssid.length());
    web_sendResultPage(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk) {
            srv->sendContent(chunk);
          }
        },
        server, "SSID too long (max 31 chars)", false);
    return;
  }

  // Валидация пароля
  if (password.length() >= 64) {
    XLOG_WARN(CAT_PROVISIONING, "AP provisioning: password too long (%d chars)",
              password.length());
    web_sendResultPage(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk) {
            srv->sendContent(chunk);
          }
        },
        server, "Password too long (max 63 chars)", false);
    return;
  }

  // Сохраняем данные через менеджер провизионинга
  ProvisioningData data;
  memset(&data, 0, sizeof(data));
  data.type = 0;  // WiFi

  strncpy(data.wifiSsid, ssid.c_str(), sizeof(data.wifiSsid) - 1);
  data.wifiSsid[sizeof(data.wifiSsid) - 1] = '\0';

  if (password.length() > 0) {
    strncpy(data.wifiPassword, password.c_str(), sizeof(data.wifiPassword) - 1);
    data.wifiPassword[sizeof(data.wifiPassword) - 1] = '\0';
  }

  auto& prov = ProvisioningManager::getInstance();
  prov.onDataReceived(data);

  XLOG_INFO(CAT_PROVISIONING, "AP provisioning: WiFi credentials saved successfully");

  // Показываем страницу успеха
  web_sendResultPage(
      [](const char* chunk, void* context) {
        WebServerClass* srv = static_cast<WebServerClass*>(context);
        if (srv && chunk) {
          srv->sendContent(chunk);
        }
      },
      server, "WiFi credentials saved ", true);

  XLOG_DEBUG(CAT_PROVISIONING, "AP saved: positive result page sent");

}
#endif  // USE_AP_PROVISIONING == 1

#endif  // FEATURE_PROVISIONING_ENABLED == 1