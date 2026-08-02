/**
 * @file provisioning_manager.cpp
 * @brief Реализация менеджера провизионинга
 */

#include "provisioning_manager.h"
#include "config_manager.h"
#include "logger.h"
#include "provisioning_ap_server.h"
#include "system_state.h"

#if PROVISIONING_METHOD != 0

#if USE_BLE_PROVISIONING == 1
#include "ble_server.h"
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

  system_state_set_bit(STATE_PROVISIONING);
  selectProvisioningMethod();
  return true;
}

void ProvisioningManager::update() {
  // AP: периодическая обработка HTTP-запросов
#if USE_AP_PROVISIONING == 1
  ap_server_update();
#endif

  // BLE: НЕ ТРЕБУЕТ update() — работает через события
}

const ProvisioningData* ProvisioningManager::getProvisioningData() {
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  // Проверяем данные от AP
  if (ap_server_has_data()) {
    strncpy(_data.wifiSsid, ap_server_get_ssid(), sizeof(_data.wifiSsid) - 1);
    _data.wifiSsid[sizeof(_data.wifiSsid) - 1] = '\0';
    strncpy(_data.wifiPassword, ap_server_get_password(),
            sizeof(_data.wifiPassword) - 1);
    _data.wifiPassword[sizeof(_data.wifiPassword) - 1] = '\0';
    ap_server_clear_data();
    _started = false;
    return &_data;
  }

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

  // Останавливаем AP-сервер
#if USE_AP_PROVISIONING == 1
  if (ap_server_is_running()) {
    ap_server_stop();
  }
#endif

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
    XLOG_WARN(CAT_PROVISIONING, "BLE failed, AP already running");
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
  const char* deviceId = ConfigManager::getInstance().getDeviceId();
  ap_server_start(deviceId);
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

#endif  // PROVISIONING_METHOD != 0