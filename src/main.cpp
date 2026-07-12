#include <Arduino.h>
#include "settings.h"

#include "config_manager.h"
#include "debug_tools.h"
#include "led.h"
#include "logger.h"
#include "ota.h"
#include "provisioning.h"
#include "reset_btn.h"
#include "wdt_manager.h"
#include "web.h"
#include "web_status_provider.h"
#include "wifi_manager.h"

#ifdef ESP32
#include <WiFi.h>
#include <esp_chip_info.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

static WiFiClient g_mqttClient;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
#include "sensor.h"
#endif

#if DEVICE_TYPE == 1
#include "fan_actuator.h"
#endif

#if DEVICE_TYPE == 3
#include "switch_actuator.h"
#endif

#if FEATURE_MQTT_ENABLED == 1
#include "mqtt.h"
#endif

#if FEATURE_ZIGBEE_ENABLED == 1
#include "zigbee.h"
#endif

// ============================================================================
// ГЛОБАЛЬНЫЕ
// ============================================================================
static unsigned long g_lastResetBtnCallback = 0;
bool g_normalMode = false;

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ============================================================================

void initNormalMode();
void processNormalMode();
void checkResetButton();
void processWebCommands();
int calculateAdaptiveSpeed(float temp,
                           float hum,
                           float baseTemp,
                           float baseHum,
                           float humRate);

// ============================================================================
// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
// ============================================================================

#if DEVICE_TYPE == 1
FanActuator fan;
#elif DEVICE_TYPE == 3
SwitchActuator switchActuator;
#endif

#if FEATURE_WEB_ENABLED == 1
#if DEVICE_TYPE == 1
#if FEATURE_MQTT_ENABLED == 1
static FanWebStatusProvider statusProvider(&fan, nullptr, &mqttManager);
#else
static FanWebStatusProvider statusProvider(&fan, nullptr, nullptr);
#endif
#elif DEVICE_TYPE == 3
#if FEATURE_MQTT_ENABLED == 1
static SwitchWebStatusProvider statusProvider(&switchActuator, &mqttManager);
#else
static SwitchWebStatusProvider statusProvider(&switchActuator, nullptr);
#endif
#endif
#endif

// ============================================================================
// ФУНКЦИИ-ОБРАБОТЧИКИ КОЛБЭКОВ
// ============================================================================

#if FEATURE_MQTT_ENABLED == 1

#if DEVICE_TYPE == 1

static void onMqttState(bool state, void* context) {
  FanActuator* fan = (FanActuator*)context;
  fan->set(state, true);

  // Обновляем конфиг при ручной команде
  if (state) {
    // Если включили вручную — отключаем сенсорный режим
    g_configManager.setSensorControlMode(false);
    g_configManager.setAdaptiveMode(false);
    fan->setAdaptiveMode(false);
  }
}

static void onMqttSpeed(int speed, void* context) {
  FanActuator* fan = (FanActuator*)context;

  // Если адаптивный режим был включён — отключаем его при ручной установке
  // скорости
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
    XLOG_WARN(CAT_MQTT, "Cannot enable sensor mode - sensor not available");
    mqttManager.publishSensorControlMode(false);
    return;
  }

  g_configManager.setSensorControlMode(enabled);
  if (!enabled) {
    g_configManager.setAdaptiveMode(false);
    fan->setAdaptiveMode(false);
  } else {
    // При включении сенсорного режима, если вентилятор выключен, включаем его
    // Это позволяет датчику управлять вентилятором с момента включения режима
    if (!fan->getState()) {
      // Не включаем принудительно, датчик сам решит при следующем обновлении
      XLOG_INFO(CAT_MQTT, "Sensor mode enabled, waiting for sensor data");
    }
  }
}

static void onMqttAdaptiveMode(bool enabled, void* context) {
  FanActuator* fan = (FanActuator*)context;

  if (enabled && !g_configManager.getSensorControlMode()) {
    XLOG_WARN(CAT_MQTT, "Cannot enable adaptive - sensor mode is OFF");
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
  // Обновляем актуатор
#if DEVICE_TYPE == 1
  fan.updateConfig(g_configManager.getAdaptiveMode(),
                   g_configManager.getDelaySeconds(),
                   g_configManager.getMaxOnTime());
#elif DEVICE_TYPE == 3
  switchActuator.updateConfig(g_configManager.getDelaySeconds(),
                              g_configManager.getMaxOnTime());
#endif
}

static void onMqttMaxOnTime(uint32_t seconds, void* /*context*/) {
  g_configManager.setMaxOnTime(seconds);
  // Обновляем актуатор
#if DEVICE_TYPE == 1
  fan.updateConfig(g_configManager.getAdaptiveMode(),
                   g_configManager.getDelaySeconds(),
                   g_configManager.getMaxOnTime());
#elif DEVICE_TYPE == 3
  switchActuator.updateConfig(g_configManager.getDelaySeconds(),
                              g_configManager.getMaxOnTime());
#endif
}

#elif DEVICE_TYPE == 3

static void onMqttState(bool state, void* context) {
  SwitchActuator* sw = (SwitchActuator*)context;
  sw->set(state, true);
}

static void onMqttDelaySec(int seconds, void* /*context*/) {
  g_configManager.setDelaySeconds(seconds);
  switchActuator.updateConfig(g_configManager.getDelaySeconds(),
                              g_configManager.getMaxOnTime());
}

static void onMqttMaxOnTime(uint32_t seconds, void* /*context*/) {
  g_configManager.setMaxOnTime(seconds);
  switchActuator.updateConfig(g_configManager.getDelaySeconds(),
                              g_configManager.getMaxOnTime());
}

#endif  // DEVICE_TYPE

#if MQTT_RESET_ENABLED == 1
static void onMqttReset(void* context) {
  XLOG_INFO(CAT_MQTT, "Reset via MQTT");
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
    XLOG_DEBUG(CAT_MQTT, "Initial config published");
  }

  if (millis() - lastHeartbeat >= STATE_PUBLISH_INTERVAL_MS) {
    mqttManager.publishOnline();

#if MQTT_PUBLISH_RSSI == 1
    mqttManager.publishRSSI(wifi_get_rssi());
#endif

    lastHeartbeat = millis();
  }
}

#endif  // FEATURE_MQTT_ENABLED == 1

// ============================================================================
// ОБРАБОТКА КОМАНД ОТ WEB
// ============================================================================

void processWebCommands() {
#if FEATURE_WEB_ENABLED == 1
  if (g_webConfigPending) {
    XLOG_INFO(CAT_MAIN, "Applying new config from Web...");

    auto& cfg = ConfigManager::getInstance();

    bool valid = true;

    if (strlen(g_webPendingConfig.wifiSsid) > 0) {
      if (!cfg.setWifiSsid(g_webPendingConfig.wifiSsid)) {
        XLOG_ERROR(CAT_MAIN, "Invalid WiFi SSID: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (strlen(g_webPendingConfig.wifiPassword) > 0) {
      if (!cfg.setWifiPassword(g_webPendingConfig.wifiPassword)) {
        XLOG_ERROR(CAT_MAIN, "Invalid WiFi password: %s", cfg.getLastError());
        valid = false;
      }
    }

#if FEATURE_MQTT_ENABLED == 1
    if (strlen(g_webPendingConfig.mqttBroker) > 0) {
      if (!cfg.setMqttBroker(g_webPendingConfig.mqttBroker)) {
        XLOG_ERROR(CAT_MAIN, "Invalid MQTT broker: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (!cfg.setMqttPort(g_webPendingConfig.mqttPort)) {
      XLOG_ERROR(CAT_MAIN, "Invalid MQTT port: %s", cfg.getLastError());
      valid = false;
    }

    if (strlen(g_webPendingConfig.mqttUser) > 0) {
      if (!cfg.setMqttUser(g_webPendingConfig.mqttUser)) {
        XLOG_ERROR(CAT_MAIN, "Invalid MQTT user: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (strlen(g_webPendingConfig.mqttPassword) > 0) {
      if (!cfg.setMqttPassword(g_webPendingConfig.mqttPassword)) {
        XLOG_ERROR(CAT_MAIN, "Invalid MQTT password: %s", cfg.getLastError());
        valid = false;
      }
    }

    if (strlen(g_webPendingConfig.mqttClientId) > 0) {
      if (!cfg.setMqttClientId(g_webPendingConfig.mqttClientId)) {
        XLOG_ERROR(CAT_MAIN, "Invalid MQTT Client ID: %s", cfg.getLastError());
        valid = false;
      }
    }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    if (!cfg.setSensorInterval(g_webPendingConfig.sensorInterval)) {
      XLOG_ERROR(CAT_MAIN, "Invalid sensor interval: %s", cfg.getLastError());
      valid = false;
    }
#endif

#if DEVICE_TYPE == 1
    if (!cfg.setLowTemp(g_webPendingConfig.lowTemp)) {
      XLOG_ERROR(CAT_MAIN, "Invalid low temp: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setHighTemp(g_webPendingConfig.highTemp)) {
      XLOG_ERROR(CAT_MAIN, "Invalid high temp: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setLowHum(g_webPendingConfig.lowHum)) {
      XLOG_ERROR(CAT_MAIN, "Invalid low hum: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setHighHum(g_webPendingConfig.highHum)) {
      XLOG_ERROR(CAT_MAIN, "Invalid high hum: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setSpeedPercent(g_webPendingConfig.speedPercent)) {
      XLOG_ERROR(CAT_MAIN, "Invalid speed: %s", cfg.getLastError());
      valid = false;
    }
    cfg.setAdaptiveMode(g_webPendingConfig.adaptiveMode);
    cfg.setSensorControlMode(g_webPendingConfig.sensorControlMode);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    if (!cfg.setDelaySeconds(g_webPendingConfig.delaySeconds)) {
      XLOG_ERROR(CAT_MAIN, "Invalid delay seconds: %s", cfg.getLastError());
      valid = false;
    }
    if (!cfg.setMaxOnTime(g_webPendingConfig.maxOnTime)) {
      XLOG_ERROR(CAT_MAIN, "Invalid max on time: %s", cfg.getLastError());
      valid = false;
    }
    cfg.setBootState(g_webPendingConfig.bootState);
#endif

    if (!valid) {
      XLOG_ERROR(CAT_MAIN, "Invalid config from Web, rejecting");
      g_webConfigPending = false;
      return;
    }

    if (cfg.save()) {
      XLOG_INFO(CAT_MAIN, "Config saved successfully!");
      g_webConfigPending = false;
      g_webRestartPending = true;
    } else {
      XLOG_ERROR(CAT_MAIN, "Failed to save config: %s", cfg.getLastError());
      g_webConfigPending = false;
    }
  }

  if (g_webRestartPending) {
    XLOG_INFO(CAT_MAIN, "Restarting due to Web command...");
    g_webRestartPending = false;
    delay(500);
    ESP.restart();
  }
#endif
}

#if DEVICE_TYPE == 1
// ============================================================================
// БИЗНЕС-ЛОГИКА: АДАПТИВНАЯ СКОРОСТЬ
// ============================================================================

int calculateAdaptiveSpeed(float temp,
                           float hum,
                           float baseTemp,
                           float baseHum,
                           float humRate) {
  float deltaTemp = temp - baseTemp;
  float deltaHum = hum - baseHum;

  int step = ADAPTIVE_STEP_SIZE;

  if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 2 ||
      deltaHum > ADAPTIVE_EPSILON_HUM * 2)
    step *= 2;
  if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 3 ||
      deltaHum > ADAPTIVE_EPSILON_HUM * 3)
    step *= 3;

  float mult = 1.0 + (humRate / ADAPTIVE_SPEED_SENSITIVITY);
  mult = constrain(mult, 0.5, 3.0);
  step = step * mult;

  return constrain(step, 5, 60);
}
#endif

// ============================================================================
// NORMAL MODE ФУНКЦИИ
// ============================================================================

void initNormalMode() {
  XLOG_INFO(CAT_MAIN, "========================================");
  XLOG_INFO(CAT_MAIN, "NORMAL MODE");
  XLOG_INFO(CAT_MAIN, "========================================");

  if (apMode) {
    wifi_stop_ap();
    XLOG_INFO(CAT_WIFI, "AP mode disabled");
  }

#if FEATURE_ZIGBEE_ENABLED == 1
  XLOG_INFO(CAT_MAIN, "Initializing ZigBee...");
  zigbeeManager.begin(g_configManager.getDeviceId());
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_init();
#if DEVICE_TYPE == 1
  if (!sensor_isOk()) {
    g_configManager.setSensorControlMode(false);
    g_configManager.setAdaptiveMode(false);
    fan.setAdaptiveMode(false);
    XLOG_WARN(CAT_SENSOR, "Sensor not found - switching to MANUAL mode");
  }
#endif
#endif

#if DEVICE_TYPE == 1
  fan.init(SWITCH_PIN, RELAY_ON_LEVEL, g_configManager.getBootState(),
           g_configManager.getSpeedPercent(), g_configManager.getAdaptiveMode(),
           g_configManager.getDelaySeconds(), g_configManager.getMaxOnTime());
#endif

#if DEVICE_TYPE == 3
  switchActuator.init(
      SWITCH_PIN, RELAY_ON_LEVEL, g_configManager.getBootState(),
      g_configManager.getDelaySeconds(), g_configManager.getMaxOnTime());
#endif

#if FEATURE_MQTT_ENABLED == 1
  mqttManager.begin(
      g_mqttClient, g_configManager.getMqttBroker(),
      g_configManager.getMqttPort(), g_configManager.getMqttClientId(),
      g_configManager.getMqttUser(), g_configManager.getMqttPassword());
  registerMqttCallbacks();
#endif

#if SCANNING_WIFI_ENABLED == 1
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

#if FEATURE_WEB_ENABLED == 1
#if DEVICE_TYPE == 1
  web_registerStatusProvider(&statusProvider);
#elif DEVICE_TYPE == 3
  web_registerStatusProvider(&statusProvider);
#endif

#if FEATURE_OTA_ENABLED == 1
  if (ota_is_available()) {
    ota_init(&server);
    XLOG_INFO(CAT_MAIN, "OTA initialized");
  } else {
    XLOG_WARN(CAT_MAIN, "OTA not available");
  }
#endif

  web_init();
#endif

  XLOG_INFO(CAT_MAIN, "System initialized successfully!");
}

void processNormalMode() {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  bool sensorDataChanged = sensor_update();
#endif

  // ============================================================
  // БИЗНЕС-ЛОГИКА: УПРАВЛЕНИЕ ВЕНТИЛЯТОРОМ (TYPE 1)
  // ============================================================
#if DEVICE_TYPE == 1

  // --- Обновляем конфигурацию актуатора ---
  fan.updateConfig(g_configManager.getAdaptiveMode(),
                   g_configManager.getDelaySeconds(),
                   g_configManager.getMaxOnTime());

  // --- Проверка аварийной остановки ---
  if (fan.isEmergencyStop()) {
    XLOG_WARN(CAT_MAIN,
              "Emergency stop detected - disabling sensor and adaptive modes");
    g_configManager.setSensorControlMode(false);
    g_configManager.setAdaptiveMode(false);
    fan.setAdaptiveMode(false);
#if FEATURE_MQTT_ENABLED == 1
    // Публикуем обновлённое состояние в MQTT
    mqttManager.publishSensorControlMode(false);
    mqttManager.publishAdaptiveMode(false);
#endif

  }

  // --- Сенсорный режим ---
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
      XLOG_INFO(CAT_SENSOR, "Auto ON: T=%.1f°C H=%.1f%%", temp, hum);
    } else if (shouldBeOff && fan.getState()) {
      fan.set(false, false);
      XLOG_INFO(CAT_SENSOR, "Auto OFF: T=%.1f°C H=%.1f%%", temp, hum);
    }

    // --- Адаптивный режим (работает ТОЛЬКО когда вентилятор включён) ---
    if (fan.getAdaptiveMode() && fan.getState() &&
        g_configManager.getSensorControlMode()) {
      static float baseTemp = 0;
      static float baseHum = 0;
      static bool baseInitialized = false;
      static unsigned long lastAdaptiveCheck = 0;

      if (!baseInitialized) {
        baseTemp = temp;
        baseHum = hum;
        baseInitialized = true;
        lastAdaptiveCheck = millis();
      }

      if (millis() - lastAdaptiveCheck >=
          g_configManager.getSensorInterval() * 1000UL) {
        lastAdaptiveCheck = millis();

        float humRate = sensor_getHumRate();
        int newSpeed =
            calculateAdaptiveSpeed(temp, hum, baseTemp, baseHum, humRate);
        int currentSpeed = fan.getSpeed();

        // Проверяем, нужно ли увеличивать скорость
        float deltaTemp = temp - baseTemp;
        float deltaHum = hum - baseHum;

        if (deltaTemp > ADAPTIVE_EPSILON_TEMP ||
            deltaHum > ADAPTIVE_EPSILON_HUM) {
          int targetSpeed = currentSpeed + newSpeed;
          if (targetSpeed > 100)
            targetSpeed = 100;
          if (targetSpeed != currentSpeed) {
            fan.setSpeed(targetSpeed, false);
            XLOG_DEBUG(CAT_SENSOR, "Adaptive speed: %d%% (T=%.1f°C H=%.1f%%)",
                       targetSpeed, temp, hum);
            // Обновляем базовые значения для следующего цикла
            baseTemp = temp;
            baseHum = hum;
          }
        } else {
          // Если условия нормализовались, плавно снижаем скорость до базовой
          int baseSpeed = g_configManager.getSpeedPercent();
          if (currentSpeed > baseSpeed) {
            int targetSpeed = currentSpeed - ADAPTIVE_STEP_SIZE;
            if (targetSpeed < baseSpeed)
              targetSpeed = baseSpeed;
            fan.setSpeed(targetSpeed, false);
            XLOG_DEBUG(CAT_SENSOR, "Adaptive speed down: %d%% -> %d%%",
                       currentSpeed, targetSpeed);
            baseTemp = temp;
            baseHum = hum;
          }
        }
      }
    } else {
      // Если адаптивный режим выключен или вентилятор выключен — сбрасываем
      // базовые значения
      static bool wasAdaptive = false;
      if (wasAdaptive) {
        wasAdaptive = false;
      }
      if (!fan.getAdaptiveMode() || !fan.getState()) {
        // Сбрасываем базовые значения при следующем включении
        static bool baseInitialized = false;
        baseInitialized = false;
      }
    }
  }

  // --- Вызов update() актуатора (стартовый импульс, таймеры) ---
  fan.update();

#elif DEVICE_TYPE == 3

  // --- Обновляем конфигурацию актуатора ---
  switchActuator.updateConfig(g_configManager.getDelaySeconds(),
                              g_configManager.getMaxOnTime());

  // --- Проверка аварийной остановки ---
  if (switchActuator.isEmergencyStop()) {
    XLOG_WARN(CAT_MAIN, "Emergency stop detected on switch");
  }

  // --- Вызов update() актуатора ---
  switchActuator.update(g_configManager.getDelaySeconds(),
                        g_configManager.getMaxOnTime());

#endif

  // ============================================================
  // ОБЩАЯ ЧАСТЬ (WiFi, MQTT, WEB, LED)
  // ============================================================

  wifi_monitor();

#if FEATURE_OTA_ENABLED == 1
  ota_loop();
#endif

#if FEATURE_MQTT_ENABLED == 1
  if (wifi_is_connected()) {
    mqttManager.process();
    publishMqttStatus();
  }
#endif

#if FEATURE_ZIGBEE_ENABLED == 1
  zigbeeManager.process();
#endif

  web_update();

  if (apMode) {
    led_setMode(LED_MODE_MORZE_S);
  } else if (!wifi_is_connected()) {
    led_setMode(LED_MODE_MORZE_E);
#if FEATURE_MQTT_ENABLED == 1
  } else if (!mqttManager.isConnected()) {
    led_setMode(LED_MODE_MORZE_I);
#endif
  } else {
    led_setMode(LED_MODE_ON);
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
#if XLOG_LEVEL > 0
  delay(2000);
#endif

  Logger::getInstance().begin((LogLevel)XLOG_LEVEL, XLOG_CATEGORIES,
                              XLOG_USE_COLOR);
  XLOG_INFO(CAT_MAIN, "SYSTEM START");
  XLOG_DEBUG(CAT_MAIN, "=== SYSTEM INFO ===");
  print_system_info();
  XLOG_DEBUG(CAT_MAIN, "==========================================");
  XLOG_DEBUG(CAT_MAIN, "Device: %s (TYPE %d)", DEVICE_PREFIX, DEVICE_TYPE);
  XLOG_DEBUG(CAT_MAIN, "BLE Prov: %s",
             USE_BLE_PROVISIONING ? "ENABLED" : "NONE");
  XLOG_DEBUG(CAT_MAIN, "AP Prov: %s", USE_AP_PROVISIONING ? "ENABLED" : "NONE");
#if TRANSPORT_TYPE == 0
  const char* transportType = "MQTT";
#elif TRANSPORT_TYPE == 1
  const char* transportType = "ZIGBEE";
#elif TRANSPORT_TYPE == 2
  const char* transportType = "MATTER";
#else
  const char* transportType = "UNKNOWN";
#endif
  XLOG_DEBUG(CAT_MAIN, "Transport: %s", transportType);
  led_init();
  led_setMode(LED_MODE_MORZE_E);
  resetBtn_init();
  wdt_init();
  g_configManager.begin();

#if SCANNING_WIFI_ENABLED == 1
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

  if (hasValidConfig && hasWifi && hasMqtt) {
    XLOG_INFO(CAT_MAIN, "Config found and corrected. Entering NORMAL mode.");
    g_normalMode = true;
    initNormalMode();
  } else {
#if USE_BLE_PROVISIONING == 1 || USE_AP_PROVISIONING == 1
    g_normalMode = false;
    XLOG_INFO(CAT_MAIN, "Starting provisioning mode");
    led_setMode(LED_MODE_MORZE_S);
    startProvisioning();
#else
#if FEATURE_ZIGBEE_ENABLED == 1
    XLOG_INFO(CAT_MAIN, "Using ZigBee mode (no provisioning needed)");
    g_normalMode = true;
#else
    XLOG_ERROR(CAT_MAIN, "No valid config and no provisioning method!");
    led_setMode(LED_MODE_OFF);
    g_normalMode = false;
#endif
#endif
  }

  XLOG_DEBUG(CAT_MAIN, "Setup complete");
}

// ============================================================================
// LOOP (ОРКЕСТРАТОР)
// ============================================================================

void loop() {
  wdt_feed();

  processWebCommands();

  if (!g_normalMode) {
    ProvisioningManager::getInstance().update();

    if (isProvisioningComplete()) {
      auto& prov = ProvisioningManager::getInstance();
      auto method = prov.getCompletedBy();

      if (method == ProvisioningMethod::FAILED) {
        XLOG_WARN(CAT_MAIN, "Provisioning FAILED! Rebooting ...");
        wdt_stop();
        ESP.restart();
        return;
      }

      XLOG_INFO(CAT_MAIN, "Provisioning completed via %s",
                method == ProvisioningMethod::BLE ? "BLE" : "AP");

      const auto* data = prov.getData();
      if (data && strlen(data->wifiSsid) > 0) {
        XLOG_INFO(CAT_MAIN, "Saving config: SSID='%s'", data->wifiSsid);

        auto& cfg = ConfigManager::getInstance();
        cfg.setWifiSsid(data->wifiSsid);
        cfg.setWifiPassword(data->wifiPassword);

        if (cfg.save()) {
          XLOG_INFO(CAT_MAIN, "Config saved successfully! Restarting...");
          ESP.restart();
        } else {
          XLOG_ERROR(CAT_MAIN, "Failed to save config!");
          return;
        }
      }

      if (apMode) {
        wifi_stop_ap();
        XLOG_INFO(CAT_WIFI, "AP mode disabled");
      }

      g_normalMode = true;
      initNormalMode();

      XLOG_INFO(CAT_MAIN,
                "System running in NORMAL mode with new configuration");
    }
  } else {
    processNormalMode();
  }

  led_update();
  ResetButtonStage btnState = resetBtn_getState();

  switch (btnState) {
    case PRESSED:
      led_setMode(LED_MODE_MORZE_E);
      break;
    case STAGE_1S:
      led_setMode(LED_MODE_MORZE_I);
      break;
    case STAGE_2S:
      led_setMode(LED_MODE_MORZE_S);
      break;
    case STAGE_3S:
      XLOG_INFO(CAT_MAIN, "!!! RESET TRIGGERED !!!");
      wdt_stop();
      if (g_configManager.reset()) {
        ESP.restart();
      }
      break;
    case RELEASED:
      break;
  }
}