/**
 * @file state_provider.cpp
 * @brief Реализация единого источника данных о состоянии устройства
 */

#include "state_provider.h"
#include "logger.h"

// ============================================================================
// СИНГЛТОН
// ============================================================================

StateProvider& StateProvider::getInstance() {
  static StateProvider instance;
  return instance;
}

// ============================================================================
// КОНСТРУКТОР
// ============================================================================

StateProvider::StateProvider() {
  memset(&_state, 0, sizeof(_state));
  _state.start_time = millis();
  _state.uptime = 0;
  _initialized = true;
  XLOG_INFO(CAT_SYSTEM, "StateProvider initialized");
}

// ============================================================================
// ЧТЕНИЕ СОСТОЯНИЯ
// ============================================================================

const DeviceState* StateProvider::get_state() const {
  return &_state;
}

// ============================================================================
// ОБНОВЛЕНИЕ ПОКАЗАНИЙ ДАТЧИКА
// ============================================================================

void StateProvider::update_sensor(float temp,
                                  float hum,
                                  bool valid,
                                  const char* error) {
  _state.temperature = temp;
  _state.humidity = hum;
  _state.sensor_valid = valid;
  _state.sensor_error = error;
  XLOG_DEBUG(CAT_SENSOR, "Sensor: %.1f°C, %.1f%%, valid=%d", temp, hum, valid);
}

// ============================================================================
// ОБНОВЛЕНИЕ ОПЕРАТИВНОГО СОСТОЯНИЯ
// ============================================================================

void StateProvider::update_operational(bool on,
                                       uint8_t speed,
                                       bool manual,
                                       bool adaptive) {
  bool changed = false;

  if (_state.is_on != on) {
    _state.is_on = on;
    changed = true;
  }
  if (_state.speed != speed) {
    _state.speed = speed;
    changed = true;
  }
  if (_state.manual_mode != manual) {
    _state.manual_mode = manual;
    changed = true;
  }
  if (_state.adaptive_active != adaptive) {
    _state.adaptive_active = adaptive;
    changed = true;
  }

  if (changed) {
    XLOG_DEBUG(CAT_DEVICE,
               "Operational: on=%d, speed=%d, manual=%d, adaptive=%d", on,
               speed, manual, adaptive);
  }
}

// ============================================================================
// ОБНОВЛЕНИЕ СОСТОЯНИЯ ПОДКЛЮЧЕНИЙ
// ============================================================================

void StateProvider::update_connection(bool wifi, bool mqtt, int rssi) {
  bool changed = false;

  if (_state.wifi_connected != wifi) {
    _state.wifi_connected = wifi;
    changed = true;
  }
  if (_state.mqtt_connected != mqtt) {
    _state.mqtt_connected = mqtt;
    changed = true;
  }
  if (_state.wifi_rssi != rssi) {
    _state.wifi_rssi = rssi;
    changed = true;
  }

  if (changed) {
    XLOG_DEBUG(CAT_WIFI, "Connection: wifi=%d, mqtt=%d, rssi=%d", wifi, mqtt,
               rssi);
  }
}

// ============================================================================
// ОБНОВЛЕНИЕ СИСТЕМНЫХ ФЛАГОВ
// ============================================================================

void StateProvider::update_provisioning(bool active) {
  if (_state.provisioning != active) {
    _state.provisioning = active;
    XLOG_INFO(CAT_PROVISIONING, "Provisioning: %s", active ? "ON" : "OFF");
  }
}

void StateProvider::update_emergency(bool active) {
  if (_state.emergency != active) {
    _state.emergency = active;
    XLOG_WARN(CAT_SYSTEM, "Emergency: %s", active ? "ON" : "OFF");
  }
}

void StateProvider::update_restart(bool pending) {
  if (_state.restart_pending != pending) {
    _state.restart_pending = pending;
    XLOG_INFO(CAT_RESTART, "Restart pending: %s", pending ? "ON" : "OFF");
  }
}

// ============================================================================
// ОБНОВЛЕНИЕ СОСТОЯНИЯ КНОПКИ
// ============================================================================

void StateProvider::update_button(bool pressed, uint8_t stage) {
  bool changed = false;

  if (_state.button_pressed != pressed) {
    _state.button_pressed = pressed;
    changed = true;
  }
  if (_state.button_stage != stage) {
    _state.button_stage = stage;
    changed = true;
  }

  if (changed) {
    XLOG_DEBUG(CAT_RESET_BTN, "Button: pressed=%d, stage=%d", pressed, stage);
  }
}

// ============================================================================
// ОБНОВЛЕНИЕ ВРЕМЕНИ
// ============================================================================

void StateProvider::update_uptime(unsigned long uptime) {
  _state.uptime = uptime;
}