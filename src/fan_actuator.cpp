#include "fan_actuator.h"
#include "logger.h"

#if DEVICE_TYPE == 1

// Определяем, какой API использовать для LEDC
#if defined(ESP32)
// Пытаемся определить по макросам платформы
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
// ESP32-C6 или H2 с новым API (ESP-IDF 5.0+)
#define USE_LEDC_NEW_API 1
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
// Для ESP32-C3 проверяем, есть ли ledcAttach
#ifdef ledcAttach
#define USE_LEDC_NEW_API 1
#else
#define USE_LEDC_NEW_API 0
#endif
#else
// Остальные ESP32
#define USE_LEDC_NEW_API 0
#endif
#endif

static int percentToPWMValue(int percent) {
  if (percent <= 0)
    return 0;
  if (percent >= 100)
    return 255;
  return map(percent, 0, 100, 0, 255);
}

FanActuator::FanActuator()
    : _pin(0),
      _relayOnLevel(LOW),
      _currentSpeed(0),
      _adaptiveMode(false),
      _pwmActive(false),
      _delaySeconds(0),
      _maxOnTime(0),
      _startingPulseActive(false),
      _startingPulseStart(0) {
  _base.onSetPhysicalCallback = FanActuator::onSetPhysicalCallback;
  _base.onForceStopCallback = FanActuator::onForceStopCallback;
  _base.onManualCommandCallback = FanActuator::onManualCommandCallback;
  _base.callbackContext = this;
}

void FanActuator::init(uint8_t pin,
                       uint8_t relayOnLevel,
                       bool bootState,
                       uint16_t defaultSpeed,
                       bool adaptiveMode,
                       int delaySeconds,
                       uint32_t maxOnTime) {
  _pin = pin;
  _relayOnLevel = relayOnLevel;
  _currentSpeed = defaultSpeed;
  _adaptiveMode = adaptiveMode;
  _delaySeconds = delaySeconds;
  _maxOnTime = maxOnTime;
  _pwmActive = false;
  _startingPulseActive = false;

#if defined(ESP32)
#if USE_LEDC_NEW_API == 1
// Новый API для ESP32-C6, H2 (ESP-IDF 5.0+)
// В некоторых версиях Arduino Core для C6 используется ledcAttach
#ifdef ledcAttach
  ledcAttach(_pin, PWM_FREQUENCY, PWM_RESOLUTION);
#else
  // Fallback на старый API
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
#endif
#else
  // Старый API для ESP32, ESP32-S2, ESP32-S3, ESP32-C3
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
#endif
#elif defined(ESP8266)
  analogWriteFreq(PWM_FREQUENCY);
  analogWriteRange(255);
#endif

  _base.init(pin, relayOnLevel, bootState, delaySeconds, maxOnTime);
}

void FanActuator::set(bool on, bool manual) {
  _base.set(on, manual);
}

bool FanActuator::getState() const {
  return _base.getState();
}

void FanActuator::update() {
  // Стартовый импульс
  if (_startingPulseActive) {
    if (millis() - _startingPulseStart >= PWM_STARTING) {
      if (_currentSpeed <= 0) {
        _base.set(false, false);
        applySpeed(0);
        _startingPulseActive = false;
        XLOG_DEBUG(CAT_FAN, "Start pulse done, speed 0% -> OFF");
      } else {
        applySpeed(_currentSpeed);
        _startingPulseActive = false;
        XLOG_DEBUG(CAT_FAN, "Start pulse done, speed=%d%%", _currentSpeed);
      }
    }
    return;
  }

  // Передаём текущие настройки в базовый класс
  _base.update(_delaySeconds, _maxOnTime);
}

void FanActuator::setSpeed(int percent, bool manual) {
  (void)manual;

  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  _currentSpeed = percent;

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

void FanActuator::updateConfig(bool adaptiveMode,
                               int delaySeconds,
                               uint32_t maxOnTime) {
  _adaptiveMode = adaptiveMode;
  _delaySeconds = delaySeconds;
  _maxOnTime = maxOnTime;
}

// Статические колбэки
void FanActuator::onSetPhysicalCallback(void* context, bool on) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;

  if (on) {
    if (self->_currentSpeed < 100 && self->_currentSpeed > 0) {
      self->applySpeed(100);
      self->_startingPulseActive = true;
      self->_startingPulseStart = millis();
    } else if (self->_currentSpeed >= 100) {
      self->disablePWM();
      digitalWrite(self->_pin, self->_relayOnLevel);
    } else {
      self->applySpeed(0);
    }
  } else {
    self->applySpeed(0);
    self->_startingPulseActive = false;
    XLOG_INFO(CAT_FAN, "OFF");
  }
}

void FanActuator::onForceStopCallback(void* context) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;
  XLOG_INFO(CAT_FAN, "Force stop triggered");
}

void FanActuator::onManualCommandCallback(void* context) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;
  XLOG_INFO(CAT_FAN, "Manual command received");
}

// Приватные методы PWM
void FanActuator::applySpeed(int percent) {
  if (percent <= 0) {
    disablePWM();
    digitalWrite(_pin, !_relayOnLevel);
  } else if (percent >= 100) {
    disablePWM();
    digitalWrite(_pin, _relayOnLevel);
  } else {
    int pwmValue = percentToPWMValue(percent);
#if RELAY_ON_LEVEL == LOW
    pwmValue = 255 - pwmValue;
#endif
    enablePWM();
#if defined(ESP32)
#if USE_LEDC_NEW_API == 1
// Новый API
#ifdef ledcWrite
    ledcWrite(_pin, pwmValue);
#else
    ledcWrite(0, pwmValue);
#endif
#else
    // Старый API
    ledcWrite(0, pwmValue);
#endif
#elif defined(ESP8266)
    analogWrite(_pin, pwmValue);
#endif
  }
}

void FanActuator::enablePWM() {
  if (_pwmActive)
    return;
#if defined(ESP32)
#if USE_LEDC_NEW_API == 1
// Новый API
#ifdef ledcAttach
  ledcAttach(_pin, PWM_FREQUENCY, PWM_RESOLUTION);
#else
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
#endif
#else
  // Старый API
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
#endif
#elif defined(ESP8266)
  // Для ESP8266 не требуется отдельного включения
#endif
  _pwmActive = true;
}

void FanActuator::disablePWM() {
  if (!_pwmActive)
    return;
#if defined(ESP32)
#if USE_LEDC_NEW_API == 1
// Новый API
#ifdef ledcDetach
  ledcDetach(_pin);
#else
  ledcDetachPin(_pin);
#endif
#else
  // Старый API
  ledcDetachPin(_pin);
#endif
#elif defined(ESP8266)
  analogWrite(_pin, 1024);
  delayMicroseconds(10);
  pinMode(_pin, OUTPUT);
#endif
  _pwmActive = false;
}

#endif  // DEVICE_TYPE == 1