#include "fan_actuator.h"
#include "logger.h"

#ifdef ESP32
#include <esp32-hal-ledc.h>
#endif

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
  _adaptiveMode = g_configManager.getAdaptiveMode();
  _pwmActive = false;
  _adaptiveActive = false;
  _startingPulseActive = false;

#ifdef ESP32
  // Используем правильный API для всех ESP32
  // ledcSetup - настройка канала ШИМ
  // ledcAttachPin - привязка пина к каналу
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
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
        if (_adaptiveMode && sensor_isOk() &&
            g_configManager.getSensorControlMode()) {
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

  if (_adaptiveMode && getState() && g_configManager.getSensorControlMode() &&
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
      g_configManager.setAdaptiveMode(false);
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
    self->_currentSpeed = g_configManager.getSpeedPercent();
    LOG_INFO(CAT_FAN, "OFF - restored speed to %d%%", self->_currentSpeed);
  }
}

void FanActuator::onForceStopCallback(void* context) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;
  g_configManager.setSensorControlMode(false);
  g_configManager.setAdaptiveMode(false);
  self->_adaptiveMode = false;
  self->_adaptiveActive = false;
}

void FanActuator::onManualCommandCallback(void* context) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;

  if (g_configManager.getDelaySeconds() != 0) {
    g_configManager.setDelaySeconds(0);
    LOG_INFO(CAT_FAN, "Manual command - delaySeconds temporarily disabled");
  }

#if DEVICE_TYPE == 1
  if (g_configManager.getSensorControlMode()) {
    g_configManager.setSensorControlMode(false);
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
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
#elif defined(ESP8266)
  // Для ESP8266 не требуется отдельного включения
#endif
  _pwmActive = true;
}

void FanActuator::disablePWM() {
  if (!_pwmActive)
    return;
#ifdef ESP32
  ledcDetachPin(_pin);
#elif defined(ESP8266)
  analogWrite(_pin, 1024);
  delayMicroseconds(10);
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

  if (millis() - _lastAdaptiveCheck <
      g_configManager.getSensorInterval() * 1000UL)
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