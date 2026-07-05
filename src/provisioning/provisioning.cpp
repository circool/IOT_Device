/**
 * @file provisioning.cpp
 * @brief Реализация менеджера провизионинга
 */

#include "provisioning.h"
#include "ble/ble_server.h"
#include "config_manager.h"
#include "led.h"
#include "logger.h"
#include "settings.h"
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
 * @brief Колбэк при получении конфигурации через BLE
 * @param bleConfig Указатель на полученные данные
 */
static void onBleConfigReceived(const BleConfigData* bleConfig) {
  if (!bleConfig) {
    LOG_ERROR(CAT_PROVISIONING, "BLE config is null!");
    return;
  }

  LOG_INFO(CAT_PROVISIONING, "BLE config received");
  LOG_INFO(CAT_PROVISIONING, "WiFi SSID: %s", bleConfig->wifiSsid);

  auto& cfg = ConfigManager::getInstance();

  // Сохраняем настройки WiFi
  if (strlen(bleConfig->wifiSsid) > 0) {
    cfg.setWifiSsid(bleConfig->wifiSsid);
  }
  if (strlen(bleConfig->wifiPassword) > 0) {
    cfg.setWifiPassword(bleConfig->wifiPassword);
  }

#if IS_MQTT_ENABLED
  // Сохраняем настройки MQTT (если есть)
  if (strlen(bleConfig->mqttBroker) > 0) {
    cfg.setMqttBroker(bleConfig->mqttBroker);
  }
  if (bleConfig->mqttPort > 0) {
    cfg.setMqttPort(bleConfig->mqttPort);
  }
  if (strlen(bleConfig->mqttUser) > 0) {
    cfg.setMqttUser(bleConfig->mqttUser);
  }
  if (strlen(bleConfig->mqttPassword) > 0) {
    cfg.setMqttPassword(bleConfig->mqttPassword);
  }
  if (strlen(bleConfig->mqttClientId) > 0) {
    cfg.setMqttClientId(bleConfig->mqttClientId);
  }
#endif

#if IS_ZIGBEE_ENABLED
  // Сохраняем настройки Zigbee (если есть)
  if (strlen(bleConfig->zigbeeNetworkKey) > 0) {
    cfg.setZigbeeNetworkKey(bleConfig->zigbeeNetworkKey);
  }
  cfg.setZigbeePanId(bleConfig->zigbeePanId);
  if (bleConfig->zigbeeChannel >= 11 && bleConfig->zigbeeChannel <= 26) {
    cfg.setZigbeeChannel(bleConfig->zigbeeChannel);
  }
  cfg.setProtocolMode(bleConfig->protocolMode);
#endif

  // Сохраняем конфигурацию в EEPROM
  if (cfg.save()) {
    LOG_INFO(CAT_PROVISIONING, "Config saved successfully!");
    // Перезагружаемся для применения настроек
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

  // LOG_INFO(CAT_PROVISIONING, "Provisioning begin...");

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

void ProvisioningManager::process() {
  if (_state != InternalState::WAITING)
    return;

  // WiFiProv сам обрабатывает все события через sysProvEvent
  // Нам нужно только ждать завершения

  if (_timeoutMs > 0 && (millis() - _startTime) > _timeoutMs) {
    _state = InternalState::ERROR;
    _completed = true;
    LOG_WARN(CAT_PROVISIONING, "Provisioning timeout");
    // Переключаемся на AP как fallback
    startApProvisioning();
  }
}

void ProvisioningManager::update() {
  process();
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
  // ===== BLE (только WiFi) =====
#if USE_BLE_PROVISIONING == 1
  const char* deviceId = ConfigManager::getInstance().getDeviceId();

#if defined(ESP32) && !defined(ESP8266)
  _mode = ProvisioningMode::BLE;

  g_bleServer = new BleProvisioningServer(deviceId);

  if (g_bleServer->begin(onBleConfigReceived, onBleStatusChanged, nullptr,
                         nullptr)) {
    led_setMode(LED_MODE_MORZE_S);
  } else {
    LOG_ERROR(CAT_PROVISIONING, "Failed to start BLE provisioning");
    delete g_bleServer;
    g_bleServer = nullptr;
    _mode = ProvisioningMode::NONE;
    // Пробуем AP как fallback
    startApProvisioning();
  }
#else
  LOG_WARN(CAT_PROVISIONING, "BLE not supported, falling back to AP");
  _mode = ProvisioningMode::AP;
  startApProvisioning();
#endif

  // ===== AP + Web (полная настройка) =====
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

// ============================================================================
// ПРОСТЫЕ ФУНКЦИИ-ОБЁРТКИ ДЛЯ MAIN
// ============================================================================

void startProvisioning() {


  auto& prov = ProvisioningManager::getInstance();

  // Запускаем с колбэком, который переключает режим
  prov.begin(
      [](void*) {
        LOG_INFO(CAT_MAIN, "Provisioning completed!");
        // Флаг g_normalMode переключается в main.cpp
      },
      nullptr, BLE_PROVISIONING_TIMEOUT_MS);
}

void runProvisioning() {
  auto& prov = ProvisioningManager::getInstance();
  prov.update();  // Вызывает process()

  // Если AP режим — обновляем web
  if (prov.getMode() == ProvisioningMode::AP) {
    web_update();
  }
}

bool isProvisioningComplete() {
  return ProvisioningManager::getInstance().isCompleted();
}