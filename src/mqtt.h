#ifndef MQTT_H
#define MQTT_H
#if MQTT_ENABLED == 1
#include <Arduino.h>
#include <functional>
#include <PubSubClient.h>
#include "config.h"

#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

/**
 * @brief Менеджер MQTT-соединения
 * 
 * Обеспечивает:
 * - Автоматическое переподключение к брокеру
 * - Публикацию состояния, показаний датчиков, конфигурации
 * - Обработку входящих команд
 */
class MQTTManager {
public:
    MQTTManager();
    ~MQTTManager();
    
    /**
     * @brief Инициализация MQTT-клиента
     * @param broker Адрес брокера (IP или домен)
     * @param port Порт брокера (обычно 1883)
     * @param clientId Уникальный идентификатор клиента
     * @param user Имя пользователя (опционально)
     * @param password Пароль (опционально)
     * @return true — успешно, false — ошибка (нет брокера)
     */
    bool begin(const char* broker, uint16_t port, const char* clientId,
               const char* user = nullptr, const char* password = nullptr);
    
    /**
     * @brief Периодический вызов в loop()
     * Обрабатывает входящие сообщения и переподключение
     */
    void process();
    
    /**
     * @brief Проверить соединение с брокером
     * @return true — подключён, false — нет
     */
    bool isConnected();
    
    /**
     * @brief Принудительно отключиться от брокера
     */
    void disconnect();
    
    // --- Публикации ---
    void publishOnline();                    // Статус Online/Offline (LWT)
    void publishState(bool on);              // Состояние вентилятора/выключателя
    void publishSpeed(int percent);          // Текущая скорость (0-100)
    void publishDelaySec(int seconds);       // Таймер отложенного включения
    void publishMaxOnTime(uint32_t seconds); // Таймер аварийного отключения
    void publishSensorControlMode(bool enabled);  // Режим AUTO/MANUAL
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    void publishSensor(float temp, float hum);  // Показания датчика
    #endif
    
    #if DEVICE_TYPE == 1
    void publishAdaptiveMode(bool enabled);     // Состояние адаптивного режима
    void publishThresholds(float lowTemp, float highTemp, float lowHum, float highHum);
    #endif
    
    #if MQTT_PUBLISH_RSSI == 1
    void publishRSSI(int rssi);                 // Уровень WiFi-сигнала
    #endif
    
    #if MQTT_PUBLISH_VERSION == 1
    void publishVersion(const char* version);
    #endif
    
    #if MQTT_PUBLISH_RESET_REASON == 1
    void publishResetReason(const char* reason);  // Причина последней перезагрузки
    #endif
    
    // --- Колбэки на входящие команды ---
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
    
private:
    void reconnect();                    // Попытка переподключения к брокеру
    void setupTopics();                  // Формирование MQTT-топиков на основе clientId
    void subscribe();                    // Подписка на управляющие топики
    void callback(char* topic, byte* payload, unsigned int length);
    static void staticCallback(char* topic, byte* payload, unsigned int length);
    void handleCommand(const char* topic, const String& payload);
    
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    
    // Хранение топиков (pre-allocated, не String)
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
    
    // Сохранённые параметры для reconnect
    char _broker[64];
    uint16_t _port;
    char _user[32];
    char _password[64];
    
    // Колбэки (std::function допустим на ESP32, на ESP8266 экономит Flash)
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

extern MQTTManager mqttManager;
#endif // MQTT_ENABLED == 1
#endif // MQTT_H