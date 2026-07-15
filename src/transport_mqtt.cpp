#include "transport_mqtt.h"
#include "logger.h"


// ============================================================================
// СТАТИЧЕСКИЕ ОБЪЕКТЫ
// ============================================================================

static Transport g_mqttTransport;
static bool g_initialized = false;

// ============================================================================
// КОНТЕЙНЕРЫ ДЛЯ КОЛБЭКОВ
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
// ОБЁРТКИ КОЛБЭКОВ (MQTTManager → Transport)
// ============================================================================

static void onStateWrapper(bool value, void* context) {
  (void)context;
  if (_onState.func)
    _onState.func(value, _onState.context);
}

static void onSpeedWrapper(int value, void* context) {
  (void)context;
  if (_onSpeed.func)
    _onSpeed.func(value, _onSpeed.context);
}

static void onDelaySecWrapper(int value, void* context) {
  (void)context;
  if (_onDelaySec.func)
    _onDelaySec.func(value, _onDelaySec.context);
}

static void onMaxOnTimeWrapper(uint32_t value, void* context) {
  (void)context;
  if (_onMaxOnTime.func)
    _onMaxOnTime.func(value, _onMaxOnTime.context);
}

static void onSensorControlModeWrapper(bool value, void* context) {
  (void)context;
  if (_onSensorControlMode.func)
    _onSensorControlMode.func(value, _onSensorControlMode.context);
}

#if DEVICE_TYPE == 1
static void onAdaptiveModeWrapper(bool value, void* context) {
  (void)context;
  if (_onAdaptiveMode.func)
    _onAdaptiveMode.func(value, _onAdaptiveMode.context);
}

static void onLowTempWrapper(float value, void* context) {
  (void)context;
  if (_onLowTemp.func)
    _onLowTemp.func(value, _onLowTemp.context);
}

static void onHighTempWrapper(float value, void* context) {
  (void)context;
  if (_onHighTemp.func)
    _onHighTemp.func(value, _onHighTemp.context);
}

static void onLowHumWrapper(float value, void* context) {
  (void)context;
  if (_onLowHum.func)
    _onLowHum.func(value, _onLowHum.context);
}

static void onHighHumWrapper(float value, void* context) {
  (void)context;
  if (_onHighHum.func)
    _onHighHum.func(value, _onHighHum.context);
}
#endif

#if MQTT_RESET_ENABLED == 1
static void onResetWrapper(void* context) {
  (void)context;
  if (_onReset.func)
    _onReset.func(_onReset.context);
}
#endif

// ============================================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ ТРАНСПОРТА
// ============================================================================

static bool mqtt_begin(Client* client, const ConfigData* config) {
  if (!client || !config) {
    XLOG_ERROR(CAT_MQTT, "mqtt_begin: invalid params");
    return false;
  }

  return mqttManager.begin(*client, config->mqttBroker, config->mqttPort,
                           config->mqttClientId, config->mqttUser,
                           config->mqttPassword);
}

static void mqtt_process() {
  mqttManager.process();
}

static bool mqtt_isConnected() {
  return mqttManager.isConnected();
}

static void mqtt_disconnect() {
  mqttManager.disconnect();
}

static const char* mqtt_getName() {
  return "MQTT";
}

// ============================================================================
// ПУБЛИКАЦИЯ
// ============================================================================

static void mqtt_publishOnline() {
  mqttManager.publishOnline();
}

static void mqtt_publishState(bool on) {
  mqttManager.publishState(on);
}

static void mqtt_publishSpeed(int percent) {
  mqttManager.publishSpeed(percent);
}

static void mqtt_publishDelaySec(int seconds) {
  mqttManager.publishDelaySec(seconds);
}

static void mqtt_publishMaxOnTime(uint32_t seconds) {
  mqttManager.publishMaxOnTime(seconds);
}

static void mqtt_publishSensorControlMode(bool enabled) {
  mqttManager.publishSensorControlMode(enabled);
}

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
static void mqtt_publishSensor(float temp, float hum) {
  mqttManager.publishSensor(temp, hum);
}
#endif

#if DEVICE_TYPE == 1
static void mqtt_publishAdaptiveMode(bool enabled) {
  mqttManager.publishAdaptiveMode(enabled);
}

static void mqtt_publishThresholds(float lowTemp,
                                   float highTemp,
                                   float lowHum,
                                   float highHum) {
  mqttManager.publishThresholds(lowTemp, highTemp, lowHum, highHum);
}
#endif

// --- ДОБАВЛЯЕМ НЕДОСТАЮЩИЕ МЕТОДЫ ---

static void mqtt_publishRSSI(int rssi) {
#if MQTT_PUBLISH_RSSI == 1
  mqttManager.publishRSSI(rssi);
#else
  (void)rssi;
#endif
}

static void mqtt_publishVersion(const char* version) {
#if MQTT_PUBLISH_VERSION == 1
  mqttManager.publishVersion(version);
#else
  (void)version;
#endif
}

static void mqtt_publishResetReason(const char* reason) {
#if MQTT_PUBLISH_RESET_REASON == 1
  mqttManager.publishResetReason(reason);
#else
  (void)reason;
#endif
}

// ============================================================================
// РЕГИСТРАЦИЯ КОЛБЭКОВ
// ============================================================================

static void mqtt_onState(TransportBoolCallback callback, void* context) {
  _onState.func = callback;
  _onState.context = context;
  mqttManager.onState(onStateWrapper, nullptr);
}

static void mqtt_onSpeed(TransportIntCallback callback, void* context) {
  _onSpeed.func = callback;
  _onSpeed.context = context;
  mqttManager.onSpeed(onSpeedWrapper, nullptr);
}

static void mqtt_onDelaySec(TransportIntCallback callback, void* context) {
  _onDelaySec.func = callback;
  _onDelaySec.context = context;
  mqttManager.onDelaySec(onDelaySecWrapper, nullptr);
}

static void mqtt_onMaxOnTime(TransportUintCallback callback, void* context) {
  _onMaxOnTime.func = callback;
  _onMaxOnTime.context = context;
  mqttManager.onMaxOnTime(onMaxOnTimeWrapper, nullptr);
}

static void mqtt_onSensorControlMode(TransportBoolCallback callback,
                                     void* context) {
  _onSensorControlMode.func = callback;
  _onSensorControlMode.context = context;
  mqttManager.onSensorControlMode(onSensorControlModeWrapper, nullptr);
}

#if DEVICE_TYPE == 1
static void mqtt_onAdaptiveMode(TransportBoolCallback callback, void* context) {
  _onAdaptiveMode.func = callback;
  _onAdaptiveMode.context = context;
  mqttManager.onAdaptiveMode(onAdaptiveModeWrapper, nullptr);
}

static void mqtt_onLowTemp(TransportFloatCallback callback, void* context) {
  _onLowTemp.func = callback;
  _onLowTemp.context = context;
  mqttManager.onLowTemp(onLowTempWrapper, nullptr);
}

static void mqtt_onHighTemp(TransportFloatCallback callback, void* context) {
  _onHighTemp.func = callback;
  _onHighTemp.context = context;
  mqttManager.onHighTemp(onHighTempWrapper, nullptr);
}

static void mqtt_onLowHum(TransportFloatCallback callback, void* context) {
  _onLowHum.func = callback;
  _onLowHum.context = context;
  mqttManager.onLowHum(onLowHumWrapper, nullptr);
}

static void mqtt_onHighHum(TransportFloatCallback callback, void* context) {
  _onHighHum.func = callback;
  _onHighHum.context = context;
  mqttManager.onHighHum(onHighHumWrapper, nullptr);
}
#endif

#if MQTT_RESET_ENABLED == 1
static void mqtt_onReset(TransportVoidCallback callback, void* context) {
  _onReset.func = callback;
  _onReset.context = context;
  mqttManager.onReset(onResetWrapper, nullptr);
}
#endif

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ ТРАНСПОРТА
// ============================================================================

Transport* getMQTTTransport() {
  if (g_initialized) {
    return &g_mqttTransport;
  }

  // Управление
  g_mqttTransport.begin = mqtt_begin;
  g_mqttTransport.process = mqtt_process;
  g_mqttTransport.isConnected = mqtt_isConnected;
  g_mqttTransport.disconnect = mqtt_disconnect;
  g_mqttTransport.getName = mqtt_getName;

  // Публикация
  g_mqttTransport.publishOnline = mqtt_publishOnline;
  g_mqttTransport.publishState = mqtt_publishState;
  g_mqttTransport.publishSpeed = mqtt_publishSpeed;
  g_mqttTransport.publishDelaySec = mqtt_publishDelaySec;
  g_mqttTransport.publishMaxOnTime = mqtt_publishMaxOnTime;
  g_mqttTransport.publishSensorControlMode = mqtt_publishSensorControlMode;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  g_mqttTransport.publishSensor = mqtt_publishSensor;
#endif

#if DEVICE_TYPE == 1
  g_mqttTransport.publishAdaptiveMode = mqtt_publishAdaptiveMode;
  g_mqttTransport.publishThresholds = mqtt_publishThresholds;
#endif

  // --- ДОБАВЛЯЕМ ---
  g_mqttTransport.publishRSSI = mqtt_publishRSSI;
  g_mqttTransport.publishVersion = mqtt_publishVersion;
  g_mqttTransport.publishResetReason = mqtt_publishResetReason;

  // Колбэки
  g_mqttTransport.onState = mqtt_onState;
  g_mqttTransport.onSpeed = mqtt_onSpeed;
  g_mqttTransport.onDelaySec = mqtt_onDelaySec;
  g_mqttTransport.onMaxOnTime = mqtt_onMaxOnTime;
  g_mqttTransport.onSensorControlMode = mqtt_onSensorControlMode;

#if DEVICE_TYPE == 1
  g_mqttTransport.onAdaptiveMode = mqtt_onAdaptiveMode;
  g_mqttTransport.onLowTemp = mqtt_onLowTemp;
  g_mqttTransport.onHighTemp = mqtt_onHighTemp;
  g_mqttTransport.onLowHum = mqtt_onLowHum;
  g_mqttTransport.onHighHum = mqtt_onHighHum;
#endif

#if MQTT_RESET_ENABLED == 1
  g_mqttTransport.onReset = mqtt_onReset;
#endif

  g_initialized = true;
  return &g_mqttTransport;
}