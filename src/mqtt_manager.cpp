/**
 * @file mqtt_manager.cpp
 * @brief Реализация MQTTManager
 */

#include "logger.h"
#include "mqtt_manager.h"
#include "system_state.h"

#if FEATURE_MQTT_ENABLED == 1

// ============================================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================================

MQTTManager::MQTTManager()
    : _mqttClient(),
      _initialized(false),
      _lastReconnectAttempt(0),
      _port(1883) {
  memset(&_topics, 0, sizeof(_topics));
  memset(_broker, 0, sizeof(_broker));
  memset(_user, 0, sizeof(_user));
  memset(_password, 0, sizeof(_password));
  memset(_clientId, 0, sizeof(_clientId));
}

MQTTManager::~MQTTManager() {
  disconnect();
}

// ============================================================================
// УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ
// ============================================================================

bool MQTTManager::begin(Client& client,
                        const char* broker,
                        uint16_t port,
                        const char* clientId,
                        const char* user,
                        const char* password) {
  if (strlen(broker) == 0) {
    XLOG_INFO(CAT_MQTT, "No broker configured");
    return false;
  }

  // Сохраняем параметры для reconnect
  strncpy(_clientId, clientId, sizeof(_clientId) - 1);
  _clientId[sizeof(_clientId) - 1] = '\0';

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

  _mqttClient.setClient(client);
  _mqttClient.setServer(broker, port);
  _mqttClient.setCallback(staticCallback);
  _mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);
  _mqttClient.setBufferSize(512);  // Для JSON-сообщений

  _initialized = true;

  XLOG_DEBUG(CAT_MQTT, "Initialized for %s", _clientId);
  return true;
}

void MQTTManager::update() {
  if (!_initialized)
    return;

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

// ============================================================================
// ПЕРЕПОДКЛЮЧЕНИЕ
// ============================================================================

void MQTTManager::reconnect() {

  if (!_initialized)
    return;
  if (isConnected())
    return;

  unsigned long now = millis();

  if (!firstAttempt) {
    if (now - _lastReconnectAttempt < MQTT_RECONNECT_DELAY_MS)
      return;
  }

  
  _lastReconnectAttempt = now;
  
  if (firstAttempt) {
    XLOG_DEBUG(CAT_MQTT,
               "Connecting to broker " ANSI_BOLD "%s" ANSI_BOLD_RESET
               ", client " ANSI_BOLD  "%s",
               _broker, _clientId);
    firstAttempt = false;
  } else {
    XLOG_DEBUG(CAT_MQTT, "Reconnecting...");
  }

  bool connected;
  if (strlen(_user) > 0) {
    connected = _mqttClient.connect(_clientId, _user, _password, _topics.online,
                                    1, true, "Offline");
  } else {
    connected =
        _mqttClient.connect(_clientId, _topics.online, 1, true, "Offline");
  }

  if (connected) {
    XLOG_INFO(CAT_MQTT, "Connected to " ANSI_BOLD "%s." ANSI_RESET, _broker);
    
    firstAttempt = true;
    publishOnline();
    subscribe();
    system_state_set_bit(STATE_MQTT_OK);

  } else {
    XLOG_ERROR(CAT_MQTT, "Failed, state=%d", _mqttClient.state());
  }
}

// ============================================================================
// ТОПИКИ И ПОДПИСКИ
// ============================================================================

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
  snprintf(_topics.speedControl, sizeof(_topics.speedControl), "%s/c/fan/speed",
           prefix);
  snprintf(_topics.delaySec, sizeof(_topics.delaySec), "%s/fan/delaySec",
           prefix);
  snprintf(_topics.delaySecControl, sizeof(_topics.delaySecControl),
           "%s/c/fan/delaySec", prefix);
  snprintf(_topics.maxOnTime, sizeof(_topics.maxOnTime), "%s/fan/maxOnTime",
           prefix);
  snprintf(_topics.maxOnTimeControl, sizeof(_topics.maxOnTimeControl),
           "%s/c/fan/maxOnTime", prefix);
  snprintf(_topics.sensorControlMode, sizeof(_topics.sensorControlMode),
           "%s/fan/sensorControlMode", prefix);
  snprintf(_topics.sensorControlModeControl,
           sizeof(_topics.sensorControlModeControl),
           "%s/c/fan/sensorControlMode", prefix);
  snprintf(_topics.adaptiveMode, sizeof(_topics.adaptiveMode),
           "%s/fan/adaptiveMode", prefix);
  snprintf(_topics.adaptiveModeControl, sizeof(_topics.adaptiveModeControl),
           "%s/c/fan/adaptiveMode", prefix);

  snprintf(_topics.lowTemp, sizeof(_topics.lowTemp), "%s/sensor/lowTemp",
           prefix);
  snprintf(_topics.highTemp, sizeof(_topics.highTemp), "%s/sensor/highTemp",
           prefix);
  snprintf(_topics.lowHum, sizeof(_topics.lowHum), "%s/sensor/lowHum", prefix);
  snprintf(_topics.highHum, sizeof(_topics.highHum), "%s/sensor/highHum",
           prefix);
  snprintf(_topics.lowTempControl, sizeof(_topics.lowTempControl),
           "%s/c/sensor/lowTemp", prefix);
  snprintf(_topics.highTempControl, sizeof(_topics.highTempControl),
           "%s/c/sensor/highTemp", prefix);
  snprintf(_topics.lowHumControl, sizeof(_topics.lowHumControl),
           "%s/c/sensor/lowHum", prefix);
  snprintf(_topics.highHumControl, sizeof(_topics.highHumControl),
           "%s/c/sensor/highHum", prefix);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  snprintf(_topics.temperature, sizeof(_topics.temperature),
           "%s/sensor/temperature", prefix);
  snprintf(_topics.humidity, sizeof(_topics.humidity), "%s/sensor/humidity",
           prefix);
#endif

#if DEVICE_TYPE == 3
  snprintf(_topics.state, sizeof(_topics.state), "%s/switch/state", prefix);
  snprintf(_topics.control, sizeof(_topics.control), "%s/c/switch/state",
           prefix);
  snprintf(_topics.delaySec, sizeof(_topics.delaySec), "%s/switch/delaySec",
           prefix);
  snprintf(_topics.delaySecControl, sizeof(_topics.delaySecControl),
           "%s/c/switch/delaySec", prefix);
  snprintf(_topics.maxOnTime, sizeof(_topics.maxOnTime), "%s/switch/maxOnTime",
           prefix);
  snprintf(_topics.maxOnTimeControl, sizeof(_topics.maxOnTimeControl),
           "%s/c/switch/maxOnTime", prefix);
#endif
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

  XLOG_DEBUG(CAT_MQTT, "Subscribed to control topics");
}

// ============================================================================
// ОБРАБОТКА СООБЩЕНИЙ
// ============================================================================

void MQTTManager::staticCallback(char* topic,
                                 byte* payload,
                                 unsigned int length) {
  mqttManager.callback(topic, payload, length);
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';
  handleCommand(topic, String(msg));
}

void MQTTManager::handleCommand(const char* topic, const String& payload) {
  XLOG_INFO(CAT_MQTT, "Command: %s = %s", topic, payload.c_str());

  // Команда сброса
#if MQTT_RESET_ENABLED == 1
  if (strcmp(topic, _topics.reset) == 0) {
    String lower = payload;
    lower.toLowerCase();
    if (lower == "1" || lower == "on" || lower == "true" || lower == "reset") {
      if (_onReset.func)
        _onReset.func(_onReset.context);
    }
    return;
  }
#endif

  // Команда состояния (ON/OFF)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (strcmp(topic, _topics.control) == 0) {
    bool state = (payload == "ON" || payload == "1");
    if (_onState.func)
      _onState.func(state, _onState.context);
    return;
  }

  // Команда скорости
  if (strcmp(topic, _topics.speedControl) == 0) {
    int speed = payload.toInt();
    if (speed >= 0 && speed <= 100 && _onSpeed.func) {
      _onSpeed.func(speed, _onSpeed.context);
    }
    return;
  }

  // Команда задержки включения
  if (strcmp(topic, _topics.delaySecControl) == 0) {
    int delaySec = payload.toInt();
    if (delaySec >= 0 && delaySec <= 86400 && _onDelaySec.func) {
      _onDelaySec.func(delaySec, _onDelaySec.context);
    }
    return;
  }

  // Команда таймера аварийного отключения
  if (strcmp(topic, _topics.maxOnTimeControl) == 0) {
    uint32_t maxOnTime = payload.toInt();
    if (maxOnTime <= 86400 && _onMaxOnTime.func) {
      _onMaxOnTime.func(maxOnTime, _onMaxOnTime.context);
    }
    return;
  }

  // Команда сенсорного режима
  if (strcmp(topic, _topics.sensorControlModeControl) == 0) {
    bool enabled = (payload == "ON" || payload == "1" || payload == "AUTO");
    if (_onSensorControlMode.func) {
      _onSensorControlMode.func(enabled, _onSensorControlMode.context);
    }
    return;
  }
#endif

  // Команды для TYPE 1
#if DEVICE_TYPE == 1
  if (strcmp(topic, _topics.adaptiveModeControl) == 0) {
    bool enabled = (payload == "1" || payload == "ON");
    if (_onAdaptiveMode.func)
      _onAdaptiveMode.func(enabled, _onAdaptiveMode.context);
    return;
  }

  if (strcmp(topic, _topics.lowTempControl) == 0) {
    float value = payload.toFloat();
    if (_onLowTemp.func)
      _onLowTemp.func(value, _onLowTemp.context);
    return;
  }

  if (strcmp(topic, _topics.highTempControl) == 0) {
    float value = payload.toFloat();
    if (_onHighTemp.func)
      _onHighTemp.func(value, _onHighTemp.context);
    return;
  }

  if (strcmp(topic, _topics.lowHumControl) == 0) {
    float value = payload.toFloat();
    if (_onLowHum.func)
      _onLowHum.func(value, _onLowHum.context);
    return;
  }

  if (strcmp(topic, _topics.highHumControl) == 0) {
    float value = payload.toFloat();
    if (_onHighHum.func)
      _onHighHum.func(value, _onHighHum.context);
    return;
  }
#endif
}

// ============================================================================
// РЕГИСТРАЦИЯ КОЛБЭКОВ
// ============================================================================

void MQTTManager::onState(BoolCallback func, void* context) {
  _onState.func = func;
  _onState.context = context;
}

void MQTTManager::onSpeed(IntCallback func, void* context) {
  _onSpeed.func = func;
  _onSpeed.context = context;
}

void MQTTManager::onDelaySec(IntCallback func, void* context) {
  _onDelaySec.func = func;
  _onDelaySec.context = context;
}

void MQTTManager::onMaxOnTime(UintCallback func, void* context) {
  _onMaxOnTime.func = func;
  _onMaxOnTime.context = context;
}

void MQTTManager::onSensorControlMode(BoolCallback func, void* context) {
  _onSensorControlMode.func = func;
  _onSensorControlMode.context = context;
}

#if DEVICE_TYPE == 1
void MQTTManager::onAdaptiveMode(BoolCallback func, void* context) {
  _onAdaptiveMode.func = func;
  _onAdaptiveMode.context = context;
}

void MQTTManager::onLowTemp(FloatCallback func, void* context) {
  _onLowTemp.func = func;
  _onLowTemp.context = context;
}

void MQTTManager::onHighTemp(FloatCallback func, void* context) {
  _onHighTemp.func = func;
  _onHighTemp.context = context;
}

void MQTTManager::onLowHum(FloatCallback func, void* context) {
  _onLowHum.func = func;
  _onLowHum.context = context;
}

void MQTTManager::onHighHum(FloatCallback func, void* context) {
  _onHighHum.func = func;
  _onHighHum.context = context;
}
#endif

#if MQTT_RESET_ENABLED == 1
void MQTTManager::onReset(VoidCallback func, void* context) {
  _onReset.func = func;
  _onReset.context = context;
}
#endif

// ============================================================================
// ПУБЛИКАЦИЯ ДАННЫХ
// ============================================================================

void MQTTManager::publishOnline() {
  if (!isConnected())
    return;
  _mqttClient.publish(_topics.online, "Online", true);
}

void MQTTManager::publishState(bool on) {
  if (!isConnected())
    return;
  _mqttClient.publish(_topics.state, on ? "ON" : "OFF");
}

void MQTTManager::publishSpeed(int percent) {
  if (!isConnected())
    return;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", percent);
  _mqttClient.publish(_topics.speed, buf);
}

void MQTTManager::publishDelaySec(int seconds) {
  if (!isConnected())
    return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", seconds);
  _mqttClient.publish(_topics.delaySec, buf);
}

void MQTTManager::publishMaxOnTime(uint32_t seconds) {
  if (!isConnected())
    return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%u", seconds);
  _mqttClient.publish(_topics.maxOnTime, buf);
}

void MQTTManager::publishSensorControlMode(bool enabled) {
  if (!isConnected())
    return;
  _mqttClient.publish(_topics.sensorControlMode, enabled ? "1" : "0");
}

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
void MQTTManager::publishSensor(float temp, float hum) {
  if (!isConnected())
    return;
  char tempBuf[16], humBuf[16];
  snprintf(tempBuf, sizeof(tempBuf), "%.2f", temp);
  snprintf(humBuf, sizeof(humBuf), "%.2f", hum);
  _mqttClient.publish(_topics.temperature, tempBuf);
  _mqttClient.publish(_topics.humidity, humBuf);
}
#endif

#if DEVICE_TYPE == 1
void MQTTManager::publishAdaptiveMode(bool enabled) {
  if (!isConnected())
    return;
  _mqttClient.publish(_topics.adaptiveMode, enabled ? "1" : "0");
}

void MQTTManager::publishThresholds(float lowTemp,
                                    float highTemp,
                                    float lowHum,
                                    float highHum) {
  if (!isConnected())
    return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f", lowTemp);
  _mqttClient.publish(_topics.lowTemp, buf);
  snprintf(buf, sizeof(buf), "%.1f", highTemp);
  _mqttClient.publish(_topics.highTemp, buf);
  snprintf(buf, sizeof(buf), "%.1f", lowHum);
  _mqttClient.publish(_topics.lowHum, buf);
  snprintf(buf, sizeof(buf), "%.1f", highHum);
  _mqttClient.publish(_topics.highHum, buf);
}
#endif

#if MQTT_PUBLISH_RSSI == 1
void MQTTManager::publishRSSI(int rssi) {
  if (!isConnected())
    return;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", rssi);
  _mqttClient.publish(_topics.rssi, buf);
}
#endif

#if MQTT_PUBLISH_VERSION == 1
void MQTTManager::publishVersion(const char* version) {
  if (!isConnected())
    return;
  _mqttClient.publish(_topics.version, version, true);
}
#endif

#if MQTT_PUBLISH_RESET_REASON == 1
void MQTTManager::publishResetReason(const char* reason) {
  if (!isConnected())
    return;

#if MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS == 1
  if (strcmp(reason, "POWER_ON") == 0 || strcmp(reason, "SOFT_RESTART") == 0) {
    return;
  }
#endif

  char topic[64];
  snprintf(topic, sizeof(topic), "%s/last_reset", _clientId);
  _mqttClient.publish(topic, reason, true);
}
#endif

// ============================================================================
// ГЛОБАЛЬНЫЙ ЭКЗЕМПЛЯР
// ============================================================================

MQTTManager mqttManager;

#endif  // FEATURE_MQTT_ENABLED == 1