/**
 * @file transport_wifi_mqtt.cpp
 * @version 0.12
 * @brief MQTT-менеджер — реализация
 */

#include "transport_wifi_mqtt.h"
#include "logger.h"
#include "settings.h"

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#ifdef USE_MQTT

// ============================================================================
// КОНСТАНТЫ
// ============================================================================

#define MQTT_RECONNECT_DELAY_MS \
  5000                        ///< Задержка между попытками переподключения (мс)
#define MQTT_KEEPALIVE_SEC 3  ///< Интервал keep-alive для MQTT (сек)

// ============================================================================
// КОНСТРУКТОР
// ============================================================================

/**
 * @brief Конструктор MQTTManager
 * @details Инициализирует все поля значениями по умолчанию
 */
MQTTManager::MQTTManager()
    : _running(false),
      _connected(false),
      _lastReconnectAttempt(0),
      _eventCallback(nullptr),
      _eventContext(nullptr) {
  memset(&_config, 0, sizeof(_config));
  _clientId[0] = '\0';
}

// ============================================================================
// УПРАВЛЕНИЕ
// ============================================================================

/**
 * @brief Инициализация MQTT-менеджера
 * @param config Указатель на конфигурацию транспорта
 * @return true — инициализация успешна, false — ошибка
 * @note Если брокер не настроен, MQTT отключается
 *       Client ID формируется из deviceId или используется "mqtt_client"
 *       Подключение к брокеру происходит асинхронно в update()
 */
bool MQTTManager::begin(const TransportConfig* config) {
  if (!config) {
    XLOG_ERROR(CAT_MQTT, "config is NULL");
    return false;
  }

  memcpy(&_config, config, sizeof(TransportConfig));

  // Проверяем, есть ли брокер
  if (strlen(config->mqttBroker) == 0 || config->mqttPort == 0) {
    XLOG_WARN(CAT_MQTT, "MQTT broker not configured — MQTT disabled");
    _running = false;
    _connected = false;
    return false;
  }

  // Проверяем Client ID
  if (strlen(config->mqttClientId) == 0) {
    XLOG_WARN(CAT_MQTT, "MQTT Client ID is empty — using deviceId");
    if (strlen(config->deviceId) > 0) {
      strncpy(_clientId, config->deviceId, sizeof(_clientId) - 1);
      _clientId[sizeof(_clientId) - 1] = '\0';
    } else {
      strcpy(_clientId, "mqtt_client");
    }
  } else {
    strncpy(_clientId, config->mqttClientId, sizeof(_clientId) - 1);
    _clientId[sizeof(_clientId) - 1] = '\0';
  }

  // Настраиваем PubSubClient
  _mqttClient.setClient(_wifiClient);
  _mqttClient.setServer(config->mqttBroker, config->mqttPort);
  _mqttClient.setCallback(
      std::bind(&MQTTManager::callback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  _mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);

  _running = true;
  _connected = false;
  _lastReconnectAttempt = 0;

  // ===== НЕ ПЫТАЕМСЯ ПОДКЛЮЧИТЬСЯ СРАЗУ! =====
  // Подключение будет выполнено в update() после инициализации WiFi
  // reconnect();  // ← УБРАТЬ!

  XLOG_INFO(CAT_MQTT,
            "MQTT transport initialized (will connect in background)");
  return true;
}

/**
 * @brief Периодическая обработка MQTT-менеджера
 * @details Вызывается в loop(). Поддерживает соединение с брокером,
 *          обрабатывает входящие сообщения и переподключается при потере связи
 *          Первое подключение выполняется после того, как WiFi готов
 */
void MQTTManager::update() {
  if (!_running) {
    // Если MQTT отключён, но есть брокер — пытаемся переподключиться
    if (strlen(_config.mqttBroker) > 0 && _config.mqttPort > 0) {
      unsigned long now = millis();
      if (now - _lastReconnectAttempt > MQTT_RECONNECT_DELAY_MS) {
        reconnect();
        _lastReconnectAttempt = now;
      }
    }
    return;
  }

  // Проверяем соединение
  if (!_mqttClient.connected()) {
    _connected = false;
    unsigned long now = millis();

    // Пытаемся переподключиться с задержкой
    if (now - _lastReconnectAttempt > MQTT_RECONNECT_DELAY_MS) {
      XLOG_DEBUG(CAT_MQTT, "MQTT disconnected — reconnecting...");
      reconnect();
      _lastReconnectAttempt = now;
    }
  } else {
    _mqttClient.loop();

    // Если было отключено, а теперь подключились — обновляем флаг
    if (!_connected) {
      _connected = true;
      XLOG_INFO(CAT_MQTT, "MQTT reconnected");
    }
  }
} 

/**
   * @brief Проверить, подключён ли MQTT-клиент
   * @return true — подключён, false — не подключён
   */
bool MQTTManager::isConnected() {
  _connected = _mqttClient.connected();
  return _connected;
}

/**
 * @brief Принудительное отключение MQTT-клиента
 * @details Публикует статус "Offline" через LWT и разрывает соединение
 */
void MQTTManager::disconnect() {
  XLOG_INFO(CAT_MQTT, "disconnect() called");
  if (_mqttClient.connected()) {
    _mqttClient.publish(_topics.online, "Offline", true);
    _mqttClient.disconnect();
  }
  _running = false;
  _connected = false;
}

/**
 * @brief Получить имя MQTT-менеджера для логов
 * @return Строка "MQTT"
 */
const char* MQTTManager::getName() const {
  return "MQTT";
}

// ============================================================================
// КОЛБЭКИ
// ============================================================================

/**
 * @brief Регистрация колбэка для событий
 * @param callback Функция обратного вызова
 * @param context Контекст для колбэка
 */
void MQTTManager::onEvent(TransportEventCallback callback, void* context) {
  _eventCallback = callback;
  _eventContext = context;
  XLOG_DEBUG(CAT_MQTT, "onEvent() registered");
}

// ============================================================================
// ПУБЛИКАЦИЯ
// ============================================================================

/**
 * @brief Опубликовать статус Online
 */
void MQTTManager::publishOnline() {
  if (!isConnected())
    return;
  _mqttClient.publish(_topics.online, "Online", true);
  XLOG_DEBUG(CAT_MQTT, "publishOnline() → %s = Online", _topics.online);
}

/**
 * @brief Опубликовать состояние актуатора
 * @param on true — включён, false — выключен
 */
void MQTTManager::publishState(bool on) {
  if (!isConnected())
    return;
  snprintf(_topics.state, sizeof(_topics.state), "%s/state", _clientId);
  _mqttClient.publish(_topics.state, on ? "ON" : "OFF");
  XLOG_DEBUG(CAT_MQTT, "publishState(%s) → %s = %s", on ? "ON" : "OFF",
             _topics.state, on ? "ON" : "OFF");
}

/**
 * @brief Опубликовать скорость
 * @param percent Скорость 0-100%
 */
void MQTTManager::publishSpeed(int percent) {
  if (!isConnected())
    return;
  snprintf(_topics.speed, sizeof(_topics.speed), "%s/speed", _clientId);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", percent);
  _mqttClient.publish(_topics.speed, buf);
  XLOG_DEBUG(CAT_MQTT, "publishSpeed(%d%%) → %s = %d", percent, _topics.speed,
             percent);
}

/**
 * @brief Опубликовать задержку включения
 * @param seconds Задержка в секундах
 */
void MQTTManager::publishDelaySec(int seconds) {
  if (!isConnected())
    return;
  snprintf(_topics.delaySec, sizeof(_topics.delaySec), "%s/delaySec",
           _clientId);
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", seconds);
  _mqttClient.publish(_topics.delaySec, buf);
  XLOG_DEBUG(CAT_MQTT, "publishDelaySec(%d) → %s = %d", seconds,
             _topics.delaySec, seconds);
}

/**
 * @brief Опубликовать максимальное время работы
 * @param seconds Время в секундах
 */
void MQTTManager::publishMaxOnTime(uint32_t seconds) {
  if (!isConnected())
    return;
  snprintf(_topics.maxOnTime, sizeof(_topics.maxOnTime), "%s/maxOnTime",
           _clientId);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", seconds);
  _mqttClient.publish(_topics.maxOnTime, buf);
  XLOG_DEBUG(CAT_MQTT, "publishMaxOnTime(%lu) → %s = %lu", seconds,
             _topics.maxOnTime, seconds);
}

/**
 * @brief Опубликовать режим управления по датчику
 * @param enabled true — AUTO, false — MANUAL
 */
void MQTTManager::publishSensorControlMode(bool enabled) {
  if (!isConnected())
    return;
  snprintf(_topics.sensorMode, sizeof(_topics.sensorMode), "%s/sensorMode",
           _clientId);
  _mqttClient.publish(_topics.sensorMode, enabled ? "1" : "0");
  XLOG_DEBUG(CAT_MQTT, "publishSensorControlMode(%s) → %s = %s",
             enabled ? "AUTO" : "MANUAL", _topics.sensorMode,
             enabled ? "1" : "0");
}

/**
 * @brief Опубликовать показания датчика
 * @param temp Температура в °C
 * @param hum Влажность в %
 */
void MQTTManager::publishSensor(float temp, float hum) {
  if (!isConnected())
    return;
  char tempTopic[48], humTopic[48];
  snprintf(tempTopic, sizeof(tempTopic), "%s/temperature", _clientId);
  snprintf(humTopic, sizeof(humTopic), "%s/humidity", _clientId);
  char tbuf[16], hbuf[16];
  snprintf(tbuf, sizeof(tbuf), "%.1f", temp);
  snprintf(hbuf, sizeof(hbuf), "%.1f", hum);
  _mqttClient.publish(tempTopic, tbuf);
  _mqttClient.publish(humTopic, hbuf);
  XLOG_DEBUG(CAT_MQTT, "publishSensor(%.1f°C, %.1f%%) → %s, %s", temp, hum,
             tempTopic, humTopic);
}

/**
 * @brief Опубликовать состояние адаптивного режима
 * @param enabled true — включён, false — выключен
 */
void MQTTManager::publishAdaptiveMode(bool enabled) {
  if (!isConnected())
    return;
  snprintf(_topics.adaptiveMode, sizeof(_topics.adaptiveMode),
           "%s/adaptiveMode", _clientId);
  _mqttClient.publish(_topics.adaptiveMode, enabled ? "1" : "0");
  XLOG_DEBUG(CAT_MQTT, "publishAdaptiveMode(%s) → %s = %s",
             enabled ? "ON" : "OFF", _topics.adaptiveMode, enabled ? "1" : "0");
}

/**
 * @brief Опубликовать пороговые значения
 * @param lowTemp Нижний порог температуры
 * @param highTemp Верхний порог температуры
 * @param lowHum Нижний порог влажности
 * @param highHum Верхний порог влажности
 */
void MQTTManager::publishThresholds(float lowTemp,
                                    float highTemp,
                                    float lowHum,
                                    float highHum) {
  if (!isConnected())
    return;
  char lt[48], ht[48], lh[48], hh[48];
  snprintf(lt, sizeof(lt), "%s/config/lowTemp", _clientId);
  snprintf(ht, sizeof(ht), "%s/config/highTemp", _clientId);
  snprintf(lh, sizeof(lh), "%s/config/lowHum", _clientId);
  snprintf(hh, sizeof(hh), "%s/config/highHum", _clientId);
  char tbuf[16], hbuf[16];
  snprintf(tbuf, sizeof(tbuf), "%.1f", lowTemp);
  _mqttClient.publish(lt, tbuf);
  snprintf(tbuf, sizeof(tbuf), "%.1f", highTemp);
  _mqttClient.publish(ht, tbuf);
  snprintf(hbuf, sizeof(hbuf), "%.1f", lowHum);
  _mqttClient.publish(lh, hbuf);
  snprintf(hbuf, sizeof(hbuf), "%.1f", highHum);
  _mqttClient.publish(hh, hbuf);
  XLOG_DEBUG(CAT_MQTT, "publishThresholds(T:%.1f-%.1f, H:%.1f-%.1f)", lowTemp,
             highTemp, lowHum, highHum);
}

/**
 * @brief Опубликовать уровень сигнала WiFi (RSSI)
 * @param rssi Уровень сигнала в dBm
 */
void MQTTManager::publishRSSI(int rssi) {
  if (!isConnected())
    return;
  snprintf(_topics.rssi, sizeof(_topics.rssi), "%s/rssi", _clientId);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", rssi);
  _mqttClient.publish(_topics.rssi, buf);
  XLOG_DEBUG(CAT_MQTT, "publishRSSI(%d dBm) → %s = %d", rssi, _topics.rssi,
             rssi);
}

/**
 * @brief Опубликовать версию прошивки
 * @param version Строка с версией
 */
void MQTTManager::publishVersion(const char* version) {
  if (!isConnected())
    return;
  snprintf(_topics.version, sizeof(_topics.version), "%s/version", _clientId);
  _mqttClient.publish(_topics.version, version);
  XLOG_DEBUG(CAT_MQTT, "publishVersion(%s) → %s = %s", version, _topics.version,
             version);
}

/**
 * @brief Опубликовать причину последней перезагрузки
 * @param reason Строка с причиной (например, "WATCHDOG", "SOFT_RESET")
 */
void MQTTManager::publishResetReason(const char* reason) {
  if (!isConnected())
    return;
  snprintf(_topics.reset, sizeof(_topics.reset), "%s/last_reset", _clientId);
  _mqttClient.publish(_topics.reset, reason);
  XLOG_DEBUG(CAT_MQTT, "publishResetReason(%s) → %s = %s", reason,
             _topics.reset, reason);
}

// ============================================================================
// ВНУТРЕННИЕ МЕТОДЫ
// ============================================================================

/**
 * @brief Попытка подключения к MQTT-брокеру
 * @details Использует LWT (Last Will and Testament) для уведомления о потере
 * связи. При успешном подключении подписывается на команды и публикует статус
 * Online.
 */
void MQTTManager::reconnect() {
  if (!_running)
    return;

  XLOG_DEBUG(CAT_MQTT, "Attempting MQTT connection...");

  // Формируем LWT
  snprintf(_topics.online, sizeof(_topics.online), "%s/status", _clientId);

  bool connected = false;

  // Пытаемся подключиться с учётом логина/пароля
  if (strlen(_config.mqttUser) > 0) {
    connected =
        _mqttClient.connect(_clientId, _config.mqttUser, _config.mqttPassword,
                            _topics.online, 1, true, "Offline");
  } else {
    connected =
        _mqttClient.connect(_clientId, _topics.online, 1, true, "Offline");
  }

  if (connected) {
    _connected = true;
    XLOG_INFO(CAT_MQTT,
              "MQTT connected to " ANSI_BOLD ANSI_GREEN "%s" ANSI_RESET ":%d",
              _config.mqttBroker, _config.mqttPort);

    // Подписываемся на команды
    subscribe();

    // Публикуем Online
    publishOnline();

    // Публикуем версию
    publishVersion(VERSION);

  } else {
    _connected = false;
    XLOG_WARN(CAT_MQTT, "MQTT connection failed (state: %d)",
              _mqttClient.state());
  }
}

/**
 * @brief Подписка на управляющие топики
 * @details Подписывается на топики команд управления и конфигурации
 */
void MQTTManager::subscribe() {
  char topic[64];

  // Команды управления
  snprintf(topic, sizeof(topic), "%s/c/state", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);

  snprintf(topic, sizeof(topic), "%s/c/speed", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);

  snprintf(topic, sizeof(topic), "%s/c/sensorMode", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);

#if DEVICE_TYPE == 1
  snprintf(topic, sizeof(topic), "%s/c/adaptiveMode", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);

  snprintf(topic, sizeof(topic), "%s/c/config/lowTemp", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);

  snprintf(topic, sizeof(topic), "%s/c/config/highTemp", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);

  snprintf(topic, sizeof(topic), "%s/c/config/lowHum", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);

  snprintf(topic, sizeof(topic), "%s/c/config/highHum", _clientId);
  _mqttClient.subscribe(topic);
  XLOG_DEBUG(CAT_MQTT, "Subscribed to %s", topic);
#endif
}

/**
 * @brief Обработчик входящих MQTT-сообщений
 * @param topic Топик сообщения
 * @param payload Данные сообщения
 * @param length Длина данных
 * @details Парсит команды управления (DEVICE_COMMAND) и
 *          настройки устройства (DEVICE_CONFIG), передаёт их через
 * _eventCallback
 */
void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
  // Преобразуем payload в строку
  char message[64];
  if (length > sizeof(message) - 1) {
    length = sizeof(message) - 1;
  }
  memcpy(message, payload, length);
  message[length] = '\0';

  XLOG_DEBUG(CAT_MQTT, "Received command: %s = %s", topic, message);

  if (!_eventCallback) {
    XLOG_WARN(CAT_MQTT, "No event callback registered, ignoring message");
    return;
  }

  TransportEventData event;
  event.state = nullptr;
  event.transport = nullptr;
  event.device = nullptr;
  event.deviceState = nullptr;

  // ===== КОМАНДЫ УПРАВЛЕНИЯ (DEVICE_COMMAND) =====
  if (strstr(topic, "/c/state") != nullptr) {
    event.event = DEVICE_COMMAND;
    DeviceState newState;
    memset(&newState, 0, sizeof(newState));
    newState.isOn = (strcmp(message, "ON") == 0 || strcmp(message, "1") == 0);
    event.deviceState = &newState;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/speed") != nullptr) {
    event.event = DEVICE_COMMAND;
    DeviceState newState;
    memset(&newState, 0, sizeof(newState));
    newState.speed = (uint8_t)atoi(message);
    event.deviceState = &newState;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/sensorMode") != nullptr) {
    event.event = DEVICE_COMMAND;
    DeviceState newState;
    memset(&newState, 0, sizeof(newState));
    newState.sensorMode = (strcmp(message, "1") == 0);
    event.deviceState = &newState;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/adaptiveMode") != nullptr) {
    event.event = DEVICE_COMMAND;
    DeviceState newState;
    memset(&newState, 0, sizeof(newState));
    newState.adaptiveMode = (strcmp(message, "1") == 0);
    event.deviceState = &newState;
    _eventCallback(&event, _eventContext);
    return;
  }

  // ===== НАСТРОЙКИ УСТРОЙСТВА (DEVICE_CONFIG) =====
  // Создаём новый DeviceConfig с изменённым полем
  // Остальные поля будут заполнены в Оркестраторе из ConfigManager
  DeviceConfig newConfig;
  memset(&newConfig, 0, sizeof(DeviceConfig));

  if (strstr(topic, "/c/config/lowTemp") != nullptr) {
    newConfig.lowTemp = atof(message);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/highTemp") != nullptr) {
    newConfig.highTemp = atof(message);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/lowHum") != nullptr) {
    newConfig.lowHum = atof(message);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/highHum") != nullptr) {
    newConfig.highHum = atof(message);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/delaySec") != nullptr) {
    newConfig.delaySeconds = (uint32_t)atoi(message);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/maxOnTime") != nullptr) {
    newConfig.maxOnTime = (uint32_t)atoi(message);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/sensorMode") != nullptr) {
    newConfig.sensorMode = (atoi(message) == 1);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/adaptiveMode") != nullptr) {
    newConfig.adaptiveMode = (atoi(message) == 1);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/speedPercent") != nullptr) {
    newConfig.speedPercent = (uint8_t)atoi(message);
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  if (strstr(topic, "/c/config/bootState") != nullptr) {
    newConfig.bootState = (atoi(message) == 1) ? 1 : 0;
    event.event = DEVICE_CONFIG;
    event.device = &newConfig;
    _eventCallback(&event, _eventContext);
    return;
  }

  // ===== НЕИЗВЕСТНЫЙ ТОПИК =====
  XLOG_WARN(CAT_MQTT, "Unknown topic: %s", topic);
}

#endif  // USE_MQTT