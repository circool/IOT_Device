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
extern char devicePrefix[18];
extern char lastWillTopic[32];

extern char switchStateTopic[50];
extern char switchControlTopic[50];
extern char slowModeStateTopic[50];
extern char slowModeControlTopic[50];
extern char slowModeDutyStateTopic[50];
extern char slowModeDutyControlTopic[50];
extern char tempStateTopic[50];
extern char humStateTopic[50];
extern char lowTempStateTopic[50];
extern char highTempStateTopic[50];
extern char lowHumStateTopic[50];
extern char highHumStateTopic[50];
extern char lowTempControlTopic[50];
extern char highTempControlTopic[50];
extern char lowHumControlTopic[50];
extern char highHumControlTopic[50];
extern char delaySecStateTopic[50];
extern char delaySecControlTopic[50];
extern char autoModeStateTopic[50];
extern char autoModeControlTopic[50];
extern char errorTopic[50];

void mqtt_init();
void mqtt_setupTopics(const char* mac);
void mqtt_reconnect();
void mqtt_publishState();
void mqtt_publishSensor();
void mqtt_publishOnline();
void mqtt_publishOffline();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
bool mqtt_isConnected();

#endif