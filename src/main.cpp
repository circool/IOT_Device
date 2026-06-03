#include <Arduino.h>
#include "config.h"
#include "led.h"
#include "ansi.h"
#include "ota_check.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
#include "sensor.h"
#endif

#if DEVICE_TYPE == 1
  #include "fan.h"
#endif

#if DEVICE_TYPE == 3
  #include "switch.h"
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
  #ifdef ESP32
    AsyncWebServer server(80);
  #elif defined(ESP8266)
    ESP8266WebServer server(80);
  #endif
#endif

#if defined(ESP32)  && WDT_ENABLED==1
  #include <esp_task_wdt.h>  
#endif



// ======================== MQTT FUNCTIONS ========================
#if MQTT_ENABLED==1
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
  esp_task_wdt_init(WDT_TIMER_MS / 1000, true);  // конвертация в секунды
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


#if WIFI_ENABLED == 1
// ======================== WIFI ========================
#ifndef DEBUG_WIFI_ENABLED
  #define DEBUG_WIFI_ENABLED 0
#endif

void wifi_beginAsync() {
  
  #if STATUS_LED_PIN > 0
    led_setMode(LED_MODE_SLOW_BLINK);  // Пытаемся подключиться к WiFi
  #endif
  
  if (strlen(config.wifiSsid) == 0) {
    #if LOG_WIFI == 1
      Serial.println("[WIFI] No SSID configured");
    #endif
    return;
  }
  
  if (WiFi.status() == WL_CONNECTED) return;
  if (wifiConnecting) return;
  
  #if LOG_WIFI == 1
    Serial.printf("[WIFI] Starting async connection to %s\n", config.wifiSsid);
  #endif
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSsid, config.wifiPassword);
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
        led_setMode(LED_MODE_FAST_BLINK);  // WiFi есть, ждём MQTT
      #else
        led_setMode(LED_MODE_ON);  // Только WiFi, горим постоянно
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
      led_setMode(LED_MODE_SLOW_BLINK);  // Не удалось подключиться
    #endif
  }
}

// ======================== AP ========================
void checkWiFiFallbackToAP() {
  if (strlen(config.wifiSsid) == 0) return;
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
        led_setMode(LED_MODE_SLOW_BLINK);  // Потеря WiFi
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

#if MQTT_ENABLED == 1
void publishSensorData() {
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (sensor_isOk()) {
    static float lastTemp = 0, lastHum = 0;
    const float EPSILON = 0.05;
    if (fabs(currentTemp - lastTemp) > EPSILON || fabs(currentHum - lastHum) > EPSILON) {
        mqttManager.publishSensor(currentTemp, currentHum);
        lastTemp = currentTemp;
        lastHum = currentHum;
    }
  }
  #endif
}
#endif

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
    // PSRAM информация
    #ifdef CONFIG_SPIRAM_SUPPORT
      Serial.printf("PSRAM size: %u bytes\n", ESP.getPsramSize());
      Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    #else
      Serial.println("PSRAM: not supported/enabled");
    #endif

    // Heap информация
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
    
    // Информация о flash
    uint32_t flashSize = ESP.getFlashChipSize();
    Serial.printf("Flash chip size: %u bytes (%u MB)\n", flashSize, flashSize / (1024 * 1024));
    // Реальная flash память (если доступно)
    uint32_t realFlashSize = ESP.getFlashChipRealSize();
    if (realFlashSize > 0 && realFlashSize != flashSize) {
      Serial.printf("\033[31mReal flash chip size: %u bytes (%u MB)\033[0m\n", realFlashSize, realFlashSize / (1024 * 1024));
    }

    Serial.printf("\nFlash chip speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("Flash chip mode: %d (0=QIO, 1=QOUT, 2=DIO, 3=DOUT)\n", ESP.getFlashChipMode());
    
    
    
    // Версия SDK
    Serial.printf("SDK version: %s\n", system_get_sdk_version());
  #endif
  
  Serial.println("===================\n");
  Serial.printf("Sketch size: %u bytes\n", ESP.getSketchSize());
  Serial.printf("Free sketch space: %u bytes\n", ESP.getFreeSketchSpace());
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Firmware ver. %s\n",VERSION);
  #if MQTT_PUBLISH_RESET_REASON == 1
    getResetReason();
    Serial.printf("Reset reason: %s\n", lastResetReason);
  #endif
  
  Serial.println("===================\n");
#endif
  

  Serial.println("\n==========================================");
  Serial.printf("Device starting with %s mode\n", DEVICE_PREFIX);
  Serial.println("==========================================");

  #if STATUS_LED_PIN > 0
    led_init();
    led_setMode(LED_MODE_SLOW_BLINK);  // Начальный режим - ожидание конфигурации
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
    esp_task_wdt_delete(NULL);  // временно отключаем WDT
  #endif
  Serial.println("[WIFI] Scanning...");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    bool isTarget = (ssid == config.wifiSsid);
    
    if (isTarget) {
        Serial.printf("[WIFI] %s (RSSI: %d) " ANSI_BRIGHT_GREEN "<<< TARGET" ANSI_RESET "\n", 
                      ssid.c_str(), WiFi.RSSI(i));
    } else {
        Serial.printf("[WIFI] %s (RSSI: %d)\n", 
                      ssid.c_str(), WiFi.RSSI(i));
    }
    #if defined(ESP32) && WDT_ENABLED == 1
      esp_task_wdt_add(NULL);  // снова включаем WDT
    #endif
  }
  WiFi.scanDelete();
  #endif
  
  #if DEBUG_ENABLED==1
    config_print();  
  #endif
  
  bool hasValidConfig = (configValid && strlen(config.wifiSsid) > 0);
  
  if (hasValidConfig) {
    #if LOG_CONFIG == 1
      Serial.println("[CONFIG] Normal mode - starting with saved config");
    #endif    
    
    #if MQTT_ENABLED==1
      // Инициализация MQTT
      mqttManager.begin(config);
      
      // Настройка колбэков для MQTT команд
      
      #if DEVICE_TYPE == 1
      
      mqttManager.onStateCommand([](bool state) {
        fan_set(state);
      });

      
      mqttManager.onSpeedCommand([](int speed) {
        
        if (config.adaptiveMode) {
          config.adaptiveMode = false;
          adaptiveActive = false;
          mqttManager.publishAdaptiveMode(false);
        }
        if (speed == 0 || speed < MIN_SPEED_PERCENT) {
          config.speedPercent = speed;
          fan_set(false);
        } else {
          config.speedPercent = speed;
          if (fanOn && !startingPulseActive) {
            fan_applySpeed(config.speedPercent);
          }
        }
        mqttManager.publishSpeed(config.speedPercent);
      });

      
      mqttManager.onSensorControlModeCommand([](bool enabled) {
        fan_setOverrideMode(enabled);
        mqttManager.publishSensorControlMode(config.sensorControlMode);
      });
      
      
      mqttManager.onAdaptiveModeCommand([](bool enabled) {
        if (enabled && !config.sensorControlMode) {
          #if DEBUG_ENABLED == 1
            Serial.println("[MQTT] Cannot enable adaptive mode - sensor control mode is OFF");
          #endif
          mqttManager.publishAdaptiveMode(false);
          return;
        }
        
        config.adaptiveMode = enabled;
        
        if (fanOn && config.sensorControlMode && config.adaptiveMode && sensor_isOk()) {
          adaptiveActive = true;
          baseTemp = currentTemp;
          baseHum = currentHum;
          lastAdaptiveCheck = millis();
        } else if (!config.adaptiveMode) {
          adaptiveActive = false;
        }
        mqttManager.publishAdaptiveMode(config.adaptiveMode);
      });
      
      
      mqttManager.onLowTempCommand([](float value) {
        config.lowTemp = value;
        mqttManager.publishLowTemp(config.lowTemp);
      });
        
      mqttManager.onHighTempCommand([](float value) {
        config.highTemp = value;
        mqttManager.publishHighTemp(config.highTemp);
      });

        
      mqttManager.onLowHumCommand([](float value) {
        config.lowHum = value;
        mqttManager.publishLowHum(config.lowHum);
      });
      
      
      mqttManager.onHighHumCommand([](float value) {
        config.highHum = value;
        mqttManager.publishHighHum(config.highHum);
      }); 
      
      #endif


      #if DEVICE_TYPE == 3  
      mqttManager.onStateCommand([](bool state) {
        switch_set(state);
      });
      #endif  
        
      #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
      mqttManager.onDelaySecCommand([](int delaySec) {
        int oldDelay = config.delaySeconds;
        config.delaySeconds = delaySec;
        if (delayActive) {
          long remaining = (delayTimer - millis()) + (delaySec - oldDelay) * 1000L;
          if (remaining > 0) {
            delayTimer = millis() + remaining;
          } else {
            delayActive = false;
            #if DEVICE_TYPE == 1
              fan_set(true);
            #endif
            #if DEVICE_TYPE == 3
              switch_set(true);
            #endif

          }
        }
        mqttManager.publishDelaySec(config.delaySeconds);
      });
      
      mqttManager.onMaxOnTimeCommand([](uint32_t maxOnTime) {
        config.maxOnTime = maxOnTime;
        mqttManager.publishMaxOnTime(config.maxOnTime);
      });
      #endif
      
      #if MQTT_RESET_ENABLED == 1
      mqttManager.onResetCommand([]() {
        #if LOG_MQTT == 1 
          Serial.println("[MQTT] Resetting due MQTT RESET");
        #endif
        mqttManager.disconnect();
        config_clear();
        delay(1000);
        #if DEBUG_ENABLED == 1
          Serial.println("[DEBUG] Restarting...");
        #endif
        ESP.restart();
      });
      #endif
    
    #endif

    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
      sensor_init();
      
      #if DEVICE_TYPE == 1
      if (!sensorOk && (
          #if SENSOR_TYPE == 1
          strcmp(sensorError, "AHT10 not found") == 0
          #elif SENSOR_TYPE == 2
          strcmp(sensorError, "DHT read failed (NaN)") == 0
          #endif
      )) {
        config.sensorControlMode = false;
        config.adaptiveMode = false;
        #if DEBUG_ENABLED == 1
        Serial.println("[SENSOR] Not found - switching to MANUAL mode");
        #endif
      }
      #endif //DEVICE_TYPE == 1   
    #endif //DEVICE_TYPE == 1 || DEVICE_TYPE == 2

    #if DEVICE_TYPE == 1 
      fan_init();
    #endif //DEVICE_TYPE == 1
    
    #if DEVICE_TYPE == 3 
      switch_init();
    #endif //DEVICE_TYPE == 3

    #if WIFI_ENABLED == 1
      #if DEBUG_WIFI_ENABLED == 1
        #ifdef ESP8266
          // ESP8266 специфичные настройки
          WiFi.setSleepMode(WIFI_NONE_SLEEP);
          WiFi.setPhyMode(WIFI_PHY_MODE_11G);
          // WiFi.setOutputPower(15.0);
          delay(100);
        #elif defined(ESP32)
          // ESP32 специфичные настройки
          WiFi.setSleep(false);  // отключаем энергосбережение
          // WiFi.setTxPower(WIFI_POWER_19_5dBm);  // опционально
          delay(100);
        #endif
      #endif //DEBUG_WIFI_ENABLED == 1

      wifi_beginAsync();
    #endif //WIFI_ENABLED == 1

    #if WEB_ENABLED == 1
      web_init();
    #endif
    
  } else {
    
    #if LOG_CONFIG == 1
      Serial.println("[CONFIG] Configuration mode - starting AP for setup");
    #endif
    
    #if AP_ENABLED == 1
      web_initAP();
    #endif
  }
}

void loop() {
  
  #if WDT_ENABLED == 1
    wdt_feed();
  #endif

  checkResetButton();

  #if STATUS_LED_PIN > 0
    led_update();
  #endif

  #if WEB_ENABLED == 1
    if (!configValid || strlen(config.wifiSsid) == 0) {
      web_update();
      delay(100);
      return;
    }
  #endif

  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    sensor_read();
  #endif
  
  #if DEVICE_TYPE == 1
    fan_update();
  #endif
  
  #if DEVICE_TYPE == 3
    switch_update();
  #endif

  #if WIFI_ENABLED == 1
    wifi_checkAsync();   
  #endif

  #if AP_ENABLED == 1
    checkWiFiFallbackToAP();
  #endif

  #if MQTT_ENABLED == 1
    
    if (WiFi.status() == WL_CONNECTED) {
      mqttManager.process();
      publishSensorData();
      
      // Управление LED
      #if STATUS_LED_PIN > 0
        if (!mqttManager.isConnected()) {
          led_setMode(LED_MODE_FAST_BLINK);
        } else {
          led_setMode(LED_MODE_ON);
        }
      #endif //STATUS_LED_PIN > 0
      
      // Heartbeat
      static unsigned long lastHeartbeat = 0;
      if (mqttManager.isConnected()) {
        if (millis() - lastHeartbeat >= STATE_PUBLISH_INTERVAL_MS) {
          mqttManager.publishOnline();
          #if MQTT_PUBLISH_RSSI == 1
            mqttManager.publishRSSI();
          #endif
          lastHeartbeat = millis();
        }
      }
    } else {
      #if STATUS_LED_PIN > 0
        led_setMode(LED_MODE_SLOW_BLINK);
      #endif
    }
    
  #endif //MQTT_ENABLED == 1

  #if WEB_ENABLED == 1
    web_update();
  #endif
  
  delay(50);
}