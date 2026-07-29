/**
 * @file device_controller.cpp
 * @brief Реализация бизнес-логики устройства
 */

#include "device_controller.h"
#include "logger.h"
#include "system_state.h"

DeviceController::DeviceController()
    : _config(nullptr)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
      ,
      _actuator(nullptr)
#endif
      ,
      _callback(nullptr) {
  _state.is_on = false;
  _state.speed = 0;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  _state.manual_mode = false;
  _state.timer_mode = false;
  _state.delay_remain_sec = 0;
#endif

#if DEVICE_TYPE == 1
  _state.adaptive_mode_active = false;
#endif
}

void DeviceController::init(const ConfigData* config
#if DEVICE_TYPE == 1
                            ,
                            FanActuator* actuator
#elif DEVICE_TYPE == 3
                            ,
                            SwitchActuator* actuator
#endif
) {
  _config = config;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  _actuator = actuator;
#endif

  if (_config) {
    _state.is_on = _config->bootState;

#if DEVICE_TYPE == 1
    _state.speed = _config->speedPercent;
    _state.adaptive_mode_active = _config->adaptiveMode;
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    _state.manual_mode = false;  // всегда начинаем с AUTO
    _state.timer_mode = false;
    _state.delay_remain_sec = 0;
#endif
  }

  XLOG_INFO(CAT_DEVICE, "DeviceController initialized");
}

void DeviceController::update() {
#if DEVICE_TYPE == 1
  update_fan();
#elif DEVICE_TYPE == 2
  update_sensor();
#elif DEVICE_TYPE == 3
  update_switch();
#endif
}

// ===== TYPE 1: FAN =====

#if DEVICE_TYPE == 1
void DeviceController::update_fan() {
  if (!_config || !_actuator)
    return;

  // 1. Проверка аварийного отключения
  if (_actuator->isEmergencyStop()) {
    // Если авария, блокируем логику
    return;
  }

  // 2. Проверка ручного режима
  if (_state.manual_mode) {
    // В ручном режиме датчик и таймер игнорируются
    return;
  }
  
  if (!sensor_isOk()) {
    // Датчик невалиден — ничего не делаем
    return;
  }
  
  float temp = sensor_getTemperature();
  float hum = sensor_getHumidity();

  // 4. Автоматический режим (пороги)
  bool should_be_on = false;
  if (temp >= _config->highTemp || hum >= _config->highHum) {
    should_be_on = true;
  } else if (temp < _config->lowTemp && hum < _config->lowHum) {
    should_be_on = false;
  } else {
    // В зоне гистерезиса — сохраняем текущее состояние
    should_be_on = _state.is_on;
  }

  // 5. Применяем решение
  bool state_changed = false;

  if (should_be_on && !_state.is_on) {
    // Включение
    _state.is_on = true;
    _actuator->set(true, false);  // auto mode
    state_changed = true;
  } else if (!should_be_on && _state.is_on) {
    // Выключение
    _state.is_on = false;
    _state.speed = _config->speedPercent;  // сброс скорости
    _actuator->set(false, false);          // auto mode
    state_changed = true;
  }

  // 6. Адаптивный режим
  if (_state.is_on && _state.adaptive_mode_active && _config->adaptiveMode &&
      _state.speed < 100) {
    // Адаптивная логика
    int new_speed = _state.speed;

    if (temp > _config->highTemp + 1.0f || hum > _config->highHum + 5.0f) {
      // Ухудшение → увеличиваем скорость
      new_speed += 10;
      if (new_speed > 100)
        new_speed = 100;
    } else if (temp < _config->lowTemp - 1.0f && hum < _config->lowHum - 5.0f) {
      // Улучшение → снижаем скорость
      new_speed -= 10;
      if (new_speed < _config->speedPercent)
        new_speed = _config->speedPercent;
    }

    if (new_speed != _state.speed) {
      _state.speed = new_speed;
      _actuator->setSpeed(_state.speed, false);
      state_changed = true;
    }
  }

  // 7. Уведомление об изменении
  if (state_changed) {
    notify_change(false);
  }
}
#endif

// ===== TYPE 2: SENSOR =====

#if DEVICE_TYPE == 2
void DeviceController::update_sensor() {
  // TYPE 2 ничего не делает, только публикует данные через колбэк
  // Публикация будет в оркестраторе
}
#endif

// ===== TYPE 3: SWITCH =====

#if DEVICE_TYPE == 3
void DeviceController::update_switch() {
  if (!_config || !_actuator)
    return;

  // 1. Проверка аварийного отключения
  if (_actuator->isEmergencyStop()) {
    return;
  }

  // 2. Ручной режим — ничего не делаем
  if (_state.manual_mode) {
    return;
  }

  // 3. Таймер отложенного включения (только в автоматическом режиме)
  if (_config->delaySeconds > 0 && !_state.is_on && !_state.timer_mode) {
    _state.timer_mode = true;
    _state.delay_remain_sec = _config->delaySeconds;
    XLOG_INFO(CAT_DEVICE, "Delay timer started: %d sec", _config->delaySeconds);
  }

  if (_state.timer_mode) {
    if (_state.delay_remain_sec > 0) {
      _state.delay_remain_sec--;
      // Каждую секунду обновляем
      if (_state.delay_remain_sec % 5 == 0) {
        notify_change(false);
      }
    } else {
      // Таймер сработал
      _state.is_on = true;
      _state.timer_mode = false;
      _state.manual_mode = true;  // переход в ручной режим
      _actuator->set(true, true);
      notify_change(false);
      XLOG_INFO(CAT_DEVICE, "Delay timer expired, switch ON (manual mode)");
    }
  }
}
#endif

// ===== ОБЩИЕ МЕТОДЫ =====

void DeviceController::handle_command(command_type_t type, float value) {
  bool need_save = false;

  switch (type) {
    case CMD_DO_RESET:
      // Сброс к заводским — оркестратор сам обработает
      XLOG_WARN(CAT_DEVICE, "Factory reset requested");
      break;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    case CMD_SET_ACTUATOR:
      set_state((bool)value);
      break;

    case CMD_SET_DELAY_SEC:
      if (_config) {
        const_cast<ConfigData*>(_config)->delaySeconds = (int)value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "Delay seconds set to %d", (int)value);
      }
      break;

    case CMD_SET_MAX_ON_TIME:
      if (_config) {
        const_cast<ConfigData*>(_config)->maxOnTime = (uint32_t)value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "Max on time set to %lu", (uint32_t)value);
      }
      break;

    case CMD_SET_BOOT_STATE:
      if (_config) {
        const_cast<ConfigData*>(_config)->bootState = (bool)value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "Boot state set to %s", (bool)value ? "ON" : "OFF");
      }
      break;
#endif

#if DEVICE_TYPE == 1
    case CMD_SET_SPEED:
      set_speed((int)value);
      break;

    case CMD_SET_LOW_TEMP:
      if (_config) {
        const_cast<ConfigData*>(_config)->lowTemp = value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "Low temp set to %.1f", value);
      }
      break;

    case CMD_SET_HIGH_TEMP:
      if (_config) {
        const_cast<ConfigData*>(_config)->highTemp = value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "High temp set to %.1f", value);
      }
      break;

    case CMD_SET_LOW_HUM:
      if (_config) {
        const_cast<ConfigData*>(_config)->lowHum = value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "Low hum set to %.1f", value);
      }
      break;

    case CMD_SET_HIGH_HUM:
      if (_config) {
        const_cast<ConfigData*>(_config)->highHum = value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "High hum set to %.1f", value);
      }
      break;

    case CMD_SET_SENSOR_CONTROL_MODE:
      if (_config) {
        const_cast<ConfigData*>(_config)->sensorControlMode = (bool)value;
        need_save = true;
        // Если включаем AUTO — выходим из ручного режима
        if ((bool)value) {
          _state.manual_mode = false;
        }
        XLOG_INFO(CAT_DEVICE, "Sensor control mode: %s",
                  (bool)value ? "AUTO" : "MANUAL");
      }
      break;

    case CMD_SET_ADAPTIVE_MODE:
      if (_config) {
        const_cast<ConfigData*>(_config)->adaptiveMode = (bool)value;
        _state.adaptive_mode_active = (bool)value;
        need_save = true;
        XLOG_INFO(CAT_DEVICE, "Adaptive mode: %s", (bool)value ? "ON" : "OFF");
      }
      break;
#endif

    default:
      XLOG_WARN(CAT_DEVICE, "Unknown command: %d", type);
      break;
  }

  if (need_save) {
    notify_change(true);
  }
}

void DeviceController::set_state(bool on) {
  if (_state.is_on == on)
    return;

  _state.is_on = on;
  _state.manual_mode = true;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (_actuator) {
    _actuator->set(on, true);
  }
#endif

  if (!on) {
    // При выключении сбрасываем скорость
#if DEVICE_TYPE == 1
    if (_config) {
      _state.speed = _config->speedPercent;
    }
#endif
  }

  notify_change(false);
  XLOG_INFO(CAT_DEVICE, "State set to %s (manual mode)", on ? "ON" : "OFF");
}

void DeviceController::set_speed(int percent) {
#if DEVICE_TYPE == 1
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  if (_state.speed == percent)
    return;

  _state.speed = percent;
  _state.manual_mode = true;

  if (_actuator) {
    _actuator->setSpeed(percent, true);
  }

  notify_change(false);
  XLOG_INFO(CAT_DEVICE, "Speed set to %d%% (manual mode)", percent);
#else
  // TYPE 2 и TYPE 3 не имеют скорости
  (void)percent;
#endif
}

void DeviceController::set_manual_mode(bool enabled) {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (_state.manual_mode == enabled)
    return;

  _state.manual_mode = enabled;

  if (!enabled && _config) {
    XLOG_INFO(CAT_DEVICE, "Manual mode OFF, returning to AUTO");
  } else if (enabled) {
    XLOG_INFO(CAT_DEVICE, "Manual mode ON");
  }

  notify_change(false);
#else
  // TYPE 2 не имеет ручного режима
  (void)enabled;
#endif
}

const operational_state_t* DeviceController::get_state() const {
  return &_state;
}

void DeviceController::set_state_callback(state_callback_t callback) {
  _callback = callback;
}

void DeviceController::apply_state() {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (!_actuator)
    return;

  bool current_on = _actuator->getState();
  if (current_on != _state.is_on) {
    _actuator->set(_state.is_on, !_state.manual_mode);
  }

#if DEVICE_TYPE == 1
  int current_speed = _actuator->getSpeed();
  if (current_speed != _state.speed) {
    _actuator->setSpeed(_state.speed, !_state.manual_mode);
  }
#endif
#endif
}

void DeviceController::notify_change(bool need_save) {
  if (_callback) {
    _callback(&_state, need_save);
  }
}