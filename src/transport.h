#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <Arduino.h>
#include "config_manager.h"
#include "transport_types.h"

// ============================================================================
// СТРУКТУРА ТРАНСПОРТА
// ============================================================================

typedef struct Transport {
  // ========================================================================
  // УПРАВЛЕНИЕ
  // ========================================================================

  bool (*begin)(Client* client, const ConfigData* config);
  void (*process)();
  bool (*isConnected)();
  void (*disconnect)();
  const char* (*getName)();

  // ========================================================================
  // ПУБЛИКАЦИЯ (устройство → сеть)
  // ========================================================================

  void (*publishOnline)();
  void (*publishState)(bool on);
  void (*publishSpeed)(int percent);
  void (*publishDelaySec)(int seconds);
  void (*publishMaxOnTime)(uint32_t seconds);
  void (*publishSensorControlMode)(bool enabled);

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  void (*publishSensor)(float temp, float hum);
#endif

#if DEVICE_TYPE == 1
  void (*publishAdaptiveMode)(bool enabled);
  void (*publishThresholds)(float lowTemp,
                            float highTemp,
                            float lowHum,
                            float highHum);
#endif

  // --- ДОБАВЛЯЕМ НЕДОСТАЮЩИЕ МЕТОДЫ ---
  void (*publishRSSI)(int rssi);
  void (*publishVersion)(const char* version);
  void (*publishResetReason)(const char* reason);

  // ========================================================================
  // РЕГИСТРАЦИЯ КОЛБЭКОВ (сеть → устройство)
  // ========================================================================

  void (*onState)(TransportBoolCallback callback, void* context);
  void (*onSpeed)(TransportIntCallback callback, void* context);
  void (*onDelaySec)(TransportIntCallback callback, void* context);
  void (*onMaxOnTime)(TransportUintCallback callback, void* context);
  void (*onSensorControlMode)(TransportBoolCallback callback, void* context);

#if DEVICE_TYPE == 1
  void (*onAdaptiveMode)(TransportBoolCallback callback, void* context);
  void (*onLowTemp)(TransportFloatCallback callback, void* context);
  void (*onHighTemp)(TransportFloatCallback callback, void* context);
  void (*onLowHum)(TransportFloatCallback callback, void* context);
  void (*onHighHum)(TransportFloatCallback callback, void* context);
#endif

#if MQTT_RESET_ENABLED == 1
  void (*onReset)(TransportVoidCallback callback, void* context);
#endif

} Transport;

// ============================================================================
// ГЛОБАЛЬНЫЙ УКАЗАТЕЛЬ
// ============================================================================

extern Transport* g_transport;

#endif  // TRANSPORT_H