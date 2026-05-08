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
char slowModeStateTopic[56];
char slowModeControlTopic[56];
char slowModeDutyStateTopic[56];
char slowModeDutyControlTopic[56];
char delaySecStateTopic[56];
char delaySecControlTopic[56];
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
char autoModeStateTopic[56];
char autoModeControlTopic[56];
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
  snprintf(slowModeStateTopic, sizeof(slowModeStateTopic), "%s/fan/slowMode", devicePrefix);
  snprintf(slowModeControlTopic, sizeof(slowModeControlTopic), "%s/c/fan/slowMode", devicePrefix);
  snprintf(slowModeDutyStateTopic, sizeof(slowModeDutyStateTopic), "%s/fan/slowModeDuty", devicePrefix);
  snprintf(slowModeDutyControlTopic, sizeof(slowModeDutyControlTopic), "%s/c/fan/slowModeDuty", devicePrefix);
  snprintf(delaySecStateTopic, sizeof(delaySecStateTopic), "%s/fan/delaySec", devicePrefix);
  snprintf(delaySecControlTopic, sizeof(delaySecControlTopic), "%s/c/fan/delaySec", devicePrefix);
  #elif DEVICE_TYPE == 3
  snprintf(stateTopic, sizeof(stateTopic), "%s/switch/state", devicePrefix);
  snprintf(controlTopic, sizeof(controlTopic), "%s/c/switch/state", devicePrefix);
  snprintf(slowModeStateTopic, sizeof(slowModeStateTopic), "%s/switch/slowMode", devicePrefix);
  snprintf(slowModeControlTopic, sizeof(slowModeControlTopic), "%s/c/switch/slowMode", devicePrefix);
  snprintf(slowModeDutyStateTopic, sizeof(slowModeDutyStateTopic), "%s/switch/slowModeDuty", devicePrefix);
  snprintf(slowModeDutyControlTopic, sizeof(slowModeDutyControlTopic), "%s/c/switch/slowModeDuty", devicePrefix);
  snprintf(delaySecStateTopic, sizeof(delaySecStateTopic), "%s/switch/delaySec", devicePrefix);
  snprintf(delaySecControlTopic, sizeof(delaySecControlTopic), "%s/c/switch/delaySec", devicePrefix);
  #endif
  #endif
  
  // TYPE 1 и TYPE 2: датчик
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  snprintf(tempStateTopic, sizeof(tempStateTopic), "%s/sensor/temperature", devicePrefix);
  snprintf(humStateTopic, sizeof(humStateTopic), "%s/sensor/humidity", devicePrefix);
  #endif
  
  // TYPE 1: пороги датчика и автоматика
  #if DEVICE_TYPE == 1
  snprintf(lowTempStateTopic, sizeof(lowTempStateTopic), "%s/sensor/lowTemp", devicePrefix);
  snprintf(highTempStateTopic, sizeof(highTempStateTopic), "%s/sensor/highTemp", devicePrefix);
  snprintf(lowHumStateTopic, sizeof(lowHumStateTopic), "%s/sensor/lowHum", devicePrefix);
  snprintf(highHumStateTopic, sizeof(highHumStateTopic), "%s/sensor/highHum", devicePrefix);
  snprintf(lowTempControlTopic, sizeof(lowTempControlTopic), "%s/c/sensor/lowTemp", devicePrefix);
  snprintf(highTempControlTopic, sizeof(highTempControlTopic), "%s/c/sensor/highTemp", devicePrefix);
  snprintf(lowHumControlTopic, sizeof(lowHumControlTopic), "%s/c/sensor/lowHum", devicePrefix);
  snprintf(highHumControlTopic, sizeof(highHumControlTopic), "%s/c/sensor/highHum", devicePrefix);
  snprintf(autoModeStateTopic, sizeof(autoModeStateTopic), "%s/fan/autoMode", devicePrefix);
  snprintf(autoModeControlTopic, sizeof(autoModeControlTopic), "%s/c/fan/autoMode", devicePrefix);
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
  #endif
  
  #if DEVICE_TYPE == 1
  mqttClient.publish(autoModeStateTopic, manualOverride ? "0" : "1");
  #endif
  
  #ifdef DEBUG_MQTT
    #if DEVICE_TYPE == 1
    Serial.printf("[MQTT] State published: state=%s, auto=%s\n", 
                  fan_getRealState() ? "ON" : "OFF", manualOverride ? "0" : "1");
    #elif DEVICE_TYPE == 3
    Serial.printf("[MQTT] State published: state=%s\n", fan_getRealState() ? "ON" : "OFF");
    #endif
  #endif
}

void mqtt_publishConfig() {
  if (!mqttClient.connected()) return;
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  mqttClient.publish(slowModeStateTopic, config.slowModeEnabled ? "1" : "0");
  mqttClient.publish(slowModeDutyStateTopic, String(config.slowModeDuty).c_str());
  mqttClient.publish(delaySecStateTopic, String(config.delaySeconds).c_str());
  #endif
  
  #if DEVICE_TYPE == 1
  mqttClient.publish(lowTempStateTopic, String(config.lowTemp).c_str());
  mqttClient.publish(highTempStateTopic, String(config.highTemp).c_str());
  mqttClient.publish(lowHumStateTopic, String(config.lowHum).c_str());
  mqttClient.publish(highHumStateTopic, String(config.highHum).c_str());
  mqttClient.publish(autoModeStateTopic, config.automaticMode ? "1" : "0");
  #endif
  
  #ifdef DEBUG_MQTT
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    Serial.printf("[MQTT] Config published: slow=%d, duty=%d, delay=%d",
                  config.slowModeEnabled, config.slowModeDuty, config.delaySeconds);
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
    mqttClient.publish(tempStateTopic, String(currentTemp).c_str());
    mqttClient.publish(humStateTopic, String(currentHum).c_str());
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
  
  if (strcmp(topic, resetControlTopic) == 0) {
    if (msg == "1") {
      mqtt_publishOffline();
      config_clear();
      delay(1000);
      ESP.restart();
    }
    return;
  }
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  
  if (strcmp(topic, controlTopic) == 0) {
    if (msg == "ON" || msg == "1") { 
      fan_setOverrideMode(true);
      fan_set(true);
    } else if (msg == "OFF" || msg == "0") { 
      fan_setOverrideMode(true);
      fan_set(false);
    }
  }
  else if (strcmp(topic, slowModeControlTopic) == 0) {
    config.slowModeEnabled = (msg == "1" || msg == "ON");
    config_write();
    if (fanOn) {
      if (config.slowModeEnabled) {
        #ifdef ESP32
          ledcAttachPin(SWITCH_PIN, 0);
          ledcWrite(0, config.slowModeDuty);
        #elif defined(ESP8266)
          analogWrite(SWITCH_PIN, config.slowModeDuty);
        #endif
      } else {
        #ifdef ESP32
          ledcDetachPin(SWITCH_PIN);
          digitalWrite(SWITCH_PIN, HIGH);
        #elif defined(ESP8266)
          digitalWrite(SWITCH_PIN, HIGH);
        #endif
      }
    }
    mqtt_publishConfig();
  }
  else if (strcmp(topic, slowModeDutyControlTopic) == 0) {
    int val = msg.toInt();
    if (val >= 0 && val <= 255) {
      config.slowModeDuty = val;
      config_write();
      if (fanOn && config.slowModeEnabled) {
        #ifdef ESP32
          ledcWrite(0, config.slowModeDuty);
        #elif defined(ESP8266)
          analogWrite(SWITCH_PIN, config.slowModeDuty);
        #endif
      }
      mqtt_publishConfig();
    }
  }
  else if (strcmp(topic, delaySecControlTopic) == 0) {
    int newDelay = msg.toInt();
    if (newDelay >= 0 && newDelay <= 3600) {
      if (delayActive) {
        long remaining = (delayTimer - millis()) + (newDelay - config.delaySeconds) * 1000L;
        if (remaining > 0) {
          delayTimer = millis() + remaining;
        } else {
          delayActive = false;
          fan_set(true);
        }
      }
      config.delaySeconds = newDelay;
      config_write();
      mqtt_publishConfig();
    }
  }
  #endif
  
  #if DEVICE_TYPE == 1
  
  if (strcmp(topic, autoModeControlTopic) == 0) {
    if (msg == "AUTO" || msg == "1") { 
      fan_setOverrideMode(false);
    } else if (msg == "0") {
      fan_setOverrideMode(true);
    }
  }
  else if (strcmp(topic, lowTempControlTopic) == 0) {
    float val = msg.toFloat();
    if (val >= -40.0 && val <= 85.0) {
      config.lowTemp = val;
      config_write();
      mqtt_publishConfig();
    }
  }
  else if (strcmp(topic, highTempControlTopic) == 0) {
    float val = msg.toFloat();
    if (val >= -40.0 && val <= 85.0) {
      config.highTemp = val;
      config_write();
      mqtt_publishConfig();
    }
  }
  else if (strcmp(topic, lowHumControlTopic) == 0) {
    float val = msg.toFloat();
    if (val >= 0.0 && val <= 100.0) {
      config.lowHum = val;
      config_write();
      mqtt_publishConfig();
    }
  }
  else if (strcmp(topic, highHumControlTopic) == 0) {
    float val = msg.toFloat();
    if (val >= 0.0 && val <= 100.0) {
      config.highHum = val;
      config_write();
      mqtt_publishConfig();
    }
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
                         lastWillTopic, 1, true, "Offline", MQTT_KEEPALIVE_SEC)) {
    #ifdef DEBUG_MQTT
      Serial.println("[MQTT] Connected to broker");
    #endif
    
    mqtt_publishOnline();
    
    mqttClient.subscribe(resetControlTopic, 1);
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    mqttClient.subscribe(controlTopic, 1);
    mqttClient.subscribe(slowModeControlTopic, 1);
    mqttClient.subscribe(slowModeDutyControlTopic, 1);
    mqttClient.subscribe(delaySecControlTopic, 1);
    #endif
    
    #if DEVICE_TYPE == 1
    mqttClient.subscribe(autoModeControlTopic, 1);
    mqttClient.subscribe(lowTempControlTopic, 1);
    mqttClient.subscribe(highTempControlTopic, 1);
    mqttClient.subscribe(lowHumControlTopic, 1);
    mqttClient.subscribe(highHumControlTopic, 1);
    #endif
    
    #ifdef DEBUG_MQTT
      Serial.println("[MQTT] Subscribed to control topics");
    #endif
    
    mqtt_publishState();
    mqtt_publishSensor();
    mqtt_publishConfig();
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