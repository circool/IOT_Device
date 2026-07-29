#include "transport_zigbee.h"
#include "logger.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE

static Transport g_zigbeeTransport;
static bool g_initialized = false;

// ============================================================================
// КОНТЕЙНЕРЫ ДЛЯ КОЛБЭКОВ (заглушки)
// ============================================================================

static TransportCallback<TransportBoolCallback> _onState;
static TransportCallback<TransportIntCallback> _onSpeed;
static TransportCallback<TransportIntCallback> _onDelaySec;
static TransportCallback<TransportUintCallback> _onMaxOnTime;
static TransportCallback<TransportBoolCallback> _onSensorControlMode;

#if DEVICE_TYPE == 1
static TransportCallback<TransportBoolCallback> _onAdaptiveMode;
static TransportCallback<TransportFloatCallback> _onLowTemp;
static TransportCallback<TransportFloatCallback> _onHighTemp;
static TransportCallback<TransportFloatCallback> _onLowHum;
static TransportCallback<TransportFloatCallback> _onHighHum;
#endif

#if MQTT_RESET_ENABLED == 1
static TransportCallback<TransportVoidCallback> _onReset;
#endif

// ============================================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ ТРАНСПОРТА (ЗАГЛУШКИ)
// ============================================================================

static bool zigbee_begin(Client* client, const ConfigData* config) {
  (void)client;
  (void)config;
  XLOG_WARN(CAT_MAIN, "ZigBee transport: begin() not implemented yet");
  return false;
}

static void zigbee_update() {
  // TODO: реальная обработка ZigBee
}

static bool zigbee_isConnected() {
  return false;  // всегда отключено, пока нет реализации
}

static void zigbee_disconnect() {
  // TODO
}

static const char* zigbee_getName() {
  return "ZigBee (stub)";
}

// ============================================================================
// ПУБЛИКАЦИЯ (ЗАГЛУШКИ)
// ============================================================================

static void zigbee_publishOnline() {
  // TODO
}

static void zigbee_publishState(bool on) {
  (void)on;
  // TODO
}

static void zigbee_publishSpeed(int percent) {
  (void)percent;
  // TODO
}

static void zigbee_publishDelaySec(int seconds) {
  (void)seconds;
  // TODO
}

static void zigbee_publishMaxOnTime(uint32_t seconds) {
  (void)seconds;
  // TODO
}

static void zigbee_publishSensorControlMode(bool enabled) {
  (void)enabled;
  // TODO
}

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
static void zigbee_publishSensor(float temp, float hum) {
  (void)temp;
  (void)hum;
  // TODO
}
#endif

#if DEVICE_TYPE == 1
static void zigbee_publishAdaptiveMode(bool enabled) {
  (void)enabled;
  // TODO
}

static void zigbee_publishThresholds(float lowTemp,
                                     float highTemp,
                                     float lowHum,
                                     float highHum) {
  (void)lowTemp;
  (void)highTemp;
  (void)lowHum;
  (void)highHum;
  // TODO
}
#endif

static void zigbee_publishRSSI(int rssi) {
  (void)rssi;
  // TODO
}

static void zigbee_publishVersion(const char* version) {
  (void)version;
  // TODO
}

static void zigbee_publishResetReason(const char* reason) {
  (void)reason;
  // TODO
}

// ============================================================================
// РЕГИСТРАЦИЯ КОЛБЭКОВ (ЗАГЛУШКИ) — просто сохраняем колбэки, но не используем
// ============================================================================

static void zigbee_onState(TransportBoolCallback callback, void* context) {
  _onState.func = callback;
  _onState.context = context;
  // TODO: реальная подписка
}

static void zigbee_onSpeed(TransportIntCallback callback, void* context) {
  _onSpeed.func = callback;
  _onSpeed.context = context;
  // TODO
}

static void zigbee_onDelaySec(TransportIntCallback callback, void* context) {
  _onDelaySec.func = callback;
  _onDelaySec.context = context;
  // TODO
}

static void zigbee_onMaxOnTime(TransportUintCallback callback, void* context) {
  _onMaxOnTime.func = callback;
  _onMaxOnTime.context = context;
  // TODO
}

static void zigbee_onSensorControlMode(TransportBoolCallback callback,
                                       void* context) {
  _onSensorControlMode.func = callback;
  _onSensorControlMode.context = context;
  // TODO
}

#if DEVICE_TYPE == 1
static void zigbee_onAdaptiveMode(TransportBoolCallback callback,
                                  void* context) {
  _onAdaptiveMode.func = callback;
  _onAdaptiveMode.context = context;
  // TODO
}

static void zigbee_onLowTemp(TransportFloatCallback callback, void* context) {
  _onLowTemp.func = callback;
  _onLowTemp.context = context;
  // TODO
}

static void zigbee_onHighTemp(TransportFloatCallback callback, void* context) {
  _onHighTemp.func = callback;
  _onHighTemp.context = context;
  // TODO
}

static void zigbee_onLowHum(TransportFloatCallback callback, void* context) {
  _onLowHum.func = callback;
  _onLowHum.context = context;
  // TODO
}

static void zigbee_onHighHum(TransportFloatCallback callback, void* context) {
  _onHighHum.func = callback;
  _onHighHum.context = context;
  // TODO
}
#endif

#if MQTT_RESET_ENABLED == 1
static void zigbee_onReset(TransportVoidCallback callback, void* context) {
  _onReset.func = callback;
  _onReset.context = context;
  // TODO
}
#endif

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ ТРАНСПОРТА
// ============================================================================

Transport* getZigbeeTransport() {
  if (g_initialized) {
    return &g_zigbeeTransport;
  }

  // Управление
  g_zigbeeTransport.begin = zigbee_begin;
  g_zigbeeTransport.update = zigbee_update;
  g_zigbeeTransport.isConnected = zigbee_isConnected;
  g_zigbeeTransport.disconnect = zigbee_disconnect;
  g_zigbeeTransport.getName = zigbee_getName;

  // Публикация
  g_zigbeeTransport.publishOnline = zigbee_publishOnline;
  g_zigbeeTransport.publishState = zigbee_publishState;
  g_zigbeeTransport.publishSpeed = zigbee_publishSpeed;
  g_zigbeeTransport.publishDelaySec = zigbee_publishDelaySec;
  g_zigbeeTransport.publishMaxOnTime = zigbee_publishMaxOnTime;
  g_zigbeeTransport.publishSensorControlMode = zigbee_publishSensorControlMode;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  g_zigbeeTransport.publishSensor = zigbee_publishSensor;
#endif

#if DEVICE_TYPE == 1
  g_zigbeeTransport.publishAdaptiveMode = zigbee_publishAdaptiveMode;
  g_zigbeeTransport.publishThresholds = zigbee_publishThresholds;
#endif

  g_zigbeeTransport.publishRSSI = zigbee_publishRSSI;
  g_zigbeeTransport.publishVersion = zigbee_publishVersion;
  g_zigbeeTransport.publishResetReason = zigbee_publishResetReason;

  // Колбэки
  g_zigbeeTransport.onState = zigbee_onState;
  g_zigbeeTransport.onSpeed = zigbee_onSpeed;
  g_zigbeeTransport.onDelaySec = zigbee_onDelaySec;
  g_zigbeeTransport.onMaxOnTime = zigbee_onMaxOnTime;
  g_zigbeeTransport.onSensorControlMode = zigbee_onSensorControlMode;

#if DEVICE_TYPE == 1
  g_zigbeeTransport.onAdaptiveMode = zigbee_onAdaptiveMode;
  g_zigbeeTransport.onLowTemp = zigbee_onLowTemp;
  g_zigbeeTransport.onHighTemp = zigbee_onHighTemp;
  g_zigbeeTransport.onLowHum = zigbee_onLowHum;
  g_zigbeeTransport.onHighHum = zigbee_onHighHum;
#endif

#if MQTT_RESET_ENABLED == 1
  g_zigbeeTransport.onReset = zigbee_onReset;
#endif

  g_initialized = true;
  return &g_zigbeeTransport;
}

#else  // TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE == 0

Transport* getZigbeeTransport() {
  return nullptr;
}

#endif