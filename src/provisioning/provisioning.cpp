/**
 * @file provisioning.cpp
 * @brief Реализация менеджера провизионинга
 */

#include "provisioning.h"
#include "ble/ble_server.h"
#include "config_manager.h"
#include "led.h"
#include "logger.h"
#include "web.h"
#include "wifi_manager.h"

// ============================================================================
// ГЛОБАЛЬНЫЙ УКАЗАТЕЛЬ НА BLE-СЕРВЕР
// ============================================================================

static BleProvisioningServer* g_bleServer = nullptr;

// ============================================================================
// КОЛБЭКИ ДЛЯ BLE-СЕРВЕРА
// ============================================================================

/**
 * @brief Колбэк при получении WiFi через BLE
 * @param bleConfig Указатель на полученные WiFi данные
 */
static void onBleConfigReceived(const BleWifiConfig* bleConfig) {
  if (!bleConfig) {
    LOG_ERROR(CAT_PROVISIONING, "BLE config is null!");
    return;
  }

  LOG_INFO(CAT_PROVISIONING, "WiFi config received: %s", bleConfig->wifiSsid);

  auto& cfg = ConfigManager::getInstance();

  // BLE даёт только WiFi настройки
  // Остальные настройки (MQTT/Zigbee) берутся из defaults или EEPROM
  if (strlen(bleConfig->wifiSsid) > 0) {
    cfg.setWifiSsid(bleConfig->wifiSsid);
  }
  if (strlen(bleConfig->wifiPassword) > 0) {
    cfg.setWifiPassword(bleConfig->wifiPassword);
  }

  // Если нужны MQTT/Zigbee настройки, они должны быть:
  // 1. В EEPROM (уже сохранены ранее)
  // 2. В defaults (заводские настройки из credentials.h)
  // 3. Или позже через AP/Web

  if (cfg.save()) {
    LOG_INFO(CAT_PROVISIONING, "Config saved successfully!");
    delay(100);
    ESP.restart();
  } else {
    LOG_ERROR(CAT_PROVISIONING, "Failed to save config!");
  }
}

/**
 * @brief Колбэк при изменении статуса BLE-провизионинга
 * @param status Код статуса
 */
static void onBleStatusChanged(uint8_t status) {
  LOG_DEBUG(CAT_PROVISIONING, "BLE status: 0x%02X", status);
}

// ============================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ PROVISIONING MANAGER
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
  _started = false;
  _completed = false;

  // Проверяем, есть ли уже конфигурация
  if (ConfigManager::getInstance().isValid()) {
    LOG_INFO(CAT_PROVISIONING, "Config already exists, skipping provisioning");
    _state = InternalState::COMPLETED;
    _completed = true;
    if (_callback) {
      _callback(_userData);
    }
    return true;
  }

  // Выбираем метод
  selectProvisioningMethod();

  if (_mode == ProvisioningMode::NONE) {
    _state = InternalState::ERROR;
    LOG_ERROR(CAT_PROVISIONING, "No provisioning method available!");
    return false;
  }

  _state = InternalState::WAITING;
  _started = true;
  return true;
}

void ProvisioningManager::update() {
  if (_state != InternalState::WAITING)
    return;

  // Проверяем таймаут
  if (_timeoutMs > 0 && (millis() - _startTime) > _timeoutMs) {
    _state = InternalState::ERROR;
    _completed = true;
    LOG_WARN(CAT_PROVISIONING, "Provisioning timeout");
    // Переключаемся на AP как fallback
    if (_mode == ProvisioningMode::BLE) {
      startApProvisioning();
    }
    return;
  }

  // BLE режим — проверяем завершение
  if (_mode == ProvisioningMode::BLE) {
    if (isBleProvisioningComplete()) {
      _state = InternalState::COMPLETED;
      _completed = true;
      if (_callback) {
        _callback(_userData);
      }
    }
    return;
  }

  // AP режим — обновляем веб-интерфейс
  if (_mode == ProvisioningMode::AP) {
    web_update();

    // Проверяем, не сохранил ли пользователь конфиг через веб
    if (ConfigManager::getInstance().isValid()) {
      LOG_INFO(CAT_PROVISIONING, "AP provisioning completed (config saved)");
      _state = InternalState::COMPLETED;
      _completed = true;
      if (_callback) {
        _callback(_userData);
      }
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

bool ProvisioningManager::complete() {
  if (_mode == ProvisioningMode::BLE && g_bleServer) {
    g_bleServer->stop();
    delete g_bleServer;
    g_bleServer = nullptr;
  }
  _state = InternalState::COMPLETED;
  _completed = true;
  return true;
}

void ProvisioningManager::reset() {
  if (_mode == ProvisioningMode::BLE && g_bleServer) {
    g_bleServer->stop();
    delete g_bleServer;
    g_bleServer = nullptr;
  }
  _state = InternalState::IDLE;
  _mode = ProvisioningMode::NONE;
  _started = false;
  _completed = false;
}

// ============================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ============================================================================

void ProvisioningManager::selectProvisioningMethod() {
#if USE_BLE_PROVISIONING == 1
  const char* deviceId = ConfigManager::getInstance().getDeviceId();

#if defined(ESP32) && !defined(ESP8266)
  _mode = ProvisioningMode::BLE;

  g_bleServer = new BleProvisioningServer(deviceId);

  if (g_bleServer->begin(onBleConfigReceived, onBleStatusChanged)) {
    led_setMode(LED_MODE_MORZE_S);
    LOG_INFO(CAT_PROVISIONING, "BLE provisioning started");
  } else {
    LOG_ERROR(CAT_PROVISIONING, "Failed to start BLE provisioning");
    delete g_bleServer;
    g_bleServer = nullptr;
    _mode = ProvisioningMode::NONE;
    startApProvisioning();  // Fallback
  }
#else
  LOG_WARN(CAT_PROVISIONING, "BLE not supported, falling back to AP");
  _mode = ProvisioningMode::AP;
  startApProvisioning();
#endif

#elif USE_AP_PROVISIONING == 1
  LOG_INFO(CAT_PROVISIONING, "Selecting AP provisioning method...");
  _mode = ProvisioningMode::AP;
  startApProvisioning();

#else
  LOG_ERROR(CAT_PROVISIONING, "No provisioning method selected!");
  _mode = ProvisioningMode::NONE;
#endif
}

void ProvisioningManager::startApProvisioning() {
  LOG_INFO(CAT_PROVISIONING, "Starting AP provisioning...");
  LOG_INFO(CAT_PROVISIONING, "Connect to WiFi '%s' and visit 192.168.4.1",
           ConfigManager::getInstance().getDeviceId());

  _mode = ProvisioningMode::AP;
  _state = InternalState::WAITING;
  _completed = false;
  web_initAP();
  led_setMode(LED_MODE_MORZE_S);
}

bool ProvisioningManager::isBleProvisioningComplete() {
  // Проверяем, завершён ли BLE-провизионинг
  // BLE сам вызывает колбэк onBleConfigReceived при получении данных
  // и перезагружает устройство после сохранения
  // Поэтому здесь просто проверяем, не остановлен ли сервер
  if (g_bleServer && !g_bleServer->isActive()) {
    return true;
  }
  return false;
}

// ============================================================================
// ПРОСТЫЕ ФУНКЦИИ-ОБЁРТКИ ДЛЯ MAIN
// ============================================================================

void startProvisioning() {
  auto& prov = ProvisioningManager::getInstance();
  prov.begin([](void*) { LOG_INFO(CAT_MAIN, "Provisioning completed!"); },
             nullptr, BLE_PROVISIONING_TIMEOUT_MS);
}

bool isProvisioningComplete() {
  return ProvisioningManager::getInstance().isCompleted();
}