#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <functional>
#include <PubSubClient.h>
#include "config.h"

#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

class MQTTManager {
public:
    MQTTManager();
    ~MQTTManager();
    
    // Инициализация и управление
    bool begin(const Config& cfg);
    void process();
    bool isConnected();
    void disconnect();
    
    // Публикации состояния
    void publishOnline();
    void publishState(bool on);
    void publishSpeed(uint16_t speed);           
    void publishDelaySec(int seconds);
    void publishMaxOnTime(uint32_t seconds);
    void publishSensorControlMode(bool enabled);
    void publishConfig();
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    void publishSensor(float temp, float hum);
    #endif
    
    #if DEVICE_TYPE == 1
    void publishAdaptiveMode(bool enabled);
    void publishLowTemp(float temp);
    void publishHighTemp(float temp);
    void publishLowHum(float hum);
    void publishHighHum(float hum);
    void publishThresholds();
    #endif
    
    #if MQTT_PUBLISH_RSSI == 1
    void publishRSSI();
    #endif
    
    // Установка колбэков для команд
    void onStateCommand(std::function<void(bool)> callback);
    void onSpeedCommand(std::function<void(int)> callback);        
    void onDelaySecCommand(std::function<void(int)> callback);
    void onMaxOnTimeCommand(std::function<void(uint32_t)> callback);
    void onSensorControlModeCommand(std::function<void(bool)> callback);
    
    #if DEVICE_TYPE == 1
    void onAdaptiveModeCommand(std::function<void(bool)> callback);
    void onLowTempCommand(std::function<void(float)> callback);
    void onHighTempCommand(std::function<void(float)> callback);
    void onLowHumCommand(std::function<void(float)> callback);
    void onHighHumCommand(std::function<void(float)> callback);
    #endif
    
    #if MQTT_RESET_ENABLED == 1
    void onResetCommand(std::function<void()> callback);
    #endif

    #if MQTT_PUBLISH_RESET_REASON == 1
      void publishResetReason();
    #endif
    
private:
    void reconnect();
    void setupTopics();
    void subscribe();
    void callback(char* topic, byte* payload, unsigned int length);
    static void staticCallback(char* topic, byte* payload, unsigned int length);
    void handleCommand(const char* topic, const String& payload);
    
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    
    struct Topics {
        char online[48];
        char version[48];
        char reset[48];
        char state[48];
        char control[48];
        char speed[48];              
        char speedControl[48];       
        char delaySec[48];
        char delaySecControl[48];
        char maxOnTime[48];
        char maxOnTimeControl[48];
        char sensorControlMode[48];
        char sensorControlModeControl[48];
        
        #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
        char temperature[48];
        char humidity[48];
        #endif
        
        #if DEVICE_TYPE == 1
        char adaptiveMode[48];
        char adaptiveModeControl[48];
        char lowTemp[48];
        char highTemp[48];
        char lowHum[48];
        char highHum[48];
        char lowTempControl[48];
        char highTempControl[48];
        char lowHumControl[48];
        char highHumControl[48];
        #endif
        
        #if MQTT_PUBLISH_RSSI == 1
        char rssi[48];
        #endif
    } _topics;
    
    char _clientId[24];
    bool _initialized;
    unsigned long _lastReconnectAttempt;
    
    // Колбэки
    std::function<void(bool)> _stateCallback;
    std::function<void(int)> _speedCallback;        
    std::function<void(int)> _delaySecCallback;
    std::function<void(uint32_t)> _maxOnTimeCallback;
    std::function<void(bool)> _sensorControlModeCallback;
    
    #if DEVICE_TYPE == 1
    std::function<void(bool)> _adaptiveModeCallback;
    std::function<void(float)> _lowTempCallback;
    std::function<void(float)> _highTempCallback;
    std::function<void(float)> _lowHumCallback;
    std::function<void(float)> _highHumCallback;
    #endif
    
    #if MQTT_RESET_ENABLED == 1
    std::function<void()> _resetCallback;
    #endif
};

// Глобальный экземпляр
extern MQTTManager mqttManager;
#if MQTT_PUBLISH_RESET_REASON == 1
  extern char lastResetReason[32];
#endif
#endif // MQTT_H