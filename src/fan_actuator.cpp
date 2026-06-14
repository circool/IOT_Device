#include "fan_actuator.h"
#include "logger.h"

#if DEVICE_TYPE == 1
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
      _adaptiveActive(false),
      _baseTemp(0),
      _baseHum(0),
      _lastAdaptiveCheck(0),
      _startingPulseActive(false),
      _startingPulseStart(0),
      _rampUpActive(false),
      _lastRampUpTime(0) {
  _base.onSetPhysicalCallback = FanActuator::onSetPhysicalCallback;
  _base.onForceStopCallback = FanActuator::onForceStopCallback;
  _base.onManualCommandCallback = FanActuator::onManualCommandCallback;
  _base.callbackContext = this;
}

void FanActuator::init(uint8_t pin,
                       uint8_t relayOnLevel,
                       bool bootState,
                       uint16_t defaultSpeed) {
  _pin = pin;
  _relayOnLevel = relayOnLevel;
  _currentSpeed = defaultSpeed;
  _adaptiveMode = config_get()->adaptiveMode;
  _pwmActive = false;
  _adaptiveActive = false;
  _startingPulseActive = false;

#ifdef ESP32
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
#elif defined(ESP8266)
  analogWriteFreq(PWM_FREQUENCY);
  analogWriteRange(255);
#endif

  _base.init(pin, relayOnLevel, bootState);
}

void FanActuator::set(bool on, bool manual) {
  _base.set(on, manual);
}

bool FanActuator::getState() const {
  return _base.getState();
}

void FanActuator::update() {
  if (_startingPulseActive) {
    if (millis() - _startingPulseStart >= PWM_STARTING) {
      if (_currentSpeed <= 0) {
        _base.set(false, false);
        applySpeed(0);
        _startingPulseActive = false;
        LOG_DEBUG(CAT_FAN, "Start pulse done, speed 0% -> OFF");
      } else {
        applySpeed(_currentSpeed);
        _startingPulseActive = false;
        if (_adaptiveMode && sensor_isOk() && config_get()->sensorControlMode) {
          _adaptiveActive = true;
          _baseTemp = sensor_getTemperature();
          _baseHum = sensor_getHumidity();
          _lastAdaptiveCheck = millis();
        }
        LOG_DEBUG(CAT_FAN, "Start pulse done, speed=%d%%", _currentSpeed);
      }
    }
    return;
  }

  _base.update();

  if (_adaptiveMode && getState() && config_get()->sensorControlMode &&
      sensor_isOk()) {
    adaptiveUpdate();
  }
}

void FanActuator::setSpeed(int percent, bool manual) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  if (manual) {
    if (_adaptiveMode) {
      config_setAdaptiveMode(false);
      _adaptiveMode = false;
      _adaptiveActive = false;
    }
    if (getState()) {
      applySpeed(percent);
    }
  }

  _currentSpeed = percent;
  if (!manual && getState() && !_startingPulseActive) {
    applySpeed(percent);
  }

  LOG_DEBUG(CAT_FAN, "Speed set to %d%%", percent);
}

int FanActuator::getSpeed() const {
  return _currentSpeed;
}

void FanActuator::setAdaptiveMode(bool enabled) {
  _adaptiveMode = enabled;
  if (!enabled) {
    _adaptiveActive = false;
  } else if (getState() && sensor_isOk()) {
    _adaptiveActive = true;
    _baseTemp = sensor_getTemperature();
    _baseHum = sensor_getHumidity();
    _lastAdaptiveCheck = millis();
  }
}

bool FanActuator::getAdaptiveMode() const {
  return _adaptiveMode;
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
    self->_adaptiveActive = false;
    self->_startingPulseActive = false;
    // ВОССТАНОВЛЕНИЕ СКОРОСТИ ИЗ КОНФИГА
    self->_currentSpeed = config_get()->speedPercent;
    LOG_INFO(CAT_FAN, "OFF - restored speed to %d%%", self->_currentSpeed);
  }
}

void FanActuator::onForceStopCallback(void* context) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;
  config_setSensorControlMode(false);
  config_setAdaptiveMode(false);
  self->_adaptiveMode = false;
  self->_adaptiveActive = false;
}

void FanActuator::onManualCommandCallback(void* context) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;

  // Временно отключаем таймер до перезагрузки (для TYPE 1 и TYPE 3)
  if (config_get()->delaySeconds != 0) {
    config_setDelaySeconds(0);
    LOG_INFO(CAT_FAN, "Manual command - delaySeconds temporarily disabled");
  }

#if DEVICE_TYPE == 1
  // Переход в ручной режим при любой ручной команде
  if (config_get()->sensorControlMode) {
    config_setSensorControlMode(false);
    LOG_INFO(CAT_FAN, "Manual command - switching to MANUAL mode");
  }
#endif
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
#ifdef ESP32
    ledcWrite(0, pwmValue);
#elif defined(ESP8266)
    analogWrite(_pin, pwmValue);
#endif
  }
}

void FanActuator::enablePWM() {
  if (_pwmActive)
    return;
#ifdef ESP32
  ledcAttachPin(_pin, 0);
#endif
  _pwmActive = true;
}

void FanActuator::disablePWM() {
  if (!_pwmActive)
    return;
#ifdef ESP32
  ledcDetachPin(_pin);
#elif defined(ESP8266)
  // 1024 > default range (1023) — forces PWM hardware to release the pin
  // without this, analogWrite(pin,0) would keep PWM active at 0% duty cycle
  analogWrite(_pin, 1024);
  delayMicroseconds(10);  // Wait for PWM cycle to complete
  pinMode(_pin, OUTPUT);
#endif
  _pwmActive = false;
}

void FanActuator::adaptiveUpdate() {
  if (!sensor_isOk()) {
    if (_adaptiveActive) {
      _adaptiveActive = false;
      if (getState() && !_startingPulseActive) {
        setSpeed(100, false);
      }
    }
    return;
  }

  if (!_adaptiveActive) {
    _adaptiveActive = true;
    _baseTemp = sensor_getTemperature();
    _baseHum = sensor_getHumidity();
    _lastAdaptiveCheck = millis();
    return;
  }

  if (millis() - _lastAdaptiveCheck < config_get()->sensorInterval * 1000UL)
    return;
  _lastAdaptiveCheck = millis();

  float deltaTemp = sensor_getTemperature() - _baseTemp;
  float deltaHum = sensor_getHumidity() - _baseHum;
  int step = calculateAdaptiveStep(deltaTemp, deltaHum, sensor_getHumRate());

  if (deltaTemp > ADAPTIVE_EPSILON_TEMP || deltaHum > ADAPTIVE_EPSILON_HUM) {
    int newSpeed = _currentSpeed + step;
    if (newSpeed > 100)
      newSpeed = 100;
    if (newSpeed != _currentSpeed) {
      setSpeed(newSpeed, false);
      _baseTemp = sensor_getTemperature();
      _baseHum = sensor_getHumidity();
    }
  }
}

int FanActuator::calculateAdaptiveStep(float deltaTemp,
                                       float deltaHum,
                                       float humRate) {
  int step = ADAPTIVE_STEP_SIZE;
  if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 2 ||
      deltaHum > ADAPTIVE_EPSILON_HUM * 2)
    step *= 2;
  if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 3 ||
      deltaHum > ADAPTIVE_EPSILON_HUM * 3)
    step *= 3;
  float mult = 1.0 + (humRate / ADAPTIVE_SPEED_SENSITIVITY);
  mult = constrain(mult, 0.5, 3.0);
  step = step * mult;
  return constrain(step, 5, 60);
}
#endif