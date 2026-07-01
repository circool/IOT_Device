#include "provisioning.h"
#include "ble/ble_server.h"
#include "config_manager.h"
#include "logger.h"
#include "storage/provisioning_storage.h"

// ============================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ============================================================================

ProvisioningManager& ProvisioningManager::getInstance() {
  static ProvisioningManager instance;
  return instance;
}

bool ProvisioningManager::begin(ProvisioningCallback callback,
                                void* userData,
                                uint32_t timeoutMs) {
  if (_state != InternalState::IDLE)
    return false;

  _callback = callback;
  _userData = userData;
  _timeoutMs = timeoutMs;
  _startTime = millis();

  // Проверяем сохраненную конфигурацию
  if (loadFromStorage()) {
    _state = InternalState::COMPLETED;
    return true;
  }

  // Выбираем метод
  selectProvisioningMethod();

  if (_mode == ProvisioningMode::NONE) {
    _state = InternalState::ERROR;
    return false;
  }

  _state = InternalState::WAITING;
  return true;
}

void ProvisioningManager::process() {
  if (_state != InternalState::WAITING)
    return;

  if (_mode == ProvisioningMode::BLE) {
    BleProvisioningServer* ble = getBleServer();
    if (!ble)
      return;

    ble->process();

    BleProvisioningState state = ble->getState();

    switch (state) {
      case BleProvisioningState::WIFI_CONNECTED:
        _state = InternalState::RECEIVED;
        LOG_INFO(CAT_PROVISIONING, "Provisioning completed successfully");
        break;

      case BleProvisioningState::MAX_ATTEMPTS_REACHED:
        LOG_ERROR(CAT_PROVISIONING, "Max attempts reached");
        _state = InternalState::ERROR;
        break;

      case BleProvisioningState::TIMEOUT:
        LOG_ERROR(CAT_PROVISIONING, "Timeout");
        _state = InternalState::ERROR;
        break;

      case BleProvisioningState::STOPPED:
        LOG_ERROR(CAT_PROVISIONING, "Stopped");
        _state = InternalState::ERROR;
        break;

      default:
        break;
    }
  }
}

bool ProvisioningManager::isActive() const {
  return _state == InternalState::WAITING;
}

ProvisioningMode ProvisioningManager::getMode() const {
  return _mode;
}

bool ProvisioningManager::complete(const ProvisioningConfig* config) {
  if (config) {
    if (!saveConfig(config))
      return false;
    memcpy(&_config, config, sizeof(ProvisioningConfig));
  }

  if (_mode == ProvisioningMode::BLE && _bleServer) {
    _bleServer->stop();
    delete _bleServer;
    _bleServer = nullptr;
  }

  _state = InternalState::COMPLETED;
  return true;
}

void ProvisioningManager::reset() {
  ProvisioningStorage::getInstance().clear();

  if (_mode == ProvisioningMode::BLE && _bleServer) {
    _bleServer->stop();
    delete _bleServer;
    _bleServer = nullptr;
  }

  _state = InternalState::IDLE;
  _mode = ProvisioningMode::NONE;
  memset(&_config, 0, sizeof(_config));
}

bool ProvisioningManager::getSavedConfig(ProvisioningConfig* config) const {
  if (!config)
    return false;
  return ProvisioningStorage::getInstance().load(config);
}

bool ProvisioningManager::saveConfig(const ProvisioningConfig* config) {
  if (!config)
    return false;
  return ProvisioningStorage::getInstance().save(config);
}

void ProvisioningManager::onConfigReceived(const ProvisioningConfig* config) {
  if (!config || _state != InternalState::WAITING)
    return;

  memcpy(&_config, config, sizeof(ProvisioningConfig));
  saveConfig(config);

  if (_callback) {
    _callback(&_config, _userData);
  }

  _state = InternalState::RECEIVED;
}

// ============================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ============================================================================

BleProvisioningServer* ProvisioningManager::getBleServer() const {
  return _bleServer;  // <-- ИСПРАВЛЕНО
}

void ProvisioningManager::selectProvisioningMethod() {
#if defined(ESP32) && !defined(ESP8266)
// Проверяем поддержку BLE на конкретной платформе
#if defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32S3) || \
    defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32)

  _mode = ProvisioningMode::BLE;
  _bleServer = new BleProvisioningServer(g_configManager.getDeviceId());

  if (_bleServer->begin(
          [this](const BleConfigData* config) {
            ProvisioningConfig provConfig;
            memset(&provConfig, 0, sizeof(provConfig));

            strncpy(provConfig.wifiSsid, config->wifiSsid,
                    sizeof(provConfig.wifiSsid) - 1);
            strncpy(provConfig.wifiPassword, config->wifiPassword,
                    sizeof(provConfig.wifiPassword) - 1);
            strncpy(provConfig.mqttBroker, config->mqttBroker,
                    sizeof(provConfig.mqttBroker) - 1);
            provConfig.mqttPort = config->mqttPort;
            strncpy(provConfig.mqttUser, config->mqttUser,
                    sizeof(provConfig.mqttUser) - 1);
            strncpy(provConfig.mqttPassword, config->mqttPassword,
                    sizeof(provConfig.mqttPassword) - 1);
            strncpy(provConfig.deviceName, g_configManager.getDeviceId(),
                    sizeof(provConfig.deviceName) - 1);

            this->onConfigReceived(&provConfig);
          },
          nullptr, BLE_PROVISIONING_TIMEOUT_MS)) {
    LOG_INFO(CAT_PROVISIONING, "BLE provisioning started");
  } else {
    LOG_ERROR(CAT_PROVISIONING, "Failed to start BLE provisioning");
    delete _bleServer;
    _bleServer = nullptr;
    _mode = ProvisioningMode::NONE;
  }
#else
  _mode = ProvisioningMode::AP;
  LOG_INFO(CAT_PROVISIONING, "Using AP provisioning (fallback)");
#endif
#else
  _mode = ProvisioningMode::AP;
  LOG_INFO(CAT_PROVISIONING, "Using AP provisioning");
#endif
}

bool ProvisioningManager::loadFromStorage() {
  ProvisioningConfig config;
  if (ProvisioningStorage::getInstance().load(&config)) {
    if (strlen(config.wifiSsid) > 0) {
      memcpy(&_config, &config, sizeof(ProvisioningConfig));
      return true;
    }
  }
  return false;
}