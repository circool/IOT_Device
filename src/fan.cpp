#include "fan.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

#include "config.h"
#include "sensor.h"
#include "mqtt.h"

bool manualOverride = false;
bool fanOn = false;
unsigned long fanStartTime = 0;
unsigned long delayTimer = 0;
bool delayActive = false;

void fan_init() {
  pinMode(SWITCH_PIN, OUTPUT);
  
  #ifdef ESP32
    ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  #elif defined(ESP8266)
    analogWriteFreq(PWM_FREQUENCY);
    analogWriteRange(255);
  #endif
  
  bool currentPinState = (digitalRead(SWITCH_PIN) == HIGH);
  
  if (config.forceOffOnBoot) {
    fanOn = false;
    digitalWrite(SWITCH_PIN, LOW);
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Force OFF on boot - forcing OFF");
    #endif
  } else {
    fanOn = currentPinState;
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] Keep state on boot - synced with pin state: %s\n", 
                    fanOn ? "ON" : "OFF");
    #endif
  }
  
  manualOverride = false;
  delayActive = false;
  delayTimer = 0;
  fanStartTime = 0;
}

void fan_set(bool on) {
  if (fanOn == on) return;
  
  fanOn = on;
  
  if (fanOn) {
    fanStartTime = millis();
  } else {
    fanStartTime = 0;
  }
  
  if (on) {
    if (config.slowModeEnabled) {
      #ifdef ESP32
        ledcAttachPin(SWITCH_PIN, 0);
        ledcWrite(0, config.slowModeDuty);
      #elif defined(ESP8266)
        analogWrite(SWITCH_PIN, config.slowModeDuty);
      #endif
    } else {
      #ifdef ESP32
        ledcDetachPin(SWITCH_PIN);
        digitalWrite(SWITCH_PIN, HIGH);
      #elif defined(ESP8266)
        digitalWrite(SWITCH_PIN, HIGH);
      #endif
    }
  } else {
    #ifdef ESP32
      ledcDetachPin(SWITCH_PIN);
      digitalWrite(SWITCH_PIN, LOW);
    #elif defined(ESP8266)
      digitalWrite(SWITCH_PIN, LOW);
    #endif
  }
  
  #ifdef DEBUG_ENABLE
    delay(10);
    bool pinState = (digitalRead(SWITCH_PIN) == HIGH);
    if (!config.slowModeEnabled && pinState != on) {
      Serial.printf("[FAN] WARNING: Pin state mismatch! Expected: %s, Real: %s\n", 
                    on ? "ON" : "OFF", pinState ? "ON" : "OFF");
    }
    Serial.println(on ? "[FAN] Fan turned ON" : "[FAN] Fan turned OFF");
  #endif
}

bool fan_getState() {
  return fanOn;
}

bool fan_getRealState() {
  return fanOn;
}

void fan_setOverrideMode(bool override) {
  if (override) {
    manualOverride = true;
    delayActive = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Manual override mode ON");
    #endif
  } else {
    manualOverride = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Manual override mode OFF (auto mode)");
    #endif
  }
}

void fan_checkMaxOnTime() {
  if (fanOn && fanStartTime != 0 && 
      (millis() - fanStartTime) > config.maxOnTime * 1000UL) {
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Max on time exceeded, forcing OFF");
    #endif
    fan_set(false);
    if (config.delaySeconds > 0 && !manualOverride) {
      fan_delayTimer(true);
    }
  }
}

bool fan_delayTimer(bool start) {
  if (start) {
    if (config.delaySeconds > 0) {
      delayActive = true;
      delayTimer = millis() + config.delaySeconds * 1000UL;
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] Delay ON timer started: %d seconds\n", config.delaySeconds);
      #endif
      return false;
    }
    delayActive = false;
    return false;
  } else {
    if (delayActive && millis() >= delayTimer) {
      delayActive = false;
      fan_set(true);
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Delay ON timer finished - turning ON");
      #endif
      return true;
    }
    return false;
  }
}

void fan_update() {
  fan_delayTimer(false);
  
  if (manualOverride) {
    return;
  }
  
  bool timerOn = false;
  bool sensorOn = fanOn;
  
  // Таймер
  if (config.delaySeconds > 0) {
    if (!fanOn && !delayActive) {
      fan_delayTimer(true);
    }
    timerOn = !delayActive && (delayTimer > 0);
  }
  
  // Датчики (только TYPE 1)
  #if DEVICE_TYPE == 1
  if (sensor_isOk()) {
    bool tempExceed = (currentTemp >= config.highTemp);
    bool humExceed = (currentHum >= config.highHum);
    bool tempLow = (currentTemp <= config.lowTemp);
    bool humLow = (currentHum <= config.lowHum);
    
    if (tempExceed || humExceed) {
      sensorOn = true;
    } else if (tempLow && humLow) {
      sensorOn = false;
    }
  }
  #endif
  
  if (timerOn || sensorOn) {
    fan_set(true);
  } else {
    fan_set(false);
  }
  
  fan_checkMaxOnTime();
}

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3