/**
 * @file transport_null.cpp
 * @brief Реализация NullTransport — заглушка для отсутствующего транспорта
 * @version 0.12
 */

#include "transport_null.h"
#include "logger.h"

// ============================================================================
// СТАТИЧЕСКИЙ ЭКЗЕМПЛЯР
// ============================================================================

static Transport g_nullTransport;

// ============================================================================
// ЗАГЛУШКИ ДЛЯ ВСЕХ МЕТОДОВ
// ============================================================================

static bool null_begin(Client* client,
                       const TransportConfig* config,
                       const TransportState*& outState) {
  (void)client;
  (void)config;
  (void)outState;
  return false;
}

static void null_setDeviceConfig(const DeviceConfig* config) {
  (void)config;
}

static void null_setDeviceState(const DeviceState* state) {
  (void)state;
}

static void null_update() {}

static bool null_isConnected() {
  return false;
}

static void null_disconnect() {}

static const char* null_getName() {
  return "NullTransport";
}

static void null_publishOnline() {}
static void null_publishState(bool on) {
  (void)on;
}
static void null_publishSpeed(int percent) {
  (void)percent;
}
static void null_publishDelaySec(int seconds) {
  (void)seconds;
}
static void null_publishMaxOnTime(uint32_t seconds) {
  (void)seconds;
}
static void null_publishSensorControlMode(bool enabled) {
  (void)enabled;
}
static void null_publishSensor(float temp, float hum) {
  (void)temp;
  (void)hum;
}
static void null_publishAdaptiveMode(bool enabled) {
  (void)enabled;
}
static void null_publishThresholds(float lowTemp,
                                   float highTemp,
                                   float lowHum,
                                   float highHum) {
  (void)lowTemp;
  (void)highTemp;
  (void)lowHum;
  (void)highHum;
}
static void null_publishRSSI(int rssi) {
  (void)rssi;
}
static void null_publishVersion(const char* version) {
  (void)version;
}
static void null_publishResetReason(const char* reason) {
  (void)reason;
}

static void null_publishFullState(const DeviceState* state) {
  (void)state;
}

static void null_publishConfig(const DeviceConfig* config) {
  (void)config;
}

static void null_onEvent(TransportEventCallback callback, void* context) {
  (void)callback;
  (void)context;
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ СТРУКТУРЫ TRANSPORT
// ============================================================================

static void initNullTransport() {
  g_nullTransport.begin = null_begin;
  g_nullTransport.setDeviceConfig = null_setDeviceConfig;
  g_nullTransport.setDeviceState = null_setDeviceState;
  g_nullTransport.update = null_update;
  g_nullTransport.isConnected = null_isConnected;
  g_nullTransport.disconnect = null_disconnect;
  g_nullTransport.getName = null_getName;

  g_nullTransport.publishOnline = null_publishOnline;
  g_nullTransport.publishState = null_publishState;
  g_nullTransport.publishSpeed = null_publishSpeed;
  g_nullTransport.publishDelaySec = null_publishDelaySec;
  g_nullTransport.publishMaxOnTime = null_publishMaxOnTime;
  g_nullTransport.publishSensorControlMode = null_publishSensorControlMode;
  g_nullTransport.publishSensor = null_publishSensor;
  g_nullTransport.publishAdaptiveMode = null_publishAdaptiveMode;
  g_nullTransport.publishThresholds = null_publishThresholds;
  g_nullTransport.publishRSSI = null_publishRSSI;
  g_nullTransport.publishVersion = null_publishVersion;
  g_nullTransport.publishResetReason = null_publishResetReason;
  g_nullTransport.publishFullState = null_publishFullState;
  g_nullTransport.publishConfig = null_publishConfig;
  g_nullTransport.onEvent = null_onEvent;
}

Transport* getNullTransport() {
  static bool initialized = false;
  if (!initialized) {
    initNullTransport();
    initialized = true;
  }
  return &g_nullTransport;
}