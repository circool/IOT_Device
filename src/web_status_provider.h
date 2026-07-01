#ifndef WEB_STATUS_PROVIDER_H
#define WEB_STATUS_PROVIDER_H

#include <Arduino.h>
#include "config_manager.h"

/**
 * @brief Интерфейс для получения статуса устройства
 * Web слой использует этот интерфейс вместо прямых вызовов к компонентам
 */
class IWebStatusProvider {
 public:
  virtual ~IWebStatusProvider() = default;

  // ========== Основная информация ==========
  virtual bool isDeviceOn() const = 0;
  virtual bool isEmergencyStop() const = 0;
  virtual bool isDelayActive() const = 0;
  virtual unsigned long getDelayTimer() const = 0;
  virtual unsigned long getStartTime() const = 0;

  // ========== Информация о вентиляторе (только TYPE 1) ==========
  virtual int getSpeedPercent() const = 0;
  virtual bool isAdaptiveModeActive() const = 0;

  // ========== Информация о датчике (TYPE 1 и 2) ==========
  virtual bool isSensorOk() const = 0;
  virtual float getTemperature() const = 0;
  virtual float getHumidity() const = 0;
  virtual const char* getSensorError() const = 0;

  // ========== Информация о подключениях ==========
  virtual bool isMqttConnected() const = 0;
  virtual int getWifiRssi() const = 0;
};

/**
 * @brief Реализация интерфейса для TYPE 1 (вентилятор с датчиком)
 */
class FanWebStatusProvider : public IWebStatusProvider {
 public:
  FanWebStatusProvider(class FanActuator* fan,
                       class Sensor* sensor,
                       class MQTTManager* mqtt);

  bool isDeviceOn() const override;
  bool isEmergencyStop() const override;
  bool isDelayActive() const override;
  unsigned long getDelayTimer() const override;
  unsigned long getStartTime() const override;

  int getSpeedPercent() const override;
  bool isAdaptiveModeActive() const override;

  bool isSensorOk() const override;
  float getTemperature() const override;
  float getHumidity() const override;
  const char* getSensorError() const override;

  bool isMqttConnected() const override;
  int getWifiRssi() const override;

 private:
  class FanActuator* _fan;
  class Sensor* _sensor;
  class MQTTManager* _mqtt;
};

/**
 * @brief Реализация интерфейса для TYPE 3 (выключатель без датчика)
 */
class SwitchWebStatusProvider : public IWebStatusProvider {
 public:
  SwitchWebStatusProvider(class SwitchActuator* sw, class MQTTManager* mqtt);

  bool isDeviceOn() const override;
  bool isEmergencyStop() const override;
  bool isDelayActive() const override;
  unsigned long getDelayTimer() const override;
  unsigned long getStartTime() const override;

  int getSpeedPercent() const override { return 0; }
  bool isAdaptiveModeActive() const override { return false; }

  bool isSensorOk() const override { return false; }
  float getTemperature() const override { return 0.0f; }
  float getHumidity() const override { return 0.0f; }
  const char* getSensorError() const override { return "N/A"; }

  bool isMqttConnected() const override;
  int getWifiRssi() const override;

 private:
  class SwitchActuator* _switch;
  class MQTTManager* _mqtt;
};

#endif  // WEB_STATUS_PROVIDER_H