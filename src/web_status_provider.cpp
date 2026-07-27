#include "web_status_provider.h"
#include "fan_actuator.h"
#include "sensor.h"
#include "switch_actuator.h"
#include "transport.h"
#include "wifi_manager.h"

#if FEATURE_MQTT_ENABLED == 1
#include "mqtt.h"
#endif

// ============================================================================
// TYPE 1 — FanWebStatusProvider
// ============================================================================

#if DEVICE_TYPE == 1

#if FEATURE_MQTT_ENABLED == 1
FanWebStatusProvider::FanWebStatusProvider(FanActuator* fan, MQTTManager* mqtt)
    : _fan(fan), _mqtt(mqtt) {}
#else
FanWebStatusProvider::FanWebStatusProvider(FanActuator* fan) : _fan(fan) {}
#endif

// ----- Основная информация -----
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

// ----- Вентилятор -----
int FanWebStatusProvider::getSpeedPercent() const {
  return _fan ? _fan->getSpeed() : 0;
}

bool FanWebStatusProvider::isAdaptiveModeActive() const {
  return _fan ? _fan->getAdaptiveMode() : false;
}

// ----- Датчик -----
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

// ----- Подключения -----
bool FanWebStatusProvider::isMqttConnected() const {
#if FEATURE_MQTT_ENABLED == 1
  if (_mqtt) {
    return _mqtt->isConnected();
  }
#endif
  return g_transport ? g_transport->isConnected() : false;
}

int FanWebStatusProvider::getWifiRssi() const {
  return wifi_get_rssi();
}

// ----- Параметры из Config -----
float FanWebStatusProvider::getLowTemp() const {
  return g_configManager.getLowTemp();
}

float FanWebStatusProvider::getHighTemp() const {
  return g_configManager.getHighTemp();
}

float FanWebStatusProvider::getLowHum() const {
  return g_configManager.getLowHum();
}

float FanWebStatusProvider::getHighHum() const {
  return g_configManager.getHighHum();
}

int FanWebStatusProvider::getDelaySeconds() const {
  return g_configManager.getDelaySeconds();
}

uint32_t FanWebStatusProvider::getMaxOnTime() const {
  return g_configManager.getMaxOnTime();
}

int FanWebStatusProvider::getSensorInterval() const {
  return g_configManager.getSensorInterval();
}

bool FanWebStatusProvider::isSensorControlMode() const {
  return g_configManager.getSensorControlMode();
}

// ============================================================================
// TYPE 2 — SensorWebStatusProvider
// ============================================================================

#elif DEVICE_TYPE == 2

#if FEATURE_MQTT_ENABLED == 1
SensorWebStatusProvider::SensorWebStatusProvider(MQTTManager* mqtt)
    : _mqtt(mqtt) {}
#else
SensorWebStatusProvider::SensorWebStatusProvider() {}
#endif

// ----- Датчик -----
bool SensorWebStatusProvider::isSensorOk() const {
  return sensor_isOk();
}

float SensorWebStatusProvider::getTemperature() const {
  return sensor_getTemperature();
}

float SensorWebStatusProvider::getHumidity() const {
  return sensor_getHumidity();
}

const char* SensorWebStatusProvider::getSensorError() const {
  return sensor_getError();
}

// ----- Подключения -----
bool SensorWebStatusProvider::isMqttConnected() const {
#if FEATURE_MQTT_ENABLED == 1
  if (_mqtt) {
    return _mqtt->isConnected();
  }
#endif
  return g_transport ? g_transport->isConnected() : false;
}

int SensorWebStatusProvider::getWifiRssi() const {
  return wifi_get_rssi();
}

// ----- Параметры из Config -----
int SensorWebStatusProvider::getSensorInterval() const {
  return g_configManager.getSensorInterval();
}

// ============================================================================
// TYPE 3 — SwitchWebStatusProvider
// ============================================================================

#elif DEVICE_TYPE == 3

#if FEATURE_MQTT_ENABLED == 1
SwitchWebStatusProvider::SwitchWebStatusProvider(SwitchActuator* sw,
                                                 MQTTManager* mqtt)
    : _switch(sw), _mqtt(mqtt) {}
#else
SwitchWebStatusProvider::SwitchWebStatusProvider(SwitchActuator* sw)
    : _switch(sw) {}
#endif

// ----- Актуатор -----
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

// ----- Подключения -----
bool SwitchWebStatusProvider::isMqttConnected() const {
#if FEATURE_MQTT_ENABLED == 1
  if (_mqtt) {
    return _mqtt->isConnected();
  }
#endif
  return g_transport ? g_transport->isConnected() : false;
}

int SwitchWebStatusProvider::getWifiRssi() const {
  return wifi_get_rssi();
}

// ----- Параметры из Config -----
int SwitchWebStatusProvider::getDelaySeconds() const {
  return g_configManager.getDelaySeconds();
}

uint32_t SwitchWebStatusProvider::getMaxOnTime() const {
  return g_configManager.getMaxOnTime();
}

#endif  // DEVICE_TYPE