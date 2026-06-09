#include "config.h"
#include "led.h"
#include "ansi.h"
#include "wifi_manager.h"

#if MQTT_ENABLED == 1
#include "mqtt.h"

MQTTManager::MQTTManager() 
    : _mqttClient(_wifiClient)
    , _initialized(false)
    , _lastReconnectAttempt(0)
    , _port(1883) {
    memset(&_topics, 0, sizeof(_topics));
    memset(_broker, 0, sizeof(_broker));
    memset(_user, 0, sizeof(_user));
    memset(_password, 0, sizeof(_password));
}

MQTTManager::~MQTTManager() {
    disconnect();
}

bool MQTTManager::begin(const char* broker, uint16_t port, const char* clientId,
                        const char* user, const char* password) {
    if (strlen(broker) == 0) {
        #if DEBUG_ENABLED == 1
        Serial.println("[MQTT] No broker configured");
        #endif
        return false;
    }
    
    strncpy(_clientId, clientId, sizeof(_clientId) - 1);
    _clientId[sizeof(_clientId) - 1] = '\0';
    
    // Сохраняем для reconnect
    strncpy(_broker, broker, sizeof(_broker) - 1);
    _broker[sizeof(_broker) - 1] = '\0';
    _port = port;
    
    if (user) {
        strncpy(_user, user, sizeof(_user) - 1);
        _user[sizeof(_user) - 1] = '\0';
    }
    if (password) {
        strncpy(_password, password, sizeof(_password) - 1);
        _password[sizeof(_password) - 1] = '\0';
    }
    
    setupTopics();
    
    _mqttClient.setServer(broker, port);
    _mqttClient.setCallback(staticCallback);
    _mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);
    _mqttClient.setBufferSize(512);
    
    _initialized = true;
    
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Initialized for %s with keepalive = %d sec\n", _clientId, MQTT_KEEPALIVE_SEC);
    #endif
    
    return true;
}

void MQTTManager::setupTopics() {
    const char* prefix = _clientId;
    
    snprintf(_topics.online, sizeof(_topics.online), "%s/status", prefix);
    
    #if MQTT_PUBLISH_VERSION == 1
    snprintf(_topics.version, sizeof(_topics.version), "%s/version", prefix);
    #endif

    snprintf(_topics.reset, sizeof(_topics.reset), "%s/c/system/reset", prefix);
    
    #if MQTT_PUBLISH_RSSI == 1
    snprintf(_topics.rssi, sizeof(_topics.rssi), "%s/rssi", prefix);
    #endif


    #if DEVICE_TYPE == 1
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
    
    snprintf(_topics.lowTemp, sizeof(_topics.lowTemp), "%s/sensor/lowTemp", prefix);
    snprintf(_topics.highTemp, sizeof(_topics.highTemp), "%s/sensor/highTemp", prefix);
    snprintf(_topics.lowHum, sizeof(_topics.lowHum), "%s/sensor/lowHum", prefix);
    snprintf(_topics.highHum, sizeof(_topics.highHum), "%s/sensor/highHum", prefix);
    snprintf(_topics.lowTempControl, sizeof(_topics.lowTempControl), "%s/c/sensor/lowTemp", prefix);
    snprintf(_topics.highTempControl, sizeof(_topics.highTempControl), "%s/c/sensor/highTemp", prefix);
    snprintf(_topics.lowHumControl, sizeof(_topics.lowHumControl), "%s/c/sensor/lowHum", prefix);
    snprintf(_topics.highHumControl, sizeof(_topics.highHumControl), "%s/c/sensor/highHum", prefix);
    #endif

    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    snprintf(_topics.temperature, sizeof(_topics.temperature), "%s/sensor/temperature", prefix);
    snprintf(_topics.humidity, sizeof(_topics.humidity), "%s/sensor/humidity", prefix);
    #endif

    #if DEVICE_TYPE == 3
    snprintf(_topics.state, sizeof(_topics.state), "%s/switch/state", prefix);
    snprintf(_topics.control, sizeof(_topics.control), "%s/c/switch/state", prefix);
    snprintf(_topics.delaySec, sizeof(_topics.delaySec), "%s/switch/delaySec", prefix);
    snprintf(_topics.delaySecControl, sizeof(_topics.delaySecControl), "%s/c/switch/delaySec", prefix);
    snprintf(_topics.maxOnTime, sizeof(_topics.maxOnTime), "%s/switch/maxOnTime", prefix);
    snprintf(_topics.maxOnTimeControl, sizeof(_topics.maxOnTimeControl), "%s/c/switch/maxOnTime", prefix);
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
    static bool wasConnectedBefore = false;  // Был ли хотя бы один успешный коннект?

    static bool lostLogged = false;
    if (!wasConnectedBefore) {
        // Первое подключение в жизни устройства
        #if LOG_MQTT == 1
        Serial.println("[MQTT] Connecting to broker...");
        #endif
    } else if (!lostLogged) {
        // Были подключены, но потеряли связь
        #if LOG_MQTT == 1
        Serial.println("[MQTT] Connection lost, attempting to reconnect...");
        #endif
        lostLogged = true;
    }
    
    #if STATUS_LED_PIN > 0
    led_setMode(LED_MODE_FAST_BLINK);
    #endif
    
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Connecting to broker as %s\n", _clientId);
    #endif
    
    bool connected;
    if (strlen(_user) > 0) {
        connected = _mqttClient.connect(_clientId, _user, _password,
                                        _topics.online, 1, true, "Offline");
    } else {
        connected = _mqttClient.connect(_clientId, _topics.online, 1, true, "Offline");
    }
    
    if (connected) {
        #if LOG_MQTT == 1
        Serial.printf("[MQTT] Connected! Broker: %s\n", _broker);
        #endif
        
        wasConnectedBefore = true;
        
        lostLogged = false;
        
        #if STATUS_LED_PIN > 0
        led_setMode(LED_MODE_ON);
        #endif
        
        publishOnline();
        subscribe();
    
    } else {
        #if LOG_MQTT == 1
        static bool failLogged = false;
        if (!failLogged) {
            Serial.printf("[MQTT] Failed to connect, state=%d\n", _mqttClient.state());
            failLogged = true;
        }
        #endif
    }
}

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
    
    #if MQTT_RESET_ENABLED == 1
    if (strcmp(topic, _topics.reset) == 0) {
        if (payload == "1" && _resetCallback) {
            _resetCallback();
        }
        return;
    }
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    if (strcmp(topic, _topics.control) == 0) {
        bool state = (payload == "ON" || payload == "1");
        if (_stateCallback) _stateCallback(state);
        return;
    }
    
    if (strcmp(topic, _topics.speedControl) == 0) {
        int speed = payload.toInt();
        if (speed >= 0 && speed <= 100 && _speedCallback) {
            _speedCallback(speed);
        }
        return;
    }
    
    if (strcmp(topic, _topics.delaySecControl) == 0) {
        int delaySec = payload.toInt();
        if (delaySec >= 0 && delaySec <= 86400 && _delaySecCallback) {
            _delaySecCallback(delaySec);
        }
        return;
    }
    
    if (strcmp(topic, _topics.maxOnTimeControl) == 0) {
        uint32_t maxOnTime = payload.toInt();
        if (maxOnTime <= 86400 && _maxOnTimeCallback) {
            _maxOnTimeCallback(maxOnTime);
        }
        return;
    }
    
    if (strcmp(topic, _topics.sensorControlModeControl) == 0) {
        bool enabled = (payload == "ON" || payload == "1" || payload == "AUTO");
        if (_sensorControlModeCallback) _sensorControlModeCallback(enabled);
        return;
    }
    #endif
    
    #if DEVICE_TYPE == 1
    if (strcmp(topic, _topics.adaptiveModeControl) == 0) {
        bool enabled = (payload == "1" || payload == "ON");
        if (_adaptiveModeCallback) _adaptiveModeCallback(enabled);
        return;
    }
    
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
    #endif
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

void MQTTManager::publishSpeed(int percent) {
    if (!isConnected()) return;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", percent);
    _mqttClient.publish(_topics.speed, buf);
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Speed published: %d%% -> %s\n", percent, _topics.speed);
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
    Serial.printf("[MQTT] Max on time published: %u -> %s\n", seconds, _topics.maxOnTime);
    #endif
}

void MQTTManager::publishSensorControlMode(bool enabled) {
    if (!isConnected()) return;
    _mqttClient.publish(_topics.sensorControlMode, enabled ? "1" : "0");
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Sensor control mode published: %s -> %s\n", enabled ? "ON" : "OFF", _topics.sensorControlMode);
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

void MQTTManager::publishThresholds(float lowTemp, float highTemp, float lowHum, float highHum) {
    if (!isConnected()) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", lowTemp);
    _mqttClient.publish(_topics.lowTemp, buf);
    snprintf(buf, sizeof(buf), "%.1f", highTemp);
    _mqttClient.publish(_topics.highTemp, buf);
    snprintf(buf, sizeof(buf), "%.1f", lowHum);
    _mqttClient.publish(_topics.lowHum, buf);
    snprintf(buf, sizeof(buf), "%.1f", highHum);
    _mqttClient.publish(_topics.highHum, buf);
    #if LOG_MQTT == 1
    Serial.println("[MQTT] Thresholds published");
    #endif
}
#endif

#if MQTT_PUBLISH_RSSI == 1
void MQTTManager::publishRSSI(int rssi) {
    if (!isConnected()) return;
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", rssi);
    _mqttClient.publish(_topics.rssi, buffer);
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] WiFi RSSI published: %d dBm -> %s\n", rssi, _topics.rssi);
    #endif
}
#endif

#if MQTT_PUBLISH_VERSION == 1
void MQTTManager::publishVersion(const char* version) {
    if (!isConnected()) return;
    _mqttClient.publish(_topics.version, version, true);  // retain = true
    #if LOG_MQTT == 1
        Serial.printf("[MQTT] Version published: %s -> %s\n", version, _topics.version);
    #endif
}
#endif

#if MQTT_PUBLISH_RESET_REASON == 1
void MQTTManager::publishResetReason(const char* reason) {
    if (!isConnected()) return;
    
    #ifdef MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS
    if (strcmp(reason, "POWER_ON") == 0 || strcmp(reason, "SOFT_RESTART") == 0) {
        #if LOG_MQTT == 1
        Serial.printf("[MQTT] Skipping publish reset reason: %s\n", reason);
        #endif
        return;
    }
    #endif
    
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/last_reset", _clientId);
    _mqttClient.publish(topic, reason, true);
    #if LOG_MQTT == 1
    Serial.printf("[MQTT] Reset reason published: %s -> %s\n", reason, topic);
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