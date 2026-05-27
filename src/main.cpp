#include <Arduino.h>
#include "config.h"

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
void wifi_beginAsync() {
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
      Serial.println("[WIFI] Connected! IP: " + WiFi.localIP().toString());
    #endif
    
    if (apMode) {
      WiFi.softAPdisconnect(true);
      apMode = false;
    }
    
  } else if (millis() - wifiConnectStartTime > WIFI_CONNECT_TIMEOUT_MS) {  // ← исправлено: магическое число заменено на константу
    #if LOG_WIFI == 1
      Serial.println("[WIFI] Connection timeout");
    #endif
    wifiConnecting = false;
    WiFi.disconnect();
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
        Serial.println("[WIFI] WiFi lost, starting fallback timer");
      #endif
    } else if (millis() - wifiLostTime > AP_FALLBACK_TIMEOUT_MS) {
      #if LOG_WIFI == 1
        Serial.printf("[WIFI] WiFi lost for %d ms, switching to AP mode\n", AP_FALLBACK_TIMEOUT_MS);
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
    Serial.println("\n\n\n=== SYSTEM INFO ===");
    #ifdef ESP32
      Serial.println("Platform: ESP32");
    #elif defined(ESP8266)
      Serial.println("Platform: ESP8266");
    #else
      Serial.println("Platform: Unknown");
    #endif

    Serial.printf("Sketch size: %u bytes\n", ESP.getSketchSize());
    Serial.printf("Free sketch space: %u bytes\n", ESP.getFreeSketchSpace());
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());

    Serial.printf("Actual DEVICE_TYPE value: %d\n", DEVICE_TYPE);
    Serial.printf("DEVICE_PREFIX: %s\n", DEVICE_PREFIX);

    #if MQTT_PUBLISH_RESET_REASON == 1
      getResetReason();
      Serial.printf("Reset reason: %s\n", lastResetReason);
    #endif
    Serial.println("===================\n");
  #endif
  

  Serial.println("\n==========================================");
  Serial.printf("Device starting with %s mode\n", DEVICE_PREFIX);
  Serial.println("==========================================");
  
  #if WDT_ENABLED == 1
    wdt_init();
  #endif
  
  checkResetButton();
  config_init();
  config_print();  
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
    #endif
    
    #if DEVICE_TYPE == 1 
      fan_init();
    #endif
    
    #if DEVICE_TYPE == 3 
      switch_init();
    #endif

    #if WIFI_ENABLED == 1
      wifi_beginAsync();
    #endif

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
  #elif SOFT_WDT_ENABLED
    //   // Watchdog для loop() - если loop завис на время больше STATE_PUBLISH_INTERVAL_MS * LOOP_WATCHDOG_MULTIPLIER
    //   static unsigned long lastLoop = 0;
    //   if (millis() - lastLoop > STATE_PUBLISH_INTERVAL_MS * LOOP_WATCHDOG_MULTIPLIER) {
    //   Serial.printf("[SOFT WDT] Loop stuck! Resetting! (elapsed=%lu threshold=%lu lastLoop=%lu)\n", 
    //                 millis() - lastLoop, 
    //                 STATE_PUBLISH_INTERVAL_MS * LOOP_WATCHDOG_MULTIPLIER, 
    //                 lastLoop);
    //     ESP.restart();
    //   }
    //   lastLoop = millis();
    // #endif
  #endif

  checkResetButton();

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

  #if MQTT_ENABLED==1
  if (WiFi.status() == WL_CONNECTED) {
    mqttManager.process();
    publishSensorData();
    
    // Периодический heartbeat: Online + RSSI (раз в STATE_PUBLISH_INTERVAL_MS)
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= STATE_PUBLISH_INTERVAL_MS) {
      mqttManager.publishOnline();
      
      #if MQTT_PUBLISH_RSSI == 1
        mqttManager.publishRSSI();
      #endif
      
      lastHeartbeat = millis();
    }
  }
  #endif

  #if WEB_ENABLED == 1
    web_update();
  #endif
  
  
  delay(50);
  
}