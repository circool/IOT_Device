/**
 * @file provisioning.cpp
 * @brief Реализация менеджера провизионинга
 */

#include "provisioning.h"
#include "ble_server.h"
#include "config_manager.h"
#include "logger.h"
#include "web.h"
#include "wifi_manager.h"

// ============================================================================
// ГЛОБАЛЬНЫЙ УКАЗАТЕЛЬ НА BLE-СЕРВЕР (только если BLE включён)
// ============================================================================

#if USE_BLE_PROVISIONING == 1
static BleProvisioningServer* g_bleServer = nullptr;
#endif

// ============================================================================
// КОЛБЭК ДЛЯ BLE-СЕРВЕРА (только если BLE включён)
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
  strncpy(data.wifiSsid, bleConfig->wifiSsid, sizeof(data.wifiSsid) - 1);
  strncpy(data.wifiPassword, bleConfig->wifiPassword,
          sizeof(data.wifiPassword) - 1);

  prov.onDataReceived(data);
}

#endif  // USE_BLE_PROVISIONING == 1

// ============================================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ КЛАССА
// ============================================================================

ProvisioningManager& ProvisioningManager::getInstance() {
  static ProvisioningManager instance;
  return instance;
}

bool ProvisioningManager::begin(ProvisioningCallback callback, void* context) {
  if (_started)
    return false;

  _callback = callback;
  _context = context;
  _started = true;
  _state = ProvisioningState::IDLE;
  _retryCount = 0;
  _completedBy = ProvisioningMethod::NONE;
  _apStarted = false;
  memset(&_data, 0, sizeof(_data));

  if (ConfigManager::getInstance().isValid()) {
    XLOG_INFO(CAT_PROVISIONING, "Config already exists, skipping provisioning");
    _state = ProvisioningState::COMPLETED;
    _completedBy = ProvisioningMethod::NONE;
    if (_callback) {
      _callback(ProvisioningMethod::NONE, _context);
    }
    return true;
  }

  selectProvisioningMethod();
  return true;
}

void ProvisioningManager::update() {
  if (_state == ProvisioningState::COMPLETED ||
      _state == ProvisioningState::FAILED) {
    return;
  }

  if (_apStarted && wifi_is_ap_mode()) {
    web_update();
    if (isApComplete()) {
      _state = ProvisioningState::COMPLETED;
      _completedBy = ProvisioningMethod::AP;
      if (_callback) {
        _callback(ProvisioningMethod::AP, _context);
      }
      return;
    }
  }
}

// @deprecated Не используется
// bool ProvisioningManager::isActive() const {
//   return _started && (_state == ProvisioningState::ACTIVE);
// }

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
  XLOG_DEBUG(CAT_PROVISIONING, "Provisioning data received");

  memcpy(&_data, &data, sizeof(ProvisioningData));

  _state = ProvisioningState::COMPLETED;
  _completedBy = ProvisioningMethod::BLE;

  if (_callback) {
    _callback(ProvisioningMethod::BLE, _context);
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
      XLOG_DEBUG(CAT_PROVISIONING, "Max retries exceeded, provisioning FAILED");
      _state = ProvisioningState::FAILED;
      _completedBy = ProvisioningMethod::FAILED;
      if (_callback) {
        _callback(ProvisioningMethod::FAILED, _context);
      }
    }
  }
#else
  // Если BLE отключён, игнорируем статус
  (void)status;
  XLOG_DEBUG(CAT_PROVISIONING, "BLE status received but BLE is disabled");
#endif
}

// ============================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ============================================================================

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

// Порядок BLE -> AP важен!
#if USE_BLE_PROVISIONING == 1

#ifdef ESP32
  startBleProvisioning();
  ProvisioningState::ACTIVE;
#elif defined(ESP8266)
  XLOG_WARN(CAT_PROVISIONING, "ESP 8266 not supported BLE!");
#endif
#endif  // USE_BLE_PROVISIONING == 1

#if USE_AP_PROVISIONING == 1
  startApProvisioning();
  _state = ProvisioningState::ACTIVE;
#endif
}

void ProvisioningManager::startBleProvisioning() {
#if USE_BLE_PROVISIONING == 1
#if defined(ESP32) && !defined(ESP8266)
  const char* deviceId = ConfigManager::getInstance().getDeviceId();

  g_bleServer = new BleProvisioningServer(deviceId);

  if (g_bleServer->begin(onBleConfigReceived, nullptr)) {
    // XLOG_DEBUG(CAT_PROVISIONING, "BLE provisioning started sussefull.");
  } else {
    XLOG_ERROR(CAT_PROVISIONING, "Failed to start BLE provisioning");
    delete g_bleServer;
    g_bleServer = nullptr;

    if (!_apStarted) {
      XLOG_WARN(CAT_PROVISIONING, "BLE failed, falling back to AP");
      startApProvisioning();
    } else {
      XLOG_ERROR(CAT_PROVISIONING, "BLE start failed, provisioning FAILED");
      _state = ProvisioningState::FAILED;
      _completedBy = ProvisioningMethod::FAILED;
      if (_callback) {
        _callback(ProvisioningMethod::FAILED, _context);
      }
    }
  }
#else
  XLOG_WARN(CAT_PROVISIONING, "BLE not supported on this platform");
#endif
#else
  // BLE отключён — ничего не делаем
  XLOG_DEBUG(CAT_PROVISIONING, "BLE provisioning disabled");
#endif
}

void ProvisioningManager::startApProvisioning() {
  // XLOG_DEBUG(CAT_PROVISIONING, "Starting AP provisioning...");
  // XLOG_DEBUG(CAT_PROVISIONING,
  //            "Find AP '" ANSI_BOLD "%s'" ANSI_RESET
  //            ", connect and visit " ANSI_BOLD "192.168.4.1",
  //            ConfigManager::getInstance().getDeviceId());

  _apStarted = true;

  const char* deviceId = ConfigManager::getInstance().getDeviceId();
  wifi_start_ap(deviceId);
  web_init(true);
}

bool ProvisioningManager::isApComplete() {
  return ConfigManager::getInstance().isValid() &&
         ConfigManager::getInstance().getWifiSsid()[0] != '\0';
}

// ============================================================================
// СТАТИЧЕСКАЯ ФУНКЦИЯ-ОБРАБОТЧИК ДЛЯ MAIN
// ============================================================================

static void onProvisioningComplete(ProvisioningMethod method, void* context) {
  (void)context;

  auto& prov = ProvisioningManager::getInstance();

  if (method == ProvisioningMethod::FAILED) {
    XLOG_ERROR(CAT_PROVISIONING, "Provisioning FAILED permanently!");
    return;
  }

  if (method == ProvisioningMethod::NONE) {
    XLOG_DEBUG(CAT_PROVISIONING, "Provisioning skipped (config exists)");
    return;
  }

  const auto* data = prov.getData();
  if (data && strlen(data->wifiSsid) > 0) {
    XLOG_DEBUG(CAT_PROVISIONING, "Provisioning complete - received SSID='%s'",data->wifiSsid);
  }
}

// ============================================================================
// ПРОСТЫЕ ФУНКЦИИ-ОБЁРТКИ ДЛЯ MAIN
// ============================================================================

void startProvisioning() {
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