#include "fan_actuator.h"
#include "logger.h"

#ifdef ESP32
#include <esp32-hal-ledc.h>
#endif

static int percentToPWMValue(int percent) {
  if (percent <= 0)
    return 0;
  if (percent >= 100)
    return 255;
  return map(percent, 0, 100, 0, 255);
}

FanActuator::FanActuator()
    : ActuatorBase(),
      _currentSpeed(0),
      _pwmActive(false),
      _startingPulseActive(false),
      _startingPulseStart(0) {}

void FanActuator::init(uint8_t pin,
                       bool bootState,
                       uint16_t defaultSpeed,
                       uint32_t maxOnTime) {
  _currentSpeed = defaultSpeed;
  _pwmActive = false;
  _startingPulseActive = false;

#ifdef ESP32
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
#elif defined(ESP8266)
  analogWriteFreq(PWM_FREQUENCY);
  analogWriteRange(255);
#endif

  ActuatorBase::init(pin, bootState, maxOnTime);
  LOG_INFO(CAT_FAN,
           "Init: speed=%d%%, maxOnTime=%lu sec, active level=%s", _currentSpeed,
           maxOnTime, ACTIVE_LEVEL == LOW ? "LOW" : "HIGH");
}

void FanActuator::setSpeed(int percent, bool manual) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  if (manual) {
    LOG_DEBUG(CAT_FAN, "Manual speed change to %d%%", percent);
  }

  _currentSpeed = percent;

  if (_state && !_startingPulseActive) {
    applySpeed(_currentSpeed);
  }
}

int FanActuator::getSpeed() const {
  return _currentSpeed;
}

void FanActuator::update() {
  if (_startingPulseActive) {
    if (millis() - _startingPulseStart >= PWM_STARTING) {
      _startingPulseActive = false;
      applySpeed(_currentSpeed);
      LOG_DEBUG(CAT_FAN, "Start pulse done, speed=%d%%", _currentSpeed);
    }
    return;
  }

  ActuatorBase::update();
}

void FanActuator::onSetPhysical(bool on) {
  if (on) {
    if (_currentSpeed < 100 && _currentSpeed > 0) {
      _startingPulseActive = true;
      _startingPulseStart = millis();
      applySpeed(100);
    } else {
      applySpeed(_currentSpeed);
    }
  } else {
    applySpeed(0);
    _startingPulseActive = false;
  }
}

void FanActuator::applySpeed(int percent) {
  if (percent <= 0) {
    disablePWM();
    digitalWrite(_pin, !ACTIVE_LEVEL);
  } else if (percent >= 100) {
    disablePWM();
    digitalWrite(_pin, ACTIVE_LEVEL);
  } else {
    int pwmValue = percentToPWMValue(percent);
#if ACTIVE_LEVEL == LOW
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
  analogWrite(_pin, 1024);
  delayMicroseconds(10);
  pinMode(_pin, OUTPUT);
#endif
  _pwmActive = false;
}