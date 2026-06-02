#include "config.h"

#include "led.h"
#include "ansi.h"

#if MQTT_ENABLED == 1
#include "mqtt.h"

MQTTManager::MQTTManager() 
    : _mqttClient(_wifiClient)
    , _initialized(false)
    , _lastReconnectAttempt(0) {
    memset(&_topics, 0, sizeof(_topics));
}

MQTTManager::~MQTTManager() {
    disconnect();
}

bool MQTTManager::begin(const Config& cfg) {
    if (strlen(cfg.mqttBroker) == 0) {
        #if DEBUG_ENABLED == 1
        Serial.println("[MQTT] No broker configured");
        #endif
        return false;
    }
    
    strncpy(_clientId, cfg.mqttClientId, sizeof(_clientId) - 1);
    _clientId[sizeof(_clientId) - 1] = '\0';
    
    setupTopics();
    
    _mqttClient.setServer(cfg.mqttBroker, cfg.mqttPort);
    _mqttClient.setCallback(staticCallback);
    _mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);
    _mqttClient.setBufferSize(512);


    _initialized = true;
    #if LOG_MQTT == 1
        Serial.print(ANSI_BRIGHT_MAGENTA);
        Serial.printf("[MQTT] Initialized for %s with keepalive = %d sec\n", _clientId,MQTT_KEEPALIVE_SEC);
        Serial.print(ANSI_RESET);
    #endif    
    return true;
}

void MQTTManager::setupTopics() {
    
    const char* prefix = _clientId;
    
    // Общие топики
    snprintf(_topics.online, sizeof(_topics.online), "%s/status", prefix);
    snprintf(_topics.version, sizeof(_topics.version), "%s/version", prefix);
    snprintf(_topics.reset, sizeof(_topics.reset), "%s/c/system/reset", prefix);
    
    #if DEVICE_TYPE == 1
    // TYPE 1: вентилятор с датчиком
    snprintf(_topics.state, sizeof(_topics.state), "%s/fan/state", prefix);
    snprintf(_topics.control, sizeof(_topics.control), "%s/c/fan/state", prefix);
    snprintf(_topics.speed, sizeof(_topics.speed), "%s/fan/speed", prefix);                    
    snprintf(_topics.speedControl, sizeof(_topics.speedControl), "%s/c/fan/speed", prefix);    
    snprintf(_topics.delaySec, sizeof(_topics.delaySec), "%s/fan/delaySec", prefix);
    snprintf(_topics.delaySecControl, sizeof(_topics.delaySecControl), "%s/c/fan/delaySec", prefix);
    snprintf(_topics.maxOnTime, sizeof(_topics.maxOnTime), "%s/fan/maxOnTime", prefix);
    snprintf(_topics.maxOnTimeControl, sizeof(_topics.maxOnTimeControl), "%s/c/fan/maxOnTime", prefix);
    snprintf(_topics.sensorControlMode, sizeof(_topics.sensorControlMode), "%s/fan/sensorControlMode", prefix);
    snprintf(_topics.sensorControlModeControl, sizeof(_topics.sensorControlModeControl), "%s/c/fan/sensorControlMode", prefix);
    snprintf(_topics.adaptiveMode, sizeof(_topics.adaptiveMode), "%s/fan/adaptiveMode", prefix);
    snprintf(_topics.adaptiveModeControl, sizeof(_topics.adaptiveModeControl), "%s/c/fan/adaptiveMode", prefix);
    
    // Датчик
    snprintf(_topics.temperature, sizeof(_topics.temperature), "%s/sensor/temperature", prefix);
    snprintf(_topics.humidity, sizeof(_topics.humidity), "%s/sensor/humidity", prefix);
    
    // Пороги
    snprintf(_topics.lowTemp, sizeof(_topics.lowTemp), "%s/sensor/lowTemp", prefix);
    snprintf(_topics.highTemp, sizeof(_topics.highTemp), "%s/sensor/highTemp", prefix);
    snprintf(_topics.lowHum, sizeof(_topics.lowHum), "%s/sensor/lowHum", prefix);
    snprintf(_topics.highHum, sizeof(_topics.highHum), "%s/sensor/highHum", prefix);
    snprintf(_topics.lowTempControl, sizeof(_topics.lowTempControl), "%s/c/sensor/lowTemp", prefix);
    snprintf(_topics.highTempControl, sizeof(_topics.highTempControl), "%s/c/sensor/highTemp", prefix);
    snprintf(_topics.lowHumControl, sizeof(_topics.lowHumControl), "%s/c/sensor/lowHum", prefix);
    snprintf(_topics.highHumControl, sizeof(_topics.highHumControl), "%s/c/sensor/highHum", prefix);
    
    #elif DEVICE_TYPE == 2
    // TYPE 2: только датчик
    snprintf(_topics.temperature, sizeof(_topics.temperature), "%s/sensor/temperature", prefix);
    snprintf(_topics.humidity, sizeof(_topics.humidity), "%s/sensor/humidity", prefix);
    
    #elif DEVICE_TYPE == 3
    // TYPE 3: управляемый выключатель
    snprintf(_topics.state, sizeof(_topics.state), "%s/switch/state", prefix);
    snprintf(_topics.control, sizeof(_topics.control), "%s/c/switch/state", prefix);
    
    snprintf(_topics.delaySec, sizeof(_topics.delaySec), "%s/switch/delaySec", prefix);
    snprintf(_topics.delaySecControl, sizeof(_topics.delaySecControl), "%s/c/switch/delaySec", prefix);
    
    snprintf(_topics.maxOnTime, sizeof(_topics.maxOnTime), "%s/switch/maxOnTime", prefix);
    snprintf(_topics.maxOnTimeControl, sizeof(_topics.maxOnTimeControl), "%s/c/switch/maxOnTime", prefix);

    #endif
    
    #if MQTT_PUBLISH_RSSI == 1
    snprintf(_topics.rssi, sizeof(_topics.rssi), "%s/rssi", prefix);
    #endif
    
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Topics configured with prefix: %s\n", prefix);
    Serial.printf("[MQTT] Online topic: %s\n", _topics.online);
    Serial.printf("[MQTT] State topic: %s\n", _topics.state);
    Serial.printf("[MQTT] Speed topic: %s\n", _topics.speed);
    #endif
}

void MQTTManager::process() {
    if (!_initialized) return;
    
    if (!isConnected()) {
        reconnect();
    }
    
    if (isConnected()) {
        _mqttClient.loop();
    }
}

bool MQTTManager::isConnected() {
    return _mqttClient.connected();
}

void MQTTManager::disconnect() {
    if (_mqttClient.connected()) {
        _mqttClient.publish(_topics.online, "Offline", true);
        _mqttClient.disconnect();
    }
}

void MQTTManager::reconnect() {
    if (!_initialized) return;
    if (isConnected()) return;
    
    unsigned long now = millis();
    if (now - _lastReconnectAttempt < MQTT_RECONNECT_DELAY_MS) return;
    _lastReconnectAttempt = now;
    
    
    static bool lostLogged = false;
    if (!lostLogged) {
        #if LOG_MQTT == 1
            Serial.print(ANSI_BRIGHT_RED);
            Serial.println("[MQTT] Connection lost, attempting to reconnect...");
            Serial.print(ANSI_RESET);
        #endif
        lostLogged = true;
    }
    
    #if LOG_MQTT == 1
        Serial.printf("[MQTT] Connecting to broker as %s\n", _clientId);
    #endif

    #if STATUS_LED_PIN > 0
        led_setMode(LED_MODE_FAST_BLINK);
    #endif

    if (_mqttClient.connect(_clientId, config.mqttUser, config.mqttPassword,
                            _topics.online, 1, true, "Offline")) {
        #if LOG_MQTT == 1
            Serial.printf(ANSI_BRIGHT_MAGENTA "[MQTT] Connected! MQTT: " ANSI_BOLD "%s" ANSI_RESET "\n",config.mqttBroker);
        #endif
        lostLogged = false;

        #if STATUS_LED_PIN > 0
            led_setMode(LED_MODE_ON);
        #endif

        publishOnline();
        
        #if MQTT_PUBLISH_RESET_REASON == 1
            if (strlen(lastResetReason) > 0) {
                publishResetReason();
            }
        #endif
        subscribe();
        publishConfig();
    
    } else {
        
        #if LOG_MQTT == 1
            // Только одно сообщение об ошибке, не спамим
            static bool failLogged = false;
            if (!failLogged) {
                Serial.printf("[MQTT] Failed to connect, state=%d\n", _mqttClient.state());
                failLogged = true;
            }
        #endif
    }
}


#if MQTT_PUBLISH_RESET_REASON == 1
void MQTTManager::publishResetReason() {
  if (!isConnected()) return;
  
  bool shouldIgnore = false;
  #ifdef MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS
    if (strcmp(lastResetReason, "POWER_ON") == 0 ||
        strcmp(lastResetReason, "SOFT_RESTART") == 0) {
      shouldIgnore = true;
    }
  #endif
  
  if (shouldIgnore) {
    #if LOG_MQTT == 1
      Serial.printf("[MQTT] Skipping publish reset reason: " ANSI_BOLD "%s" ANSI_RESET "\n", lastResetReason);
    #endif
    return;
  }
  
  char topic[64];
  snprintf(topic, sizeof(topic), "%s/last_reset", _clientId);
  _mqttClient.publish(topic, lastResetReason, true);
  #if LOG_MQTT == 1
    Serial.printf("[MQTT] Reset reason published: " ANSI_BOLD "%s -> %s" ANSI_RESET "\n", lastResetReason, topic);
  #endif
}
#endif


void MQTTManager::subscribe() {
    #if MQTT_RESET_ENABLED == 1
    _mqttClient.subscribe(_topics.reset);
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    _mqttClient.subscribe(_topics.control);        
    _mqttClient.subscribe(_topics.delaySecControl);
    _mqttClient.subscribe(_topics.maxOnTimeControl);  
    #endif
    
    #if DEVICE_TYPE == 1
    _mqttClient.subscribe(_topics.speedControl);
    _mqttClient.subscribe(_topics.sensorControlModeControl);
    _mqttClient.subscribe(_topics.adaptiveModeControl);
    _mqttClient.subscribe(_topics.lowTempControl);
    _mqttClient.subscribe(_topics.highTempControl);
    _mqttClient.subscribe(_topics.lowHumControl);
    _mqttClient.subscribe(_topics.highHumControl);
    #endif
    
    #if LOG_MQTT == 1
    Serial.println("[MQTT] Subscribed to control topics");
    #endif
}

void MQTTManager::staticCallback(char* topic, byte* payload, unsigned int length) {
    mqttManager.callback(topic, payload, length);
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    char msg[length + 1];
    memcpy(msg, payload, length);
    msg[length] = '\0';

        handleCommand(topic, msg);
    }

void MQTTManager::handleCommand(const char* topic, const String& payload) {
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Command received: %s = %s\n", topic, payload.c_str());
    #endif
    
    // Сброс настроек
    #if MQTT_RESET_ENABLED == 1
    if (strcmp(topic, _topics.reset) == 0) {
        if (payload == "1" && _resetCallback) {
            _resetCallback();
        }
        return;
    }
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    
    // Прямое управление ON/OFF
    if (strcmp(topic, _topics.control) == 0) {
        bool state = (payload == "ON" || payload == "1");
        if (_stateCallback) _stateCallback(state);
        return;
    }
    
    // Управление скоростью (ШИМ)
    if (strcmp(topic, _topics.speedControl) == 0) {
        int speed = payload.toInt();
        if (speed >= 0 && speed <= 100 && _speedCallback) {
            _speedCallback(speed);
        }
        return;
    }
    
    // Задержка отложенного включения
    if (strcmp(topic, _topics.delaySecControl) == 0) {
        int delaySec = payload.toInt();
        if (delaySec >= 0 && delaySec <= 86400 && _delaySecCallback) {
            _delaySecCallback(delaySec);
        }
        return;
    }
    
    // Аварийное отключение
    if (strcmp(topic, _topics.maxOnTimeControl) == 0) {
        uint32_t maxOnTime = payload.toInt();
        if (maxOnTime <= 86400 && _maxOnTimeCallback) {
            _maxOnTimeCallback(maxOnTime);
        }
        return;
    }
    
    // Режим управления сенсором
    if (strcmp(topic, _topics.sensorControlModeControl) == 0) {
        bool enabled = (payload == "ON" || payload == "1" || payload == "AUTO");
        if (_sensorControlModeCallback) _sensorControlModeCallback(enabled);
        return;
    }
    
    #endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    
    #if DEVICE_TYPE == 1
    
    // Адаптивный режим
    if (strcmp(topic, _topics.adaptiveModeControl) == 0) {
        bool enabled = (payload == "1" || payload == "ON");
        if (_adaptiveModeCallback) _adaptiveModeCallback(enabled);
        return;
    }
    
    // Пороги температуры
    if (strcmp(topic, _topics.lowTempControl) == 0) {
        float value = payload.toFloat();
        if (_lowTempCallback) _lowTempCallback(value);
        return;
    }
    
    if (strcmp(topic, _topics.highTempControl) == 0) {
        float value = payload.toFloat();
        if (_highTempCallback) _highTempCallback(value);
        return;
    }
    
    // Пороги влажности
    if (strcmp(topic, _topics.lowHumControl) == 0) {
        float value = payload.toFloat();
        if (_lowHumCallback) _lowHumCallback(value);
        return;
    }
    
    if (strcmp(topic, _topics.highHumControl) == 0) {
        float value = payload.toFloat();
        if (_highHumCallback) _highHumCallback(value);
        return;
    }
    
    #endif // DEVICE_TYPE == 1
}

// ========== ПУБЛИКАЦИИ ==========

void MQTTManager::publishOnline() {
    if (!isConnected()) return;
    _mqttClient.publish(_topics.online, "Online", true);
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Online published to: %s\n", _topics.online);
    #endif
}

void MQTTManager::publishState(bool on) {
    if (!isConnected()) return;
    _mqttClient.publish(_topics.state, on ? "ON" : "OFF");
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] State published: %s -> %s\n", on ? "ON" : "OFF", _topics.state);
    #endif
}

void MQTTManager::publishSpeed(uint16_t speed) {
    if (!isConnected()) return;
    
    char speedBuf[8];
    snprintf(speedBuf, sizeof(speedBuf), "%d", speed);
    _mqttClient.publish(_topics.speed, speedBuf);
        #if LOG_MQTT == 1
        Serial.printf("[MQTT] Speed published: %d%% -> %s\n", speed, _topics.speed);
        #endif
    }

void MQTTManager::publishDelaySec(int seconds) {
    if (!isConnected()) return;
    
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", seconds);
    _mqttClient.publish(_topics.delaySec, buf);
    
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Delay seconds published: %d -> %s\n", seconds, _topics.delaySec);
    #endif
}

void MQTTManager::publishMaxOnTime(uint32_t seconds) {
    if (!isConnected()) return;
    
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", seconds);  
    _mqttClient.publish(_topics.maxOnTime, buf);
    
    
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Max on time published: %d -> %s\n", seconds, _topics.maxOnTime);
    #endif
}

void MQTTManager::publishSensorControlMode(bool enabled) {
    if (!isConnected()) return;
    _mqttClient.publish(_topics.sensorControlMode, enabled ? "1" : "0");
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Sensor control mode published: %s -> %s\n", enabled ? "ON" : "OFF", _topics.sensorControlMode);
    #endif
}

void MQTTManager::publishConfig() {
    if (!isConnected()) return;
    
    // Публикуем версию прошивки (только один раз)
    static bool versionPublished = false;
    if (!versionPublished) {
        _mqttClient.publish(_topics.version, VERSION, true);
        versionPublished = true;
        #if LOG_MQTT == 1
        Serial.printf("[MQTT] Version published: %s -> %s\n", VERSION, _topics.version);
        #endif
    }
    
    // Публикуем основные параметры
    #if DEVICE_TYPE == 1
    publishSpeed(config.speedPercent);
    publishSensorControlMode(config.sensorControlMode);
    publishThresholds();
    #endif

    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3        
    publishDelaySec(config.delaySeconds);
    publishMaxOnTime(config.maxOnTime);
    #endif 
    
    #if LOG_MQTT == 1
    Serial.println("[MQTT] Config published");
    #endif
}

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
void MQTTManager::publishSensor(float temp, float hum) {
    if (!isConnected()) return;

    char tempBuf[16], humBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.2f", temp);
    snprintf(humBuf, sizeof(humBuf), "%.2f", hum);
    _mqttClient.publish(_topics.temperature, tempBuf);
    _mqttClient.publish(_topics.humidity, humBuf);
    
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Sensor published: T=%.2f°C, H=%.2f%%\n", temp, hum);
    #endif
}
#endif

#if DEVICE_TYPE == 1
void MQTTManager::publishAdaptiveMode(bool enabled) {
    if (!isConnected()) return;
    _mqttClient.publish(_topics.adaptiveMode, enabled ? "1" : "0");
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Adaptive mode published: %s -> %s\n", enabled ? "ON" : "OFF", _topics.adaptiveMode);
    #endif
}

void MQTTManager::publishLowTemp(float temp) {
    if (!isConnected()) return;   
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", temp);  
    _mqttClient.publish(_topics.lowTemp, buf);
}

void MQTTManager::publishHighTemp(float temp) {
    if (!isConnected()) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", temp);
    _mqttClient.publish(_topics.highTemp, buf);
}

void MQTTManager::publishLowHum(float hum) {
    if (!isConnected()) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", hum);
    _mqttClient.publish(_topics.lowHum, buf);
}

void MQTTManager::publishHighHum(float hum) {
    if (!isConnected()) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", hum);
    _mqttClient.publish(_topics.highHum, buf);
}

void MQTTManager::publishThresholds() {
    if (!isConnected()) return;
    publishLowTemp(config.lowTemp);
    publishHighTemp(config.highTemp);
    publishLowHum(config.lowHum);
    publishHighHum(config.highHum);
    #if LOG_MQTT == 1
    Serial.println("[MQTT] Thresholds published");
    #endif
}
#endif

#if MQTT_PUBLISH_RSSI == 1
void MQTTManager::publishRSSI() {
    if (!isConnected()) return;
    if (WiFi.status() != WL_CONNECTED) return;
    
    int rssi = WiFi.RSSI();
    char buffer[8];  // достаточно для "-100" + null terminator
    snprintf(buffer, sizeof(buffer), "%d", rssi);
    _mqttClient.publish(_topics.rssi, buffer);

    #if LOG_MQTT == 1
    Serial.printf("[MQTT] WiFi RSSI published: %d dBm -> %s\n", rssi, _topics.rssi);
    #endif
}
#endif

// ========== УСТАНОВКА КОЛБЭКОВ ==========

void MQTTManager::onStateCommand(std::function<void(bool)> callback) {
    _stateCallback = callback;
}

void MQTTManager::onSpeedCommand(std::function<void(int)> callback) {
    _speedCallback = callback;
}

void MQTTManager::onDelaySecCommand(std::function<void(int)> callback) {
    _delaySecCallback = callback;
}

void MQTTManager::onMaxOnTimeCommand(std::function<void(uint32_t)> callback) {
    _maxOnTimeCallback = callback;
}

void MQTTManager::onSensorControlModeCommand(std::function<void(bool)> callback) {
    _sensorControlModeCallback = callback;
}

#if DEVICE_TYPE == 1
void MQTTManager::onAdaptiveModeCommand(std::function<void(bool)> callback) {
    _adaptiveModeCallback = callback;
}

void MQTTManager::onLowTempCommand(std::function<void(float)> callback) {
    _lowTempCallback = callback;
}

void MQTTManager::onHighTempCommand(std::function<void(float)> callback) {
    _highTempCallback = callback;
}

void MQTTManager::onLowHumCommand(std::function<void(float)> callback) {
    _lowHumCallback = callback;
}

void MQTTManager::onHighHumCommand(std::function<void(float)> callback) {
    _highHumCallback = callback;
}
#endif

#if MQTT_RESET_ENABLED == 1
void MQTTManager::onResetCommand(std::function<void()> callback) {
    _resetCallback = callback;
}
#endif

// Глобальный экземпляр
MQTTManager mqttManager;

#endif