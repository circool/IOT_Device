#include <Arduino.h>
#include "config_manager.h"
#include "led.h"
#include "logger.h"
#include "ota.h"
#include "settings.h"
#include "wdt_manager.h"
#include "wifi_manager.h"
#ifdef ESP32
#include <WiFi.h>
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

#ifdef ESP32
#include <esp_chip_info.h>
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

// ======================== MQTT FUNCTIONS ========================
#if MQTT_ENABLED == 1

#include "mqtt.h"
unsigned long lastMQTTAttempt = 0;

#if MQTT_PUBLISH_RESET_REASON == 1
const char* getResetReason() {
#ifdef ESP8266
  struct rst_info* resetInfo = system_get_rst_info();
  uint8_t reason = resetInfo->reason;

  switch (reason) {
    case REASON_DEFAULT_RST:
      return "POWER_ON";
    case REASON_WDT_RST:
      return "WATCHDOG_CRASH";
    case REASON_EXCEPTION_RST:
      return "EXCEPTION_CRASH";
    case REASON_SOFT_WDT_RST:
      return "SOFT_WDT_CRASH";
    case REASON_SOFT_RESTART:
      return "SOFT_RESTART";
    case REASON_EXT_SYS_RST:
      return "EXT_RESET";
    default:
      return "UNKNOWN";
  }

#elif defined(ESP32)
  esp_reset_reason_t reason = esp_reset_reason();

  switch (reason) {
    case ESP_RST_POWERON:
      return "POWER_ON";
    case ESP_RST_EXT:
      return "EXT_RESET";
    case ESP_RST_SW:
      return "SOFT_RESTART";
    case ESP_RST_PANIC:
      return "PANIC_CRASH";
    case ESP_RST_INT_WDT:
      return "INT_WDT_CRASH";
    case ESP_RST_TASK_WDT:
      return "TASK_WDT_CRASH";
    case ESP_RST_WDT:
      return "WDT_CRASH";
    case ESP_RST_DEEPSLEEP:
      return "DEEP_SLEEP_WAKE";
    default:
      return "UNKNOWN";
  }
#else
  return "UNKNOWN_PLATFORM";
#endif
}

#endif
#endif

// ======================== КНОПКА СБРОСА ========================

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

// ======================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ АКТУАТОРОВ
// ========================

#if DEVICE_TYPE == 1
FanActuator fan;
#elif DEVICE_TYPE == 3
SwitchActuator switchActuator;
#endif

// ======================== WEB STATUS PROVIDER ========================

#if WEB_ENABLED == 1
#if DEVICE_TYPE == 1
static FanWebStatusProvider statusProvider(&fan, nullptr, &mqttManager);
#elif DEVICE_TYPE == 3
static SwitchWebStatusProvider statusProvider(&switchActuator, &mqttManager);
#endif
#endif

// ============================================================================
// КОЛБЭК ДЛЯ PROVISIONING (ОБЪЯВЛЕН ВНЕ SETUP — НА УРОВНЕ ФАЙЛА)
// ============================================================================

#if USE_BLE_PROVISIONING == 1

static void onProvisioningComplete(const ProvisioningConfig* config,
                                   void* userData) {
  (void)userData;

  LOG_INFO(CAT_MAIN, "========================================");
  LOG_INFO(CAT_MAIN, "PROVISIONING COMPLETE!");
  LOG_INFO(CAT_MAIN, "========================================");
  LOG_INFO(CAT_MAIN, "WiFi SSID: %s", config->wifiSsid);
  LOG_INFO(CAT_MAIN, "MQTT Broker: %s:%d", config->mqttBroker,
           config->mqttPort);

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
  } else {
    LOG_ERROR(CAT_MAIN, "Failed to save config!");
    return;
  }

  LOG_INFO(CAT_MAIN, "Rebooting in 1 second...");
  delay(1000);
  ESP.restart();
}

#endif  // USE_BLE_PROVISIONING == 1

// ======================== SETUP ========================

void setup() {
  delay(2000);
  Logger::getInstance().begin((LogLevel)LOG_LEVEL, LOG_CATEGORIES,
                              LOG_USE_COLOR);

  LOG_INFO(CAT_MAIN, "SYSTEM START");
  LOG_DEBUG(CAT_MAIN, "=== SYSTEM INFO ===");

#ifdef ESP32
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  const char* chip_name = "Unknown";
  switch (chip_info.model) {
    case CHIP_ESP32:
      chip_name = "ESP32";
      break;
    case CHIP_ESP32S2:
      chip_name = "ESP32-S2";
      break;
    case CHIP_ESP32S3:
      chip_name = "ESP32-S3";
      break;
    case CHIP_ESP32C3:
      chip_name = "ESP32-C3";
      break;
#ifdef CHIP_ESP32C6
    case CHIP_ESP32C6:
      chip_name = "ESP32-C6";
      break;
#endif
#ifdef CHIP_ESP32H2
    case CHIP_ESP32H2:
      chip_name = "ESP32-H2";
      break;
#endif
    default:
      chip_name = "Unknown";
      break;
  }

  LOG_DEBUG(CAT_MAIN, "Chip: %s (revision v%d.%d)", chip_name,
            chip_info.revision / 100, chip_info.revision % 100);
  LOG_DEBUG(CAT_MAIN, "Cores: %d, Frequency: %d MHz", chip_info.cores,
            getCpuFrequencyMhz());
  LOG_DEBUG(CAT_MAIN, "Chip ID: %08X", (uint32_t)ESP.getEfuseMac());

  uint32_t flashSize = ESP.getFlashChipSize();
  LOG_DEBUG(CAT_MAIN, "Flash: %u MB (%d MHz, mode %d)",
            flashSize / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000,
            ESP.getFlashChipMode());

#ifdef CONFIG_SPIRAM_SUPPORT
  LOG_DEBUG(CAT_MAIN, "PSRAM: %u bytes (free: %u)", ESP.getPsramSize(),
            ESP.getFreePsram());
#else
  LOG_DEBUG(CAT_MAIN, "PSRAM: not supported");
#endif

  LOG_DEBUG(CAT_MAIN, "Heap: %u bytes free (min: %u, max alloc: %u)",
            ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
  LOG_DEBUG(CAT_MAIN, "ESP-IDF: %s", esp_get_idf_version());

#ifdef CONFIG_IDF_TARGET_ESP32C3
  LOG_DEBUG(CAT_MAIN, "Architecture: RISC-V");
  LOG_DEBUG(CAT_MAIN, "WiFi: %s, BLE: %s",
            chip_info.features & CHIP_FEATURE_WIFI_BGN ? "Yes" : "No",
            chip_info.features & CHIP_FEATURE_BLE ? "Yes" : "No");
#endif

#elif defined(ESP8266)
  LOG_DEBUG(CAT_MAIN, "Platform: ESP8266");
  LOG_DEBUG(CAT_MAIN, "Chip ID: %08X", ESP.getChipId());
  LOG_DEBUG(CAT_MAIN, "Core: %s, CPU: %d MHz", ESP.getCoreVersion().c_str(),
            ESP.getCpuFreqMHz());
  LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());

  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t realFlashSize = ESP.getFlashChipRealSize();
  LOG_DEBUG(CAT_MAIN, "Flash: %u MB (%d MHz, mode %d)",
            realFlashSize / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000,
            ESP.getFlashChipMode());

  LOG_DEBUG(CAT_MAIN, "SDK: %s", system_get_sdk_version());
#endif

  LOG_DEBUG(CAT_MAIN, "==========================================");
  LOG_DEBUG(CAT_MAIN, "Sketch size: %u bytes", ESP.getSketchSize());
  LOG_DEBUG(CAT_MAIN, "Free sketch space: %u bytes", ESP.getFreeSketchSpace());
  LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
  LOG_DEBUG(CAT_MAIN, "Firmware ver. %s", VERSION);
#if MQTT_PUBLISH_RESET_REASON == 1
  LOG_DEBUG(CAT_MAIN, "Reset reason: %s", getResetReason());
#endif

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
  // ЕСЛИ ЕСТЬ КОНФИГУРАЦИЯ — ПЕРЕХОДИМ В NORMAL MODE
  // ================================================================

  if (hasValidConfig) {
    LOG_INFO(CAT_MAIN, "Valid config found, starting normal mode...");
    goto normal_mode;
  }

  // ================================================================
  // НЕТ КОНФИГУРАЦИИ — ЗАПУСКАЕМ PROVISIONING
  // ================================================================

#if USE_BLE_PROVISIONING == 1

  LOG_INFO(CAT_MAIN, "No valid config found, starting BLE provisioning...");

  {
    auto& prov = ProvisioningManager::getInstance();

    bool started = prov.begin(onProvisioningComplete, nullptr, 120000);

    if (started) {
      LOG_INFO(CAT_MAIN, "BLE provisioning started");
      LOG_INFO(CAT_MAIN, "Device name: %s", g_configManager.getDeviceId());
      LOG_INFO(CAT_MAIN, "Use nRF Connect or ESP BLE Provisioning app");

      // ===== УСТАНАВЛИВАЕМ РЕЖИМ LED =====
      led_setMode(LED_MODE_MORZE_S);  // 3 вспышки — режим настройки
      return;

    } else {
      LOG_ERROR(CAT_MAIN, "Failed to start BLE provisioning!");
#if AP_ENABLED == 1
      LOG_INFO(CAT_MAIN, "Falling back to AP mode...");
      web_initAP();
      return;
#else
      while (1) {
        led_setMode(LED_MODE_MORZE_S);
        led_update();
        delay(1000);
        LOG_ERROR(CAT_MAIN, "No provisioning method available!");
      }
#endif
    }
  }

#else  // USE_BLE_PROVISIONING == 0

  // ===== AP + Web (ESP8266, ESP32 без BLE) =====
  LOG_INFO(CAT_CONFIG, "Starting AP for setup...");
#if AP_ENABLED == 1
  web_initAP();
  return;
#else
  while (1) {
    led_setMode(LED_MODE_MORZE_S);
    led_update();
    delay(1000);
    LOG_ERROR(CAT_MAIN, "No provisioning method available!");
  }
#endif

#endif  // USE_BLE_PROVISIONING == 1

  // ================================================================
  // NORMAL MODE
  // ================================================================

normal_mode:

  if (!g_configManager.isValid()) {
    LOG_ERROR(CAT_MAIN, "No valid config!");
    while (1) {
      led_setMode(LED_MODE_MORZE_S);
      led_update();
      delay(1000);
    }
  }

  LOG_INFO(CAT_MAIN, "========================================");
  LOG_INFO(CAT_MAIN, "NORMAL MODE");
  LOG_INFO(CAT_MAIN, "========================================");

  // ========== MQTT ==========
#if MQTT_ENABLED == 1
  mqttManager.begin(
      g_mqttClient, g_configManager.getMqttBroker(),
      g_configManager.getMqttPort(), g_configManager.getMqttClientId(),
      g_configManager.getMqttUser(), g_configManager.getMqttPassword());

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
#endif

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

// ========== WIFI ==========
// TCP/IP стек инициализируется здесь
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
  // OTA должен регистрироваться ПОСЛЕ инициализации TCP/IP
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

// ======================== LOOP ========================

void loop() {
  wdt_feed();
  checkResetButton();

  // ================================================================
  // 1. ОБРАБОТКА PROVISIONING (если активен)
  // ================================================================

#if USE_BLE_PROVISIONING == 1
  {
    auto& prov = ProvisioningManager::getInstance();
    prov.process();

    if (prov.isActive()) {
      led_update();
      return;
    }
  }
#endif

  // ================================================================
  // 2. LED ИНДИКАЦИЯ
  // ================================================================

  static LedMode lastMode = LED_MODE_OFF;
  LedMode newMode = LED_MODE_ON;

#if DEVICE_TYPE == 1
  if (fan.isEmergencyStop()) {
    newMode = LED_SLOW_BLINK;
  } else
#elif DEVICE_TYPE == 3
  if (switchActuator.isEmergencyStop()) {
    newMode = LED_SLOW_BLINK;
  } else
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

  if (newMode != lastMode) {
    led_setMode(newMode);
    lastMode = newMode;
  }

  led_update();

  // ================================================================
  // 3. ОСНОВНАЯ ЛОГИКА
  // ================================================================

  if (!g_configManager.isValid() ||
      strlen(g_configManager.getWifiSsid()) == 0) {
    web_update();
    return;
  }

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

#if DEVICE_TYPE == 1
    static bool lastFanState = false;
    bool currentFanState = fan.getState();
    if (currentFanState != lastFanState) {
      mqttManager.publishState(currentFanState);
      lastFanState = currentFanState;
    }
#elif DEVICE_TYPE == 3
    static bool lastSwitchState = false;
    bool currentSwitchState = switchActuator.getState();
    if (currentSwitchState != lastSwitchState) {
      mqttManager.publishState(currentSwitchState);
      lastSwitchState = currentSwitchState;
    }
#endif

#if DEVICE_TYPE == 1
    static uint16_t lastSpeedPercent = 0;
    if (fan.getSpeed() != lastSpeedPercent) {
      mqttManager.publishSpeed(fan.getSpeed());
      lastSpeedPercent = fan.getSpeed();
    }
#endif

#if DEVICE_TYPE == 1
    static bool lastSensorControlMode = false;
    if (g_configManager.getSensorControlMode() != lastSensorControlMode) {
      mqttManager.publishSensorControlMode(
          g_configManager.getSensorControlMode());
      lastSensorControlMode = g_configManager.getSensorControlMode();
    }
#endif

#if DEVICE_TYPE == 1
    static bool lastAdaptiveMode = false;
    if (fan.getAdaptiveMode() != lastAdaptiveMode) {
      mqttManager.publishAdaptiveMode(fan.getAdaptiveMode());
      lastAdaptiveMode = fan.getAdaptiveMode();
    }
#endif

#if DEVICE_TYPE == 1
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

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    static int lastDelaySeconds = -1;
    if (g_configManager.getDelaySeconds() != lastDelaySeconds) {
      mqttManager.publishDelaySec(g_configManager.getDelaySeconds());
      lastDelaySeconds = g_configManager.getDelaySeconds();
    }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
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

    if (mqttManager.isConnected()) {
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

#if MQTT_PUBLISH_RESET_REASON == 1
        mqttManager.publishResetReason(getResetReason());
#endif

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
    } else {
      initialConfigPublished = false;
    }
  }
#endif

  // ========== WEB ==========
  web_update();
}