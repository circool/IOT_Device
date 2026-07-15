#include "web_status_provider.h"
#include "fan_actuator.h"
#include "mqtt.h"
#include "sensor.h"
#include "switch_actuator.h"
#include "transport.h"
#include "wifi_manager.h"

// ============================================================================
// FanWebStatusProvider
// ============================================================================

FanWebStatusProvider::FanWebStatusProvider(FanActuator* fan,
                                           Sensor* sensor,
                                           MQTTManager* mqtt)
    : _fan(fan), _sensor(sensor), _mqtt(mqtt) {}

bool FanWebStatusProvider::isDeviceOn() const {
  return _fan ? _fan->getState() : false;
}

bool FanWebStatusProvider::isEmergencyStop() const {
  return _fan ? _fan->isEmergencyStop() : false;
}

bool FanWebStatusProvider::isDelayActive() const {
  return _fan ? _fan->isDelayActive() : false;
}

unsigned long FanWebStatusProvider::getDelayTimer() const {
  return _fan ? _fan->getDelayTimer() : 0;
}

unsigned long FanWebStatusProvider::getStartTime() const {
  return _fan ? _fan->getStartTime() : 0;
}

int FanWebStatusProvider::getSpeedPercent() const {
  return _fan ? _fan->getSpeed() : 0;
}

bool FanWebStatusProvider::isAdaptiveModeActive() const {
  return _fan ? _fan->getAdaptiveMode() : false;
}

bool FanWebStatusProvider::isSensorOk() const {
  return sensor_isOk();
}

float FanWebStatusProvider::getTemperature() const {
  return sensor_getTemperature();
}

float FanWebStatusProvider::getHumidity() const {
  return sensor_getHumidity();
}

const char* FanWebStatusProvider::getSensorError() const {
  return sensor_getError();
}

bool FanWebStatusProvider::isMqttConnected() const {
  if (_mqtt) {
    return _mqtt->isConnected();
  }
  return g_transport ? g_transport->isConnected()
                     : false;  
}

int FanWebStatusProvider::getWifiRssi() const {
  return wifi_get_rssi();
}

// ============================================================================
// SwitchWebStatusProvider
// ============================================================================

SwitchWebStatusProvider::SwitchWebStatusProvider(SwitchActuator* sw,
                                                 MQTTManager* mqtt)
    : _switch(sw), _mqtt(mqtt) {}

bool SwitchWebStatusProvider::isDeviceOn() const {
  return _switch ? _switch->getState() : false;
}

bool SwitchWebStatusProvider::isEmergencyStop() const {
  return _switch ? _switch->isEmergencyStop() : false;
}

bool SwitchWebStatusProvider::isDelayActive() const {
  return _switch ? _switch->isDelayActive() : false;
}

unsigned long SwitchWebStatusProvider::getDelayTimer() const {
  return _switch ? _switch->getDelayTimer() : 0;
}

unsigned long SwitchWebStatusProvider::getStartTime() const {
  return _switch ? _switch->getStartTime() : 0;
}

bool SwitchWebStatusProvider::isMqttConnected() const {
  return _mqtt ? _mqtt->isConnected() : false;
}

int SwitchWebStatusProvider::getWifiRssi() const {
  return wifi_get_rssi();
}