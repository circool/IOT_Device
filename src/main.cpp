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
// ГЛОБАЛЬНЫЙ ФЛАГ РЕЖИМА
// ============================================================================

/**
 * @brief Флаг режима работы устройства
 * @note true — нормальная работа, false — режим настройки (провизионинг)
 */
bool g_normalMode = false;

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ (из normal.cpp)
// ============================================================================

void initNormalMode();     ///< Инициализация нормального режима
void processNormalMode();  ///< Цикл нормального режима

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ (локальные)
// ============================================================================

void checkResetButton();  ///< Обработка кнопки сброса

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

    mqttManager.publishResetReason(getResetReason());

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
// ИНИЦИАЛИЗАЦИЯ NORMAL MODE (перенесена из main)
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
// ЦИКЛ NORMAL MODE (перенесена из main)
// ============================================================================

void processNormalMode() {
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
  // Устанавливаем режим LED в зависимости от состояния
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

void processProvisioning() {
  auto& prov = ProvisioningManager::getInstance();
  prov.update();

  // ===== AP РЕЖИМ — ЖДЁМ СОХРАНЕНИЯ КОНФИГА =====
  if (prov.getMode() == ProvisioningMode::AP) {
    web_update();

    // Проверяем, не сохранил ли пользователь конфиг через веб
    if (g_configManager.isValid()) {
      LOG_INFO(CAT_MAIN, "AP provisioning completed (config saved)");
      g_normalMode = true;
      initNormalMode();
      return;
    }
  }

  // ===== BLE РЕЖИМ — ПРОВЕРЯЕМ ЗАВЕРШЕНИЕ =====
  if (isProvisioningComplete()) {
    LOG_INFO(CAT_MAIN, "Provisioning done, switching to NORMAL mode");
    g_normalMode = true;
    initNormalMode();
    return;
  }

  // ===== LED =====
  led_setMode(LED_MODE_MORZE_S);
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

  // ================================================================
  // ПРОВЕРКА КОНФИГУРАЦИИ
  // ================================================================

  bool hasValidConfig = g_configManager.isValid();
  bool hasWifi = (strlen(g_configManager.getWifiSsid()) > 0);

#if IS_MQTT_ENABLED
  bool hasMqtt = (strlen(g_configManager.getMqttBroker()) > 0);
#else
  bool hasMqtt = true;
#endif

  // ================================================================
  // ВЫБОР РЕЖИМА РАБОТЫ
  // ================================================================

#if USE_BLE_PROVISIONING == 1
  // BLE — только WiFi
  if (hasValidConfig && hasWifi) {
    LOG_INFO(CAT_MAIN, "WiFi configured. Entering NORMAL mode.");
    LOG_INFO(CAT_MAIN, "Complete setup at device IP.");
    g_normalMode = true;
    initNormalMode();
  } else {
    LOG_INFO(CAT_MAIN, "No WiFi config. Starting BLE provisioning.");
    g_normalMode = false;
    startProvisioning();
  }

#elif USE_AP_PROVISIONING == 1
  // AP + Web — полная настройка
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
  // Fallback: AP
  if (hasValidConfig && hasWifi) {
    g_normalMode = true;
    initNormalMode();
  } else {
    g_normalMode = false;
    startProvisioning();
  }
#endif

  LOG_INFO(CAT_MAIN, "Setup complete, entering loop()");
}

// ============================================================================
// LOOP (ОРКЕСТРАТОР)
// ============================================================================

void loop() {
  wdt_feed();
  checkResetButton();

  if (g_normalMode) {
    processNormalMode();
  } else {
    processProvisioning();
  }

  led_update();
}