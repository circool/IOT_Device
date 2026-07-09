#include <Arduino.h>
#include "config_manager.h"
#include "debug_tools.h"
#include "led.h"
#include "logger.h"
#include "ota.h"
#include "settings.h"
#include "wdt_manager.h"
#include "wifi_manager.h"

#ifdef ESP32
#include <WiFi.h>
#include <esp_chip_info.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// ============================================================================
// PROVISIONING
// ============================================================================

#include "provisioning.h"

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
// ГЛОБАЛЬНЫЙ ФЛАГ РЕЖИМА
// ============================================================================

bool g_normalMode = false;

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ============================================================================

void initNormalMode();
void processNormalMode();
void checkResetButton();
void processWebCommands();

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
#if MQTT_ENABLED == 1
static FanWebStatusProvider statusProvider(&fan, nullptr, &mqttManager);
#else
static FanWebStatusProvider statusProvider(&fan, nullptr, nullptr);
#endif
#elif DEVICE_TYPE == 3
#if MQTT_ENABLED == 1
static SwitchWebStatusProvider statusProvider(&switchActuator, &mqttManager);
#else
static SwitchWebStatusProvider statusProvider(&switchActuator, nullptr);
#endif
#endif
#endif

// ============================================================================
// СТАТИЧЕСКИЕ ФУНКЦИИ-ОБРАБОТЧИКИ ДЛЯ MQTT КОЛБЭКОВ
// ============================================================================

#if MQTT_ENABLED == 1

#if DEVICE_TYPE == 1

static void onMqttState(bool state, void* context) {
  FanActuator* fan = (FanActuator*)context;
  fan->set(state, true);
}

static void onMqttSpeed(int speed, void* context) {
  FanActuator* fan = (FanActuator*)context;

  if (g_configManager.getAdaptiveMode()) {
    g_configManager.setAdaptiveMode(false);
    fan->setAdaptiveMode(false);
  }

  if (speed == 0 || speed < MIN_SPEED_PERCENT) {
    fan->set(false, true);
  } else {
    fan->setSpeed(speed, true);
    if (fan->getState()) {
      fan->setSpeed(speed, false);
    }
  }
}

static void onMqttSensorControlMode(bool enabled, void* context) {
  FanActuator* fan = (FanActuator*)context;

  if (enabled && !sensor_isOk()) {
    LOG_WARN(CAT_MQTT, "Cannot enable sensor mode - sensor not available");
    mqttManager.publishSensorControlMode(false);
    return;
  }

  g_configManager.setSensorControlMode(enabled);
  if (!enabled) {
    fan->setAdaptiveMode(false);
  }
}

static void onMqttAdaptiveMode(bool enabled, void* context) {
  FanActuator* fan = (FanActuator*)context;

  if (enabled && !g_configManager.getSensorControlMode()) {
    LOG_WARN(CAT_MQTT, "Cannot enable adaptive - sensor mode is OFF");
    return;
  }

  g_configManager.setAdaptiveMode(enabled);
  fan->setAdaptiveMode(enabled);
}

static void onMqttLowTemp(float value, void* /*context*/) {
  g_configManager.setLowTemp(value);
}

static void onMqttHighTemp(float value, void* /*context*/) {
  g_configManager.setHighTemp(value);
}

static void onMqttLowHum(float value, void* /*context*/) {
  g_configManager.setLowHum(value);
}

static void onMqttHighHum(float value, void* /*context*/) {
  g_configManager.setHighHum(value);
}

static void onMqttDelaySec(int seconds, void* /*context*/) {
  g_configManager.setDelaySeconds(seconds);
}

static void onMqttMaxOnTime(uint32_t seconds, void* /*context*/) {
  g_configManager.setMaxOnTime(seconds);
}

#elif DEVICE_TYPE == 3

static void onMqttState(bool state, void* context) {
  SwitchActuator* sw = (SwitchActuator*)context;
  sw->set(state, true);
}

static void onMqttDelaySec(int seconds, void* /*context*/) {
  g_configManager.setDelaySeconds(seconds);
}

static void onMqttMaxOnTime(uint32_t seconds, void* /*context*/) {
  g_configManager.setMaxOnTime(seconds);
}

#endif  // DEVICE_TYPE

#if MQTT_RESET_ENABLED == 1
static void onMqttReset(void* context) {
  LOG_INFO(CAT_MQTT, "Reset via MQTT");
  mqttManager.disconnect();
  g_configManager.reset();
  delay(1000);
  ESP.restart();
}
#endif

// ============================================================================
// РЕГИСТРАЦИЯ КОЛБЭКОВ
// ============================================================================

static void registerMqttCallbacks() {
#if DEVICE_TYPE == 1
  mqttManager.onState(onMqttState, &fan);
  mqttManager.onSpeed(onMqttSpeed, &fan);
  mqttManager.onSensorControlMode(onMqttSensorControlMode, &fan);
  mqttManager.onAdaptiveMode(onMqttAdaptiveMode, &fan);
  mqttManager.onLowTemp(onMqttLowTemp, nullptr);
  mqttManager.onHighTemp(onMqttHighTemp, nullptr);
  mqttManager.onLowHum(onMqttLowHum, nullptr);
  mqttManager.onHighHum(onMqttHighHum, nullptr);
  mqttManager.onDelaySec(onMqttDelaySec, nullptr);
  mqttManager.onMaxOnTime(onMqttMaxOnTime, nullptr);

#if MQTT_RESET_ENABLED == 1
  mqttManager.onReset(onMqttReset, nullptr);
#endif

#elif DEVICE_TYPE == 3
  mqttManager.onState(onMqttState, &switchActuator);
  mqttManager.onDelaySec(onMqttDelaySec, nullptr);
  mqttManager.onMaxOnTime(onMqttMaxOnTime, nullptr);

#if MQTT_RESET_ENABLED == 1
  mqttManager.onReset(onMqttReset, nullptr);
#endif
#endif
}

// ============================================================================
// ПУБЛИКАЦИЯ СТАТУСА В MQTT
// ============================================================================

static void publishMqttStatus() {
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

  static bool initialConfigPublished = false;
  static unsigned long lastHeartbeat = 0;

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

    mqttManager.publishResetReason(getResetReason());

#if MQTT_PUBLISH_VERSION == 1
    mqttManager.publishVersion(VERSION);
#endif

    initialConfigPublished = true;
    LOG_DEBUG(CAT_MQTT, "Initial config published");
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
// ОБРАБОТКА КОМАНД ОТ WEB
// ============================================================================

void processWebCommands() {
#if WEB_ENABLED == 1
  // ==== 1. НОВЫЕ НАСТРОЙКИ ====
  if (g_webConfigPending) {
    LOG_INFO(CAT_MAIN, "Applying new config from Web...");

    auto& cfg = ConfigManager::getInstance();

    bool valid = true;

    if (strlen(g_webPendingConfig.wifiSsid) > 0) {
      if (!cfg.setWifiSsid(g_webPendingConfig.wifiSsid)) {
        LOG_ERROR(CAT_MAIN, "Invalid WiFi SSID: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (strlen(g_webPendingConfig.wifiPassword) > 0) {
      if (!cfg.setWifiPassword(g_webPendingConfig.wifiPassword)) {
        LOG_ERROR(CAT_MAIN, "Invalid WiFi password: %s", cfg.getLastError());
        valid = false;
      }
    }

#if MQTT_ENABLED == 1
    if (strlen(g_webPendingConfig.mqttBroker) > 0) {
      if (!cfg.setMqttBroker(g_webPendingConfig.mqttBroker)) {
        LOG_ERROR(CAT_MAIN, "Invalid MQTT broker: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (!cfg.setMqttPort(g_webPendingConfig.mqttPort)) {
      LOG_ERROR(CAT_MAIN, "Invalid MQTT port: %s", cfg.getLastError());
      valid = false;
    }

    if (strlen(g_webPendingConfig.mqttUser) > 0) {
      if (!cfg.setMqttUser(g_webPendingConfig.mqttUser)) {
        LOG_ERROR(CAT_MAIN, "Invalid MQTT user: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (strlen(g_webPendingConfig.mqttPassword) > 0) {
      if (!cfg.setMqttPassword(g_webPendingConfig.mqttPassword)) {
        LOG_ERROR(CAT_MAIN, "Invalid MQTT password: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (strlen(g_webPendingConfig.mqttClientId) > 0) {
      if (!cfg.setMqttClientId(g_webPendingConfig.mqttClientId)) {
        LOG_ERROR(CAT_MAIN, "Invalid MQTT Client ID: %s", cfg.getLastError());
        valid = false;
      }
    }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    if (!cfg.setSensorInterval(g_webPendingConfig.sensorInterval)) {
      LOG_ERROR(CAT_MAIN, "Invalid sensor interval: %s", cfg.getLastError());
      valid = false;
    }
#endif

#if DEVICE_TYPE == 1
    if (!cfg.setLowTemp(g_webPendingConfig.lowTemp)) {
      LOG_ERROR(CAT_MAIN, "Invalid low temp: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setHighTemp(g_webPendingConfig.highTemp)) {
      LOG_ERROR(CAT_MAIN, "Invalid high temp: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setLowHum(g_webPendingConfig.lowHum)) {
      LOG_ERROR(CAT_MAIN, "Invalid low hum: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setHighHum(g_webPendingConfig.highHum)) {
      LOG_ERROR(CAT_MAIN, "Invalid high hum: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setSpeedPercent(g_webPendingConfig.speedPercent)) {
      LOG_ERROR(CAT_MAIN, "Invalid speed: %s", cfg.getLastError());
      valid = false;
    }
    cfg.setAdaptiveMode(g_webPendingConfig.adaptiveMode);
    cfg.setSensorControlMode(g_webPendingConfig.sensorControlMode);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    if (!cfg.setDelaySeconds(g_webPendingConfig.delaySeconds)) {
      LOG_ERROR(CAT_MAIN, "Invalid delay seconds: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setMaxOnTime(g_webPendingConfig.maxOnTime)) {
      LOG_ERROR(CAT_MAIN, "Invalid max on time: %s", cfg.getLastError());
      valid = false;
    }
    cfg.setBootState(g_webPendingConfig.bootState);
#endif

    if (!valid) {
      LOG_ERROR(CAT_MAIN, "Invalid config from Web, rejecting");
      g_webConfigPending = false;
      return;
    }

    // Сохраняем в EEPROM
    if (cfg.save()) {
      LOG_INFO(CAT_MAIN, "Config saved successfully!");
      g_webConfigPending = false;

      // Перезагружаемся после применения
      g_webRestartPending = true;
    } else {
      LOG_ERROR(CAT_MAIN, "Failed to save config: %s", cfg.getLastError());
      g_webConfigPending = false;
    }
  }

  // ==== 2. ПЕРЕЗАГРУЗКА ====
  if (g_webRestartPending) {
    LOG_INFO(CAT_MAIN, "Restarting due to Web command...");
    g_webRestartPending = false;
    delay(500);
    ESP.restart();
  }
#endif
}

// ============================================================================
// NORMAL MODE ФУНКЦИИ
// ============================================================================

void initNormalMode() {
  LOG_INFO(CAT_MAIN, "========================================");
  LOG_INFO(CAT_MAIN, "NORMAL MODE");
  LOG_INFO(CAT_MAIN, "========================================");

  if (apMode) {
    wifi_stop_ap();
    LOG_INFO(CAT_WIFI, "AP mode disabled");
  }

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_init();
#if DEVICE_TYPE == 1
  if (!sensor_isOk()) {
    g_configManager.setSensorControlMode(false);
    g_configManager.setAdaptiveMode(false);
    fan.setAdaptiveMode(false);
    LOG_WARN(CAT_SENSOR, "Sensor not found - switching to MANUAL mode");
  }
#endif
#endif

#if DEVICE_TYPE == 1
  fan.init(SWITCH_PIN, RELAY_ON_LEVEL, g_configManager.getBootState(),
           g_configManager.getSpeedPercent());
  fan.setAdaptiveMode(g_configManager.getAdaptiveMode());
#endif

#if DEVICE_TYPE == 3
  switchActuator.init(SWITCH_PIN, RELAY_ON_LEVEL,
                      g_configManager.getBootState());
#endif

#if MQTT_ENABLED == 1
  mqttManager.begin(
      g_mqttClient, g_configManager.getMqttBroker(),
      g_configManager.getMqttPort(), g_configManager.getMqttClientId(),
      g_configManager.getMqttUser(), g_configManager.getMqttPassword());
  registerMqttCallbacks();
#endif

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

#if WEB_ENABLED == 1
#if DEVICE_TYPE == 1
  web_registerStatusProvider(&statusProvider);
#elif DEVICE_TYPE == 3
  web_registerStatusProvider(&statusProvider);
#endif

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

void processNormalMode() {
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

#if DEVICE_TYPE == 1
  fan.update();
#endif

#if DEVICE_TYPE == 3
  switchActuator.update();
#endif

  wifi_monitor();

#if OTA_ENABLED == 1
  ota_loop();
#endif

#if MQTT_ENABLED == 1
  if (wifi_is_connected()) {
    mqttManager.process();
    publishMqttStatus();
  }
#endif

  web_update();

  if (apMode) {
    led_setMode(LED_MODE_MORZE_S);
  } else if (!wifi_is_connected()) {
    led_setMode(LED_MODE_MORZE_E);
#if MQTT_ENABLED == 1
  } else if (!mqttManager.isConnected()) {
    led_setMode(LED_MODE_MORZE_I);
#endif
  } else {
    led_setMode(LED_MODE_ON);
  }
}

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
// SETUP
// ============================================================================

void setup() {
  delay(2000);
  Logger::getInstance().begin((LogLevel)LOG_LEVEL, LOG_CATEGORIES,
                              LOG_USE_COLOR);
  LOG_INFO(CAT_MAIN, "SYSTEM START");
  LOG_INFO(CAT_MAIN, "=== SYSTEM INFO ===");
  print_system_info();
  LOG_INFO(CAT_MAIN, "==========================================");
  LOG_INFO(CAT_MAIN, "Device: %s (TYPE %d)", DEVICE_PREFIX, DEVICE_TYPE);

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
  bool hasWifi = (strlen(g_configManager.getWifiSsid()) > 0);

#if IS_MQTT_ENABLED
  bool hasMqtt = (strlen(g_configManager.getMqttBroker()) > 0);
#else
  bool hasMqtt = true;
#endif

#if USE_BLE_PROVISIONING == 1
  if (hasValidConfig && hasWifi) {
    LOG_INFO(CAT_MAIN, "WiFi configured. Entering NORMAL mode.");
    g_normalMode = true;
    initNormalMode();
  } else {
    LOG_WARN(CAT_MAIN, "No WiFi config.");
    g_normalMode = false;
    led_setMode(LED_MODE_MORZE_S);
    startProvisioning();
  }

#elif USE_AP_PROVISIONING == 1
  if (hasValidConfig && hasWifi && hasMqtt) {
    LOG_INFO(CAT_MAIN, "Full config found. Entering NORMAL mode.");
    g_normalMode = true;
    initNormalMode();
  } else {
    LOG_INFO(CAT_MAIN, "Incomplete config. Starting AP provisioning.");
    g_normalMode = false;
    startProvisioning();
  }

#else
  if (hasValidConfig && hasWifi) {
    g_normalMode = true;
    initNormalMode();
  } else {
    g_normalMode = false;
    startProvisioning();
  }
#endif

  LOG_DEBUG(CAT_MAIN, "Setup complete");
}

// ============================================================================
// LOOP (ОРКЕСТРАТОР)
// ============================================================================

void loop() {
  wdt_feed();
  checkResetButton();

  // ==== ОБРАБОТКА КОМАНД ОТ WEB ====
  processWebCommands();

  if (!g_normalMode) {
    ProvisioningManager::getInstance().update();

    if (isProvisioningComplete()) {
      auto& prov = ProvisioningManager::getInstance();
      auto method = prov.getCompletedBy();

      if (method == ProvisioningMethod::FAILED) {
        LOG_WARN(CAT_MAIN, "Provisioning FAILED! Rebooting ...");
        wdt_stop();
        ESP.restart();
        return;
      }

      LOG_INFO(CAT_MAIN, "Provisioning completed via %s",
               method == ProvisioningMethod::BLE ? "BLE" : "AP");

      const auto* data = prov.getData();
      if (data && strlen(data->wifiSsid) > 0) {
        LOG_INFO(CAT_MAIN, "Saving config: SSID='%s'", data->wifiSsid);

        auto& cfg = ConfigManager::getInstance();
        cfg.setWifiSsid(data->wifiSsid);
        cfg.setWifiPassword(data->wifiPassword);

        if (cfg.save()) {
          LOG_INFO(CAT_MAIN, "Config saved successfully!");
        } else {
          LOG_ERROR(CAT_MAIN, "Failed to save config!");
          return;
        }
      }

      if (apMode) {
        wifi_stop_ap();
        LOG_INFO(CAT_WIFI, "AP mode disabled");
      }

      g_normalMode = true;
      initNormalMode();

      LOG_INFO(CAT_MAIN,"System running in NORMAL mode with new configuration");
    }
  } else {
    processNormalMode();
  }

  led_update();
}