#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>

#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#include <PubSubClient.h>

extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern char devicePrefix[24];
extern char lastWillTopic[48];

// Общие для всех
extern char resetControlTopic[56];
extern char onlineTopic[56];

// Общие для TYPE 1 и TYPE 3 (исполнительное устройство)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
extern char stateTopic[56];
extern char controlTopic[56];
extern char slowModeStateTopic[56];
extern char slowModeControlTopic[56];
extern char slowModeDutyStateTopic[56];
extern char slowModeDutyControlTopic[56];
extern char delaySecStateTopic[56];
extern char delaySecControlTopic[56];
#endif

// Общие для TYPE 1 и TYPE 2 (датчик)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
extern char tempStateTopic[56];
extern char humStateTopic[56];
#endif

// Только TYPE 1 (пороги датчика + автоматика)
#if DEVICE_TYPE == 1
extern char lowTempStateTopic[56];
extern char highTempStateTopic[56];
extern char lowHumStateTopic[56];
extern char highHumStateTopic[56];
extern char lowTempControlTopic[56];
extern char highTempControlTopic[56];
extern char lowHumControlTopic[56];
extern char highHumControlTopic[56];
extern char autoModeStateTopic[56];
extern char autoModeControlTopic[56];
extern char errorTopic[56];
#endif

void mqtt_init();
void mqtt_setupTopics(const char* prefix);
void mqtt_reconnect();
void mqtt_publishState();
void mqtt_publishSensor();
void mqtt_publishConfig();
void mqtt_publishOnline();
void mqtt_publishOffline();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
bool mqtt_isConnected();

#endif