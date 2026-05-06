#include "fan.h"
#include "config.h"
#include "sensor.h"

bool manualOverride = false;
bool fanShouldBeOn = false;
unsigned long fanOnStartTime = 0;
unsigned long delayOnTimer = 0;
bool delayOnActive = false;
unsigned long lastRelayCheck = 0;
bool lastRelayError = false;

void fan_init() {
  #ifdef ESP32
    ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(SWITCH_PIN, 0);
  #elif defined(ESP8266)
    pinMode(SWITCH_PIN, OUTPUT);
    analogWriteFreq(PWM_FREQUENCY);
    analogWriteRange((1 << PWM_RESOLUTION) - 1);
  #endif
  
  bool currentPinState = false;
  #ifdef ESP8266
    currentPinState = (digitalRead(SWITCH_PIN) == HIGH);
  #endif
  
  if (config.setSwitchOff) {
    fanShouldBeOn = false;
    fan_applyPwmOrDigital(false);
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] SET_SWITCH_OFF enabled - forcing OFF");
    #endif
    delay(50);
    #ifdef ESP8266
      if (digitalRead(SWITCH_PIN) == HIGH) {
        #ifdef DEBUG_ENABLE
          Serial.println("[FAN] WARNING: Relay did not turn OFF!");
        #endif
        lastRelayError = true;
      }
    #endif
  } else {
    fanShouldBeOn = currentPinState;
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] SET_SWITCH_OFF disabled - synced with pin state: %s\n", 
                    fanShouldBeOn ? "ON" : "OFF");
    #endif
  }
  
  manualOverride = false;
  delayOnActive = false;
  delayOnTimer = 0;
  fanOnStartTime = 0;
  lastRelayCheck = 0;
  lastRelayError = false;
}

void fan_resetDelayTimer() {
  if (config.delaySeconds > 0) {
    delayOnActive = true;
    delayOnTimer = millis() + config.delaySeconds * 1000UL;
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] Delay ON timer started: %d seconds\n", config.delaySeconds);
    #endif
  } else {
    delayOnActive = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Delay ON timer disabled (delaySeconds = 0)");
    #endif
  }
}

bool fan_cancelDelayTimer() {
  if (delayOnActive) {
    delayOnActive = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Delay ON timer cancelled");
    #endif
    return true;
  }
  return false;
}

void fan_applyPwmOrDigital(bool on) {
  if (on) {
    if (config.slowModeEnabled) {
      #ifdef ESP32
        ledcWrite(0, config.slowModeDuty);
      #elif defined(ESP8266)
        analogWrite(SWITCH_PIN, config.slowModeDuty);
      #endif
    } else {
      #ifdef ESP32
        ledcWrite(0, (1 << PWM_RESOLUTION) - 1);
      #elif defined(ESP8266)
        digitalWrite(SWITCH_PIN, HIGH);
      #endif
    }
  } else {
    #ifdef ESP32
      ledcWrite(0, 0);
    #elif defined(ESP8266)
      digitalWrite(SWITCH_PIN, LOW);
    #endif
  }
  
  delay(10);
  #ifdef ESP8266
    bool realState = (digitalRead(SWITCH_PIN) == HIGH);
    if (realState != on) {
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] WARNING: Relay state mismatch! Expected: %s, Real: %s\n", 
                      on ? "ON" : "OFF", realState ? "ON" : "OFF");
      #endif
      lastRelayError = true;
    } else {
      lastRelayError = false;
    }
  #endif
}

void fan_set(bool on) {
  if (fanShouldBeOn != on) {
    fanShouldBeOn = on;
    if (fanShouldBeOn) {
      fanOnStartTime = millis();
    } else {
      fanOnStartTime = 0;
    }
    fan_applyPwmOrDigital(fanShouldBeOn);
    #ifdef DEBUG_ENABLE
      Serial.println(fanShouldBeOn ? "[FAN] Fan turned ON" : "[FAN] Fan turned OFF");
    #endif
  }
}

bool fan_getState() {
  return fanShouldBeOn;
}

bool fan_getRealState() {
  #ifdef ESP8266
    return (digitalRead(SWITCH_PIN) == HIGH);
  #elif defined(ESP32)
    return fanShouldBeOn;
  #endif
}

bool fan_getRealStateForMqtt() {
  bool realState = fan_getRealState();
  fan_checkRelayConsistency();
  return realState;
}

void fan_toggle() {
  fan_set(!fanShouldBeOn);
}

void fan_checkMaxOnTime() {
  if (fanShouldBeOn && fanOnStartTime != 0 && 
      (millis() - fanOnStartTime) > config.maxOnTime * 1000UL) {
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Max on time exceeded, forcing OFF");
    #endif
    fan_set(false);
  }
}

void fan_checkRelayConsistency() {
  if (millis() - lastRelayCheck < 10000) return;
  lastRelayCheck = millis();
  
  bool realState = fan_getRealState();
  
  if (realState != fanShouldBeOn) {
    if (!lastRelayError) {
      lastRelayError = true;
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] RELAY ERROR! Expected: %s, Real: %s\n", 
                      fanShouldBeOn ? "ON" : "OFF", realState ? "ON" : "OFF");
      #endif
    }
  } else {
    if (lastRelayError) {
      lastRelayError = false;
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Relay error cleared");
      #endif
    }
  }
}

void fan_checkDelayTimer() {
  if (delayOnActive && millis() >= delayOnTimer) {
    delayOnActive = false;
    manualOverride = true;      // Переводим в ручной режим
    fan_set(true);
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Delay ON timer finished - turning ON (manual mode set)");
    #endif
  }
}

void fan_update() {
  fan_checkDelayTimer();
  
  bool newShouldBeOn = fanShouldBeOn;

  if (!manualOverride && config.automaticMode && sensor_isOk()) {
    bool tempExceed = (currentTemp >= config.highTemp);
    bool humExceed = (currentHum >= config.highHum);
    bool tempLow = (currentTemp <= config.lowTemp);
    bool humLow = (currentHum <= config.lowHum);
    
    if (tempExceed || humExceed) {
      newShouldBeOn = true;
      if (fan_cancelDelayTimer()) {
        #ifdef DEBUG_ENABLE
          Serial.println("[FAN] Delay timer cancelled due to conditions exceeded");
        #endif
      }
      if (newShouldBeOn != fanShouldBeOn) {
        #ifdef DEBUG_ENABLE
          Serial.println("[FAN] Conditions exceeded - turning ON");
        #endif
      }
    } else if (tempLow && humLow) {
      newShouldBeOn = false;
      if (newShouldBeOn != fanShouldBeOn) {
        #ifdef DEBUG_ENABLE
          Serial.println("[FAN] Conditions normal - turning OFF");
        #endif
      }
    } else {
      newShouldBeOn = fanShouldBeOn;
    }
  }

  if (newShouldBeOn != fanShouldBeOn) {
    fan_set(newShouldBeOn);
  }
  
  fan_checkMaxOnTime();
  fan_checkRelayConsistency();
}