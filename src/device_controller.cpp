/**
 * @file device_controller.cpp
 * @brief Реализация бизнес-логики устройства
 * @version 0.13
 * @date 11.08.2026
 */

#include "device_controller.h"
#include <string.h>
#include "logger.h"
#include "settings.h"

// ============================================================
// КОНСТРУКТОР
// ============================================================

DeviceController::DeviceController()
    : _config(nullptr),
      _changed(false),
      _callback(nullptr),
      _delayTimerRunning(false),
      _delayTimerStart(0) {
  memset(&_state, 0, sizeof(_state));
}

// ============================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================

void DeviceController::init(const DeviceConfig* config,
                            const DeviceState*& outState) {
  if (!config) {
    XLOG_ERROR(CAT_DEVICE, "init: config is NULL");
    outState = nullptr;
    return;
  }

  _config = config;

  // Датчик
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_init(&_sensor, SENSOR_TYPE, SENSOR_PIN);
  XLOG_INFO(CAT_DEVICE, "Sensor initialized: type=%d", SENSOR_TYPE);
#endif

  // Актуатор
#if DEVICE_TYPE == 1
  _actuator.init(SWITCH_PIN, RELAY_ON_LEVEL, _config->bootState,
                 _config->speedPercent, _config->adaptiveMode,
                 _config->delaySeconds, _config->maxOnTime);
  XLOG_INFO(CAT_DEVICE, "FanActuator initialized: pin=%d", SWITCH_PIN);
#elif DEVICE_TYPE == 3
  _actuator.init(SWITCH_PIN, RELAY_ON_LEVEL, _config->bootState,
                 _config->delaySeconds, _config->maxOnTime);
  XLOG_INFO(CAT_DEVICE, "SwitchActuator initialized: pin=%d", SWITCH_PIN);
#endif

  // Состояние из конфига
  _state.isOn = _config->bootState;
  _state.speed = _config->speedPercent;

  if (_config->bootState) {
    _state.manualMode = true;
    _state.sensorMode = false;
    _state.adaptiveMode = false;
  } else {
    _state.manualMode = !_config->sensorControlMode;
    _state.sensorMode = _config->sensorControlMode;
    _state.adaptiveMode = _config->adaptiveMode;
  }

  _state.delayRemain = _config->delaySeconds;
  _state.maxOnRemain = _config->maxOnTime;
  _state.sensorValid = false;
  _state.temperature = 0.0f;
  _state.humidity = 0.0f;

  _delayTimerRunning = false;
  _delayTimerStart = 0;

  applyStateToActuator();
  _changed = true;

  // ===== ПЕРЕДАЁМ УКАЗАТЕЛЬ НА _state НАРУЖУ =====
  outState = &_state;

  notifyChange(0xFFFFFFFF);

  XLOG_INFO(CAT_DEVICE, "Init complete: isOn=%d, speed=%d, manualMode=%d",
            _state.isOn, _state.speed, _state.manualMode);
}

// ============================================================
// СЕТТЕРЫ
// ============================================================

void DeviceController::setOn(bool on) {
  if (_state.isOn == on)
    return;

  uint32_t changes = 0;

  _state.isOn = on;
  changes |= STATE_CHANGED_IS_ON;

  if (!_state.manualMode) {
    _state.manualMode = true;
    changes |= STATE_CHANGED_MANUAL_MODE;
  }
  if (!on && _state.speed != _config->speedPercent) {
    _state.speed = _config->speedPercent;
    changes |= STATE_CHANGED_SPEED;
  }

  if (_state.sensorMode) {
    _state.sensorMode = false;
    changes |= STATE_CHANGED_SENSOR_MODE;
  }

  if (_state.adaptiveMode) {
    _state.adaptiveMode = false;
    changes |= STATE_CHANGED_ADAPTIVE_MODE;
  }

  XLOG_DEBUG(CAT_DEVICE, "setOn: %s", on ? "ON" : "OFF");

  _changed = true;
  applyStateToActuator();
  notifyChange(changes);
}

void DeviceController::setSpeed(uint8_t percent) {
  if (percent > 100)
    percent = 100;
  if (!_state.isOn) {
    XLOG_DEBUG(CAT_DEVICE, "setSpeed: %d%% ignored (actuator OFF)", percent);
    return;
  }
  if (_state.speed == percent)
    return;

  uint32_t changes = 0;

  _state.speed = percent;
  changes |= STATE_CHANGED_SPEED;

  if (!_state.manualMode) {
    _state.manualMode = true;
    changes |= STATE_CHANGED_MANUAL_MODE;
  }

  if (_state.adaptiveMode) {
    _state.adaptiveMode = false;
    changes |= STATE_CHANGED_ADAPTIVE_MODE;
  }

  XLOG_DEBUG(CAT_DEVICE, "setSpeed: %d%%", percent);

  _changed = true;
  applyStateToActuator();
  notifyChange(changes);
}

void DeviceController::setSensorMode(bool on) {
  if (_state.sensorMode == on)
    return;

  uint32_t changes = 0;

  _state.sensorMode = on;
  changes |= STATE_CHANGED_SENSOR_MODE;

  if (on && _state.manualMode) {
    _state.manualMode = false;
    changes |= STATE_CHANGED_MANUAL_MODE;
  }

  XLOG_DEBUG(CAT_DEVICE, "setSensorMode: %s", on ? "ON" : "OFF");

  _changed = true;
  notifyChange(changes);
}

void DeviceController::setAdaptiveMode(bool on) {
  if (_state.adaptiveMode == on)
    return;

  uint32_t changes = 0;

  _state.adaptiveMode = on;
  changes |= STATE_CHANGED_ADAPTIVE_MODE;

  if (on) {
    if (_state.manualMode) {
      _state.manualMode = false;
      changes |= STATE_CHANGED_MANUAL_MODE;
    }
  } else {
    if (_state.isOn && _state.speed != _config->speedPercent) {
      _state.speed = _config->speedPercent;
      changes |= STATE_CHANGED_SPEED;
    }
  }

  XLOG_DEBUG(CAT_DEVICE, "setAdaptiveMode: %s",on ? "ON" : "OFF");

  _changed = true;
  applyStateToActuator();
  notifyChange(changes);
}

// ============================================================
// ВНУТРЕННИЕ МЕТОДЫ — ТАЙМЕРЫ
// ============================================================

bool DeviceController::updateDelayTimer(uint32_t& changes) {
  bool changed = false;
  changes = 0;

  static unsigned long lastUpdateTime = 0;
  unsigned long now = millis();
  if (now - lastUpdateTime < 1000)
    return false;
  lastUpdateTime = now;

  // ===== ЗАПУСК ТАЙМЕРА =====
  if (_config->delaySeconds > 0 && !_state.isOn && !_delayTimerRunning &&
      !_state.manualMode) {
    _delayTimerRunning = true;
    _delayTimerStart = millis();
    _state.delayRemain = _config->delaySeconds;
    changed = true;
    changes |= STATE_CHANGED_DELAY_REMAIN;
    XLOG_INFO(CAT_DEVICE, "Delay timer started: %lu sec",
              _config->delaySeconds);
    return changed;
  }

  // ===== ПРОВЕРКА ТАЙМЕРА =====
  if (_delayTimerRunning) {
    unsigned long elapsed = (millis() - _delayTimerStart) / 1000UL;
    if (elapsed >= _config->delaySeconds) {
      _delayTimerRunning = false;

      _state.delayRemain = 0;
      changes |= STATE_CHANGED_DELAY_REMAIN;

      if (!_state.isOn) {
        _state.isOn = true;
        changes |= STATE_CHANGED_IS_ON;
      }

      if (!_state.manualMode) {
        _state.manualMode = true;
        changes |= STATE_CHANGED_MANUAL_MODE;
      }

      if (_state.sensorMode) {
        _state.sensorMode = false;
        changes |= STATE_CHANGED_SENSOR_MODE;
      }

      if (_state.adaptiveMode) {
        _state.adaptiveMode = false;
        changes |= STATE_CHANGED_ADAPTIVE_MODE;
      }

      changed = true;
      XLOG_INFO(CAT_DEVICE, "Delay timer expired: %lu sec",
                _config->delaySeconds);
    } else {
      uint32_t remain = _config->delaySeconds - elapsed;
      if (remain != _state.delayRemain) {
        _state.delayRemain = remain;
        changed = true;
        changes |= STATE_CHANGED_DELAY_REMAIN;
      }
    }
  }

  // ===== ОТМЕНА ТАЙМЕРА =====
  if (_state.isOn && _delayTimerRunning) {
    _delayTimerRunning = false;
    if (_state.delayRemain != 0) {
      _state.delayRemain = 0;
      changes |= STATE_CHANGED_DELAY_REMAIN;
    }
    changed = true;
    XLOG_DEBUG(CAT_DEVICE, "Delay timer cancelled");
  }

  if (!_delayTimerRunning && _state.delayRemain != 0) {
    _state.delayRemain = 0;
    changed = true;
    changes |= STATE_CHANGED_DELAY_REMAIN;
  }

  return changed;
}

bool DeviceController::updateTimerRemains(uint32_t& changes) {
  bool changed = false;
  changes = 0;

  static unsigned long lastUpdateTime = 0;
  unsigned long now = millis();
  if (now - lastUpdateTime < 1000)
    return false;
  lastUpdateTime = now;

  uint32_t maxOnRemain = 0;
  if (_config->maxOnTime > 0 && _config->maxOnTime <= 86400) {
    if (_state.isOn) {
      unsigned long elapsed = (millis() - _actuator.getStartTime()) / 1000UL;
      if (elapsed >= _config->maxOnTime) {
        maxOnRemain = (uint32_t)-1;
      } else {
        maxOnRemain = _config->maxOnTime - elapsed;
      }
    } else {
      maxOnRemain = _config->maxOnTime;
    }
  }

  if (maxOnRemain != _state.maxOnRemain) {
    _state.maxOnRemain = maxOnRemain;
    changed = true;
    changes |= STATE_CHANGED_MAX_ON_REMAIN;
    XLOG_DEBUG(CAT_DEVICE, "TIMER: maxOnRemain=%lu", maxOnRemain);
  }

  return changed;
}

// ============================================================
// АДАПТИВНЫЙ РЕЖИМ
// ============================================================

int DeviceController::calculateAdaptiveSpeed() const {
  if (!_state.adaptiveMode || _state.manualMode || !_state.isOn ||
      _state.speed >= 100 || !_state.sensorValid) {
    return -1;
  }

  int newSpeed = _state.speed;

  if (_state.temperature > _config->highTemp + 1.0f ||
      _state.humidity > _config->highHum + 5.0f) {
    newSpeed += 10;
    if (newSpeed > 100)
      newSpeed = 100;
  } else if (_state.temperature < _config->lowTemp - 1.0f &&
             _state.humidity < _config->lowHum - 5.0f) {
    newSpeed -= 10;
    if (newSpeed < _config->speedPercent) {
      newSpeed = _config->speedPercent;
    }
  } else {
    return -1;
  }

  if (newSpeed == _state.speed)
    return -1;
  return newSpeed;
}

// ============================================================
// UPDATE
// ============================================================

void DeviceController::update() {
  if (!_config)
    return;

#if DEVICE_TYPE == 1
  bool changed = false;
  uint32_t changes = 0;
  // ===== АКТУАТОР (обработка стартового импульса, таймеров) =====
  _actuator.update();
  
  // ===== 1. ДАТЧИК =====
  sensor_update(&_sensor);
  float newTemp = sensor_getTemperature(&_sensor);
  float newHum = sensor_getHumidity(&_sensor);
  bool newValid = sensor_isOk(&_sensor);

  if (newTemp != _state.temperature) {
    _state.temperature = newTemp;
    changed = true;
    changes |= STATE_CHANGED_TEMPERATURE;
  }
  if (newHum != _state.humidity) {
    _state.humidity = newHum;
    changed = true;
    changes |= STATE_CHANGED_HUMIDITY;
  }
  if (newValid != _state.sensorValid) {
    _state.sensorValid = newValid;
    changed = true;
    changes |= STATE_CHANGED_SENSOR_VALID;
    XLOG_DEBUG(CAT_DEVICE, "SENSOR: valid=%d, temp=%.1f, hum=%.1f", newValid,
               newTemp, newHum);
  }

  // ===== 2. АВАРИЯ =====
  if (_actuator.isEmergencyStop()) {
    if (changed) {
      _changed = true;
      notifyChange(changes);
    }
    return;
  }

  // ===== 3. ТАЙМЕР ОТЛОЖЕННОГО ВКЛЮЧЕНИЯ =====
  uint32_t timerChanges = 0;
  if (updateDelayTimer(timerChanges)) {
    changed = true;
    changes |= timerChanges;

    if (!_delayTimerRunning && _state.delayRemain == 0 && !_state.isOn) {
      _state.isOn = true;
      changes |= STATE_CHANGED_IS_ON;

      _state.manualMode = true;
      changes |= STATE_CHANGED_MANUAL_MODE;

      _state.sensorMode = false;
      changes |= STATE_CHANGED_SENSOR_MODE;

      _state.adaptiveMode = false;
      changes |= STATE_CHANGED_ADAPTIVE_MODE;

      XLOG_INFO(CAT_DEVICE, "Delay timer expired: turning ON (manual mode)");
    }
  }

  // ===== 4. АВТОМАТИКА ПО ПОРОГАМ =====
  if (_state.sensorMode && !_state.manualMode && _state.sensorValid) {
    bool shouldBeOn = false;
    if (_state.temperature >= _config->highTemp ||
        _state.humidity >= _config->highHum) {
      shouldBeOn = true;
    } else if (_state.temperature < _config->lowTemp &&
               _state.humidity < _config->lowHum) {
      shouldBeOn = false;
    } else {
      shouldBeOn = _state.isOn;
    }

    if (shouldBeOn != _state.isOn) {
      _state.isOn = shouldBeOn;
      changed = true;
      changes |= STATE_CHANGED_IS_ON;

      if (!shouldBeOn) {
        _state.speed = _config->speedPercent;
        changes |= STATE_CHANGED_SPEED;
      }
      XLOG_DEBUG(CAT_DEVICE, "AUTO: isOn=%d", shouldBeOn);
    }
  }

  // ===== 5. АДАПТИВНЫЙ РЕЖИМ =====
  int newSpeed = calculateAdaptiveSpeed();
  if (newSpeed >= 0) {
    _state.speed = newSpeed;
    changed = true;
    changes |= STATE_CHANGED_SPEED;
    XLOG_DEBUG(CAT_DEVICE, "ADAPTIVE: speed=%d%%", newSpeed);
  }

  // ===== 6. ТАЙМЕРЫ =====
  uint32_t remainChanges = 0;
  if (updateTimerRemains(remainChanges)) {
    changed = true;
    changes |= remainChanges;
  }

  // ===== 7. ПРИМЕНЕНИЕ =====
  if (changed) {
    applyStateToActuator();
    _changed = true;
    notifyChange(changes);
  }

#elif DEVICE_TYPE == 2
  bool changed = false;
  uint32_t changes = 0;

  sensor_update(&_sensor);
  float newTemp = sensor_getTemperature(&_sensor);
  float newHum = sensor_getHumidity(&_sensor);
  bool newValid = sensor_isOk(&_sensor);

  if (newTemp != _state.temperature) {
    _state.temperature = newTemp;
    changed = true;
    changes |= STATE_CHANGED_TEMPERATURE;
  }
  if (newHum != _state.humidity) {
    _state.humidity = newHum;
    changed = true;
    changes |= STATE_CHANGED_HUMIDITY;
  }
  if (newValid != _state.sensorValid) {
    _state.sensorValid = newValid;
    changed = true;
    changes |= STATE_CHANGED_SENSOR_VALID;
    XLOG_DEBUG(CAT_DEVICE, "SENSOR: valid=%d, temp=%.1f, hum=%.1f", newValid,
               newTemp, newHum);
  }

  if (changed) {
    _changed = true;
    notifyChange(changes);
  }

#elif DEVICE_TYPE == 3
  bool changed = false;
  uint32_t changes = 0;

  _actuator.update(_config->delaySeconds, _config->maxOnTime);

  if (_actuator.isEmergencyStop()) {
    if (changed) {
      _changed = true;
      notifyChange(changes);
    }
    return;
  }

  // ===== ТАЙМЕР ОТЛОЖЕННОГО ВКЛЮЧЕНИЯ =====
  uint32_t timerChanges = 0;
  if (updateDelayTimer(timerChanges)) {
    changed = true;
    changes |= timerChanges;

    if (!_delayTimerRunning && _state.delayRemain == 0 && !_state.isOn) {
      _state.isOn = true;
      changes |= STATE_CHANGED_IS_ON;

      _state.manualMode = true;
      changes |= STATE_CHANGED_MANUAL_MODE;

      _state.sensorMode = false;
      changes |= STATE_CHANGED_SENSOR_MODE;

      _state.adaptiveMode = false;
      changes |= STATE_CHANGED_ADAPTIVE_MODE;

      XLOG_INFO(CAT_DEVICE, "Delay timer expired: turning ON (manual mode)");
    }
  }

  // ===== ТАЙМЕРЫ =====
  uint32_t remainChanges = 0;
  if (updateTimerRemains(remainChanges)) {
    changed = true;
    changes |= remainChanges;
  }

  // ===== СИНХРОНИЗАЦИЯ =====
  bool actuatorState = _actuator.getState();
  if (actuatorState != _state.isOn) {
    _state.isOn = actuatorState;
    changed = true;
    changes |= STATE_CHANGED_IS_ON;
  }

  if (changed) {
    applyStateToActuator();
    _changed = true;
    notifyChange(changes);
  }

#endif  // DEVICE_TYPE
}

// ============================================================
// КОЛБЭК
// ============================================================

void DeviceController::onStateChanged(DeviceControllerCallback callback) {
  _callback = callback;
}

// ============================================================
// ВНУТРЕННИЕ МЕТОДЫ
// ============================================================

void DeviceController::notifyChange(uint32_t changes) {
  if (!_callback)
    return;
  if (!_changed)
    return;

  _callback(changes);
  _changed = false;

  // XLOG_DEBUG(CAT_DEVICE, "notifyChange: changes=0x%08X", changes);
}

void DeviceController::applyStateToActuator() {
#if DEVICE_TYPE == 1
  bool currentOn = _actuator.getState();
  if (currentOn != _state.isOn) {
    _actuator.set(_state.isOn, _state.manualMode);
    XLOG_DEBUG(CAT_ACTUATOR, "APPLY: isOn %s -> %s", currentOn ? "ON" : "OFF",
               _state.isOn ? "ON" : "OFF");
  }

  int currentSpeed = _actuator.getSpeed();
  if (currentSpeed != _state.speed) {
    _actuator.setSpeed(_state.speed, _state.manualMode);
    XLOG_DEBUG(CAT_ACTUATOR, "APPLY: speed %d%% -> %d%%", currentSpeed,
               _state.speed);
  }

#elif DEVICE_TYPE == 3
  bool currentOn = _actuator.getState();
  if (currentOn != _state.isOn) {
    _actuator.set(_state.isOn, _state.manualMode);
    XLOG_DEBUG(CAT_ACTUATOR, "APPLY: isOn %s -> %s", currentOn ? "ON" : "OFF",
               _state.isOn ? "ON" : "OFF");
  }
#endif
}