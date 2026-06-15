// ============================================================================
// @file main.cpp
// @brief Главный модуль, оркестратор устройства
// ============================================================================

#include <Arduino.h>
#include "config.h"
#include "led.h"
#include "logger.h"
#include "web.h"
#include "wifi_manager.h"

#ifdef ESP32
#include <esp_chip_info.h>
#endif

// ============================================================================
// ПОДКЛЮЧЕНИЕ МОДУЛЕЙ (по флагам компиляции)
// ============================================================================

#if MQTT_ENABLED == 1
#include "mqtt.h"
static WiFiClient g_mqttClient;
#endif

#if WDT_ENABLED == 1
#include "wdt_manager.h"
#endif

#if DEVICE_TYPE == 1
#include "fan_actuator.h"
#elif DEVICE_TYPE == 3
#include "switch_actuator.h"
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
#include "sensor.h"
#endif

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

bool apMode = false;
static Config g_activeConfig;

// ============================================================================
// ОБЪЕКТЫ АКТУАТОРОВ
// ============================================================================

#if DEVICE_TYPE == 1
static FanActuator fan;
static unsigned long lastAdaptiveCheck = 0;
static float baseTemp = 0;
static float baseHum = 0;
static bool adaptiveActive = false;
#elif DEVICE_TYPE == 3
static SwitchActuator switchActuator;
#endif

// ============================================================================
// ТАЙМЕРЫ ОРКЕСТРАТОРА
// ============================================================================

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
static unsigned long delayTimerExpiry = 0;
static bool delayActive = false;
#endif

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

static bool isAutoModeEnabled() {
  return g_activeConfig.sensorControlMode && sensor_isOk();
}

// ============================================================================
// ЛОГИКА УПРАВЛЕНИЯ ВЕНТИЛЯТОРОМ (TYPE 1)
// ============================================================================

#if DEVICE_TYPE == 1

static void processSensorControl() {
  static bool lastAutoWasWorking = false;
  bool autoEnabled = isAutoModeEnabled();

  if (autoEnabled != lastAutoWasWorking) {
    if (autoEnabled) {
      LOG_INFO(CAT_SENSOR, "Sensor control mode ACTIVE (sensor OK)");
    } else {
      if (!sensor_isOk()) {
        LOG_WARN(CAT_SENSOR,
                 "Sensor control mode DISABLED: sensor not available");
      } else if (!g_activeConfig.sensorControlMode) {
        LOG_INFO(CAT_SENSOR, "Sensor control mode DISABLED: user turned off");
      }
    }
    lastAutoWasWorking = autoEnabled;
  }

  if (!autoEnabled)
    return;

  float temp = sensor_getTemperature();
  float hum = sensor_getHumidity();

  bool shouldBeOn =
      (temp >= g_activeConfig.highTemp || hum >= g_activeConfig.highHum);
  bool shouldBeOff =
      (temp <= g_activeConfig.lowTemp && hum <= g_activeConfig.lowHum);

  if (shouldBeOn && !fan.getState()) {
    LOG_INFO(CAT_SENSOR, "Auto ON: T=%.1f°C H=%.1f%%", temp, hum);
    fan.set(true, false);
    adaptiveActive = false;
  } else if (shouldBeOff && fan.getState()) {
    LOG_INFO(CAT_SENSOR, "Auto OFF: T=%.1f°C H=%.1f%%", temp, hum);
    fan.set(false, false);
    adaptiveActive = false;
  }
}

static void processAdaptiveMode() {
  if (!g_activeConfig.adaptiveMode)
    return;
  if (!isAutoModeEnabled())
    return;
  if (!fan.getState())
    return;

  unsigned long interval = g_activeConfig.sensorInterval * 1000UL;
  if (millis() - lastAdaptiveCheck < interval)
    return;
  lastAdaptiveCheck = millis();

  if (!adaptiveActive) {
    adaptiveActive = true;
    baseTemp = sensor_getTemperature();
    baseHum = sensor_getHumidity();
    LOG_DEBUG(CAT_FAN, "Adaptive mode activated: base T=%.1f°C, H=%.1f%%",
              baseTemp, baseHum);
    return;
  }

  float deltaTemp = sensor_getTemperature() - baseTemp;
  float deltaHum = sensor_getHumidity() - baseHum;
  float humRate = sensor_getHumRate();

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
  step = constrain(step, 5, 60);

  if (deltaTemp > ADAPTIVE_EPSILON_TEMP || deltaHum > ADAPTIVE_EPSILON_HUM) {
    int newSpeed = fan.getSpeed() + step;
    if (newSpeed > 100)
      newSpeed = 100;
    if (newSpeed != fan.getSpeed()) {
      fan.setSpeed(newSpeed, false);
      baseTemp = sensor_getTemperature();
      baseHum = sensor_getHumidity();
      LOG_DEBUG(CAT_FAN, "Adaptive: speed %d%% (ΔT=%.1f, ΔH=%.1f, rate=%.2f)",
                newSpeed, deltaTemp, deltaHum, humRate);
    }
  }
}

#endif  // DEVICE_TYPE == 1

// ============================================================================
// ЛОГИКА ТАЙМЕРА ОТЛОЖЕННОГО ВКЛЮЧЕНИЯ (TYPE 1 и 3)
// ============================================================================

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

static void processDelayTimer() {
  if (delayActive && !fan.getState() && g_activeConfig.delaySeconds > 0) {
    if (millis() >= delayTimerExpiry) {
      delayActive = false;
      LOG_INFO(CAT_ACTUATOR, "Delay timer expired, turning ON");
      fan.set(true, false);
    }
    return;
  }

  if (!delayActive && !fan.getState() && g_activeConfig.delaySeconds > 0) {
    delayActive = true;
    delayTimerExpiry = millis() + g_activeConfig.delaySeconds * 1000UL;
    LOG_INFO(CAT_ACTUATOR, "Delay timer started: %d sec",
             g_activeConfig.delaySeconds);
    return;
  }

  if (delayActive && fan.getState()) {
    delayActive = false;
    LOG_INFO(CAT_ACTUATOR, "Delay timer cancelled (manual ON)");
  }
}

#endif  // DEVICE_TYPE == 1 || DEVICE_TYPE == 3

// ============================================================================
// ПРИМЕНЕНИЕ КОНФИГУРАЦИИ (для нормального режима)
// ============================================================================

static void applyNormalModeConfig() {
  LOG_INFO(CAT_MAIN, "Applying normal mode configuration...");

#if MQTT_ENABLED == 1
  mqttManager.begin(g_mqttClient, g_activeConfig.mqttBroker,
                    g_activeConfig.mqttPort, g_activeConfig.mqttUser,
                    g_activeConfig.mqttPassword);
  LOG_DEBUG(CAT_MAIN, "MQTT initialized: broker=%s:%d",
            g_activeConfig.mqttBroker, g_activeConfig.mqttPort);
#endif

#if WDT_ENABLED == 1
  wdt_init();
  LOG_DEBUG(CAT_MAIN, "WDT initialized, timeout=%d ms", WDT_TIMER_MS);
#endif

  LOG_INFO(CAT_MAIN, "Normal mode configuration applied");
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // 1. Инициализация логгера
  Logger::getInstance().begin((LogLevel)LOG_LEVEL, LOG_CATEGORIES,
                              LOG_USE_COLOR);

  LOG_INFO(CAT_MAIN, "==========================================");
  LOG_INFO(CAT_MAIN, "SYSTEM START");
  LOG_DEBUG(CAT_MAIN, "=== SYSTEM INFO ===");

#ifdef ESP32
  LOG_DEBUG(CAT_MAIN, "Platform: ESP32");
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  LOG_DEBUG(CAT_MAIN, "Chip model: ESP32-%d", chip_info.model);
  LOG_DEBUG(CAT_MAIN, "Chip revision: %d", chip_info.revision);
  LOG_DEBUG(CAT_MAIN, "Cores: %d", chip_info.cores);
  LOG_DEBUG(CAT_MAIN, "CPU frequency: %d MHz", getCpuFrequencyMhz());
  LOG_DEBUG(CAT_MAIN, "Chip ID: %08X", (uint32_t)ESP.getEfuseMac());
  uint32_t flashSize = ESP.getFlashChipSize();
  LOG_DEBUG(CAT_MAIN, "Flash chip size: %u bytes (%u MB)", flashSize,
            flashSize / (1024 * 1024));
  LOG_DEBUG(CAT_MAIN, "Flash chip speed: %d MHz",
            ESP.getFlashChipSpeed() / 1000000);
  LOG_DEBUG(CAT_MAIN, "Flash chip mode: %d", ESP.getFlashChipMode());
#ifdef CONFIG_SPIRAM_SUPPORT
  LOG_DEBUG(CAT_MAIN, "PSRAM size: %u bytes", ESP.getPsramSize());
  LOG_DEBUG(CAT_MAIN, "Free PSRAM: %u bytes", ESP.getFreePsram());
#else
  LOG_DEBUG(CAT_MAIN, "PSRAM: not supported/enabled");
#endif
  LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
  LOG_DEBUG(CAT_MAIN, "Minimum free heap: %u bytes", ESP.getMinFreeHeap());
  LOG_DEBUG(CAT_MAIN, "Maximum allocatable heap: %u bytes",
            ESP.getMaxAllocHeap());
  LOG_DEBUG(CAT_MAIN, "ESP-IDF version: %s", esp_get_idf_version());
#elif defined(ESP8266)
  LOG_DEBUG(CAT_MAIN, "Platform: ESP8266");
  LOG_DEBUG(CAT_MAIN, "Chip ID: %08X", ESP.getChipId());
  LOG_DEBUG(CAT_MAIN, "Core version: %s", ESP.getCoreVersion().c_str());
  LOG_DEBUG(CAT_MAIN, "CPU frequency: %d MHz", ESP.getCpuFreqMHz());
  LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
  uint32_t flashSize = ESP.getFlashChipSize();
  LOG_DEBUG(CAT_MAIN, "Flash chip size: %u bytes (%u MB)", flashSize,
            flashSize / (1024 * 1024));
  uint32_t realFlashSize = ESP.getFlashChipRealSize();
  if (realFlashSize > 0 && realFlashSize != flashSize) {
    LOG_DEBUG(CAT_MAIN, "Real flash chip size: %u bytes (%u MB)", realFlashSize,
              realFlashSize / (1024 * 1024));
  }
  LOG_DEBUG(CAT_MAIN, "Flash chip speed: %d MHz",
            ESP.getFlashChipSpeed() / 1000000);
  LOG_DEBUG(CAT_MAIN, "Flash chip mode: %d (0=QIO, 1=QOUT, 2=DIO, 3=DOUT)",
            ESP.getFlashChipMode());
  LOG_DEBUG(CAT_MAIN, "SDK version: %s", system_get_sdk_version());
#endif

  LOG_DEBUG(CAT_MAIN, "==========================================");
  LOG_DEBUG(CAT_MAIN, "Sketch size: %u bytes", ESP.getSketchSize());
  LOG_DEBUG(CAT_MAIN, "Free sketch space: %u bytes", ESP.getFreeSketchSpace());
  LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
  LOG_DEBUG(CAT_MAIN, "Firmware version: %s", VERSION);
  LOG_INFO(CAT_MAIN, "==========================================");
  LOG_INFO(CAT_MAIN, "Device starting with %s mode", DEVICE_PREFIX);

  // 2. Инициализация LED
  led_init();
  led_setMode(LED_MODE_MORZE_E);
  led_update();
  LOG_DEBUG(CAT_MAIN, "LED initialized, pin=%d", STATUS_LED_PIN);

  // 3. Инициализация WiFi стека
  wifi_init();

  // 4. Инициализация конфигурации
  config_init();
  g_activeConfig = config_load();

  // 5. Инициализация датчика (всегда)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_init(&g_activeConfig);
  LOG_DEBUG(CAT_MAIN, "Sensor initialized: interval=%d sec, type=%d",
            g_activeConfig.sensorInterval, SENSOR_TYPE);

  if (!sensor_isOk()) {
    LOG_WARN(CAT_SENSOR, "Sensor not available: %s", sensor_getError());
    if (g_activeConfig.sensorControlMode) {
      LOG_WARN(CAT_MAIN,
               "Sensor control mode temporarily disabled (no sensor)");
    }
  }
#endif

  // 6. Инициализация актуатора (всегда)
#if DEVICE_TYPE == 1
  fan.init(SWITCH_PIN, g_activeConfig.bootState, g_activeConfig.speedPercent,
           g_activeConfig.maxOnTime);
  LOG_INFO(CAT_MAIN, "Fan initialized: speed=%d%%, maxOnTime=%lu sec",
           g_activeConfig.speedPercent, g_activeConfig.maxOnTime);
#elif DEVICE_TYPE == 3
  switchActuator.init(SWITCH_PIN, g_activeConfig.bootState,
                      g_activeConfig.maxOnTime);
  LOG_INFO(CAT_MAIN, "Switch initialized: maxOnTime=%lu sec",
           g_activeConfig.maxOnTime);
#endif

  // ========== НАСТРОЙКА КОНТЕКСТА ДЛЯ ВЕБ-СЛОЯ ==========
  web_setTransport((TransportType)TRANSPORT_TYPE);
  web_setDeviceType(DEVICE_TYPE);
  web_setConfig(&g_activeConfig);

  // 7. Проверка валидности конфига и выбор режима
  if (!g_activeConfig.valid) {
    LOG_ERROR(CAT_MAIN, "Config is INVALID: %s", g_activeConfig.error);
    LOG_INFO(CAT_MAIN, "Using defaults, entering AP mode for setup");

    g_activeConfig = config_getDefaults();
    apMode = true;

    // Запускаем AP режим (открытая сеть)
    wifi_startAP(g_activeConfig.deviceId, nullptr);
    web_setApMode(true);
    web_enableStatusPage(false);  // корень → конфиг
    web_init();
    led_setMode(LED_MODE_MORZE_S);
    LOG_INFO(CAT_MAIN, "AP mode active. SSID: %s, IP: %s (open)",
             g_activeConfig.deviceId, AP_IP_ADDRESS);
  } else {
    LOG_INFO(CAT_MAIN, "Config is VALID, entering normal mode");
    apMode = false;

    // Подключаемся к WiFi
    if (strlen(g_activeConfig.wifiSsid) > 0) {
      wifi_connect(g_activeConfig.wifiSsid, g_activeConfig.wifiPassword);
    } else {
      LOG_WARN(CAT_MAIN, "WiFi SSID is empty, staying in AP mode");
      apMode = true;
      wifi_startAP(g_activeConfig.deviceId, nullptr);
      web_setApMode(true);
      web_enableStatusPage(false);
      web_init();
      led_setMode(LED_MODE_MORZE_S);
    }

    web_setApMode(false);
    web_enableStatusPage(WEB_STATUS_ENABLED == 1);
    web_init();

    applyNormalModeConfig();
    led_setMode(LED_MODE_ON);
  }

  config_print(g_activeConfig);
  LOG_INFO(CAT_MAIN, "==========================================");
  LOG_INFO(CAT_MAIN, "Setup complete. Entering loop()");
  LOG_INFO(CAT_MAIN, "==========================================");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  led_update();

#if WDT_ENABLED == 1
  wdt_feed();
#endif

  // 1. Чтение датчика
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_update();
#endif

  // 2. Логика управления актуаторами
#if DEVICE_TYPE == 1
  processSensorControl();
  processAdaptiveMode();
  processDelayTimer();
  fan.update();

  if (fan.isEmergencyStop()) {
    led_setMode(LED_SLOW_BLINK);
  }
#elif DEVICE_TYPE == 3
  processDelayTimer();
  switchActuator.update();

  if (switchActuator.isEmergencyStop()) {
    led_setMode(LED_SLOW_BLINK);
  }
#endif

  // 3. Передача данных в веб-слой
  web_setSensorData(sensor_getTemperature(), sensor_getHumidity());
  web_setActuatorState(fan.getState(), fan.getSpeed());

  // 4. Обработка WiFi
  wifi_process();

  // 5. Веб-сервер
  web_update();

  // 6. MQTT (только в нормальном режиме и при наличии WiFi)
  // if (!apMode && wifi_isConnected() && MQTT_ENABLED == 1) {
  //   mqttManager.process();
  // }

  delay(10);
}

// ============================================================================
// ТОЧКИ ВХОДА ДЛЯ ВНЕШНИХ МОДУЛЕЙ
// ============================================================================

const Config* config_getActive() {
  return &g_activeConfig;
}

bool config_updateFromWeb(const Config& newConfig) {
  LOG_INFO(CAT_MAIN, "Updating config from web...");

  Config temp = newConfig;
  if (!config_validate(temp)) {
    LOG_ERROR(CAT_MAIN, "Invalid config from web: %s", temp.error);
    return false;
  }

  if (!config_save(temp)) {
    LOG_ERROR(CAT_MAIN, "Failed to save config to EEPROM");
    return false;
  }

  g_activeConfig = temp;
  LOG_INFO(CAT_MAIN, "Config saved successfully, restarting...");
  delay(100);
  ESP.restart();
  return true;
}