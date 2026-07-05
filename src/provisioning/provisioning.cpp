/**
 * @file provisioning.cpp
 * @brief Реализация менеджера провизионинга
 */

#include "provisioning.h"
#include "ble/ble_server.h"
#include "logger.h"
#include "settings.h"
#include "wifi_manager.h"

// ============================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ============================================================================

ProvisioningManager& ProvisioningManager::getInstance() {
  static ProvisioningManager instance;
  return instance;
}

bool ProvisioningManager::begin(ProvisioningMode mode,
                                const ProvisioningConfig& config,
                                WebUpdateCallback webUpdateCallback,
                                ProvisioningCallback callback,
                                void* userData) {
  if (_state != InternalState::IDLE)
    return false;

  
  LOG_INFO(CAT_PROVISIONING, "Starting provisioning (mode: %d)", (int)mode);

  _config = config;
  _webUpdateCallback = webUpdateCallback;
  _callback = callback;
  _userData = userData;
  _completed = false;
  _bleCompleted = false;
  _apCompleted = false;
  _mode = mode;

  memset(&_result, 0, sizeof(_result));

  // Запускаем BLE если нужно
  if (mode == ProvisioningMode::BLE || mode == ProvisioningMode::BOTH) {
    startBleProvisioning();
  }

  // Запускаем AP если нужно
  if (mode == ProvisioningMode::AP || mode == ProvisioningMode::BOTH) {
    startApProvisioning();
  }

  _state = InternalState::WAITING;

  LOG_DEBUG(CAT_PROVISIONING, "Provisioning started");
  if (mode == ProvisioningMode::BLE || mode == ProvisioningMode::BOTH) {
    LOG_DEBUG(CAT_PROVISIONING, "  BLE: device '%s', PIN: %s",
              config.deviceName, config.blePin);
  }
  if (mode == ProvisioningMode::AP || mode == ProvisioningMode::BOTH) {
    LOG_DEBUG(CAT_PROVISIONING, "  AP: WiFi '%s', IP: %s", config.deviceName,
              config.apIpAddress);
  }

  return true;
}

void ProvisioningManager::update() {
  if (_state != InternalState::WAITING)
    return;



  // Обновляем web (AP режим) через колбэк
  if (_webUpdateCallback) {
    _webUpdateCallback();
  }

  // Проверяем BLE (через opaque pointer)
  if (_bleServer) {
    auto* server = static_cast<BleProvisioningServer*>(_bleServer);
    if (!server->isActive()) {
      _bleCompleted = true;
      LOG_DEBUG(CAT_PROVISIONING, "BLE provisioning completed");
    }
  }

  // Проверяем завершение - ТОЛЬКО УСПЕШНОЕ!
  if (_bleCompleted || _apCompleted) {
    LOG_INFO(CAT_PROVISIONING, "Provisioning completed successfully via %s",
             _bleCompleted ? "BLE" : "AP");
    _state = InternalState::COMPLETED;
    _completed = true;
    _result.success = true;
    if (_callback) {
      _callback(&_result, _userData);
    }
  }
}

bool ProvisioningManager::isActive() const {
  return _state == InternalState::WAITING;
}

bool ProvisioningManager::isCompleted() const {
  return _completed || _state == InternalState::COMPLETED;
}

ProvisioningMode ProvisioningManager::getMode() const {
  return _mode;
}

void ProvisioningManager::reset() {
  if (_bleServer) {
    auto* server = static_cast<BleProvisioningServer*>(_bleServer);
    server->stop();
    delete server;
    _bleServer = nullptr;
  }
  _state = InternalState::IDLE;
  _mode = ProvisioningMode::NONE;
  _completed = false;
  _bleCompleted = false;
  _apCompleted = false;
  memset(&_result, 0, sizeof(_result));
}

// ============================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ============================================================================

void ProvisioningManager::onBleConfigReceived(const void* data, size_t size) {
  if (!data || size != sizeof(BleConfigData)) {
    LOG_ERROR(CAT_PROVISIONING, "Invalid BLE config data");
    return;
  }

  const auto* bleConfig = static_cast<const BleConfigData*>(data);

  LOG_DEBUG(CAT_PROVISIONING, "BLE config received");
  LOG_DEBUG(CAT_PROVISIONING, "WiFi SSID: %s", bleConfig->wifiSsid);

  // Сохраняем только то, что реально передаётся через BLE
  strncpy(_result.wifiSsid, bleConfig->wifiSsid, sizeof(_result.wifiSsid) - 1);
  strncpy(_result.wifiPassword, bleConfig->wifiPassword,
          sizeof(_result.wifiPassword) - 1);
  _result.success = true;

  _bleCompleted = true;

  if (_bleServer) {
    auto* server = static_cast<BleProvisioningServer*>(_bleServer);
    server->stop();
    delete server;
    _bleServer = nullptr;
  }
}

void ProvisioningManager::startBleProvisioning() {
#if defined(ESP32) && !defined(ESP8266)
  auto* server = new BleProvisioningServer(_config.deviceName);

  if (server->begin(
          [this](const BleConfigData* config) {
            this->onBleConfigReceived(config, sizeof(BleConfigData));
          },
          nullptr, nullptr, nullptr)) {
    _bleServer = server;
    LOG_DEBUG(CAT_PROVISIONING, "BLE provisioning started");
  } else {
    LOG_ERROR(CAT_PROVISIONING, "Failed to start BLE provisioning");
    delete server;
    _bleServer = nullptr;
  }
#else
  LOG_WARN(CAT_PROVISIONING, "BLE not supported on this platform");
#endif
}

void ProvisioningManager::startApProvisioning() {
  LOG_DEBUG(CAT_PROVISIONING, "AP provisioning requested");
  LOG_DEBUG(CAT_PROVISIONING, "Connect to WiFi '%s' and visit %s",
            _config.deviceName, _config.apIpAddress);

  // AP запускается оркестратором через web_initAP()
  // Здесь только отмечаем
  _apCompleted = false;
}

// ============================================================================
// ФУНКЦИИ-ОБЁРТКИ ДЛЯ MAIN
// ============================================================================

void startProvisioning(const ProvisioningConfig& config,
                       WebUpdateCallback webUpdateCallback,
                       ProvisioningCallback callback,
                       void* userData) {
  LOG_INFO(CAT_MAIN, "Starting provisioning (BLE + AP)...");

  ProvisioningManager::getInstance().begin(
      ProvisioningMode::BOTH, config, webUpdateCallback, callback, userData);
}

bool isProvisioningComplete() {
  return ProvisioningManager::getInstance().isCompleted();
}