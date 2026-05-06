#include "mqtt.h"
#include "config.h"
#include "fan.h"
#include "sensor.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

char devicePrefix[18];
char lastWillTopic[32];

char switchStateTopic[50];
char switchControlTopic[50];
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

unsigned long lastMqttReconnect = 0;

void mqtt_setupTopics(const char* prefix) {
  strcpy(devicePrefix, prefix);
  
  snprintf(lastWillTopic, sizeof(lastWillTopic), "%s/status", devicePrefix);
  snprintf(switchStateTopic, sizeof(switchStateTopic), "%s/switch/state", devicePrefix);
  snprintf(switchControlTopic, sizeof(switchControlTopic), "%s/c/switch/state", devicePrefix);
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
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Topics configured with prefix: %s\n", devicePrefix);
  #endif
}

void mqtt_publishState() {
  if (!mqttClient.connected()) return;
  
  bool realState = fan_getRealStateForMqtt();
  
  mqttClient.publish(switchStateTopic, realState ? "ON" : "OFF");
  mqttClient.publish(slowModeStateTopic, config.slowModeEnabled ? "1" : "0");
  mqttClient.publish(slowModeDutyStateTopic, String(config.slowModeDuty).c_str());
  mqttClient.publish(lowTempStateTopic, String(config.lowTemp).c_str());
  mqttClient.publish(highTempStateTopic, String(config.highTemp).c_str());
  mqttClient.publish(lowHumStateTopic, String(config.lowHum).c_str());
  mqttClient.publish(highHumStateTopic, String(config.highHum).c_str());
  mqttClient.publish(delaySecStateTopic, String(config.delaySeconds).c_str());
  mqttClient.publish(autoModeStateTopic, manualOverride ? "0" : "1");
  
  if (lastRelayError) {
    mqttClient.publish(errorTopic, "relay_mismatch");
  }
  
  #ifdef DEBUG_MQTT
    Serial.println("========== MQTT STATE PUBLISH ==========");
    Serial.printf("  Real state: %s\n", realState ? "ON" : "OFF");
    Serial.printf("  -> Published to %s: %s\n", switchStateTopic, realState ? "ON" : "OFF");
    Serial.println("=========================================");
  #endif
}

void mqtt_publishSensor() {
  if (!mqttClient.connected()) return;
  
  if (sensor_isOk()) {
    mqttClient.publish(tempStateTopic, String(currentTemp).c_str());
    mqttClient.publish(humStateTopic, String(currentHum).c_str());
    
    #ifdef DEBUG_MQTT
      Serial.println("========== MQTT SENSOR PUBLISH ==========");
      Serial.printf("  -> Published to %s: %.2f°C\n", tempStateTopic, currentTemp);
      Serial.printf("  -> Published to %s: %.2f%%\n", humStateTopic, currentHum);
      Serial.println("=========================================");
    #endif
  }
}

void mqtt_publishOnline() {
  if (!mqttClient.connected()) return;
  mqttClient.publish(lastWillTopic, "Online", true);
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Published Online to: %s\n", lastWillTopic);
  #endif
}

void mqtt_publishOffline() {
  if (mqttClient.connected()) {
    #ifdef DEBUG_MQTT
      Serial.println("[MQTT] Publishing offline status...");
    #endif
    mqttClient.publish(lastWillTopic, "Offline", true);
    delay(50);
    mqttClient.disconnect();
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  
  #ifdef DEBUG_MQTT
    Serial.println("========== MQTT COMMAND RECEIVED ==========");
    Serial.printf("  Topic: %s\n", topic);
    Serial.printf("  Payload: %s\n", msg.c_str());
  #endif
  
  if (strcmp(topic, switchControlTopic) == 0) {
    if (msg == "ON") { 
      manualOverride = true; 
      fan_cancelDelayTimer();
      fan_set(true); 
      #ifdef DEBUG_MQTT
        Serial.println("  Action: Fan turned ON (manual mode) - delay timer cancelled");
      #endif
    } else if (msg == "OFF") { 
      manualOverride = true; 
      fan_cancelDelayTimer();
      fan_set(false); 
      #ifdef DEBUG_MQTT
        Serial.println("  Action: Fan turned OFF (manual mode) - delay timer cancelled");
      #endif
    }
  }
  else if (strcmp(topic, autoModeControlTopic) == 0) {
    if (msg == "AUTO" || msg == "1") { 
      manualOverride = false; 
      fan_update();
      #ifdef DEBUG_MQTT
        Serial.println("  Action: AUTO mode enabled - delay timer NOT restarted");
      #endif
    }
  }
  else if (strcmp(topic, slowModeControlTopic) == 0) {
    config.slowModeEnabled = (msg == "1");
    config_write();
    if (!manualOverride || fan_getState()) {
      fan_applyPwmOrDigital(fan_getState());
    }
    mqttClient.publish(slowModeStateTopic, config.slowModeEnabled ? "1" : "0");
    #ifdef DEBUG_MQTT
      Serial.printf("  Action: Slow mode %s\n", config.slowModeEnabled ? "ON" : "OFF");
    #endif
  }
  else if (strcmp(topic, slowModeDutyControlTopic) == 0) {
    uint16_t val = msg.toInt();
    if (val <= (1 << PWM_RESOLUTION) - 1) {
      config.slowModeDuty = val;
      config_write();
      if (config.slowModeEnabled && (!manualOverride || fan_getState())) {
        fan_applyPwmOrDigital(fan_getState());
      }
      mqttClient.publish(slowModeDutyStateTopic, String(config.slowModeDuty).c_str());
      #ifdef DEBUG_MQTT
        Serial.printf("  Action: Slow mode duty set to %d\n", config.slowModeDuty);
      #endif
    }
  }
  else if (strcmp(topic, lowTempControlTopic) == 0) {
    config.lowTemp = msg.toFloat();
    config_write();
    #ifdef DEBUG_MQTT
      Serial.printf("  Action: Low temp set to %.1f°C\n", config.lowTemp);
    #endif
  }
  else if (strcmp(topic, highTempControlTopic) == 0) {
    config.highTemp = msg.toFloat();
    config_write();
    #ifdef DEBUG_MQTT
      Serial.printf("  Action: High temp set to %.1f°C\n", config.highTemp);
    #endif
  }
  else if (strcmp(topic, lowHumControlTopic) == 0) {
    config.lowHum = msg.toFloat();
    config_write();
    #ifdef DEBUG_MQTT
      Serial.printf("  Action: Low hum set to %.1f%%\n", config.lowHum);
    #endif
  }
  else if (strcmp(topic, highHumControlTopic) == 0) {
    config.highHum = msg.toFloat();
    config_write();
    #ifdef DEBUG_MQTT
      Serial.printf("  Action: High hum set to %.1f%%\n", config.highHum);
    #endif
  }
  else if (strcmp(topic, delaySecControlTopic) == 0) {
    config.delaySeconds = msg.toInt();
    config_write();
    #ifdef DEBUG_MQTT
      Serial.printf("  Action: Delay seconds set to %d\n", config.delaySeconds);
    #endif
  }
  
  #ifdef DEBUG_MQTT
    Serial.println("===========================================");
  #endif
}

void mqtt_reconnect() {
  if (!configValid) return;
  if (mqttClient.connected()) return;
  if (millis() - lastMqttReconnect < MQTT_RECONNECT_DELAY_MS) return;
  lastMqttReconnect = millis();
  
  mqttClient.setServer(config.mqttBroker, config.mqttPort);
  mqttClient.setCallback(mqtt_callback);
  
  int keepAlive = MQTT_KEEPALIVE_SEC;
  
  #ifdef DEBUG_MQTT
    Serial.printf("[MQTT] Connecting to %s:%d as %s (keepalive=%ds)\n", 
                  config.mqttBroker, config.mqttPort, config.mqttClientId, keepAlive);
  #endif
  
  if (mqttClient.connect(config.mqttClientId, config.mqttUser, config.mqttPassword, 
                         lastWillTopic, 1, true, "Offline", keepAlive)) {
    #ifdef DEBUG_MQTT
      Serial.println("[MQTT] Connected to broker");
    #endif
    
    mqtt_publishOnline();
    
    mqttClient.subscribe(switchControlTopic);
    mqttClient.subscribe(slowModeControlTopic);
    mqttClient.subscribe(slowModeDutyControlTopic);
    mqttClient.subscribe(lowTempControlTopic);
    mqttClient.subscribe(highTempControlTopic);
    mqttClient.subscribe(lowHumControlTopic);
    mqttClient.subscribe(highHumControlTopic);
    mqttClient.subscribe(delaySecControlTopic);
    mqttClient.subscribe(autoModeControlTopic);
    
    #ifdef DEBUG_MQTT
      Serial.println("[MQTT] Subscribed to control topics");
    #endif
    
    mqtt_publishState();
    mqtt_publishSensor();
  } else {
    #ifdef DEBUG_MQTT
      Serial.printf("[MQTT] Failed to connect, state=%d\n", mqttClient.state());
    #endif
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