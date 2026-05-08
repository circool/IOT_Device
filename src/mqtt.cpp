#include "mqtt.h"
#include "config.h"
#include "fan.h"
#include "sensor.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

char devicePrefix[18];
char lastWillTopic[32];

char resetControlTopic[50];

#if DEVICE_TYPE == 1
char fanStateTopic[50];
char fanControlTopic[50];
char slowModeStateTopic[50];
char slowModeControlTopic[50];
char slowModeDutyStateTopic[50];
char slowModeDutyControlTopic[50];
char tempStateTopic[50];
char humStateTopic[50];
char lowTempStateTopic[50];
char highTempStateTopic[50];
char lowHumStateTopic[50];
char highHumStateTopic[50];
char lowTempControlTopic[50];
char highTempControlTopic[50];
char lowHumControlTopic[50];
char highHumControlTopic[50];
char delaySecStateTopic[50];
char delaySecControlTopic[50];
char autoModeStateTopic[50];
char autoModeControlTopic[50];
char errorTopic[50];
#endif

#if DEVICE_TYPE == 2
char tempStateTopic[50];
char humStateTopic[50];
#endif

#if DEVICE_TYPE == 3
char switchStateTopic[50];
char switchControlTopic[50];
#endif

unsigned long lastMqttReconnect = 0;

void mqtt_setupTopics(const char* prefix) {
  strcpy(devicePrefix, prefix);
  
  snprintf(lastWillTopic, sizeof(lastWillTopic), "%s/status", devicePrefix);
  snprintf(resetControlTopic, sizeof(resetControlTopic), "%s/c/system/reset", devicePrefix);
  
  #if DEVICE_TYPE == 1
  snprintf(fanStateTopic, sizeof(fanStateTopic), "%s/fan/state", devicePrefix);
  snprintf(fanControlTopic, sizeof(fanControlTopic), "%s/c/fan/state", devicePrefix);
  snprintf(slowModeStateTopic, sizeof(slowModeStateTopic), "%s/fan/slowMode", devicePrefix);
  snprintf(slowModeControlTopic, sizeof(slowModeControlTopic), "%s/c/fan/slowMode", devicePrefix);
  snprintf(slowModeDutyStateTopic, sizeof(slowModeDutyStateTopic), "%s/fan/slowModeDuty", devicePrefix);
  snprintf(slowModeDutyControlTopic, sizeof(slowModeDutyControlTopic), "%s/c/fan/slowModeDuty", devicePrefix);
  snprintf(tempStateTopic, sizeof(tempStateTopic), "%s/sensor/temperature", devicePrefix);
  snprintf(humStateTopic, sizeof(humStateTopic), "%s/sensor/humidity", devicePrefix);
  snprintf(lowTempStateTopic, sizeof(lowTempStateTopic), "%s/sensor/lowTemp", devicePrefix);
  snprintf(highTempStateTopic, sizeof(highTempStateTopic), "%s/sensor/highTemp", devicePrefix);
  snprintf(lowHumStateTopic, sizeof(lowHumStateTopic), "%s/sensor/lowHum", devicePrefix);
  snprintf(highHumStateTopic, sizeof(highHumStateTopic), "%s/sensor/highHum", devicePrefix);
  snprintf(lowTempControlTopic, sizeof(lowTempControlTopic), "%s/c/sensor/lowTemp", devicePrefix);
  snprintf(highTempControlTopic, sizeof(highTempControlTopic), "%s/c/sensor/highTemp", devicePrefix);
  snprintf(lowHumControlTopic, sizeof(lowHumControlTopic), "%s/c/sensor/lowHum", devicePrefix);
  snprintf(highHumControlTopic, sizeof(highHumControlTopic), "%s/c/sensor/highHum", devicePrefix);
  snprintf(delaySecStateTopic, sizeof(delaySecStateTopic), "%s/fan/delaySec", devicePrefix);
  snprintf(delaySecControlTopic, sizeof(delaySecControlTopic), "%s/c/fan/delaySec", devicePrefix);
  snprintf(autoModeStateTopic, sizeof(autoModeStateTopic), "%s/fan/autoMode", devicePrefix);
  snprintf(autoModeControlTopic, sizeof(autoModeControlTopic), "%s/c/fan/autoMode", devicePrefix);
  snprintf(errorTopic, sizeof(errorTopic), "%s/error", devicePrefix);
  #endif
  
  #if DEVICE_TYPE == 2
  snprintf(tempStateTopic, sizeof(tempStateTopic), "%s/sensor/temperature", devicePrefix);
  snprintf(humStateTopic, sizeof(humStateTopic), "%s/sensor/humidity", devicePrefix);
  #endif
  
  #if DEVICE_TYPE == 3
  snprintf(switchStateTopic, sizeof(switchStateTopic), "%s/switch/state", devicePrefix);
  snprintf(switchControlTopic, sizeof(switchControlTopic), "%s/c/switch/state", devicePrefix);
  #endif
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Topics configured with prefix: %s\n", devicePrefix);
  #endif
}

void mqtt_publishState() {
  if (!mqttClient.connected()) return;
  
  #if DEVICE_TYPE == 1
  bool realState = fan_getRealState();
  mqttClient.publish(fanStateTopic, realState ? "ON" : "OFF");
  mqttClient.publish(autoModeStateTopic, manualOverride ? "0" : "1");
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] State published: fan=%s, auto=%s\n", 
                  realState ? "ON" : "OFF", manualOverride ? "0" : "1");
  #endif
  #endif
  
  #if DEVICE_TYPE == 3
  bool realState = fan_getRealState();
  mqttClient.publish(switchStateTopic, realState ? "ON" : "OFF");
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] State published: switch=%s\n", realState ? "ON" : "OFF");
  #endif
  #endif
}

void mqtt_publishConfig() {
  if (!mqttClient.connected()) return;
  
  #if DEVICE_TYPE == 1
  mqttClient.publish(lowTempStateTopic, String(config.lowTemp).c_str());
  mqttClient.publish(highTempStateTopic, String(config.highTemp).c_str());
  mqttClient.publish(lowHumStateTopic, String(config.lowHum).c_str());
  mqttClient.publish(highHumStateTopic, String(config.highHum).c_str());
  mqttClient.publish(slowModeStateTopic, config.slowModeEnabled ? "1" : "0");
  mqttClient.publish(slowModeDutyStateTopic, String(config.slowModeDuty).c_str());
  mqttClient.publish(delaySecStateTopic, String(config.delaySeconds).c_str());
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Config published: T(%.1f-%.1f), H(%.1f-%.1f), slow=%d, duty=%d, delay=%d\n",
                  config.lowTemp, config.highTemp, config.lowHum, config.highHum,
                  config.slowModeEnabled, config.slowModeDuty, config.delaySeconds);
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
  mqttClient.publish(lastWillTopic, "Online", true);
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Online published to: %s\n", lastWillTopic);
  #endif
}

void mqtt_publishOffline() {
  if (mqttClient.connected()) {
    mqttClient.publish(lastWillTopic, "Offline", true);
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Offline published to: %s\n", lastWillTopic);
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
      #ifdef DEBUG_MQTT
        Serial.println("[MQTT] Action: Factory reset");
      #endif
      mqtt_publishOffline();
      config_clear();
      delay(1000);
      ESP.restart();
    }
    return;
  }
  
  #if DEVICE_TYPE == 1
  if (strcmp(topic, fanControlTopic) == 0) {
    if (msg == "ON" || msg == "1") { 
      fan_setOverrideMode(true);
      fan_set(true);
    } else if (msg == "OFF" || msg == "0") { 
      fan_setOverrideMode(true);
      fan_set(false);
    }
  }
  else if (strcmp(topic, autoModeControlTopic) == 0) {
    if (msg == "AUTO" || msg == "1") { 
      fan_setOverrideMode(false);
    } else if (msg == "0") {
      fan_setOverrideMode(true);
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
  
  #if DEVICE_TYPE == 3
  if (strcmp(topic, switchControlTopic) == 0) {
    if (msg == "ON" || msg == "1") { 
      fan_setOverrideMode(true);
      fan_set(true);
    } else if (msg == "OFF" || msg == "0") { 
      fan_setOverrideMode(true);
      fan_set(false);
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
    
    #if DEVICE_TYPE == 1
    mqttClient.subscribe(fanControlTopic, 1);
    mqttClient.subscribe(autoModeControlTopic, 1);
    mqttClient.subscribe(slowModeControlTopic, 1);
    mqttClient.subscribe(slowModeDutyControlTopic, 1);
    mqttClient.subscribe(lowTempControlTopic, 1);
    mqttClient.subscribe(highTempControlTopic, 1);
    mqttClient.subscribe(lowHumControlTopic, 1);
    mqttClient.subscribe(highHumControlTopic, 1);
    mqttClient.subscribe(delaySecControlTopic, 1);
    #endif
    
    #if DEVICE_TYPE == 3
    mqttClient.subscribe(switchControlTopic, 1);
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