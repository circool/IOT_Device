/**
 * @file transport_manager_provisioning.cpp
 * @brief Провизионинг — реализация
 */

#include "transport_manager_provisioning.h"
#include "provisioning_ble_server.h"
#include "transport_wifi_web.h"

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#if defined(USE_AP) || defined(USE_BLE)

// ============================================================================
// КОНСТАНТЫ
// ============================================================================

#ifndef MAX_RETRIES
#define MAX_RETRIES 3
#endif

#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"
#endif

// ============================================================================
// КОНСТРУКТОР
// ============================================================================

ProvisioningManager::ProvisioningManager()
    : _active(false),
      _apRunning(false),
      _bleRunning(false),
      _retryCount(0),
      _currentMethod(0),
      _callback(nullptr),
      _context(nullptr),
      _resultReceived(false),
      _webManager(nullptr) {
  memset(_deviceId, 0, sizeof(_deviceId));
  memset(_lastError, 0, sizeof(_lastError));
}

// ============================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ============================================================================

bool ProvisioningManager::begin(const char* deviceId,
                                TransportEventCallback callback,
                                void* context,
                                WebManager* webManager) {
  if (_active) {
    XLOG_WARN(CAT_PROVISIONING, "Provisioning already active");
    return false;
  }

  if (!deviceId || strlen(deviceId) == 0) {
    XLOG_ERROR(CAT_PROVISIONING, "deviceId is empty");
    strcpy(_lastError, "deviceId is empty");
    return false;
  }

  strncpy(_deviceId, deviceId, sizeof(_deviceId) - 1);
  _deviceId[sizeof(_deviceId) - 1] = '\0';

  _callback = callback;
  _context = context;
  _webManager = webManager;
  _active = true;
  _retryCount = 0;
  _currentMethod = 0;
  _resultReceived = false;

  XLOG_INFO(CAT_PROVISIONING, "Provisioning started (deviceId: %s)", _deviceId);

  bool anyStarted = false;

  // ===== ЗАПУСКАЕМ ВСЕ ДОСТУПНЫЕ МЕТОДЫ =====
#ifdef USE_AP
  if (startAP()) {
    anyStarted = true;
    XLOG_INFO(CAT_PROVISIONING, "AP provisioning started");
  }
#endif

#if defined(USE_BLE) && defined(ESP32)
  if (startBLE()) {
    anyStarted = true;
    XLOG_INFO(CAT_PROVISIONING, "BLE provisioning started");
  }
#endif

  if (!anyStarted) {
    XLOG_ERROR(CAT_PROVISIONING, "No provisioning method available!");
    _active = false;
    strcpy(_lastError, "No provisioning method available");
    return false;
  }

  XLOG_INFO(CAT_PROVISIONING, "Provisioning running (AP: %s, BLE: %s)",
            _apRunning ? "YES" : "NO", _bleRunning ? "YES" : "NO");

  return true;
}

void ProvisioningManager::update() {
  if (!_active)
    return;

  // ===== AP ОБНОВЛЯЕТСЯ ЧЕРЕЗ WebManager::update() =====
  // Все HTTP-запросы обрабатываются в WebManager::handleClient()
  // Данные от AP приходят через POST /savewifi в WebManager::handleSaveWifi()
  // Там устанавливается флаг configPending, который обрабатывается в
  // оркестраторе

  // ===== BLE ОБНОВЛЯЕТСЯ ЧЕРЕЗ СОБЫТИЯ =====
  // BLE использует события (SysProvEvent), поэтому в loop ничего не делаем
}

void ProvisioningManager::stop() {
  if (!_active)
    return;

  XLOG_INFO(CAT_PROVISIONING, "Stopping provisioning...");

  stopAP();
  stopBLE();

  _active = false;
  _apRunning = false;
  _bleRunning = false;
  _callback = nullptr;
  _context = nullptr;
}

bool ProvisioningManager::isActive() const {
  return _active;
}

const char* ProvisioningManager::getLastError() const {
  return _lastError;
}

// ============================================================================
// ОБРАБОТЧИКИ СОБЫТИЙ (вызываются из BLE/AP серверов)
// ============================================================================

void ProvisioningManager::onDataReceived(const char* ssid,
                                         const char* password) {
  if (!ssid || strlen(ssid) == 0) {
    XLOG_WARN(CAT_PROVISIONING, "Received empty SSID");
    onError();
    return;
  }

  XLOG_INFO(CAT_PROVISIONING, "WiFi credentials received: SSID=%s", ssid);

  if (!_callback) {
    XLOG_WARN(CAT_PROVISIONING, "No callback registered, ignoring data");
    return;
  }

  // ===== ФОРМИРУЕМ СОБЫТИЕ TRANSPORT_CONFIG =====
  TransportConfig config;
  memset(&config, 0, sizeof(config));

  strncpy(config.deviceId, _deviceId, sizeof(config.deviceId) - 1);
  config.deviceId[sizeof(config.deviceId) - 1] = '\0';

  strncpy(config.wifiSsid, ssid, sizeof(config.wifiSsid) - 1);
  config.wifiSsid[sizeof(config.wifiSsid) - 1] = '\0';

  if (password && strlen(password) > 0) {
    strncpy(config.wifiPassword, password, sizeof(config.wifiPassword) - 1);
    config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
  } else {
    config.wifiPassword[0] = '\0';
  }

#if FEATURE_MQTT_ENABLED == 1
  // MQTT поля пока не заполняются из BLE (там нет MQTT)
  // Они заполняются из WebManager::handleSaveWifi() через pendingConfig
  config.mqttBroker[0] = '\0';
  config.mqttPort = 0;
  config.mqttUser[0] = '\0';
  config.mqttPassword[0] = '\0';
#endif

  _resultReceived = true;

  stopAP();
  stopBLE();

  TransportEventData event;
  event.event = TRANSPORT_CONFIG;
  event.state = nullptr;
  event.transport = &config;
  event.deviceState = nullptr;
  event.device = nullptr;

  _callback(&event, _context);

  _active = false;
  XLOG_INFO(CAT_PROVISIONING, "Provisioning completed successfully");
}

void ProvisioningManager::onBleError() {
  XLOG_WARN(CAT_PROVISIONING, "BLE error occurred");
  onError();
}

// ============================================================================
// ВНУТРЕННИЕ МЕТОДЫ
// ============================================================================

bool ProvisioningManager::startAP() {
  if (_apRunning)
    return true;

#ifdef USE_AP
  XLOG_INFO(CAT_PROVISIONING, "Starting AP provisioning (attempt %d)",
            _retryCount + 1);

  // ===== ПРОВЕРЯЕМ, НЕ ЗАПУЩЕН ЛИ УЖЕ AP =====
  if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
    XLOG_DEBUG(CAT_PROVISIONING, "AP already running, reusing existing AP");
    _apRunning = true;

    // ===== УСТАНАВЛИВАЕМ ФЛАГ ГОТОВНОСТИ ДЛЯ WEB =====
    if (_webManager) {
      _webManager->setWifiReady(true);
      XLOG_DEBUG(CAT_PROVISIONING,
                 "WebManager: wifiReady = true (reusing existing AP)");
    }
    return true;
  }

  // ===== ЗАПУСКАЕМ AP =====
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_AP);
  delay(200);

  IPAddress apIP;
  apIP.fromString(AP_IP_ADDRESS);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(100);
  WiFi.softAP(_deviceId);
  delay(100);

  XLOG_INFO(CAT_PROVISIONING, "AP started. SSID: %s, IP: %s", _deviceId,
            AP_IP_ADDRESS);

  // ===== ПЕРЕКЛЮЧАЕМ WebManager В РЕЖИМ ПРОВИЗИОНИНГА =====
  if (_webManager) {
    _webManager->enableProvisioningMode();
    _webManager->setWifiReady(true);
    XLOG_DEBUG(CAT_PROVISIONING,
               "WebManager switched to provisioning mode, wifiReady = true");
  }

  _apRunning = true;
  return true;
#else
  XLOG_WARN(CAT_PROVISIONING, "AP provisioning is disabled");
  return false;
#endif
}

bool ProvisioningManager::startBLE() {
  if (_bleRunning)
    return true;

#if defined(USE_BLE) && defined(ESP32)
  XLOG_INFO(CAT_PROVISIONING, "Starting BLE provisioning (attempt %d)",
            _retryCount + 1);

  if (g_bleServer) {
    delete g_bleServer;
    g_bleServer = nullptr;
  }

  g_bleServer = new BleProvisioningServer(_deviceId);
  if (g_bleServer->begin(this)) {
    _bleRunning = true;
    XLOG_INFO(CAT_PROVISIONING, "BLE provisioning started. Device: %s",
              _deviceId);
    return true;
  } else {
    XLOG_WARN(CAT_PROVISIONING, "BLE server failed to start");
    strcpy(_lastError, "BLE server failed");
    return false;
  }
#else
  XLOG_WARN(CAT_PROVISIONING, "BLE provisioning is disabled or not supported");
  return false;
#endif
}

void ProvisioningManager::stopAP() {
  if (!_apRunning)
    return;

  // ===== ВЫКЛЮЧАЕМ ФЛАГ ГОТОВНОСТИ ДЛЯ WEB =====
  if (_webManager) {
    _webManager->setWifiReady(false);
    XLOG_DEBUG(CAT_PROVISIONING, "WebManager: wifiReady = false");
  }

  WiFi.softAPdisconnect(true);
  delay(100);

  if (_webManager) {
    _webManager->disableProvisioningMode();
    XLOG_DEBUG(CAT_PROVISIONING, "WebManager provisioning mode disabled");
  }

  _apRunning = false;
  XLOG_DEBUG(CAT_PROVISIONING, "AP stopped");
}

void ProvisioningManager::stopBLE() {
  if (!_bleRunning)
    return;
#if defined(USE_BLE) && defined(ESP32)
  if (g_bleServer) {
    g_bleServer->stop();
    delete g_bleServer;
    g_bleServer = nullptr;
  }
#endif
  _bleRunning = false;
  XLOG_DEBUG(CAT_PROVISIONING, "BLE server stopped");
}

void ProvisioningManager::switchToNextMethod() {
  _retryCount++;

  if (_retryCount >= MAX_RETRIES) {
    XLOG_ERROR(CAT_PROVISIONING, "Max retries (%d) exceeded", MAX_RETRIES);
    strcpy(_lastError, "Max retries exceeded");
    // Сообщаем об ошибке через колбэк с TRANSPORT_CONFIG = NULL
    if (_callback) {
      TransportEventData event;
      event.event = TRANSPORT_CONFIG;
      event.state = nullptr;
      event.transport = nullptr;
      event.deviceState = nullptr;
      event.device = nullptr;
      _callback(&event, _context);
    }
    stop();
    return;
  }

  XLOG_INFO(CAT_PROVISIONING, "Switching provisioning method (attempt %d/%d)",
            _retryCount + 1, MAX_RETRIES);

  stopAP();
  stopBLE();

  if (_currentMethod == 0) {
    _currentMethod = 1;
    if (!startBLE()) {
      switchToNextMethod();
    }
  } else {
    _currentMethod = 0;
    if (!startAP()) {
      switchToNextMethod();
    }
  }
}

void ProvisioningManager::onError() {
  XLOG_WARN(CAT_PROVISIONING, "Provisioning error occurred");
  switchToNextMethod();
}

#endif  // PROVISIONING_METHOD != 0