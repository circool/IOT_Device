/**
 * @file fan_actuator.cpp
 * @brief Управление вентилятором с ШИМ
 * @details Поддерживает ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2
 */

#include "fan_actuator.h"
#include "logger.h"
#include "settings.h"

#if DEVICE_TYPE == 1

/**
 * @brief Определяем, какой API LEDC использовать
 *
 * Старый API (ESP-IDF 4.x):   ledcSetup(), ledcAttachPin(), ledcWrite(),
 * ledcDetachPin() Новый API (ESP-IDF 5.0+):   ledcAttach(), ledcWrite(),
 * ledcDetach()
 *
 * ESP32-C6 и ESP32-H2 используют новый API.
 * ESP32-C3 может использовать оба (зависит от версии Arduino Core).
 */
#if defined(ESP32)

// ESP32-C6 и ESP32-H2 — всегда новый API
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
#define USE_LEDC_NEW_API 1

// ESP32-C3 — проверяем наличие ledcAttach (признак нового API)
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#ifdef ledcAttach
#define USE_LEDC_NEW_API 1
#else
#define USE_LEDC_NEW_API 0
#endif

// Остальные ESP32 (S2, S3, обычный ESP32) — старый API
#else
#define USE_LEDC_NEW_API 0
#endif

#endif  // defined(ESP32)

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

  // ================================================================
  // ИНИЦИАЛИЗАЦИЯ ШИМ В ЗАВИСИМОСТИ ОТ ПЛАТФОРМЫ
  // ================================================================
#if defined(ESP32)
#if USE_LEDC_NEW_API == 1
  // API (ESP32-C6, ESP32-H2) 
  ledcAttach(_pin, PWM_FREQUENCY, PWM_RESOLUTION);
  XLOG_DEBUG(CAT_FAN, "LEDC init (new API): pin=%d, freq=%d, res=%d", _pin,
             PWM_FREQUENCY, PWM_RESOLUTION);
#else
  // API (ESP32, ESP32-S2, S3, C3) 
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
  XLOG_DEBUG(CAT_FAN, "LEDC init (old API): pin=%d, channel=0, freq=%d, res=%d",
             _pin, PWM_FREQUENCY, PWM_RESOLUTION);
#endif
#elif defined(ESP8266)
  analogWriteFreq(PWM_FREQUENCY);
  analogWriteRange(255);
  XLOG_DEBUG(CAT_FAN, "analogWrite init: freq=%d, range=255", PWM_FREQUENCY);
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

// ============================================================================
// СТАТИЧЕСКИЕ КОЛБЭКИ
// ============================================================================

void FanActuator::onSetPhysicalCallback(void* context, bool on) {
  FanActuator* self = (FanActuator*)context;
  if (!self)
    return;

  if (on) {
    if (self->_currentSpeed < 100 && self->_currentSpeed > 0) {
      // Включаем на полную для стартового импульса
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

// ============================================================================
// PWM УПРАВЛЕНИЕ
// ============================================================================

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
    ledcWrite(_pin, pwmValue);
#else
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
  // ===== НОВЫЙ API: ledcAttach(pin, freq, resolution) =====
  ledcAttach(_pin, PWM_FREQUENCY, PWM_RESOLUTION);
  XLOG_DEBUG(CAT_FAN, "PWM enabled (new API): pin=%d", _pin);
#else
  // ===== СТАРЫЙ API: ledcSetup + ledcAttachPin =====
  ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(_pin, 0);
  XLOG_DEBUG(CAT_FAN, "PWM enabled (old API): pin=%d, channel=0", _pin);
#endif
#elif defined(ESP8266)
  // ESP8266 не требует отдельного включения PWM
#endif
  _pwmActive = true;
}

void FanActuator::disablePWM() {
  if (!_pwmActive)
    return;

#if defined(ESP32)
#if USE_LEDC_NEW_API == 1
  // ===== НОВЫЙ API: ledcDetach(pin) =====
  ledcDetach(_pin);
  XLOG_DEBUG(CAT_FAN, "PWM disabled (new API): pin=%d", _pin);
#else
  // ===== СТАРЫЙ API: ledcDetachPin(pin) =====
  ledcDetachPin(_pin);
  XLOG_DEBUG(CAT_FAN, "PWM disabled (old API): pin=%d", _pin);
#endif
#elif defined(ESP8266)
  // ESP8266: отключаем analogWrite
  analogWrite(_pin, 1024);
  delayMicroseconds(10);
  pinMode(_pin, OUTPUT);
  XLOG_DEBUG(CAT_FAN, "PWM disabled (ESP8266): pin=%d", _pin);
#endif
  _pwmActive = false;
}

#endif  // DEVICE_TYPE == 1