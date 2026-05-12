#include "mqtt.h"
#include "config.h"
#include "fan.h"
#include "sensor.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

char devicePrefix[24];
char lastWillTopic[48];

// Общие для всех
char resetControlTopic[56];
char onlineTopic[56];

// Общие для TYPE 1 и TYPE 3
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
char stateTopic[56];
char controlTopic[56];
char pwmDutyStateTopic[56];
char pwmDutyControlTopic[56];
char adaptiveModeStateTopic[56];
char adaptiveModeControlTopic[56];
char delaySecStateTopic[56];
char delaySecControlTopic[56];
char sensorControlModeStateTopic[56];
char sensorControlModeControlTopic[56];
char maxOnTimeStateTopic[56];
char maxOnTimeControlTopic[56];
#endif

// Общие для TYPE 1 и TYPE 2
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
char tempStateTopic[56];
char humStateTopic[56];
#endif

// Только TYPE 1
#if DEVICE_TYPE == 1
char lowTempStateTopic[56];
char highTempStateTopic[56];
char lowHumStateTopic[56];
char highHumStateTopic[56];
char lowTempControlTopic[56];
char highTempControlTopic[56];
char lowHumControlTopic[56];
char highHumControlTopic[56];
char errorTopic[56];
#endif

unsigned long lastMqttReconnect = 0;

void mqtt_setupTopics(const char* prefix) {
  strcpy(devicePrefix, prefix);
  
  // Общие для всех
  snprintf(onlineTopic, sizeof(onlineTopic), "%s/status", devicePrefix);
  snprintf(resetControlTopic, sizeof(resetControlTopic), "%s/c/system/reset", devicePrefix);
  
  // TYPE 1 и TYPE 3: исполнительное устройство
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  #if DEVICE_TYPE == 1
  snprintf(stateTopic, sizeof(stateTopic), "%s/fan/state", devicePrefix);
  snprintf(controlTopic, sizeof(controlTopic), "%s/c/fan/state", devicePrefix);
  snprintf(pwmDutyStateTopic, sizeof(pwmDutyStateTopic), "%s/fan/pwmDuty", devicePrefix);
  snprintf(pwmDutyControlTopic, sizeof(pwmDutyControlTopic), "%s/c/fan/pwmDuty", devicePrefix);
  snprintf(adaptiveModeStateTopic, sizeof(adaptiveModeStateTopic), "%s/fan/adaptiveMode", devicePrefix);
  snprintf(adaptiveModeControlTopic, sizeof(adaptiveModeControlTopic), "%s/c/fan/adaptiveMode", devicePrefix);
  snprintf(delaySecStateTopic, sizeof(delaySecStateTopic), "%s/fan/delaySec", devicePrefix);
  snprintf(delaySecControlTopic, sizeof(delaySecControlTopic), "%s/c/fan/delaySec", devicePrefix);
  snprintf(sensorControlModeStateTopic, sizeof(sensorControlModeStateTopic), "%s/fan/sensorControlMode", devicePrefix);
  snprintf(sensorControlModeControlTopic, sizeof(sensorControlModeControlTopic), "%s/c/fan/sensorControlMode", devicePrefix);
  snprintf(maxOnTimeStateTopic, sizeof(maxOnTimeStateTopic), "%s/fan/maxOnTime", devicePrefix);
  snprintf(maxOnTimeControlTopic, sizeof(maxOnTimeControlTopic), "%s/c/fan/maxOnTime", devicePrefix);
  #elif DEVICE_TYPE == 3
  snprintf(stateTopic, sizeof(stateTopic), "%s/switch/state", devicePrefix);
  snprintf(controlTopic, sizeof(controlTopic), "%s/c/switch/state", devicePrefix);
  snprintf(pwmDutyStateTopic, sizeof(pwmDutyStateTopic), "%s/switch/pwmDuty", devicePrefix);
  snprintf(pwmDutyControlTopic, sizeof(pwmDutyControlTopic), "%s/c/switch/pwmDuty", devicePrefix);
  snprintf(delaySecStateTopic, sizeof(delaySecStateTopic), "%s/switch/delaySec", devicePrefix);
  snprintf(delaySecControlTopic, sizeof(delaySecControlTopic), "%s/c/switch/delaySec", devicePrefix);
  snprintf(sensorControlModeStateTopic, sizeof(sensorControlModeStateTopic), "%s/switch/sensorControlMode", devicePrefix);
  snprintf(sensorControlModeControlTopic, sizeof(sensorControlModeControlTopic), "%s/c/switch/sensorControlMode", devicePrefix);
  snprintf(maxOnTimeStateTopic, sizeof(maxOnTimeStateTopic), "%s/switch/maxOnTime", devicePrefix);
  snprintf(maxOnTimeControlTopic, sizeof(maxOnTimeControlTopic), "%s/c/switch/maxOnTime", devicePrefix);
  #endif
  #endif
  
  // TYPE 1 и TYPE 2: датчик
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  snprintf(tempStateTopic, sizeof(tempStateTopic), "%s/sensor/temperature", devicePrefix);
  snprintf(humStateTopic, sizeof(humStateTopic), "%s/sensor/humidity", devicePrefix);
  #endif
  
  // TYPE 1: пороги датчика
  #if DEVICE_TYPE == 1
  snprintf(lowTempStateTopic, sizeof(lowTempStateTopic), "%s/sensor/lowTemp", devicePrefix);
  snprintf(highTempStateTopic, sizeof(highTempStateTopic), "%s/sensor/highTemp", devicePrefix);
  snprintf(lowHumStateTopic, sizeof(lowHumStateTopic), "%s/sensor/lowHum", devicePrefix);
  snprintf(highHumStateTopic, sizeof(highHumStateTopic), "%s/sensor/highHum", devicePrefix);
  snprintf(lowTempControlTopic, sizeof(lowTempControlTopic), "%s/c/sensor/lowTemp", devicePrefix);
  snprintf(highTempControlTopic, sizeof(highTempControlTopic), "%s/c/sensor/highTemp", devicePrefix);
  snprintf(lowHumControlTopic, sizeof(lowHumControlTopic), "%s/c/sensor/lowHum", devicePrefix);
  snprintf(highHumControlTopic, sizeof(highHumControlTopic), "%s/c/sensor/highHum", devicePrefix);
  snprintf(errorTopic, sizeof(errorTopic), "%s/error", devicePrefix);
  #endif
  
  strcpy(lastWillTopic, onlineTopic);
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Topics configured with prefix: %s\n", devicePrefix);
  #endif
}

void mqtt_publishState() {
  if (!mqttClient.connected()) return;
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  bool realState = fan_getRealState();
  mqttClient.publish(stateTopic, realState ? "ON" : "OFF");
  mqttClient.publish(sensorControlModeStateTopic, config.sensorControlMode ? "1" : "0");
  #endif
  
  #ifdef DEBUG_MQTT
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    Serial.printf("[MQTT] State published: state=%s, sensorControlMode=%s\n", 
                  fan_getRealState() ? "ON" : "OFF", 
                  config.sensorControlMode ? "1" : "0");
    #endif
  #endif
}

void mqtt_publishConfig() {
  if (!mqttClient.connected()) return;
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  mqttClient.publish(pwmDutyStateTopic, String(config.pwmDutyPercent).c_str());
  #if DEVICE_TYPE == 1
  mqttClient.publish(adaptiveModeStateTopic, config.adaptiveMode ? "1" : "0");
  #endif
  mqttClient.publish(delaySecStateTopic, String(config.delaySeconds).c_str());
  mqttClient.publish(maxOnTimeStateTopic, String(config.maxOnTime).c_str());
  mqttClient.publish(sensorControlModeStateTopic, config.sensorControlMode ? "1" : "0");
  #endif
  
  #if DEVICE_TYPE == 1
  mqttClient.publish(lowTempStateTopic, String(config.lowTemp).c_str());
  mqttClient.publish(highTempStateTopic, String(config.highTemp).c_str());
  mqttClient.publish(lowHumStateTopic, String(config.lowHum).c_str());
  mqttClient.publish(highHumStateTopic, String(config.highHum).c_str());
  #endif
  
  #ifdef DEBUG_MQTT
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    Serial.printf("[MQTT] Config published: pwmDuty=%d%%, adaptive=%d, delay=%d, maxOnTime=%d, sensorControlMode=%s",
                  config.pwmDutyPercent,
                  #if DEVICE_TYPE == 1
                  config.adaptiveMode ? 1 : 0,
                  #else
                  0,
                  #endif
                  config.delaySeconds,
                  config.maxOnTime,
                  config.sensorControlMode ? "ON" : "OFF");
    #endif
    #if DEVICE_TYPE == 1
    Serial.printf(", T(%.1f-%.1f), H(%.1f-%.1f)\n",
                  config.lowTemp, config.highTemp, config.lowHum, config.highHum);
    #else
    Serial.println();
    #endif
  #endif
}

void mqtt_publishSensor() {
  if (!mqttClient.connected()) return;
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (sensor_isOk()) {
    mqttClient.publish(humStateTopic, String(currentHum).c_str());
    mqttClient.publish(tempStateTopic, String(currentTemp).c_str());
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Sensor published: T=%.2f°C, H=%.2f%%\n", currentTemp, currentHum);
    #endif
  }
  #endif
}


void mqtt_publishOnline() {
  if (!mqttClient.connected()) return;
  mqttClient.publish(onlineTopic, "Online", true);
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Online published to: %s\n", onlineTopic);
  #endif
}

void mqtt_publishOffline() {
  if (mqttClient.connected()) {
    mqttClient.publish(onlineTopic, "Offline", true);
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Offline published to: %s\n", onlineTopic);
    #endif
    delay(50);
    mqttClient.disconnect();
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Command received: %s = %s\n", topic, msg.c_str());
  #endif
  
  #if MQTT_RESET_ENABLED
  if (strcmp(topic, resetControlTopic) == 0) {
    if (msg == "1") {
      mqtt_publishOffline();
      config_clear();
      delay(1000);
      ESP.restart();
    }
    return;
  }
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  
  // === ПРЯМОЕ УПРАВЛЕНИЕ (ON/OFF) ===
  if (strcmp(topic, controlTopic) == 0) {
    #if DEVICE_TYPE == 1
    config.sensorControlMode = false;
    #endif
    if (msg == "ON" || msg == "1") { 
      fan_set(true);
    } else if (msg == "OFF" || msg == "0") { 
      fan_set(false);
    }
    // fan_set() сам публикует состояние через mqtt_publishState()
  }
  
  // === УПРАВЛЕНИЕ ШИМ ===
  #if DEVICE_TYPE == 1
  else if (strcmp(topic, pwmDutyControlTopic) == 0) {
    int newDuty = msg.toInt();
    if (newDuty >= 0 && newDuty <= 100) {
      bool adaptiveChanged = false;
      
      // Ручное изменение ШИМ: отключаем адаптивный режим
      if (config.adaptiveMode) {
        config.adaptiveMode = false;
        adaptiveActive = false;
        adaptiveChanged = true;
        #ifdef DEBUG_MQTT
          Serial.println("[MQTT] Manual PWM change - adaptive mode disabled");
        #endif
      }
      
      if (newDuty == 0 || newDuty < MIN_PWM_DUTY_PERCENT) {
        // pwmDuty = 0 или ниже минимальной — выключение вентилятора
        config.pwmDutyPercent = newDuty;
        fan_set(false);  // полноценное выключение: state=OFF, сброс скважности, публикация
        #ifdef DEBUG_MQTT
          if (newDuty > 0) {
            Serial.printf("[MQTT] PWM duty %d%% below minimum - turning OFF\n", newDuty);
          }
        #endif
      } else {
        // Меняем значение ТОЛЬКО в RAM, НЕ сохраняем
        config.pwmDutyPercent = newDuty;
        if (fanOn && !startingPulseActive) {
          fan_applyPWM(config.pwmDutyPercent);
        }
      }
      
      // Публикуем изменённые параметры (fan_set уже опубликовал state)
      mqttClient.publish(pwmDutyStateTopic, String(config.pwmDutyPercent).c_str());
      #ifdef DEBUG_MQTT
        Serial.printf("[MQTT] PWM duty published: %d%%\n", config.pwmDutyPercent);
      #endif
      
      if (adaptiveChanged) {
        mqttClient.publish(adaptiveModeStateTopic, "0");
        #ifdef DEBUG_MQTT
          Serial.println("[MQTT] Adaptive mode published: 0");
        #endif
      }
    }
  }
  #elif DEVICE_TYPE == 3
  else if (strcmp(topic, pwmDutyControlTopic) == 0) {
    int newDuty = msg.toInt();
    if (newDuty >= 0 && newDuty <= 100) {
      if (newDuty == 0 || newDuty < MIN_PWM_DUTY_PERCENT) {
        // pwmDuty = 0 или ниже минимальной — выключение
        config.pwmDutyPercent = newDuty;
        fan_set(false);  // полноценное выключение
        #ifdef DEBUG_MQTT
          if (newDuty > 0) {
            Serial.printf("[MQTT] PWM duty %d%% below minimum - turning OFF\n", newDuty);
          }
        #endif
      } else {
        config.pwmDutyPercent = newDuty;
        if (fanOn && !startingPulseActive) {
          fan_applyPWM(config.pwmDutyPercent);
        }
      }
      mqttClient.publish(pwmDutyStateTopic, String(config.pwmDutyPercent).c_str());
      #ifdef DEBUG_MQTT
        Serial.printf("[MQTT] PWM duty published: %d%%\n", config.pwmDutyPercent);
      #endif
    }
  }
  #endif
  
  // === АДАПТИВНЫЙ РЕЖИМ (только TYPE 1) ===
  #if DEVICE_TYPE == 1
  else if (strcmp(topic, adaptiveModeControlTopic) == 0) {
    bool newAdaptiveMode = (msg == "1" || msg == "ON");
    
    if (newAdaptiveMode && !config.sensorControlMode) {
        #ifdef DEBUG_MQTT
            Serial.println("[MQTT] Cannot enable adaptive mode - sensor control mode is OFF. Command rejected.");
        #endif
        mqttClient.publish(adaptiveModeStateTopic, "0");
        return;
    }
    
    config.adaptiveMode = newAdaptiveMode;
    
    if (fanOn && config.sensorControlMode && config.adaptiveMode && sensor_isOk()) {
        adaptiveActive = true;
        baseTemp = currentTemp;
        baseHum = currentHum;
        lastAdaptiveCheck = millis();
        #ifdef DEBUG_MQTT
            Serial.printf("[MQTT] Adaptive mode activated: base T=%.2f, H=%.2f\n", baseTemp, baseHum);
        #endif
    } else if (!config.adaptiveMode) {
        adaptiveActive = false;
    }
    mqttClient.publish(adaptiveModeStateTopic, config.adaptiveMode ? "1" : "0");
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Adaptive mode published: %s\n", config.adaptiveMode ? "1" : "0");
    #endif
  }
  #endif
  
  // === ЗАДЕРЖКА ОТЛОЖЕННОГО ВКЛЮЧЕНИЯ ===
  else if (strcmp(topic, delaySecControlTopic) == 0) {
    int newDelay = msg.toInt();
    int oldDelay = config.delaySeconds;
    config.delaySeconds = newDelay;
    if (delayActive) {
      long remaining = (delayTimer - millis()) + (newDelay - oldDelay) * 1000L;
      if (remaining > 0) {
        delayTimer = millis() + remaining;
      } else {
        delayActive = false;
        fan_set(true);
      }
    }
    mqttClient.publish(delaySecStateTopic, String(config.delaySeconds).c_str());
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Delay seconds published: %d\n", config.delaySeconds);
    #endif
  }
  
  // === АВАРИЙНОЕ ОТКЛЮЧЕНИЕ ===
  else if (strcmp(topic, maxOnTimeControlTopic) == 0) {
    config.maxOnTime = msg.toInt();
    mqttClient.publish(maxOnTimeStateTopic, String(config.maxOnTime).c_str());
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Max on time published: %d\n", config.maxOnTime);
    #endif
  }
  
  // === РЕЖИМ УПРАВЛЕНИЯ СЕНСОРОМ (только TYPE 1) ===
  #if DEVICE_TYPE == 1
  else if (strcmp(topic, sensorControlModeControlTopic) == 0) {
    if (msg == "AUTO" || msg == "1" || msg == "ON") { 
      fan_setOverrideMode(true);
    } else if (msg == "0" || msg == "OFF") {
      fan_setOverrideMode(false);
    }
    mqtt_publishState();
  }
  #endif
  
  #endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  
  // === ПОРОГИ ДАТЧИКА (только TYPE 1) ===
  #if DEVICE_TYPE == 1
  if (strcmp(topic, lowTempControlTopic) == 0) {
    config.lowTemp = msg.toFloat();
    mqttClient.publish(lowTempStateTopic, String(config.lowTemp).c_str());
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Low temp published: %.1f\n", config.lowTemp);
    #endif
  }
  else if (strcmp(topic, highTempControlTopic) == 0) {
    config.highTemp = msg.toFloat();
    mqttClient.publish(highTempStateTopic, String(config.highTemp).c_str());
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] High temp published: %.1f\n", config.highTemp);
    #endif
  }
  else if (strcmp(topic, lowHumControlTopic) == 0) {
    config.lowHum = msg.toFloat();
    mqttClient.publish(lowHumStateTopic, String(config.lowHum).c_str());
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Low hum published: %.1f\n", config.lowHum);
    #endif
  }
  else if (strcmp(topic, highHumControlTopic) == 0) {
    config.highHum = msg.toFloat();
    mqttClient.publish(highHumStateTopic, String(config.highHum).c_str());
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] High hum published: %.1f\n", config.highHum);
    #endif
  }
  #endif
}

void mqtt_reconnect() {
  if (!configValid) return;
  if (mqttClient.connected()) return;
  if (millis() - lastMqttReconnect < MQTT_RECONNECT_DELAY_MS) return;
  lastMqttReconnect = millis();
  
  mqttClient.setServer(config.mqttBroker, config.mqttPort);
  mqttClient.setCallback(mqtt_callback);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);

  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Connecting to %s:%d as %s (keepalive=%ds)\n", 
                  config.mqttBroker, config.mqttPort, config.mqttClientId, MQTT_KEEPALIVE_SEC);
  #endif
  
  if (mqttClient.connect(config.mqttClientId, config.mqttUser, config.mqttPassword, 
                         lastWillTopic, 1, true, "Offline")) {
    #ifdef DEBUG_MQTT
      Serial.println("[MQTT] Connected to broker");
    #endif
    
    mqtt_publishOnline();
    
    // Публикуем всё текущее состояние один раз при подключении
    mqtt_publishState();
    mqtt_publishConfig();
    mqtt_publishSensor();
    
    // Подписки
    #if MQTT_RESET_ENABLED
    mqttClient.subscribe(resetControlTopic, 1);
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    mqttClient.subscribe(controlTopic, 1);
    mqttClient.subscribe(pwmDutyControlTopic, 1);
    #if DEVICE_TYPE == 1
    mqttClient.subscribe(adaptiveModeControlTopic, 1);
    #endif
    mqttClient.subscribe(delaySecControlTopic, 1);
    mqttClient.subscribe(maxOnTimeControlTopic, 1);
    mqttClient.subscribe(sensorControlModeControlTopic, 1);
    #endif
    
    #if DEVICE_TYPE == 1
    mqttClient.subscribe(lowTempControlTopic, 1);
    mqttClient.subscribe(highTempControlTopic, 1);
    mqttClient.subscribe(lowHumControlTopic, 1);
    mqttClient.subscribe(highHumControlTopic, 1);
    #endif
    
    #ifdef DEBUG_MQTT
      Serial.println("[MQTT] Subscribed to control topics");
    #endif
    
  } else {
    Serial.printf("[MQTT] Failed to connect, state=%d\n", mqttClient.state());
  }
}

void mqtt_init() {
  mqttClient.setClient(espClient);
  mqttClient.setBufferSize(512);
  #ifdef DEBUG_MQTT
    Serial.println("[MQTT] Initialized");
  #endif
}

bool mqtt_isConnected() {
  return mqttClient.connected();
}