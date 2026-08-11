/**
 * @file device_controller_fan_actuator.cpp
 * @brief Реализация управления вентилятором с ШИМ
 */

#include "device_controller_fan_actuator.h"
#include "logger.h"
#include "settings.h"

#if DEVICE_TYPE == 1

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================

/**
 * @brief Преобразование процентов в значение ШИМ
 */
static int percentToPWMValue(int percent) {
    if (percent <= 0) return 0;
    if (percent >= 100) return 255;
    return map(percent, 0, 100, 0, 255);
}

// ============================================================
// КОНСТРУКТОР
// ============================================================

FanActuator::FanActuator()
    : _pin(0)
    , _relayOnLevel(LOW)
    , _currentSpeed(0)
    , _adaptiveMode(false)
    , _pwmActive(false)
    , _delaySeconds(0)
    , _maxOnTime(0)
    , _startingPulseActive(false)
    , _startingPulseStart(0) {
    
    _base.onSetPhysicalCallback = FanActuator::onSetPhysicalCallback;
    _base.onForceStopCallback = FanActuator::onForceStopCallback;
    _base.onManualCommandCallback = FanActuator::onManualCommandCallback;
    _base.callbackContext = this;
}

// ============================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ============================================================

void FanActuator::init(uint8_t pin, uint8_t relayOnLevel, bool bootState,
                       uint16_t defaultSpeed, bool adaptiveMode,
                       int delaySeconds, uint32_t maxOnTime) {
    _pin = pin;
    _relayOnLevel = relayOnLevel;
    _currentSpeed = defaultSpeed;
    _adaptiveMode = adaptiveMode;
    _delaySeconds = delaySeconds;
    _maxOnTime = maxOnTime;
    _pwmActive = false;
    _startingPulseActive = false;

#if defined(ESP32)
    // ESP32: настройка LEDC
    ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(_pin, 0);
    XLOG_DEBUG(CAT_FAN, "LEDC init: freq=%d, res=%d", PWM_FREQUENCY, PWM_RESOLUTION);
#elif defined(ESP8266)
    // ESP8266: настройка analogWrite
    analogWriteFreq(PWM_FREQUENCY);
    analogWriteRange(255);
    XLOG_DEBUG(CAT_FAN, "analogWrite init: freq=%d", PWM_FREQUENCY);
#endif

    _base.init(pin, relayOnLevel, bootState, delaySeconds, maxOnTime);
    XLOG_INFO(CAT_FAN, "Init: pin=%d, defaultSpeed=%d%%, adaptive=%s",
              pin, defaultSpeed, adaptiveMode ? "ON" : "OFF");
}

void FanActuator::update() {
  

  // ===== 1. СТАРТОВЫЙ ИМПУЛЬС =====
  if (_startingPulseActive) {
    if (millis() - _startingPulseStart >= PWM_STARTING) {
      if (_currentSpeed <= 0) {
        _base.set(false, false);
        applySpeed(0);
        _startingPulseActive = false;
        XLOG_DEBUG(CAT_FAN, "Start pulse done, speed 0%% -> OFF");
      } else {
        applySpeed(_currentSpeed);
        _startingPulseActive = false;
        XLOG_DEBUG(CAT_FAN, "Start pulse done, speed=%d%%", _currentSpeed);
      }
    }
    return;
  }

    // ===== 2. БАЗОВАЯ ОБРАБОТКА =====
    _base.update(_delaySeconds, _maxOnTime);
}

void FanActuator::set(bool on, bool manual) {
    _base.set(on, manual);
    if (on) {
      XLOG_INFO(CAT_ACTUATOR, "ON, Speed=%d%%", _currentSpeed);
    } else {
      XLOG_INFO(CAT_ACTUATOR, "OFF, Speed=%d%%", _currentSpeed);
    }
}

bool FanActuator::getState() const {
    return _base.getState();
}

void FanActuator::setSpeed(int percent, bool manual) {
    (void)manual;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    if (_currentSpeed == percent) return;

    _currentSpeed = percent;

    // Если актуатор включён и не идёт стартовый импульс — применяем скорость
    if (getState() && !_startingPulseActive) {
        applySpeed(percent);
    }

    XLOG_DEBUG(CAT_FAN, "Speed set to %d%%", percent);
}

int FanActuator::getSpeed() const {
    return _currentSpeed;
}

void FanActuator::setAdaptiveMode(bool enabled) {
    _adaptiveMode = enabled;
    XLOG_DEBUG(CAT_FAN, "Adaptive mode: %s", enabled ? "ON" : "OFF");
}

bool FanActuator::getAdaptiveMode() const {
    return _adaptiveMode;
}

void FanActuator::updateConfig(bool adaptiveMode, int delaySeconds, uint32_t maxOnTime) {
    _adaptiveMode = adaptiveMode;
    _delaySeconds = delaySeconds;
    _maxOnTime = maxOnTime;
    XLOG_DEBUG(CAT_FAN, "Config updated: adaptive=%s, delay=%d, maxOn=%lu",
               adaptiveMode ? "ON" : "OFF", delaySeconds, maxOnTime);
}

// ============================================================
// СТАТИЧЕСКИЕ КОЛБЭКИ
// ============================================================

void FanActuator::onSetPhysicalCallback(void* context, bool on) {
    FanActuator* self = (FanActuator*)context;
    if (!self) return;

    if (on) {
        // ===== ВКЛЮЧЕНИЕ =====
        if (self->_currentSpeed < 100 && self->_currentSpeed > 0) {
            // Стартовый импульс на полной мощности
            self->applySpeed(100);
            self->_startingPulseActive = true;
            self->_startingPulseStart = millis();
            XLOG_DEBUG(CAT_FAN, "Start pulse: 100%% for %d ms", PWM_STARTING);
        } else if (self->_currentSpeed >= 100) {
            self->disablePWM();
            digitalWrite(self->_pin, self->_relayOnLevel);
        } else {
            self->applySpeed(0);
        }
    } else {
        // ===== ВЫКЛЮЧЕНИЕ =====
        self->applySpeed(0);
        self->_startingPulseActive = false;
        XLOG_INFO(CAT_FAN, "OFF");
    }
}

void FanActuator::onForceStopCallback(void* context) {
    FanActuator* self = (FanActuator*)context;
    if (!self) return;
    XLOG_WARN(CAT_FAN, "Force stop triggered");
}

void FanActuator::onManualCommandCallback(void* context) {
    FanActuator* self = (FanActuator*)context;
    if (!self) return;
    XLOG_DEBUG(CAT_FAN, "Manual command received");
}

// ============================================================
// PWM УПРАВЛЕНИЕ
// ============================================================

void FanActuator::applySpeed(int percent) {
  XLOG_DEBUG(CAT_FAN, "applySpeed: percent=%d, pwmActive=%d", percent,
             _pwmActive);

  if (percent <= 0) {
    disablePWM();
    digitalWrite(_pin, !_relayOnLevel);
    XLOG_DEBUG(CAT_FAN, "applySpeed: OFF (percent<=0)");
  } else if (percent >= 100) {
    disablePWM();
    digitalWrite(_pin, _relayOnLevel);
    XLOG_DEBUG(CAT_FAN, "applySpeed: ON 100%%");
  } else {
    int pwmValue = percentToPWMValue(percent);
#if RELAY_ON_LEVEL == LOW
    pwmValue = 255 - pwmValue;
#endif
    enablePWM();
#if defined(ESP32)
    ledcWrite(0, pwmValue);
#elif defined(ESP8266)
    analogWrite(_pin, pwmValue);
#endif
    XLOG_DEBUG(CAT_FAN, "applySpeed: PWM %d%% -> value=%d", percent, pwmValue);
  }
}

void FanActuator::enablePWM() {
    if (_pwmActive) return;

#if defined(ESP32)
    ledcAttachPin(_pin, 0);
#elif defined(ESP8266)
    // ESP8266 не требует отдельного включения PWM
#endif
    _pwmActive = true;
    XLOG_DEBUG(CAT_FAN, "PWM enabled");
}

void FanActuator::disablePWM() {
    if (!_pwmActive) return;

#if defined(ESP32)
    ledcDetachPin(_pin);
#elif defined(ESP8266)
    analogWrite(_pin, 1024);
    delayMicroseconds(10);
    pinMode(_pin, OUTPUT);
#endif
    _pwmActive = false;
    XLOG_DEBUG(CAT_FAN, "PWM disabled");
}

#endif // DEVICE_TYPE == 1