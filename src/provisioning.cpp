#include "provisioning.h"
#include "config_manager.h"
#include "logger.h"
#include "wifi_manager.h"

#if USE_BLE_PROVISIONING == 1
#include "ble_server.h"
#endif

#if USE_AP_PROVISIONING == 1
#include "web.h"
#endif

#if USE_BLE_PROVISIONING == 1
static BleProvisioningServer* g_bleServer = nullptr;
#endif

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

#if FEATURE_MQTT_ENABLED == 1
  strncpy(data.wifiSsid, bleConfig->wifiSsid, sizeof(data.wifiSsid) - 1);
  strncpy(data.wifiPassword, bleConfig->wifiPassword,
          sizeof(data.wifiPassword) - 1);
#endif

  prov.onDataReceived(data);
}
#endif

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

#if USE_AP_PROVISIONING == 1
  if (_apStarted && wifi_is_ap_mode()) {
    if (isApComplete()) {
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

  XLOG_DEBUG(CAT_PROVISIONING, "Provisioning data received");
  memcpy(&_data, &data, sizeof(ProvisioningData));
  _state = ProvisioningState::COMPLETED;

#if USE_BLE_PROVISIONING == 1
  if (g_bleServer) {
    g_bleServer->stop();
    delete g_bleServer;
    g_bleServer = nullptr;
    XLOG_INFO(CAT_BLE, "BLE stopped");
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
      XLOG_DEBUG(CAT_PROVISIONING, "Max retries exceeded, provisioning FAILED");
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

#if USE_BLE_PROVISIONING == 1
void ProvisioningManager::startBleProvisioning() {
#if defined(ESP32) && !defined(ESP8266)
  const char* deviceId = ConfigManager::getInstance().getDeviceId();
  g_bleServer = new BleProvisioningServer(deviceId);

  if (g_bleServer->begin()) {
    _state = ProvisioningState::ACTIVE;
    XLOG_DEBUG(CAT_PROVISIONING, "BLE provisioning started");
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
#endif

#if USE_AP_PROVISIONING == 1
void ProvisioningManager::startApProvisioning() {
  _apStarted = true;
  const char* deviceId = ConfigManager::getInstance().getDeviceId();
  wifi_start_ap(deviceId);
  web_init(true);
  _state = ProvisioningState::ACTIVE;
  XLOG_INFO(CAT_PROVISIONING, "AP provisioning started: %s", deviceId);
}

bool ProvisioningManager::isApComplete() {
#if FEATURE_MQTT_ENABLED == 1
  return strlen(_data.wifiSsid) > 0;
#else
  return false;
#endif
}
#endif

static void onProvisioningComplete(ProvisioningMethod method, void* context) {
  (void)context;
  auto& prov = ProvisioningManager::getInstance();

  if (method == ProvisioningMethod::FAILED) {
    XLOG_ERROR(CAT_PROVISIONING, "Provisioning FAILED!");
    return;
  }

  const auto* data = prov.getData();
#if FEATURE_MQTT_ENABLED == 1
  if (data && data->type == 0 && strlen(data->wifiSsid) > 0) {
    XLOG_DEBUG(CAT_PROVISIONING, "Provisioning complete - SSID: %s",
               data->wifiSsid);
  }
#endif
}

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

const ProvisioningData* getProvisioningData() {
  return ProvisioningManager::getInstance().getData();
}