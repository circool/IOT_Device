#include <Arduino.h>
#include "config.h"

#include "led.h"
#include "ansi.h"
#include "ota_check.h"

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

#if WIFI_ENABLED == 1
  #ifdef ESP32
    #include <WiFi.h>
  #elif defined(ESP8266)
    #include <ESP8266WiFi.h>
  #endif

  unsigned long lastWiFiCheck = 0;
  unsigned long wifiLostTime = 0;
  bool wifiConnecting = false;
  unsigned long wifiConnectStartTime = 0;
#endif

#if WEB_ENABLED == 1
  #include "web.h"
  WebServerClass server(80);
#endif

#if defined(ESP32) && WDT_ENABLED == 1
  #include <esp_task_wdt.h>  
#endif

// ======================== MQTT FUNCTIONS ========================
#if MQTT_ENABLED == 1
  #include "mqtt.h"
  unsigned long lastMQTTAttempt = 0;

  #if MQTT_PUBLISH_RESET_REASON == 1
    char lastResetReason[32] = "";

    #ifdef ESP8266
      void getResetReason() {
        struct rst_info *resetInfo = system_get_rst_info();
        uint8 reason = resetInfo->reason;
        
        switch(reason) {
          case REASON_DEFAULT_RST:
            strcpy(lastResetReason, "POWER_ON");
            break;
          case REASON_WDT_RST:
            strcpy(lastResetReason, "WATCHDOG_CRASH");
            break;
          case REASON_EXCEPTION_RST:
            strcpy(lastResetReason, "EXCEPTION_CRASH");
            break;
          case REASON_SOFT_WDT_RST:
            strcpy(lastResetReason, "SOFT_WDT_CRASH");
            break;
          case REASON_SOFT_RESTART:
            strcpy(lastResetReason, "SOFT_RESTART");
            break;
          case REASON_EXT_SYS_RST:
            strcpy(lastResetReason, "EXT_RESET");
            break;
          default:
            strcpy(lastResetReason, "UNKNOWN");
        }
      }
    #endif

    #ifdef ESP32
    void getResetReason() {
      esp_reset_reason_t reason = esp_reset_reason();
      
      switch(reason) {
        case ESP_RST_POWERON:
          strcpy(lastResetReason, "POWER_ON");
          break;
        case ESP_RST_EXT:
          strcpy(lastResetReason, "EXT_RESET");
          break;
        case ESP_RST_SW:
          strcpy(lastResetReason, "SOFT_RESTART");
          break;
        case ESP_RST_PANIC:
          strcpy(lastResetReason, "PANIC_CRASH");
          break;
        case ESP_RST_INT_WDT:
          strcpy(lastResetReason, "INT_WDT_CRASH");
          break;
        case ESP_RST_TASK_WDT:
          strcpy(lastResetReason, "TASK_WDT_CRASH");
          break;
        case ESP_RST_WDT:
          strcpy(lastResetReason, "WDT_CRASH");
          break;
        case ESP_RST_DEEPSLEEP:
          strcpy(lastResetReason, "DEEP_SLEEP_WAKE");
          break;
        default:
          strcpy(lastResetReason, "UNKNOWN");
      }
    }
    #endif
  #endif
#endif

// ======================== WATCHDOG FUNCTIONS ========================
#if WDT_ENABLED == 1
void wdt_init() {  
#if defined(ESP8266)
  ESP.wdtEnable(WDT_TIMER_MS);
  Serial.printf("[WDT] ESP8266 WDT enabled, timeout=%d ms\n", WDT_TIMER_MS);
  
#elif defined(ESP32)
  esp_task_wdt_init(WDT_TIMER_MS / 1000, true);
  esp_task_wdt_add(NULL);
  Serial.printf("[WDT] ESP32 task WDT enabled, timeout=%d ms\n", WDT_TIMER_MS);
#endif
}

void wdt_feed() {
#if defined(WDT_TEST)
  return;
#endif
#if defined(ESP8266)
  ESP.wdtFeed();
#elif defined(ESP32)
  esp_task_wdt_reset();
#endif
}
#endif

// ======================== HARDWARE RESET ========================
void checkResetButton() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  delay(50);
  
  if (digitalRead(RESET_PIN) == LOW) {
    #if DEBUG_ENABLED == 1
      Serial.println("\n[MAIN] Reset button pressed...");
      Serial.println("[MAIN] Hold for 3 seconds to confirm reset...");
    #endif
    unsigned long pressStart = millis();
    
    while (digitalRead(RESET_PIN) == LOW) {
      if (millis() - pressStart >= 3000) {
        #if DEBUG_ENABLED == 1
          Serial.println("[MAIN] Reset confirmed! Waiting for button release...");
        #endif

        while (digitalRead(RESET_PIN) == LOW) {
          delay(10);
        }
        
        #if LOG_CONFIG == 1
          Serial.println("[CONFIG] Clearing configuration...");
        #endif

        if (config_clear()) {
          #if LOG_CONFIG == 1
            Serial.println("[CONFIG] Configuration cleared. Restarting...");
          #endif
          delay(1000);
          ESP.restart();  
        } else {
          #if LOG_CONFIG == 1
            Serial.println("[CONFIG] Clear failed — not restarting");
          #endif
        }
        return;
      }
      delay(10);
    }
    #if DEBUG_ENABLED == 1
      Serial.println("[MAIN] Button released too early — reset cancelled");
    #endif
  }
}

// ======================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ АКТУАТОРОВ ========================
#if DEVICE_TYPE == 1
  FanActuator fan;
#elif DEVICE_TYPE == 3
  SwitchActuator switchActuator;
#endif

#if WIFI_ENABLED == 1
// ======================== WIFI ========================
#ifndef DEBUG_WIFI_ENABLED
  #define DEBUG_WIFI_ENABLED 0
#endif

void wifi_beginAsync() {
  #if STATUS_LED_PIN > 0
    led_setMode(LED_MODE_SLOW_BLINK);
  #endif
  
  if (strlen(config_get()->wifiSsid) == 0) {
    #if LOG_WIFI == 1
      Serial.println("[WIFI] No SSID configured");
    #endif
    return;
  }
  
  if (WiFi.status() == WL_CONNECTED) return;
  if (wifiConnecting) return;
  
  #if LOG_WIFI == 1
    Serial.printf("[WIFI] Starting async connection to %s\n", config_get()->wifiSsid);
  #endif
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_get()->wifiSsid, config_get()->wifiPassword);
  wifiConnecting = true;
  wifiConnectStartTime = millis();
  wifiLostTime = 0;
}

void wifi_checkAsync() {
  if (!wifiConnecting) return;
  
  wl_status_t status = WiFi.status();
  
  if (status == WL_CONNECTED) {
    wifiConnecting = false;
    wifiLostTime = 0;
    
    #if LOG_WIFI == 1
      Serial.printf(ANSI_BRIGHT_MAGENTA "[WIFI] Connected! IP: " ANSI_BOLD "%s" ANSI_RESET "\n", WiFi.localIP().toString().c_str());
    #endif
    
    #if STATUS_LED_PIN > 0
      #if MQTT_ENABLED == 1
        led_setMode(LED_MODE_FAST_BLINK);
      #else
        led_setMode(LED_MODE_ON);
      #endif
    #endif

    if (apMode) {
      WiFi.softAPdisconnect(true);
      apMode = false;
    }
    
  } else if (millis() - wifiConnectStartTime > WIFI_CONNECT_TIMEOUT_MS) {
    #if LOG_WIFI == 1
      Serial.print(ANSI_BRIGHT_RED);
      Serial.println("[WIFI] Connection timeout!");
      Serial.print(ANSI_RESET);
    #endif
    wifiConnecting = false;
    WiFi.disconnect();

    #if STATUS_LED_PIN > 0
      led_setMode(LED_MODE_SLOW_BLINK);
    #endif
  }
}

void checkWiFiFallbackToAP() {
  if (strlen(config_get()->wifiSsid) == 0) return;
  if (apMode) return;
  
  IPAddress ip = WiFi.localIP();
  bool hasValidIp = (ip != IPAddress(0,0,0,0));
  bool isConnected = (WiFi.status() == WL_CONNECTED && hasValidIp);
  
  if (isConnected) {
    wifiLostTime = 0;
    return;
  }
  
  if (!isConnected && !wifiConnecting) {
    if (wifiLostTime == 0) {
      wifiLostTime = millis();
      #if LOG_WIFI == 1
        Serial.print(ANSI_BRIGHT_RED);
        Serial.println("[WIFI] WiFi lost, starting fallback timer");
        Serial.print(ANSI_RESET);
      #endif
      
      #if STATUS_LED_PIN > 0
        led_setMode(LED_MODE_SLOW_BLINK);
      #endif
      
    } else if (millis() - wifiLostTime > AP_FALLBACK_TIMEOUT_MS) {
      #if LOG_WIFI == 1
        Serial.print(ANSI_BRIGHT_MAGENTA);
        Serial.printf("[WIFI] WiFi lost for %d ms, switching to AP mode\n", AP_FALLBACK_TIMEOUT_MS);
        Serial.print(ANSI_RESET);
      #endif
      
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(100);
      
      web_initAP();
      wifiLostTime = 0;
    }
  } else if (!isConnected && wifiConnecting) {
    wifiLostTime = millis();
  }
}
#endif

// ======================== SETUP ========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  #if DEBUG_ENABLED == 1
  delay(2000);
  Serial.println("\n\n\n=== SYSTEM INFO ===");
  
  #ifdef ESP32
    Serial.println("Platform: ESP32");
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    Serial.printf("Chip model: ESP32-%d\n", chip_info.model);
    Serial.printf("Chip revision: %d\n", chip_info.revision);
    Serial.printf("Cores: %d\n", chip_info.cores);
    Serial.printf("CPU frequency: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("Chip ID: %08X\n", (uint32_t)ESP.getEfuseMac());
    uint32_t flashSize = ESP.getFlashChipSize();
    Serial.printf("Flash chip size: %u bytes (%u MB)\n", flashSize, flashSize / (1024 * 1024));
    Serial.printf("Flash chip speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("Flash chip mode: %d\n", ESP.getFlashChipMode());
    #ifdef CONFIG_SPIRAM_SUPPORT
      Serial.printf("PSRAM size: %u bytes\n", ESP.getPsramSize());
      Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    #else
      Serial.println("PSRAM: not supported/enabled");
    #endif

    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Minimum free heap: %u bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Maximum allocatable heap: %u bytes\n", ESP.getMaxAllocHeap());
    Serial.printf("ESP-IDF version: %s\n", esp_get_idf_version());

  #elif defined(ESP8266)
    Serial.println("Platform: ESP8266");
    Serial.printf("Chip ID: %08X\n", ESP.getChipId());
    Serial.printf("Core version: %s\n", ESP.getCoreVersion().c_str());
    Serial.printf("CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    uint32_t flashSize = ESP.getFlashChipSize();
    Serial.printf("Flash chip size: %u bytes (%u MB)\n", flashSize, flashSize / (1024 * 1024));
    uint32_t realFlashSize = ESP.getFlashChipRealSize();
    if (realFlashSize > 0 && realFlashSize != flashSize) {
      Serial.printf("\033[31mReal flash chip size: %u bytes (%u MB)\033[0m\n", realFlashSize, realFlashSize / (1024 * 1024));
    }
    Serial.printf("\nFlash chip speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("Flash chip mode: %d (0=QIO, 1=QOUT, 2=DIO, 3=DOUT)\n", ESP.getFlashChipMode());
    Serial.printf("SDK version: %s\n", system_get_sdk_version());
  #endif
  
  Serial.println("===================\n");
  Serial.printf("Sketch size: %u bytes\n", ESP.getSketchSize());
  Serial.printf("Free sketch space: %u bytes\n", ESP.getFreeSketchSpace());
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Firmware ver. %s\n", VERSION);
  #if MQTT_PUBLISH_RESET_REASON == 1
    getResetReason();
    Serial.printf("Reset reason: %s\n", lastResetReason);
  #endif
  
  Serial.println("===================\n");
#endif
  
  Serial.println("\n==========================================");
  Serial.printf("Device starting with %s mode\n", DEVICE_PREFIX);
  Serial.println("==========================================");
  
  // ========== ИНИЦИАЛИЗАЦИЯ ПОДСИСТЕМ ==========
  #if STATUS_LED_PIN > 0
    led_init();
    led_setMode(LED_MODE_SLOW_BLINK);
  #endif

  #if WDT_ENABLED == 1
    wdt_init();
  #endif

  checkResetButton();
  config_init();

  #if OTA_ENABLED == 1
    bool otaCapable = isOtaAvailable();
    #if WEB_ENABLED == 1
      web_setOtaAvailable(otaCapable);
    #endif
    #if LOG_OTA == 1
      Serial.printf("[OTA] Available: %s\n", otaCapable ? "YES" : "NO");
    #endif
  #endif

  #if DEBUG_WIFI_ENABLED == 1
    #if defined(ESP32) && WDT_ENABLED == 1
      esp_task_wdt_delete(NULL);
    #endif
    Serial.println("[WIFI] Scanning...");
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      bool isTarget = (ssid == config_get()->wifiSsid);
      if (isTarget) {
        Serial.printf("[WIFI] %s (RSSI: %d) " ANSI_BRIGHT_GREEN "<<< TARGET" ANSI_RESET "\n", ssid.c_str(), WiFi.RSSI(i));
      } else {
        Serial.printf("[WIFI] %s (RSSI: %d)\n", ssid.c_str(), WiFi.RSSI(i));
      }
    }
    WiFi.scanDelete();
    #if defined(ESP32) && WDT_ENABLED == 1
      esp_task_wdt_add(NULL);
    #endif
  #endif

  #if DEBUG_ENABLED == 1
    config_print();
  #endif

  bool hasValidConfig = (config_isValid() && strlen(config_get()->wifiSsid) > 0);

  if (hasValidConfig) {
    #if LOG_CONFIG == 1
      Serial.println("[CONFIG] Normal mode - starting with saved config");
    #endif

    // ========== MQTT ИНИЦИАЛИЗАЦИЯ ==========
    #if MQTT_ENABLED == 1
      #ifndef TEST_DEVICE_MQTT
        mqttManager.begin(config_get()->mqttBroker, 
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
          config_setSensorControlMode(enabled);
          if (!enabled) {
            fan.setAdaptiveMode(false);
          }
        });
        
        mqttManager.onAdaptiveModeCommand([](bool enabled) {
          if (enabled && !config_get()->sensorControlMode) {
            #if DEBUG_ENABLED == 1
              Serial.println("[MQTT] Cannot enable adaptive mode - sensor control mode is OFF");
            #endif
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
        
        mqttManager.onMaxOnTimeCommand([](uint32_t maxOnTime) { 
          config_setMaxOnTime(maxOnTime); 
        });
        
        #if MQTT_RESET_ENABLED == 1
        mqttManager.onResetCommand([]() {
          #if LOG_MQTT == 1
            Serial.println("[MQTT] Resetting due MQTT RESET");
          #endif
          mqttManager.disconnect();
          config_clear();
          delay(1000);
          ESP.restart();
        });
        #endif
      #endif
    #endif

    // ========== ДАТЧИК ==========
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
      #ifndef TEST_DEVICE_SENSOR
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
            #if DEBUG_ENABLED == 1
              Serial.println("[SENSOR] Not found - switching to MANUAL mode");
            #endif
          }
        #endif
      #endif
    #endif

    // ========== ВЕНТИЛЯТОР ==========
    #if DEVICE_TYPE == 1
      #ifndef TEST_DEVICE_FAN
        fan.init(SWITCH_PIN, RELAY_ON_LEVEL, config_get()->bootState, config_get()->speedPercent);
        fan.setAdaptiveMode(config_get()->adaptiveMode);
      #endif
    #endif

    // ========== ВЫКЛЮЧАТЕЛЬ ==========
    #if DEVICE_TYPE == 3
      #ifndef TEST_DEVICE_SWITCH
        switchActuator.init(SWITCH_PIN, RELAY_ON_LEVEL, config_get()->bootState);
      #endif
    #endif

    // ========== WIFI ==========
    #if WIFI_ENABLED == 1
      #if DEBUG_WIFI_ENABLED == 1
        #ifdef ESP8266
          WiFi.setSleepMode(WIFI_NONE_SLEEP);
          WiFi.setPhyMode(WIFI_PHY_MODE_11G);
          delay(100);
        #elif defined(ESP32)
          WiFi.setSleep(false);
          delay(100);
        #endif
      #endif
      #ifndef TEST_DEVICE_WIFI
        wifi_beginAsync();
      #endif
    #endif

    // ========== WEB ==========
    #if WEB_ENABLED == 1
      #ifndef TEST_DEVICE_WEB
        web_init();
      #endif
    #endif

  } else {
    #if LOG_CONFIG == 1
      Serial.println("[CONFIG] Configuration mode - starting AP for setup");
    #endif
    #if AP_ENABLED == 1
      #ifndef TEST_DEVICE_WIFI
        web_initAP();
      #endif
    #endif
  }
}

// ======================== LOOP ========================
void loop() {
  #if USE_DEVICE_CLASS == 1
    device.loop();
  #else
  
    #if WDT_ENABLED == 1
      wdt_feed();
    #endif

    checkResetButton();

    #if STATUS_LED_PIN > 0
      led_update();
    #endif

    #if WEB_ENABLED == 1
      if (!config_isValid() || strlen(config_get()->wifiSsid) == 0) {
        web_update();
        delay(100);
        return;
      }
    #endif

    // ========== ДАТЧИК ==========
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
      #ifndef TEST_DEVICE_SENSOR
        sensor_update();
      #endif
    #endif

    // ========== ВЕНТИЛЯТОР ==========
    #if DEVICE_TYPE == 1
      #ifndef TEST_DEVICE_FAN
        fan.update();
      #endif
    #endif

    // ========== ВЫКЛЮЧАТЕЛЬ ==========
    #if DEVICE_TYPE == 3
      #ifndef TEST_DEVICE_SWITCH
        switchActuator.update();
      #endif
    #endif

    // ========== WIFI ==========
    #if WIFI_ENABLED == 1
      #ifndef TEST_DEVICE_WIFI
        wifi_checkAsync();
      #endif
    #endif

    #if AP_ENABLED == 1
      #ifndef TEST_DEVICE_WIFI
        checkWiFiFallbackToAP();
      #endif
    #endif

    // ========== MQTT ==========
    #if MQTT_ENABLED == 1
      #ifndef TEST_DEVICE_MQTT
        if (WiFi.status() == WL_CONNECTED) {
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
              mqttManager.publishResetReason(lastResetReason);
              #endif
              
              initialConfigPublished = true;
              #if LOG_MQTT == 1
              Serial.println("[MQTT] Initial config published");
              #endif
            }
            
            // Heartbeat
            if (millis() - lastHeartbeat >= STATE_PUBLISH_INTERVAL_MS) {
              mqttManager.publishOnline();
              #if MQTT_PUBLISH_RSSI == 1
              mqttManager.publishRSSI(WiFi.RSSI());
              #endif
              lastHeartbeat = millis();
            }
          } else {
            // При потере соединения сбрасываем флаг, чтобы при переподключении всё опубликовать заново
            initialConfigPublished = false;
          }
          
          // Индикация LED по статусу MQTT
          #if STATUS_LED_PIN > 0
            if (!mqttManager.isConnected()) {
              led_setMode(LED_MODE_FAST_BLINK);
            } else {
              led_setMode(LED_MODE_ON);
            }
          #endif
          
        } else {
          #if STATUS_LED_PIN > 0
            led_setMode(LED_MODE_SLOW_BLINK);
          #endif
        }
      #endif
    #endif

    // ========== WEB ==========
    #if WEB_ENABLED == 1
      #ifndef TEST_DEVICE_WEB
        web_update();
      #endif
    #endif
  #endif
}