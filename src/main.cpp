#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "ota.h"
#include "wifi_manager.h"
#include "wdt_manager.h"
#include "led.h"


#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

static WiFiClient g_mqttClient;


#if OTA_ENABLED ==1
#include "ota_check.h"
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

#if WEB_ENABLED == 1
  #include "web.h"
#endif

// ======================== MQTT FUNCTIONS ========================
#if MQTT_ENABLED == 1
  
  #include "mqtt.h"
  unsigned long lastMQTTAttempt = 0;
  
  #if MQTT_PUBLISH_RESET_REASON == 1
    const char* getResetReason() {
    #ifdef ESP8266
    struct rst_info *resetInfo = system_get_rst_info();
    uint8_t reason = resetInfo->reason;
    
    switch(reason) {
      case REASON_DEFAULT_RST:    return "POWER_ON";
      case REASON_WDT_RST:        return "WATCHDOG_CRASH";
      case REASON_EXCEPTION_RST:  return "EXCEPTION_CRASH";
      case REASON_SOFT_WDT_RST:   return "SOFT_WDT_CRASH";
      case REASON_SOFT_RESTART:   return "SOFT_RESTART";
      case REASON_EXT_SYS_RST:    return "EXT_RESET";
      default:                    return "UNKNOWN";
    }
        
    #elif defined(ESP32)
    esp_reset_reason_t reason = esp_reset_reason();
    
    switch(reason) {
      case ESP_RST_POWERON:       return "POWER_ON";
      case ESP_RST_EXT:           return "EXT_RESET";
      case ESP_RST_SW:            return "SOFT_RESTART";
      case ESP_RST_PANIC:         return "PANIC_CRASH";
      case ESP_RST_INT_WDT:       return "INT_WDT_CRASH";
      case ESP_RST_TASK_WDT:      return "TASK_WDT_CRASH";
      case ESP_RST_WDT:           return "WDT_CRASH";
      case ESP_RST_DEEPSLEEP:     return "DEEP_SLEEP_WAKE";
      default:                    return "UNKNOWN";
    }
    #else
        return "UNKNOWN_PLATFORM";
    #endif
    }
    
  #endif
#endif



void checkResetButton() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  delay(50);    
  if (digitalRead(RESET_PIN) == LOW) {
    LOG_INFO(CAT_MAIN, "Reset button pressed...");      
    #if STATUS_LED_PIN > 0
        LedMode prevMode = led_getMode();
    #endif      
    unsigned long pressStart = millis();
    wdt_stop();     
    while (digitalRead(RESET_PIN) == LOW) {
      unsigned long pressedMs = millis() - pressStart;       
      #if STATUS_LED_PIN > 0
      // Меняем режим в зависимости от времени удержания
      if (pressedMs < 1000) {
          led_setMode(LED_MODE_MORZE_E);  // 0-1 сек: одиночные
      } else if (pressedMs < 2000) {
          led_setMode(LED_MODE_MORZE_I);  // 1-2 сек: двойные
      } else {
          led_setMode(LED_MODE_MORZE_S);  // 2-3 сек: тройные
      }
      led_update();
      #endif
        
      if (pressedMs >= 3000) {
        LOG_INFO(CAT_MAIN, "Auto-reset triggered!");
        
        #if STATUS_LED_PIN > 0
          led_setMode(LED_MODE_OFF);
          led_update();
        #endif
          
        if (config_clear()) {
          LOG_INFO(CAT_CONFIG, "Config cleared, restarting...");
          delay(500);
          ESP.restart();
        }
        return;
      }
      
      delay(10);
      wdt_feed();
    }
    
    // Отмена сброса
    LOG_INFO(CAT_MAIN, "Reset cancelled (released after %d ms)", millis() - pressStart);
    wdt_start();
    
    #if STATUS_LED_PIN > 0
        led_setMode(prevMode);
    #endif
  }
}

// ======================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ АКТУАТОРОВ ========================
#if DEVICE_TYPE == 1
  FanActuator fan;
#elif DEVICE_TYPE == 3
  SwitchActuator switchActuator;
#endif

// ======================== SETUP ========================
void setup() {

  Logger::getInstance().begin(
        (LogLevel)LOG_LEVEL,
        LOG_CATEGORIES,
        LOG_USE_COLOR
    );
  LOG_INFO(CAT_MAIN, "SYSTEM START");

  delay(1000);


  #if DEBUG_ENABLED == 1
  delay(2000);
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
     LOG_DEBUG(CAT_MAIN, "Flash chip size: %u bytes (%u MB)", flashSize, flashSize / (1024 * 1024));
     LOG_DEBUG(CAT_MAIN, "Flash chip speed: %d MHz", ESP.getFlashChipSpeed() / 1000000);
     LOG_DEBUG(CAT_MAIN, "Flash chip mode: %d", ESP.getFlashChipMode());
    #ifdef CONFIG_SPIRAM_SUPPORT
       LOG_DEBUG(CAT_MAIN, "PSRAM size: %u bytes", ESP.getPsramSize());
       LOG_DEBUG(CAT_MAIN, "Free PSRAM: %u bytes", ESP.getFreePsram());
    #else
       LOG_DEBUG(CAT_MAIN, "PSRAM: not supported/enabled");
    #endif

     LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
     LOG_DEBUG(CAT_MAIN, "Minimum free heap: %u bytes", ESP.getMinFreeHeap());
     LOG_DEBUG(CAT_MAIN, "Maximum allocatable heap: %u bytes", ESP.getMaxAllocHeap());
     LOG_DEBUG(CAT_MAIN, "ESP-IDF version: %s", esp_get_idf_version());

  #elif defined(ESP8266)
     LOG_DEBUG(CAT_MAIN, "Platform: ESP8266");
     LOG_DEBUG(CAT_MAIN, "Chip ID: %08X", ESP.getChipId());
     LOG_DEBUG(CAT_MAIN, "Core version: %s", ESP.getCoreVersion().c_str());
     LOG_DEBUG(CAT_MAIN, "CPU frequency: %d MHz", ESP.getCpuFreqMHz());
     LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
    uint32_t flashSize = ESP.getFlashChipSize();
     LOG_DEBUG(CAT_MAIN, "Flash chip size: %u bytes (%u MB)", flashSize, flashSize / (1024 * 1024));
    uint32_t realFlashSize = ESP.getFlashChipRealSize();
    if (realFlashSize > 0 && realFlashSize != flashSize) {
       LOG_DEBUG(CAT_MAIN, "Real flash chip size: %u bytes (%u MB)", realFlashSize, realFlashSize / (1024 * 1024));
    }
     LOG_DEBUG(CAT_MAIN, "Flash chip speed: %d MHz", ESP.getFlashChipSpeed() / 1000000);
     LOG_DEBUG(CAT_MAIN, "Flash chip mode: %d (0=QIO, 1=QOUT, 2=DIO, 3=DOUT)", ESP.getFlashChipMode());
     LOG_DEBUG(CAT_MAIN, "SDK version: %s", system_get_sdk_version());
  #endif
  
   LOG_INFO(CAT_MAIN, "==========================================");
   LOG_DEBUG(CAT_MAIN, "Sketch size: %u bytes", ESP.getSketchSize());
   LOG_DEBUG(CAT_MAIN, "Free sketch space: %u bytes", ESP.getFreeSketchSpace());
   LOG_DEBUG(CAT_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
   LOG_DEBUG(CAT_MAIN, "Firmware ver. %s", VERSION);
  #if MQTT_PUBLISH_RESET_REASON == 1
     LOG_DEBUG(CAT_MAIN, "Reset reason: %s", getResetReason());
  #endif
#endif
  
   LOG_INFO(CAT_MAIN, "==========================================");
   LOG_INFO(CAT_MAIN, "Device starting with %s mode", DEVICE_PREFIX);
  
  // ========== ИНИЦИАЛИЗАЦИЯ ПОДСИСТЕМ ==========
  #if STATUS_LED_PIN > 0
    led_init();
    led_setMode(LED_MODE_MORZE_E);
  #endif

  wdt_init();
  checkResetButton();
  config_init();

  #if OTA_ENABLED == 1
    bool otaCapable = isOtaAvailable();
    ota_set_available(otaCapable);  
  #endif

  #if SCANING_WIFI_ENABLED == 1 
    wdt_stop();
    
    const Config* cfg = config_get();
    const char* targetSsid = (cfg != nullptr) ? cfg->wifiSsid : nullptr;
    wifi_scan_and_log(targetSsid);
    
    wdt_start();
#endif

  #if DEBUG_ENABLED == 1
    config_print();
  #endif

  bool hasValidConfig = config_isValid();

  if (hasValidConfig) {
    #if LOG_CONFIG == 1
      LOG_INFO(CAT_CONFIG, "Normal mode - starting with saved config");
    #endif

    // ========== MQTT ИНИЦИАЛИЗАЦИЯ ==========
    #if MQTT_ENABLED == 1
    mqttManager.begin(g_mqttClient, 
      config_get()->mqttBroker, 
      config_get()->mqttPort, 
      config_get()->mqttClientId,
      config_get()->mqttUser, 
      config_get()->mqttPassword);
        
    // Регистрация колбэков
    #if DEVICE_TYPE == 1
    mqttManager.onStateCommand([](bool state) { 
      fan.set(state, true);
    });
        
        mqttManager.onSpeedCommand([](int speed) {
          if (config_get()->adaptiveMode) {
            config_setAdaptiveMode(false);
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
            LOG_WARN(CAT_MQTT, "Cannot enable sensor control mode - sensor not available");           
            mqttManager.publishSensorControlMode(false);
            return;
        }
           
        config_setSensorControlMode(enabled);
            if (!enabled) {
                fan.setAdaptiveMode(false);
            }
        });
        
        mqttManager.onAdaptiveModeCommand([](bool enabled) {
          if (enabled && !config_get()->sensorControlMode) {
            LOG_WARN(CAT_MQTT, "Cannot enable adaptive mode - sensor control mode is OFF");
            return;
          }
          config_setAdaptiveMode(enabled);
          fan.setAdaptiveMode(enabled);
        });
        
        mqttManager.onLowTempCommand([](float value) { 
          config_setLowTemp(value); 
        });
        mqttManager.onHighTempCommand([](float value) { 
          config_setHighTemp(value); 
        });
        mqttManager.onLowHumCommand([](float value) { 
          config_setLowHum(value); 
        });
        mqttManager.onHighHumCommand([](float value) { 
          config_setHighHum(value); 
        });
        
        mqttManager.onDelaySecCommand([](int delaySec) {
          config_setDelaySeconds(delaySec);
        });
        #endif
        
        #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
        mqttManager.onMaxOnTimeCommand([](uint32_t maxOnTime) { 
          config_setMaxOnTime(maxOnTime); 
        });
        #endif

        #if MQTT_RESET_ENABLED == 1
        mqttManager.onResetCommand([]() {
          LOG_INFO(CAT_MQTT, "Resetting due MQTT RESET");
          mqttManager.disconnect();
          config_clear();
          delay(1000);
          ESP.restart();
        });
        #endif
    #endif

    // ========== ДАТЧИК ==========
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
      #ifndef TEST_DISABLE_SENSOR
        sensor_init();
        #if DEVICE_TYPE == 1
          if (!sensor_isOk() && (
              #if SENSOR_TYPE == 1
              strcmp(sensor_getError(), "AHT10 not found") == 0
              #elif SENSOR_TYPE == 2
              strcmp(sensor_getError(), "DHT read failed (NaN)") == 0
              #endif
          )) {
            config_setSensorControlMode(false);
            config_setAdaptiveMode(false);
            fan.setAdaptiveMode(false);
            LOG_WARN(CAT_SENSOR, "Not found - switching to MANUAL mode");
          }
        #endif
      #endif
    #endif

    // ========== ВЕНТИЛЯТОР ==========
    #if DEVICE_TYPE == 1
      #ifndef TEST_DISABLE_FAN
        fan.init(SWITCH_PIN, RELAY_ON_LEVEL, config_get()->bootState, config_get()->speedPercent);
        fan.setAdaptiveMode(config_get()->adaptiveMode);
      #endif
    #endif

    // ========== ВЫКЛЮЧАТЕЛЬ ==========
    #if DEVICE_TYPE == 3
      #ifndef TEST_DISABLE_SWITCH
        switchActuator.init(SWITCH_PIN, RELAY_ON_LEVEL, config_get()->bootState);
      #endif
    #endif

    #if WEB_ENABLED == 1 && (DEVICE_TYPE == 1 || DEVICE_TYPE == 3)
      #ifndef TEST_DISABLE_WEB
        #if DEVICE_TYPE == 1
          web_registerActuators(&fan, nullptr);
        #elif DEVICE_TYPE == 3
          web_registerActuators(nullptr, &switchActuator);
        #endif
      #endif
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
    #ifndef TEST_DISABLE_WIFI
      wifi_begin();
    #endif

    // ========== WEB ==========
    #if WEB_ENABLED == 1
      #ifndef TEST_DISABLE_WEB
        web_init();
      #endif
    #endif

  } else {
    LOG_INFO(CAT_CONFIG, "Configuration mode - starting AP for setup");
    #if AP_ENABLED == 1
      #ifndef TEST_DISABLE_WIFI
        web_initAP();
      #endif
    #endif
  }
}

// ======================== LOOP ========================
void loop() {
  
  wdt_feed();
  checkResetButton();
  led_update();

  #if WEB_ENABLED == 1
  if (!config_isValid() || strlen(config_get()->wifiSsid) == 0) {
    web_update();
    delay(100);
    return;
  }
  #endif

  // ========== ДАТЧИК ==========
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    #ifndef TEST_DISABLE_SENSOR
      bool sensorDataChanged = sensor_update();
      #if DEVICE_TYPE == 1
      if (config_get()->sensorControlMode && sensor_isOk() && sensorDataChanged) {
        float temp = sensor_getTemperature();
        float hum = sensor_getHumidity();
        
        bool shouldBeOn = (temp >= config_get()->highTemp || hum >= config_get()->highHum);
        bool shouldBeOff = (temp <= config_get()->lowTemp && hum <= config_get()->lowHum);
        
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
  #endif

  // ========== ВЕНТИЛЯТОР ==========
  #if DEVICE_TYPE == 1
    #ifndef TEST_DISABLE_FAN
      fan.update();
    #endif
  #endif

  // ========== ВЫКЛЮЧАТЕЛЬ ==========
  #if DEVICE_TYPE == 3
    #ifndef TEST_DISABLE_SWITCH
      switchActuator.update();
    #endif
  #endif

  // ========== WIFI ==========
  #ifndef TEST_DISABLE_WIFI
    wifi_monitor();
  #endif

  // ========== MQTT ==========
  #if MQTT_ENABLED == 1
    #ifndef TEST_DISABLE_MQTT
      if (wifi_is_connected()) {
        mqttManager.process();
        
        // ========== ОТСЛЕЖИВАНИЕ ИЗМЕНЕНИЙ ДЛЯ ПУБЛИКАЦИИ В MQTT ==========
        
        // 1. Состояние вентилятора/выключателя
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
        
        // 2. Скорость вентилятора (только TYPE 1)
        #if DEVICE_TYPE == 1
        static uint16_t lastSpeedPercent = 0;
        if (fan.getSpeed() != lastSpeedPercent) {
          mqttManager.publishSpeed(fan.getSpeed());
          lastSpeedPercent = fan.getSpeed();
        }
        #endif
        
        // 3. Режим управления сенсором (только TYPE 1)
        #if DEVICE_TYPE == 1
        static bool lastSensorControlMode = false;
        if (config_get()->sensorControlMode != lastSensorControlMode) {
          mqttManager.publishSensorControlMode(config_get()->sensorControlMode);
          lastSensorControlMode = config_get()->sensorControlMode;
        }
        #endif
        
        // 4. Адаптивный режим (только TYPE 1)
        #if DEVICE_TYPE == 1
        static bool lastAdaptiveMode = false;
        if (fan.getAdaptiveMode() != lastAdaptiveMode) {
          mqttManager.publishAdaptiveMode(fan.getAdaptiveMode());
          lastAdaptiveMode = fan.getAdaptiveMode();
        }
        #endif
        
        // 5. Пороги температуры и влажности (только TYPE 1)
        #if DEVICE_TYPE == 1
        static float lastLowTemp = 0, lastHighTemp = 0, lastLowHum = 0, lastHighHum = 0;
        if (fabs(config_get()->lowTemp - lastLowTemp) > 0.01 ||
            fabs(config_get()->highTemp - lastHighTemp) > 0.01 ||
            fabs(config_get()->lowHum - lastLowHum) > 0.01 ||
            fabs(config_get()->highHum - lastHighHum) > 0.01) {
          mqttManager.publishThresholds(
            config_get()->lowTemp, 
            config_get()->highTemp, 
            config_get()->lowHum, 
            config_get()->highHum
          );
          lastLowTemp = config_get()->lowTemp;
          lastHighTemp = config_get()->highTemp;
          lastLowHum = config_get()->lowHum;
          lastHighHum = config_get()->highHum;
        }
        #endif
        
        // 6. Задержка отложенного включения (TYPE 1 и 3)
        #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
        static int lastDelaySeconds = -1;
        if (config_get()->delaySeconds != lastDelaySeconds) {
          mqttManager.publishDelaySec(config_get()->delaySeconds);
          lastDelaySeconds = config_get()->delaySeconds;
        }
        #endif
        
        // 7. Аварийное отключение (TYPE 1 и 3)
        #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
        static uint32_t lastMaxOnTime = 0;
        if (config_get()->maxOnTime != lastMaxOnTime) {
          mqttManager.publishMaxOnTime(config_get()->maxOnTime);
          lastMaxOnTime = config_get()->maxOnTime;
        }
        #endif
        
        // 8. Публикация датчиков при изменении (TYPE 1 и 2)
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
        
        // ========== HEARTBEAT (ONLINE + RSSI) ==========
        static unsigned long lastHeartbeat = 0;
        static bool initialConfigPublished = false;
        
        if (mqttManager.isConnected()) {
          // Публикация начальной конфигурации при первом подключении
          if (!initialConfigPublished) {
            #if DEVICE_TYPE == 1
            mqttManager.publishState(fan.getState());
            mqttManager.publishSpeed(fan.getSpeed());
            mqttManager.publishSensorControlMode(config_get()->sensorControlMode);
            mqttManager.publishAdaptiveMode(fan.getAdaptiveMode());
            mqttManager.publishThresholds(
              config_get()->lowTemp, 
              config_get()->highTemp, 
              config_get()->lowHum, 
              config_get()->highHum
            );
            
            #elif DEVICE_TYPE == 3
            mqttManager.publishState(switchActuator.getState());
            #endif
            
            #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
            mqttManager.publishDelaySec(config_get()->delaySeconds);
            mqttManager.publishMaxOnTime(config_get()->maxOnTime);
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
          
          // Heartbeat
          if (millis() - lastHeartbeat >= STATE_PUBLISH_INTERVAL_MS) {
            mqttManager.publishOnline();
            #if MQTT_PUBLISH_RSSI == 1
            mqttManager.publishRSSI(wifi_get_rssi());
            #endif
            lastHeartbeat = millis();
          }
        } else {
          // При потере соединения сбрасываем флаг, чтобы при переподключении всё опубликовать заново
          initialConfigPublished = false;
        }
        
        // Индикация LED по статусу MQTT
        // #if STATUS_LED_PIN > 0
        //   if (!mqttManager.isConnected()) {
        //     led_setMode(LED_MODE_MORZE_I);
        //   } else {
        //     led_setMode(LED_MODE_ON);
        //   }
        // #endif
        
      }
    #endif
  #endif

  // ========== WEB ==========
  #if WEB_ENABLED == 1
    #ifndef TEST_DISABLE_WEB
      web_update();
    #endif
  #endif

  #if STATUS_LED_PIN > 0
  LedMode newMode = LED_MODE_ON;

  // Приоритет: аварийное отключение > нет WiFi > нет MQTT > AP режим > всё хорошо
  #if DEVICE_TYPE == 1
  if (fan.isEmergencyStop()) {
      newMode = LED_MODE_MORZE_D;
  } else 
  
  #elif DEVICE_TYPE == 3
  if (switchActuator.isEmergencyStop()) {
      newMode = LED_MODE_MORZE_D;
  } else 
  #endif

  if (apMode) {
      newMode = LED_MODE_MORZE_S;
  } else if (!wifi_is_connected()) {
      newMode = LED_MODE_MORZE_E;
  } else if (!mqttManager.isConnected()) {
      newMode = LED_MODE_MORZE_I;
  } else {
      newMode = LED_MODE_ON;
  }

  led_setMode(newMode);
  #endif
}