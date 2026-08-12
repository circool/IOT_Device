/**
 * @file transport_wifi.cpp
 * @version 0.12
 * @brief WiFi-транспорт — реализация
 */

#include "transport_wifi.h"
#include "logger.h"

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// ============================================================================
// КОНСТАНТЫ
// ============================================================================

#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 15000  ///< Таймаут подключения к WiFi (мс)
#endif

#ifndef WIFI_RECONNECT_DELAY_MS
#define WIFI_RECONNECT_DELAY_MS \
  5000  ///< Задержка между попытками переподключения (мс)
#endif

#ifndef WIFI_MAX_RECONNECT_ATTEMPTS
#define WIFI_MAX_RECONNECT_ATTEMPTS \
  5  ///< Максимальное число попыток переподключения
#endif

#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS \
  "192.168.4.1"  ///< IP-адрес точки доступа в режиме настройки
#endif

#ifndef RSSI_POLLING_INTERVAL_MS
#define RSSI_POLLING_INTERVAL_MS 5000  ///< Интервал публикации RSSI (мс)
#endif

#ifdef USE_WIFI

// ============================================================================
// СТАТИЧЕСКИЙ ЭКЗЕМПЛЯР
// ============================================================================

static WiFiTransport g_wifiTransport;  ///< Глобальный экземпляр WiFi-транспорта

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Получить имя транспорта для логов
 * @return Строка "WiFi"
 */
static const char* transport_getName() {
  return "WiFi";
}

/**
 * @brief Установить указатель на конфигурацию устройства
 * @param config Указатель на DeviceConfig
 */
static void transport_setDeviceConfig(const DeviceConfig* config) {
  g_wifiTransport.deviceConfig = config;
  XLOG_DEBUG(CAT_TRANSPORT, "DeviceConfig set: %p", (void*)config);
}

/**
 * @brief Установить указатель на состояние устройства
 * @param state Указатель на DeviceState
 */
static void transport_setDeviceState(const DeviceState* state) {
  g_wifiTransport.deviceState = state;
  XLOG_DEBUG(CAT_TRANSPORT, "DeviceState set: %p", (void*)state);
}

// ============================================================================
// WIFI ПОДКЛЮЧЕНИЕ (НЕБЛОКИРУЮЩЕЕ)
// ============================================================================

/**
 * @brief Запустить асинхронное подключение к WiFi
 * @param ssid Имя сети
 * @param password Пароль сети
 * @return true — подключение запущено, false — ошибка (SSID пустой)
 * @note Функция неблокирующая — сразу возвращает управление.
 *       Проверку статуса выполняет wifi_connect_check()
 */
static bool wifi_connect_start(const char* ssid, const char* password) {
  if (!ssid || strlen(ssid) == 0) {
    XLOG_WARN(CAT_WIFI, "SSID is empty");
    return false;
  }

  XLOG_INFO(CAT_WIFI, "Starting async WiFi connection to SSID: %s", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, password);

  return true;
}

/**
 * @brief Проверить статус асинхронного подключения к WiFi
 * @return true — WiFi подключён, false — ещё не подключён
 * @note Вызывается в цикле для проверки статуса после wifi_connect_start()
 */
static bool wifi_connect_check() {
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    XLOG_INFO(CAT_WIFI,
              "WiFi connected! IP:" ANSI_BOLD ANSI_GREEN "%s" ANSI_RESET
              ", RSSI: %d dBm",
              WiFi.localIP().toString().c_str(), rssi);
    return true;
  }
  return false;
}

// ============================================================================
// УВЕДОМЛЕНИЕ ОБ ИЗМЕНЕНИИ СОСТОЯНИЯ
// ============================================================================

/**
 * @brief Уведомить Оркестратора об изменении состояния транспорта
 * @details Формирует событие STATE_CHANGED и передаёт его через _eventCallback
 * @note Вызывается при любом изменении _state (link_ok, gateway_ok, setup_mode)
 */
static void notifyStateChanged() {
  if (!g_wifiTransport._eventCallback)
    return;

  TransportEventData event;
  event.event = STATE_CHANGED;
  event.state = &g_wifiTransport._state;
  event.transport = nullptr;
  event.deviceState = nullptr;
  event.device = nullptr;
  g_wifiTransport._eventCallback(&event, g_wifiTransport._eventContext);
}

// ============================================================================
// УПРАВЛЕНИЕ
// ============================================================================

/**
 * @brief Инициализация WiFi-транспорта (неблокирующая)
 * @param client Указатель на WiFiClient (для MQTT)
 * @param config Конфигурация транспорта
 * @param outState Ссылка на указатель состояния транспорта
 * @return true — инициализация успешна, false — ошибка
 * @note Функция не блокирует выполнение. Подключение к WiFi происходит
 * асинхронно.
 */
static bool transport_begin(Client* client,
                            const TransportConfig* config,
                            const TransportState*& outState) {
  (void)client;
  XLOG_DEBUG(CAT_TRANSPORT, "=== TRANSPORT BEGIN START ===");

  if (!config) {
    XLOG_WARN(CAT_TRANSPORT, "config is NULL");
    return false;
  }

  g_wifiTransport.config = config;
  g_wifiTransport.initialized = true;
  g_wifiTransport.connected = false;
  g_wifiTransport.wifiState = WIFI_STATE_DISCONNECTED;

  XLOG_INFO(CAT_TRANSPORT, "WiFi SSID: %s", config->wifiSsid);

  // ===== ИНИЦИАЛИЗАЦИЯ СОСТОЯНИЯ =====
  g_wifiTransport._state.link_ok = false;
  g_wifiTransport._state.gateway_ok = false;
  g_wifiTransport._state.setup_mode = false;

  // ===== WebManager =====
  // Передаём указатели на все данные
  if (!g_wifiTransport.web.begin(config, g_wifiTransport.deviceConfig,
                                 g_wifiTransport.deviceState,
                                 &g_wifiTransport._state)) {
    XLOG_ERROR(CAT_TRANSPORT, "Web init failed");
    return false;
  }
  XLOG_DEBUG(CAT_TRANSPORT, "WebManager initialized");

  // ===== WiFi (только инициализация, без подключения) =====
  if (strlen(config->wifiSsid) == 0) {
    // Нет SSID — сразу переходим в режим настройки
    XLOG_INFO(CAT_TRANSPORT, "No SSID — starting provisioning (AP mode)");

    g_wifiTransport.wifiState = WIFI_STATE_PROVISIONING;
    g_wifiTransport._state.setup_mode = true;
    g_wifiTransport._state.link_ok = false;

    g_wifiTransport.web.enableProvisioningMode();

    g_wifiTransport.provisioning.begin(
        config->deviceId, g_wifiTransport._eventCallback,
        g_wifiTransport._eventContext, &g_wifiTransport.web);
  } else {
    // Есть SSID — начинаем асинхронное подключение в update()
    XLOG_INFO(CAT_TRANSPORT, "WiFi will be connected in background");
    g_wifiTransport.wifiState = WIFI_STATE_DISCONNECTED;
    g_wifiTransport._state.setup_mode = false;
    g_wifiTransport._state.link_ok = false;
    g_wifiTransport.web.setWifiReady(false);
  }

  // ===== MQTT =====
#if USE_MQTT
  if (!g_wifiTransport.mqtt.begin(config)) {
    XLOG_WARN(CAT_TRANSPORT, "MQTT init failed (continuing without MQTT)");
  }
#endif

  // ===== ПЕРЕДАЁМ УКАЗАТЕЛЬ НА СОСТОЯНИЕ ОРКЕСТРАТОРУ =====
  outState = &g_wifiTransport._state;

  // ===== УВЕДОМЛЯЕМ ОРКЕСТРАТОРА О НАЧАЛЬНОМ СОСТОЯНИИ =====
  if (g_wifiTransport._eventCallback) {
    TransportEventData event;
    event.event = STATE_CHANGED;
    event.state = &g_wifiTransport._state;
    event.transport = nullptr;
    event.deviceState = nullptr;
    event.device = nullptr;
    g_wifiTransport._eventCallback(&event, g_wifiTransport._eventContext);
  }

  XLOG_INFO(CAT_TRANSPORT, "Transport initialized (connecting in background)");
  return true;
}

/**
 * @brief Периодическая обработка WiFi-транспорта
 * @details Вызывается в loop(). Управляет:
 *          - асинхронным подключением к WiFi
 *          - переподключением при потере связи
 *          - публикацией RSSI
 *          - обновлением состояния MQTT
 */
static void transport_update() {
  if (!g_wifiTransport.initialized) {
    XLOG_WARN(CAT_TRANSPORT, "update() called but not initialized");
    return;
  }

  // ===== WebManager — отложенный запуск сервера =====
  g_wifiTransport.web.update();

  // ===== ПЕРВОЕ ПОДКЛЮЧЕНИЕ К WIFI =====
  static bool wifiStarted = false;
  static unsigned long connectStart = 0;

  if (!wifiStarted && !g_wifiTransport.connected &&
      !g_wifiTransport._state.setup_mode &&
      strlen(g_wifiTransport.config->wifiSsid) > 0) {
    wifiStarted = true;
    connectStart = millis();
    wifi_connect_start(g_wifiTransport.config->wifiSsid,
                       g_wifiTransport.config->wifiPassword);
  }

  // ===== ПРОВЕРКА СТАТУСА ПОДКЛЮЧЕНИЯ =====
  if (wifiStarted && !g_wifiTransport.connected) {
    if (wifi_connect_check()) {
      // Подключились!
      g_wifiTransport.connected = true;
      g_wifiTransport.wifiState = WIFI_STATE_CONNECTED;
      g_wifiTransport._state.link_ok = true;
      g_wifiTransport.web.setWifiReady(true);
      notifyStateChanged();
      wifiStarted = false;
      connectStart = 0;
    } else if (millis() - connectStart > WIFI_CONNECT_TIMEOUT_MS) {
      // Таймаут
      wifiStarted = false;
      connectStart = 0;
      XLOG_WARN(CAT_WIFI, "WiFi connection timeout! Starting provisioning...");

      // Переключаемся в режим настройки
      g_wifiTransport.wifiState = WIFI_STATE_PROVISIONING;
      g_wifiTransport._state.setup_mode = true;
      g_wifiTransport._state.link_ok = false;

      WiFi.mode(WIFI_AP);
      WiFi.softAP(g_wifiTransport.config->deviceId);
      g_wifiTransport.web.enableProvisioningMode();
      g_wifiTransport.provisioning.begin(
          g_wifiTransport.config->deviceId, g_wifiTransport._eventCallback,
          g_wifiTransport._eventContext, &g_wifiTransport.web);
      notifyStateChanged();
    }
  }

  // ===== ОБНОВЛЕНИЕ WIFI (если подключены) =====
  static unsigned long lastReconnectAttempt = 0;
  static int reconnectAttempts = 0;

  if (g_wifiTransport.connected) {
    if (WiFi.status() == WL_CONNECTED) {
      reconnectAttempts = 0;

      // ===== RSSI (читается на лету) =====
      static unsigned long lastRssiPublishTime = 0;
      unsigned long now = millis();

      if (now - lastRssiPublishTime >= RSSI_POLLING_INTERVAL_MS) {
        lastRssiPublishTime = now;
        int rssi = WiFi.RSSI();
        transport_publishRSSI(rssi);
        XLOG_DEBUG(CAT_WIFI, "RSSI published: %d dBm", rssi);
      }
    } else {
      // Потеряли связь
      g_wifiTransport.connected = false;
      g_wifiTransport.wifiState = WIFI_STATE_DISCONNECTED;
      g_wifiTransport._state.link_ok = false;
      XLOG_WARN(CAT_WIFI, "WiFi disconnected!");
      notifyStateChanged();
      wifiStarted = false;
    }
  }

  // ===== ПЕРЕПОДКЛЮЧЕНИЕ (если потеряли связь и не в режиме настройки) =====
  if (!g_wifiTransport.connected && !g_wifiTransport._state.setup_mode &&
      strlen(g_wifiTransport.config->wifiSsid) > 0 && !wifiStarted) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > WIFI_RECONNECT_DELAY_MS) {
      if (reconnectAttempts >= WIFI_MAX_RECONNECT_ATTEMPTS) {
        // Превышено число попыток — переключаемся в режим настройки
        XLOG_WARN(CAT_WIFI, "Max reconnect attempts reached (%d)",
                  WIFI_MAX_RECONNECT_ATTEMPTS);
        XLOG_WARN(CAT_TRANSPORT,
                  "Starting provisioning after failed reconnects");

        g_wifiTransport.wifiState = WIFI_STATE_PROVISIONING;
        g_wifiTransport._state.setup_mode = true;
        g_wifiTransport._state.link_ok = false;

        WiFi.mode(WIFI_AP);
        WiFi.softAP(g_wifiTransport.config->deviceId);
        g_wifiTransport.web.enableProvisioningMode();
        g_wifiTransport.provisioning.begin(
            g_wifiTransport.config->deviceId, g_wifiTransport._eventCallback,
            g_wifiTransport._eventContext, &g_wifiTransport.web);
        notifyStateChanged();
        reconnectAttempts = 0;
      } else {
        reconnectAttempts++;
        XLOG_INFO(CAT_WIFI, "Reconnect attempt %d/%d", reconnectAttempts,
                  WIFI_MAX_RECONNECT_ATTEMPTS);

        g_wifiTransport.wifiState = WIFI_STATE_CONNECTING;
        wifiStarted = true;
        connectStart = millis();

        WiFi.disconnect(true);
        delay(100);
        WiFi.begin(g_wifiTransport.config->wifiSsid,
                   g_wifiTransport.config->wifiPassword);
      }
      lastReconnectAttempt = now;
    }
  }

  // ===== MQTT =====
#if USE_MQTT
  g_wifiTransport.mqtt.update();

  bool mqttConnected = g_wifiTransport.mqtt.isConnected();
  if (g_wifiTransport._state.gateway_ok != mqttConnected) {
    g_wifiTransport._state.gateway_ok = mqttConnected;
    notifyStateChanged();
  }
#endif
}

/**
 * @brief Проверить, подключён ли WiFi-транспорт
 * @return true — транспорт подключён, false — не подключён
 */
static bool transport_isConnected() {
  if (!g_wifiTransport.initialized)
    return false;
  return g_wifiTransport.connected;
}

/**
 * @brief Принудительное отключение WiFi-транспорта
 * @details Отключает WiFi, останавливает провизионинг, отключает Web и MQTT
 */
static void transport_disconnect() {
  XLOG_INFO(CAT_TRANSPORT, "disconnect() called");

  if (g_wifiTransport._state.setup_mode) {
    g_wifiTransport.provisioning.stop();
    g_wifiTransport._state.setup_mode = false;
    g_wifiTransport._state.link_ok = false;
    notifyStateChanged();
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  g_wifiTransport.connected = false;
  g_wifiTransport.wifiState = WIFI_STATE_DISCONNECTED;
  g_wifiTransport._state.link_ok = false;
  notifyStateChanged();

  g_wifiTransport.web.disconnect();

#if USE_MQTT
  g_wifiTransport.mqtt.disconnect();
#endif
}

// ============================================================================
// ПУБЛИКАЦИЯ (ТОЛЬКО ДЛЯ MQTT)
// ============================================================================

/**
 * @brief Опубликовать статус Online
 * @details Отправляет статус "Online" в MQTT
 */
static void transport_publishOnline() {
  XLOG_INFO(CAT_TRANSPORT, "Publishing status");
#if USE_MQTT
  g_wifiTransport.mqtt.publishOnline();
#endif
}

/**
 * @brief Опубликовать состояние актуатора
 * @param on true — включён, false — выключен
 */
static void transport_publishState(bool on) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing state: %s", on ? "ON" : "OFF");
#if USE_MQTT
  g_wifiTransport.mqtt.publishState(on);
#endif
}

/**
 * @brief Опубликовать скорость
 * @param percent Скорость 0-100%
 */
static void transport_publishSpeed(int percent) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing Speed: %d%%", percent);
#if USE_MQTT
  g_wifiTransport.mqtt.publishSpeed(percent);
#endif
}

/**
 * @brief Опубликовать задержку включения
 * @param seconds Задержка в секундах
 */
static void transport_publishDelaySec(int seconds) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing DelaySec: %d", seconds);
#if USE_MQTT
  g_wifiTransport.mqtt.publishDelaySec(seconds);
#endif
}

/**
 * @brief Опубликовать максимальное время работы
 * @param seconds Время в секундах
 */
static void transport_publishMaxOnTime(uint32_t seconds) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing MaxOnTime: %lu", seconds);
#if USE_MQTT
  g_wifiTransport.mqtt.publishMaxOnTime(seconds);
#endif
}

/**
 * @brief Опубликовать режим управления по датчику
 * @param enabled true — AUTO, false — MANUAL
 */
static void transport_publishSensorControlMode(bool enabled) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing SensorControlMode: %s",
            enabled ? "ON" : "OFF");
#if USE_MQTT
  g_wifiTransport.mqtt.publishSensorControlMode(enabled);
#endif
}

/**
 * @brief Опубликовать показания датчика
 * @param temp Температура в °C
 * @param hum Влажность в %
 */
static void transport_publishSensor(float temp, float hum) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing Sensor: %.1f°C, %.1f%%", temp, hum);
#if USE_MQTT
  g_wifiTransport.mqtt.publishSensor(temp, hum);
#endif
}

/**
 * @brief Опубликовать состояние адаптивного режима
 * @param enabled true — включён, false — выключен
 */
static void transport_publishAdaptiveMode(bool enabled) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing AdaptiveMode: %s",
            enabled ? "ON" : "OFF");
#if USE_MQTT
  g_wifiTransport.mqtt.publishAdaptiveMode(enabled);
#endif
}

/**
 * @brief Опубликовать пороговые значения
 * @param lowTemp Нижний порог температуры
 * @param highTemp Верхний порог температуры
 * @param lowHum Нижний порог влажности
 * @param highHum Верхний порог влажности
 */
static void transport_publishThresholds(float lowTemp,
                                        float highTemp,
                                        float lowHum,
                                        float highHum) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing Thresholds: T:%.1f-%.1f, H:%.1f-%.1f)",
            lowTemp, highTemp, lowHum, highHum);
#if USE_MQTT
  g_wifiTransport.mqtt.publishThresholds(lowTemp, highTemp, lowHum, highHum);
#endif
}

/**
 * @brief Опубликовать уровень сигнала WiFi (RSSI)
 * @param rssi Уровень сигнала в dBm
 * @note Обновляет WebManager и публикует в MQTT
 */
void transport_publishRSSI(int rssi) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing RSSI: %d dBm", rssi);
  g_wifiTransport.web.setRSSI(rssi);
#if USE_MQTT
  g_wifiTransport.mqtt.publishRSSI(rssi);
#endif
}

/**
 * @brief Опубликовать версию прошивки
 * @param version Строка с версией
 */
static void transport_publishVersion(const char* version) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing Version: %s", version);
#if USE_MQTT
  g_wifiTransport.mqtt.publishVersion(version);
#endif
}

/**
 * @brief Опубликовать причину последней перезагрузки
 * @param reason Строка с причиной (например, "WATCHDOG", "SOFT_RESET")
 */
static void transport_publishResetReason(const char* reason) {
  XLOG_INFO(CAT_TRANSPORT, "Publishing ResetReason: %s", reason);
#if USE_MQTT
  g_wifiTransport.mqtt.publishResetReason(reason);
#endif
}

/**
 * @brief Опубликовать оперативное состояние устройства
 * @param state Указатель на DeviceState
 */
static void transport_publishFullState(const DeviceState* state) {
  if (!state) {
    XLOG_WARN(CAT_TRANSPORT, "publishFullState: state is null");
    return;
  }

  XLOG_INFO(CAT_TRANSPORT, "Publishing full device state...");

  transport_publishState(state->isOn);
  transport_publishSpeed(state->speed);
  transport_publishSensor(state->temperature, state->humidity);
  transport_publishSensorControlMode(state->sensorMode);
  transport_publishAdaptiveMode(state->adaptiveMode);
  transport_publishDelaySec(state->delayRemain);
  transport_publishMaxOnTime(state->maxOnRemain);
}

/**
 * @brief Опубликовать настройки устройства
 * @param config Указатель на DeviceConfig
 */
static void transport_publishConfig(const DeviceConfig* config) {
  if (!config) {
    XLOG_WARN(CAT_TRANSPORT, "publishConfig: config is null");
    return;
  }

  XLOG_INFO(CAT_TRANSPORT, "Publishing device config...");

  transport_publishThresholds(config->lowTemp, config->highTemp, config->lowHum,
                              config->highHum);
  transport_publishDelaySec(config->delaySeconds);
  transport_publishMaxOnTime(config->maxOnTime);
  transport_publishSensorControlMode(config->sensorMode);
  transport_publishAdaptiveMode(config->adaptiveMode);
  transport_publishSpeed(config->speedPercent);
  // transport_publishBootState(config->bootState); // если есть метод
}

// ============================================================================
// КОЛБЭКИ
// ============================================================================

/**
 * @brief Регистрация колбэка для событий транспорта
 * @param callback Функция обратного вызова
 * @param context Контекст для колбэка
 * @details Передаёт колбэк WebManager и MQTTManager для обработки событий
 */
static void transport_onEvent(TransportEventCallback callback, void* context) {
  XLOG_DEBUG(CAT_TRANSPORT, "onEvent() registered");
  g_wifiTransport._eventCallback = callback;
  g_wifiTransport._eventContext = context;

  g_wifiTransport.web.onEvent(callback, context);
#if USE_MQTT
  g_wifiTransport.mqtt.onEvent(callback, context);
#endif
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ СТРУКТУРЫ TRANSPORT
// ============================================================================

/**
 * @brief Глобальная структура Transport с указателями на функции
 * WiFi-транспорта
 * @details Инициализируется статически, все методы указывают на реализации выше
 */
static Transport g_transportImpl = {
    .begin = transport_begin,
    .setDeviceConfig = transport_setDeviceConfig,
    .setDeviceState = transport_setDeviceState,
    .update = transport_update,
    .isConnected = transport_isConnected,
    .disconnect = transport_disconnect,
    .getName = transport_getName,
    .publishOnline = transport_publishOnline,
    .publishState = transport_publishState,
    .publishSpeed = transport_publishSpeed,
    .publishDelaySec = transport_publishDelaySec,
    .publishMaxOnTime = transport_publishMaxOnTime,
    .publishSensorControlMode = transport_publishSensorControlMode,
    .publishSensor = transport_publishSensor,
    .publishAdaptiveMode = transport_publishAdaptiveMode,
    .publishThresholds = transport_publishThresholds,
    .publishRSSI = transport_publishRSSI,
    .publishVersion = transport_publishVersion,
    .publishResetReason = transport_publishResetReason,
    .publishFullState = transport_publishFullState,
    .publishConfig = transport_publishConfig,
    .onEvent = transport_onEvent,
};

// ============================================================================
// ПУБЛИЧНАЯ ФУНКЦИЯ
// ============================================================================

/**
 * @brief Получить глобальный экземпляр WiFi-транспорта
 * @return Указатель на структуру Transport
 * @note Транспорт создаётся статически, функция возвращает указатель на него
 */
Transport* getWiFiTransport() {
  return &g_transportImpl;
}

#endif  // USE_WIFI