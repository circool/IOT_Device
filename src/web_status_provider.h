/**
 * @file web_status_provider.h
 * @brief Провайдеры статуса для веб-интерфейса
 *
 * @details Адаптер между веб-слоем и бизнес-логикой.
 *          Веб-слой работает через единый интерфейс IWebStatusProvider.
 *
 * @architecture
 *   Каждый тип устройства имеет свою реализацию провайдера.
 *   #ifdef используются ТОЛЬКО для выбора реализации по DEVICE_TYPE.
 *   Это позволяет компилировать только нужный код для каждого типа.
 *
 * @note "Толстый" интерфейс с заглушками — осознанное упрощение для Embedded:
 *       - Web слой работает с одним интерфейсом (без dynamic_cast)
 *       - Заглушки тривиальны (не увеличивают код)
 *       - Экономия Flash и RAM
 */

#ifndef WEB_STATUS_PROVIDER_H
#define WEB_STATUS_PROVIDER_H

#include <Arduino.h>
#include "config_manager.h"

// ============================================================================
// ИНТЕРФЕЙС (ОБЩИЙ ДЛЯ ВСЕХ ТИПОВ)
// ============================================================================

/**
 * @brief Интерфейс для получения статуса устройства
 * @details Веб-слой работает ТОЛЬКО через этот интерфейс.
 *          Содержит методы для ВСЕХ типов устройств.
 *          Реализации возвращают значения по умолчанию для отсутствующих
 * функций.
 */
class IWebStatusProvider {
 public:
  virtual ~IWebStatusProvider() = default;

  // ===== Основная информация (TYPE 1 и 3) =====
  virtual bool isDeviceOn() const = 0;
  virtual bool isEmergencyStop() const = 0;
  virtual bool isDelayActive() const = 0;
  virtual unsigned long getDelayTimer() const = 0;
  virtual unsigned long getStartTime() const = 0;

  // ===== Информация о вентиляторе (только TYPE 1) =====
  virtual int getSpeedPercent() const = 0;
  virtual bool isAdaptiveModeActive() const = 0;

  // ===== Информация о датчике (TYPE 1 и 2) =====
  virtual bool isSensorOk() const = 0;
  virtual float getTemperature() const = 0;
  virtual float getHumidity() const = 0;
  virtual const char* getSensorError() const = 0;

  // ===== Информация о подключениях (все типы) =====
  virtual bool isMqttConnected() const = 0;
  virtual int getWifiRssi() const = 0;
};

// ============================================================================
// РЕАЛИЗАЦИИ (ВЫБОР ПО DEVICE_TYPE)
// ============================================================================

#if DEVICE_TYPE == 1

/**
 * @brief Провайдер для TYPE 1 (вентилятор с датчиком)
 *
 * @details Реализует все методы интерфейса:
 *          - Актуатор: состояние, скорость, таймеры
 *          - Датчик: температура, влажность (глобальные функции)
 *          - MQTT: статус подключения
 *          - WiFi: RSSI
 */
class FanWebStatusProvider : public IWebStatusProvider {
 public:
#if FEATURE_MQTT_ENABLED == 1
  FanWebStatusProvider(class FanActuator* fan, class MQTTManager* mqtt);
#else
  FanWebStatusProvider(class FanActuator* fan);
#endif

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
#if FEATURE_MQTT_ENABLED == 1
  MQTTManager* _mqtt;
#endif
};

#elif DEVICE_TYPE == 2

/**
 * @brief Провайдер для TYPE 2 (автономный датчик)
 *
 * @details Реализует только методы датчика.
 *          Методы актуатора возвращают заглушки.
 */
class SensorWebStatusProvider : public IWebStatusProvider {
 public:
#if FEATURE_MQTT_ENABLED == 1
  SensorWebStatusProvider(class MQTTManager* mqtt);
#else
  SensorWebStatusProvider();
#endif

  // Актуатор → заглушки
  bool isDeviceOn() const override { return false; }
  bool isEmergencyStop() const override { return false; }
  bool isDelayActive() const override { return false; }
  unsigned long getDelayTimer() const override { return 0; }
  unsigned long getStartTime() const override { return 0; }

  // Вентилятор → заглушки
  int getSpeedPercent() const override { return 0; }
  bool isAdaptiveModeActive() const override { return false; }

  // Датчик
  bool isSensorOk() const override;
  float getTemperature() const override;
  float getHumidity() const override;
  const char* getSensorError() const override;

  // Подключения
  bool isMqttConnected() const override;
  int getWifiRssi() const override;

 private:
#if FEATURE_MQTT_ENABLED == 1
  MQTTManager* _mqtt;
#endif
};

#elif DEVICE_TYPE == 3

/**
 * @brief Провайдер для TYPE 3 (управляемый выключатель)
 *
 * @details Реализует только методы актуатора.
 *          Методы датчика и вентилятора возвращают заглушки.
 */
class SwitchWebStatusProvider : public IWebStatusProvider {
 public:
#if FEATURE_MQTT_ENABLED == 1
  SwitchWebStatusProvider(class SwitchActuator* sw, class MQTTManager* mqtt);
#else
  SwitchWebStatusProvider(class SwitchActuator* sw);
#endif

  bool isDeviceOn() const override;
  bool isEmergencyStop() const override;
  bool isDelayActive() const override;
  unsigned long getDelayTimer() const override;
  unsigned long getStartTime() const override;

  // Вентилятор → заглушки
  int getSpeedPercent() const override { return 0; }
  bool isAdaptiveModeActive() const override { return false; }

  // Датчик → заглушки
  bool isSensorOk() const override { return false; }
  float getTemperature() const override { return 0.0f; }
  float getHumidity() const override { return 0.0f; }
  const char* getSensorError() const override { return "N/A"; }

  bool isMqttConnected() const override;
  int getWifiRssi() const override;

 private:
  class SwitchActuator* _switch;
#if FEATURE_MQTT_ENABLED == 1
  MQTTManager* _mqtt;
#endif
};

#endif  // DEVICE_TYPE

#endif  // WEB_STATUS_PROVIDER_H