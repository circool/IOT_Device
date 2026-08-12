/**
 * @file transport_zigbee.cpp
 * @version 0.12
 * @brief Zigbee-транспорт — заглушка
 * @note Заглушка — Zigbee ещё не реализован. Все методы логируют вызовы.
 */

#include "transport_zigbee.h"
#include "logger.h"
#include "settings.h"

#ifdef USE_ZIGBEE

// ============================================================================
// СТАТИЧЕСКИЙ ЭКЗЕМПЛЯР
// ============================================================================

static ZigbeeTransport g_zigbeeTransport;

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Получить имя транспорта для логов
 * @return Строка "Zigbee (stub)"
 */
static const char* transport_getName() {
  return "Zigbee (stub)";
}

/**
 * @brief Заглушка — установка указателя на конфигурацию устройства
 * @param config Указатель на DeviceConfig
 */
static void transport_setDeviceConfig(const DeviceConfig* config) {
  (void)config;
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] setDeviceConfig() (STUB)");
}

/**
 * @brief Заглушка — установка указателя на состояние устройства
 * @param state Указатель на DeviceState
 */
static void transport_setDeviceState(const DeviceState* state) {
  (void)state;
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] setDeviceState() (STUB)");
}

// ============================================================================
// УВЕДОМЛЕНИЕ ОБ ИЗМЕНЕНИИ СОСТОЯНИЯ (заглушка)
// ============================================================================

static void notifyStateChanged() {
  if (!g_zigbeeTransport._eventCallback)
    return;

  TransportEventData event;
  event.event = STATE_CHANGED;
  event.state = &g_zigbeeTransport._state;
  event.transport = nullptr;
  event.deviceState = nullptr;
  event.device = nullptr;
  g_zigbeeTransport._eventCallback(&event, g_zigbeeTransport._eventContext);
}

// ============================================================================
// УПРАВЛЕНИЕ (заглушки)
// ============================================================================

static bool transport_begin(Client* client,
                            const TransportConfig* config,
                            const TransportState*& outState) {
  (void)client;
  (void)config;
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] begin() called (STUB — not implemented)");
  XLOG_WARN(CAT_TRANSPORT, "[ZIGBEE] Zigbee transport is not implemented yet");

  g_zigbeeTransport.initialized = true;
  g_zigbeeTransport.connected = false;

  // Инициализация состояния
  g_zigbeeTransport._state.link_ok = false;
  g_zigbeeTransport._state.gateway_ok = false;
  g_zigbeeTransport._state.setup_mode = false;

  // Передаём указатель на состояние
  outState = &g_zigbeeTransport._state;

  // Уведомляем о начальном состоянии
  notifyStateChanged();

  return false;  // Заглушка всегда возвращает false
}

static void transport_update() {
  if (!g_zigbeeTransport.initialized)
    return;
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 10000) {
    lastLog = millis();
    XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] update() (STUB)");
  }
}

static bool transport_isConnected() {
  return false;  // Заглушка всегда возвращает false
}

static void transport_disconnect() {
  XLOG_INFO(CAT_TRANSPORT, "[ZIGBEE] disconnect() called (STUB)");
  g_zigbeeTransport.connected = false;
  g_zigbeeTransport._state.link_ok = false;
  notifyStateChanged();
}

// ============================================================================
// ПУБЛИКАЦИЯ (заглушки)
// ============================================================================

static void transport_publishOnline() {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishOnline() (STUB)");
}

static void transport_publishState(bool on) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishState(%s) (STUB)",
             on ? "ON" : "OFF");
}

static void transport_publishSpeed(int percent) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishSpeed(%d%%) (STUB)", percent);
}

static void transport_publishDelaySec(int seconds) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishDelaySec(%d) (STUB)", seconds);
}

static void transport_publishMaxOnTime(uint32_t seconds) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishMaxOnTime(%lu) (STUB)", seconds);
}

static void transport_publishSensorControlMode(bool enabled) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishSensorControlMode(%s) (STUB)",
             enabled ? "AUTO" : "MANUAL");
}

static void transport_publishSensor(float temp, float hum) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishSensor(%.1f°C, %.1f%%) (STUB)",
             temp, hum);
}

static void transport_publishAdaptiveMode(bool enabled) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishAdaptiveMode(%s) (STUB)",
             enabled ? "ON" : "OFF");
}

static void transport_publishThresholds(float lowTemp,
                                        float highTemp,
                                        float lowHum,
                                        float highHum) {
  XLOG_DEBUG(CAT_TRANSPORT,
             "[ZIGBEE] publishThresholds(T:%.1f-%.1f, H:%.1f-%.1f) (STUB)",
             lowTemp, highTemp, lowHum, highHum);
}

static void transport_publishRSSI(int rssi) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishRSSI(%d dBm) (STUB)", rssi);
}

static void transport_publishVersion(const char* version) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishVersion(%s) (STUB)", version);
}

static void transport_publishResetReason(const char* reason) {
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishResetReason(%s) (STUB)", reason);
}

static void transport_publishFullState(const DeviceState* state) {
  (void)state;
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishFullState() (STUB)");
}

static void transport_publishConfig(const DeviceConfig* config) {
  (void)config;
  XLOG_DEBUG(CAT_TRANSPORT, "[ZIGBEE] publishConfig() (STUB)");
}

// ============================================================================
// КОЛБЭКИ (заглушки)
// ============================================================================

static void transport_onEvent(TransportEventCallback callback, void* context) {
  XLOG_DEBUG(CAT_TRANSPORT,
             "[ZIGBEE] onEvent() registered (STUB — will not receive events)");
  g_zigbeeTransport._eventCallback = callback;
  g_zigbeeTransport._eventContext = context;
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ СТРУКТУРЫ TRANSPORT
// ============================================================================

static Transport g_transportImpl = {
    // Управление
    .begin = transport_begin,
    .setDeviceConfig = transport_setDeviceConfig,
    .setDeviceState = transport_setDeviceState,
    .update = transport_update,
    .isConnected = transport_isConnected,
    .disconnect = transport_disconnect,
    .getName = transport_getName,

    // Публикация
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

    // Колбэк
    .onEvent = transport_onEvent,
};

// ============================================================================
// ПУБЛИЧНАЯ ФУНКЦИЯ
// ============================================================================

Transport* getZigbeeTransport() {
  return &g_transportImpl;
}

#endif  // USE_ZIGBEE