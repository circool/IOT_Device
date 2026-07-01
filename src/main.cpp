// ===== ФАЙЛ: src/main.cpp =====

#include <Arduino.h>
#include "config_manager.h"
#include "led.h"
#include "logger.h"
#include "ota.h"
#include "settings.h"
#include "wdt_manager.h"
#include "wifi_manager.h"
#include "debug_tools.h"

#ifdef ESP32
#include <WiFi.h>
#include <esp_chip_info.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// ============================================================================
// PROVISIONING
// ============================================================================

#include "provisioning/provisioning.h"

// Если используется BLE, AP и Web отключаются
#if USE_BLE_PROVISIONING == 1
#undef AP_ENABLED
#define AP_ENABLED 0
#undef WEB_ENABLED
#define WEB_ENABLED 0
#endif

static WiFiClient g_mqttClient;

#if OTA_ENABLED == 1
#include "ota.h"
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
#include "sensor.h"
#endif

#if DEVICE_TYPE == 1
#include "fan_actuator.h"
#endif

#if DEVICE_TYPE == 3
#include "switch_actuator.h"
#endif

#include "web.h"
#include "web_status_provider.h"

#if MQTT_ENABLED == 1
#include "mqtt.h"
#endif

#if ZIGBEE_ENABLED == 1
#include "zigbee.h"
#endif

// ============================================================================
// КОНЕЧНЫЙ АВТОМАТ СИСТЕМЫ (Оркестратор)
// ============================================================================

enum class SystemPhase : uint8_t { INIT = 0, PROVISIONING, NORMAL, ERROR };

static SystemPhase g_phase = SystemPhase::INIT;
static unsigned long g_errorStartTime = 0;
static bool g_provisioningCompleted = false;

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ============================================================================

void initNormalMode();
void initProvisioningMode();
void handleProvisioning();
void handleNormal();
void handleError();
void updateLed();
void registerMqttCallbacks();
void publishMqttStatus();

// ============================================================================
// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
// ============================================================================

#if DEVICE_TYPE == 1
FanActuator fan;
#elif DEVICE_TYPE == 3
SwitchActuator switchActuator;
#endif

#if WEB_ENABLED == 1
#if DEVICE_TYPE == 1
static FanWebStatusProvider statusProvider(&fan, nullptr, &mqttManager);
#elif DEVICE_TYPE == 3
static SwitchWebStatusProvider statusProvider(&switchActuator, &mqttManager);
#endif
#endif

// ============================================================================
// MQTT FUNCTIONS
// ============================================================================

#if MQTT_ENABLED == 1

unsigned long lastMQTTAttempt = 0;

#if MQTT_PUBLISH_RESET_REASON == 1

#endif

void registerMqttCallbacks() {
#if DEVICE_TYPE == 1
  mqttManager.onStateCommand([](bool state) { fan.set(state, true); });

  mqttManager.onSpeedCommand([](int speed) {
    if (g_configManager.getAdaptiveMode()) {
      g_configManager.setAdaptiveMode(false);
      fan.setAdaptiveMode(false);
    }
    if (speed == 0 || speed < MIN_SPEED_PERCENT) {
      fan.set(false, true);
    } else {
      fan.setSpeed(speed, true);
      if (fan.getState()) {
        fan.setSpeed(speed, false);
      }
    }
  });

  mqttManager.onSensorControlModeCommand([](bool enabled) {
    if (enabled && !sensor_isOk()) {
      LOG_WARN(CAT_MQTT,
               "Cannot enable sensor control mode - sensor not available");
      mqttManager.publishSensorControlMode(false);
      return;
    }
    g_configManager.setSensorControlMode(enabled);
    if (!enabled) {
      fan.setAdaptiveMode(false);
    }
  });

  mqttManager.onAdaptiveModeCommand([](bool enabled) {
    if (enabled && !g_configManager.getSensorControlMode()) {
      LOG_WARN(CAT_MQTT,
               "Cannot enable adaptive mode - sensor control mode is OFF");
      return;
    }
    g_configManager.setAdaptiveMode(enabled);
    fan.setAdaptiveMode(enabled);
  });

  mqttManager.onLowTempCommand(
      [](float value) { g_configManager.setLowTemp(value); });
  mqttManager.onHighTempCommand(
      [](float value) { g_configManager.setHighTemp(value); });
  mqttManager.onLowHumCommand(
      [](float value) { g_configManager.setLowHum(value); });
  mqttManager.onHighHumCommand(
      [](float value) { g_configManager.setHighHum(value); });
  mqttManager.onDelaySecCommand(
      [](int delaySec) { g_configManager.setDelaySeconds(delaySec); });
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  mqttManager.onMaxOnTimeCommand(
      [](uint32_t maxOnTime) { g_configManager.setMaxOnTime(maxOnTime); });
#endif

#if MQTT_RESET_ENABLED == 1
  mqttManager.onResetCommand([]() {
    LOG_INFO(CAT_MQTT, "Resetting due MQTT RESET");
    mqttManager.disconnect();
    g_configManager.reset();
    delay(1000);
    ESP.restart();
  });
#endif
}

void publishMqttStatus() {
  if (!mqttManager.isConnected())
    return;

#if DEVICE_TYPE == 1
  static bool lastFanState = false;
  bool currentFanState = fan.getState();
  if (currentFanState != lastFanState) {
    mqttManager.publishState(currentFanState);
    lastFanState = currentFanState;
  }

  static uint16_t lastSpeedPercent = 0;
  if (fan.getSpeed() != lastSpeedPercent) {
    mqttManager.publishSpeed(fan.getSpeed());
    lastSpeedPercent = fan.getSpeed();
  }

  static bool lastSensorControlMode = false;
  if (g_configManager.getSensorControlMode() != lastSensorControlMode) {
    mqttManager.publishSensorControlMode(
        g_configManager.getSensorControlMode());
    lastSensorControlMode = g_configManager.getSensorControlMode();
  }

  static bool lastAdaptiveMode = false;
  if (fan.getAdaptiveMode() != lastAdaptiveMode) {
    mqttManager.publishAdaptiveMode(fan.getAdaptiveMode());
    lastAdaptiveMode = fan.getAdaptiveMode();
  }

  static float lastLowTemp = 0, lastHighTemp = 0, lastLowHum = 0,
               lastHighHum = 0;
  if (fabs(g_configManager.getLowTemp() - lastLowTemp) > 0.01 ||
      fabs(g_configManager.getHighTemp() - lastHighTemp) > 0.01 ||
      fabs(g_configManager.getLowHum() - lastLowHum) > 0.01 ||
      fabs(g_configManager.getHighHum() - lastHighHum) > 0.01) {
    mqttManager.publishThresholds(
        g_configManager.getLowTemp(), g_configManager.getHighTemp(),
        g_configManager.getLowHum(), g_configManager.getHighHum());
    lastLowTemp = g_configManager.getLowTemp();
    lastHighTemp = g_configManager.getHighTemp();
    lastLowHum = g_configManager.getLowHum();
    lastHighHum = g_configManager.getHighHum();
  }
#endif

#if DEVICE_TYPE == 3
  static bool lastSwitchState = false;
  bool currentSwitchState = switchActuator.getState();
  if (currentSwitchState != lastSwitchState) {
    mqttManager.publishState(currentSwitchState);
    lastSwitchState = currentSwitchState;
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  static int lastDelaySeconds = -1;
  if (g_configManager.getDelaySeconds() != lastDelaySeconds) {
    mqttManager.publishDelaySec(g_configManager.getDelaySeconds());
    lastDelaySeconds = g_configManager.getDelaySeconds();
  }

  static uint32_t lastMaxOnTime = 0;
  if (g_configManager.getMaxOnTime() != lastMaxOnTime) {
    mqttManager.publishMaxOnTime(g_configManager.getMaxOnTime());
    lastMaxOnTime = g_configManager.getMaxOnTime();
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (sensor_isOk()) {
    static float lastTemp = 0, lastHum = 0;
    const float EPSILON = 0.05;
    float temp = sensor_getTemperature();
    float hum = sensor_getHumidity();
    if (fabs(temp - lastTemp) > EPSILON || fabs(hum - lastHum) > EPSILON) {
      mqttManager.publishSensor(temp, hum);
      lastTemp = temp;
      lastHum = hum;
    }
  }
#endif

  static unsigned long lastHeartbeat = 0;
  static bool initialConfigPublished = false;

  if (!initialConfigPublished) {
#if DEVICE_TYPE == 1
    mqttManager.publishState(fan.getState());
    mqttManager.publishSpeed(fan.getSpeed());
    mqttManager.publishSensorControlMode(
        g_configManager.getSensorControlMode());
    mqttManager.publishAdaptiveMode(fan.getAdaptiveMode());
    mqttManager.publishThresholds(
        g_configManager.getLowTemp(), g_configManager.getHighTemp(),
        g_configManager.getLowHum(), g_configManager.getHighHum());
#elif DEVICE_TYPE == 3
    mqttManager.publishState(switchActuator.getState());
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    mqttManager.publishDelaySec(g_configManager.getDelaySeconds());
    mqttManager.publishMaxOnTime(g_configManager.getMaxOnTime());
#endif

// #if MQTT_PUBLISH_RESET_REASON == 1
    mqttManager.publishResetReason(getResetReason());
// #endif

#if MQTT_PUBLISH_VERSION == 1
    mqttManager.publishVersion(VERSION);
#endif

    initialConfigPublished = true;
    LOG_INFO(CAT_MQTT, "Initial config published");
  }

  if (millis() - lastHeartbeat >= STATE_PUBLISH_INTERVAL_MS) {
    mqttManager.publishOnline();
#if MQTT_PUBLISH_RSSI == 1
    mqttManager.publishRSSI(wifi_get_rssi());
#endif
    lastHeartbeat = millis();
  }
}

#endif  // MQTT_ENABLED == 1

// ============================================================================
// КНОПКА СБРОСА
// ============================================================================

void checkResetButton() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  delay(50);
  if (digitalRead(RESET_PIN) == LOW) {
    LOG_INFO(CAT_MAIN, "Reset button pressed...");

    LedMode prevMode = led_getMode();
    unsigned long pressStart = millis();
    wdt_stop();
    while (digitalRead(RESET_PIN) == LOW) {
      unsigned long pressedMs = millis() - pressStart;

      if (pressedMs < 1000) {
        led_setMode(LED_MODE_MORZE_E);
      } else if (pressedMs < 2000) {
        led_setMode(LED_MODE_MORZE_I);
      } else {
        led_setMode(LED_MODE_MORZE_S);
      }
      led_update();

      if (pressedMs >= 3000) {
        LOG_INFO(CAT_MAIN, "Auto-reset triggered!");
        led_setMode(LED_MODE_OFF);
        led_update();

        if (g_configManager.reset()) {
          LOG_INFO(CAT_CONFIG, "Config cleared, restarting...");
          ESP.restart();
        }
        return;
      }

      delay(10);
      wdt_feed();
    }

    LOG_INFO(CAT_MAIN, "Reset cancelled (released after %d ms)",
             millis() - pressStart);
    wdt_start();
    led_setMode(prevMode);
  }
}

// ============================================================================
// КОЛБЭК PROVISIONING
// ============================================================================

#if USE_BLE_PROVISIONING == 1

static void onProvisioningComplete(const ProvisioningConfig* config,
                                   void* userData) {
  (void)userData;
  LOG_INFO(CAT_MAIN, "PROVISIONING COMPLETE! WiFi SSID: %s, MQTT Broker: %s:%d",
           config->wifiSsid, config->mqttBroker, config->mqttPort);
  if (strlen(config->wifiSsid) > 0) {
    g_configManager.setWifiSsid(config->wifiSsid);
    g_configManager.setWifiPassword(config->wifiPassword);
  }

#if MQTT_ENABLED == 1
  if (strlen(config->mqttBroker) > 0) {
    g_configManager.setMqttBroker(config->mqttBroker);
    g_configManager.setMqttPort(config->mqttPort);
    g_configManager.setMqttUser(config->mqttUser);
    g_configManager.setMqttPassword(config->mqttPassword);
    g_configManager.setMqttClientId(config->mqttClientId);
  }
#endif

#if ZIGBEE_ENABLED == 1
  if (strlen(config->zigbeeNetworkKey) > 0) {
    g_configManager.setZigbeeNetworkKey(config->zigbeeNetworkKey);
  }
  g_configManager.setZigbeePanId(config->zigbeePanId);
  g_configManager.setZigbeeChannel(config->zigbeeChannel);
#endif

  if (g_configManager.save()) {
    LOG_INFO(CAT_MAIN, "Config saved successfully!");
    g_provisioningCompleted = true;
  } else {
    LOG_ERROR(CAT_MAIN, "Failed to save config!");
    return;
  }

  LOG_INFO(CAT_MAIN, "Rebooting in 1 second...");
  delay(1000);
  ESP.restart();
}

#endif  // USE_BLE_PROVISIONING == 1

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ NORMAL MODE
// ============================================================================

void initNormalMode() {
  LOG_INFO(CAT_MAIN, "========================================");
  LOG_INFO(CAT_MAIN, "NORMAL MODE");
  LOG_INFO(CAT_MAIN, "========================================");

  // ========== ДАТЧИК ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_init();
#if DEVICE_TYPE == 1
  if (!sensor_isOk() && (
#if SENSOR_TYPE == 1
                            strcmp(sensor_getError(), "AHT10 not found") == 0
#elif SENSOR_TYPE == 2
                            strcmp(sensor_getError(),
                                   "DHT read failed (NaN)") == 0
#endif
                            )) {
    g_configManager.setSensorControlMode(false);
    g_configManager.setAdaptiveMode(false);
    fan.setAdaptiveMode(false);
    LOG_WARN(CAT_SENSOR, "Not found - switching to MANUAL mode");
  }
#endif
#endif

  // ========== ВЕНТИЛЯТОР ==========
#if DEVICE_TYPE == 1
  fan.init(SWITCH_PIN, RELAY_ON_LEVEL, g_configManager.getBootState(),
           g_configManager.getSpeedPercent());
  fan.setAdaptiveMode(g_configManager.getAdaptiveMode());
#endif

  // ========== ВЫКЛЮЧАТЕЛЬ ==========
#if DEVICE_TYPE == 3
  switchActuator.init(SWITCH_PIN, RELAY_ON_LEVEL,
                      g_configManager.getBootState());
#endif

  // ========== MQTT ==========
#if MQTT_ENABLED == 1
  mqttManager.begin(
      g_mqttClient, g_configManager.getMqttBroker(),
      g_configManager.getMqttPort(), g_configManager.getMqttClientId(),
      g_configManager.getMqttUser(), g_configManager.getMqttPassword());
  registerMqttCallbacks();
#endif

  // ========== WIFI ==========
#if SCANING_WIFI_ENABLED == 1
#ifdef ESP8266
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  delay(100);
#elif defined(ESP32)
  WiFi.setSleep(false);
  delay(100);
#endif
#endif
  wifi_begin();

  // ========== WEB ==========
#if WEB_ENABLED == 1
  // Регистрируем провайдер статуса
#if DEVICE_TYPE == 1
  web_registerStatusProvider(&statusProvider);
#elif DEVICE_TYPE == 3
  web_registerStatusProvider(&statusProvider);
#endif

  // ========== OTA ==========
#if OTA_ENABLED == 1
  if (ota_is_available()) {
    ota_init(&server);
    LOG_INFO(CAT_MAIN, "OTA initialized");
  } else {
    LOG_WARN(CAT_MAIN, "OTA not available");
  }
#endif

  web_init();
#endif

  LOG_INFO(CAT_MAIN, "System initialized successfully!");
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ PROVISIONING MODE
// ============================================================================

void initProvisioningMode() {
#if USE_BLE_PROVISIONING == 1
  LOG_WARN(CAT_MAIN, "No valid config found, starting BLE provisioning...");

  auto& prov = ProvisioningManager::getInstance();
  bool started =
      prov.begin(onProvisioningComplete, nullptr, BLE_PROVISIONING_TIMEOUT_MS);

  if (started) {
    LOG_DEBUG(CAT_MAIN, "BLE provisioning device %s was started", g_configManager.getDeviceId());
    led_setMode(LED_MODE_MORZE_S);
  } else {
    LOG_ERROR(CAT_MAIN, "Failed to start BLE provisioning!");
#if AP_ENABLED == 1
    LOG_INFO(CAT_MAIN, "Falling back to AP mode...");
    web_initAP();
#else
    g_phase = SystemPhase::ERROR;
    g_errorStartTime = millis();
#endif
  }

#else  // USE_BLE_PROVISIONING == 0
  // ===== AP + Web (ESP8266, ESP32 без BLE) =====
  LOG_INFO(CAT_CONFIG, "Starting AP for setup...");
#if AP_ENABLED == 1
  web_initAP();
#else
  g_phase = SystemPhase::ERROR;
  g_errorStartTime = millis();
  LOG_ERROR(CAT_MAIN, "No provisioning method available!");
#endif
#endif  // USE_BLE_PROVISIONING == 1
}

// ============================================================================
// ОБРАБОТЧИКИ ФАЗ (вызываются из loop)
// ============================================================================

void handleProvisioning() {
#if USE_BLE_PROVISIONING == 1
  auto& prov = ProvisioningManager::getInstance();
  prov.process();

  if (prov.isActive()) {
    led_update();
    return;
  }

  // Если провизионинг завершен (колбэк установил флаг)
  if (g_provisioningCompleted) {
    g_phase = SystemPhase::NORMAL;
    initNormalMode();
    return;
  }

  // Проверяем состояние провизионинга
  if (prov.getMode() == ProvisioningMode::COMPLETED) {
    LOG_INFO(CAT_MAIN, "Provisioning completed successfully!");
    g_phase = SystemPhase::NORMAL;
    initNormalMode();
    return;
  }

  // Если провизионинг не активен и не завершен — ошибка
  LOG_ERROR(CAT_MAIN, "Provisioning stopped unexpectedly!");
  g_phase = SystemPhase::ERROR;
  g_errorStartTime = millis();
#else
  // AP mode — обрабатываем через web
  web_update();
#endif
}

void handleNormal() {
  // ========== ДАТЧИК ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  bool sensorDataChanged = sensor_update();
#if DEVICE_TYPE == 1
  if (g_configManager.getSensorControlMode() && sensor_isOk() &&
      sensorDataChanged) {
    float temp = sensor_getTemperature();
    float hum = sensor_getHumidity();

    bool shouldBeOn = (temp >= g_configManager.getHighTemp() ||
                       hum >= g_configManager.getHighHum());
    bool shouldBeOff = (temp <= g_configManager.getLowTemp() &&
                        hum <= g_configManager.getLowHum());

    if (shouldBeOn && !fan.getState()) {
      fan.set(true, false);
      LOG_INFO(CAT_SENSOR, "Auto ON: T=%.1f°C H=%.1f%%", temp, hum);
    } else if (shouldBeOff && fan.getState()) {
      fan.set(false, false);
      LOG_INFO(CAT_SENSOR, "Auto OFF: T=%.1f°C H=%.1f%%", temp, hum);
    }
  }
#endif
#endif

  // ========== ВЕНТИЛЯТОР ==========
#if DEVICE_TYPE == 1
  fan.update();
#endif

  // ========== ВЫКЛЮЧАТЕЛЬ ==========
#if DEVICE_TYPE == 3
  switchActuator.update();
#endif

  // ========== WIFI ==========
  wifi_monitor();

  // ========== OTA LOOP ==========
#if OTA_ENABLED == 1
  ota_loop();
#endif

  // ========== MQTT ==========
#if MQTT_ENABLED == 1
  if (wifi_is_connected()) {
    mqttManager.process();
    publishMqttStatus();
  }
#endif

  // ========== WEB ==========
  web_update();

  // ========== LED ==========
  updateLed();
}

void handleError() {
  led_setMode(LED_SLOW_BLINK);
  led_update();

  if (g_errorStartTime == 0) {
    g_errorStartTime = millis();
    LOG_ERROR(CAT_MAIN, "Entered ERROR state, waiting 5 seconds before reboot...");
  } else if (millis() - g_errorStartTime > 5000) {
    LOG_INFO(CAT_MAIN, "Rebooting after error...");
    ESP.restart();
  }
}

// ============================================================================
// LED ИНДИКАЦИЯ
// ============================================================================

void updateLed() {
  LedMode newMode;

  switch (g_phase) {
    case SystemPhase::PROVISIONING:
      newMode = LED_MODE_MORZE_S;
      break;

    case SystemPhase::NORMAL:
#if DEVICE_TYPE == 1
      if (fan.isEmergencyStop()) {
        newMode = LED_SLOW_BLINK;
        break;
      }
#elif DEVICE_TYPE == 3
      if (switchActuator.isEmergencyStop()) {
        newMode = LED_SLOW_BLINK;
        break;
      }
#endif
      if (apMode) {
        newMode = LED_MODE_MORZE_S;
      } else if (!wifi_is_connected()) {
        newMode = LED_MODE_MORZE_E;
#if MQTT_ENABLED == 1
      } else if (!mqttManager.isConnected()) {
        newMode = LED_MODE_MORZE_I;
#endif
      } else {
        newMode = LED_MODE_ON;
      }
      break;

    case SystemPhase::ERROR:
      newMode = LED_SLOW_BLINK;
      break;

    default:
      newMode = LED_MODE_OFF;
      break;
  }

  static LedMode lastMode = LED_MODE_OFF;
  if (newMode != lastMode) {
    led_setMode(newMode);
    lastMode = newMode;
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  delay(2000);
  Logger::getInstance().begin((LogLevel)LOG_LEVEL, LOG_CATEGORIES,LOG_USE_COLOR);
  LOG_INFO(CAT_MAIN, "SYSTEM START");
  LOG_INFO(CAT_MAIN, "=== SYSTEM INFO ===");
  print_system_info();
  LOG_INFO(CAT_MAIN, "==========================================");
  LOG_INFO(CAT_MAIN, "Device starting with %s mode", DEVICE_PREFIX);

  // ========== ИНИЦИАЛИЗАЦИЯ ПОДСИСТЕМ ==========

  led_init();
  led_setMode(LED_MODE_MORZE_E);

  wdt_init();
  g_configManager.begin();

#if SCANING_WIFI_ENABLED == 1
  wdt_stop();
  const ConfigData* cfg = g_configManager.get();
  const char* targetSsid = (cfg != nullptr) ? cfg->wifiSsid : nullptr;
  wifi_scan_and_log(targetSsid);
  wdt_start();
#endif

  g_configManager.print();
  bool hasValidConfig = g_configManager.isValid();

  // ================================================================
  // ВЫБОР РЕЖИМА РАБОТЫ (через конечный автомат)
  // ================================================================

  g_provisioningCompleted = false;

  if (hasValidConfig && strlen(g_configManager.getWifiSsid()) > 0) {
    g_phase = SystemPhase::NORMAL;
    initNormalMode();
  } else {
    g_phase = SystemPhase::PROVISIONING;
    initProvisioningMode();
  }

  LOG_INFO(CAT_MAIN, "Setup complete, entering loop()");
}

// ============================================================================
// LOOP (ОРКЕСТРАТОР)
// ============================================================================

void loop() {
  wdt_feed();
  checkResetButton();

  // ================================================================
  // ОБРАБОТКА ФАЗЫ
  // ================================================================

  switch (g_phase) {
    case SystemPhase::PROVISIONING:
      handleProvisioning();
      break;

    case SystemPhase::NORMAL:
      handleNormal();
      break;

    case SystemPhase::ERROR:
      handleError();
      break;

    default:
      // INIT — ничего не делаем
      break;
  }
}