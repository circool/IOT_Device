#ifndef ACTUATOR_BASE_H
#define ACTUATOR_BASE_H

#include <Arduino.h>
#include "config_manager.h"

/** @brief Пин управления реле/вентилятором */
#ifndef SWITCH_PIN
#ifdef ESP8266
#define SWITCH_PIN 14
#elif defined(ESP32)
#define SWITCH_PIN 4
#endif
#endif

/**
 * @brief Уровень сигнала для включения реле
 * @values HIGH или LOW
 */
#ifndef RELAY_ON_LEVEL
#define RELAY_ON_LEVEL LOW
#endif

/**
 * @brief Базовый класс для управления исполнительным механизмом
 * (вентилятор/выключатель)
 *
 * Обеспечивает:
 * - Включение/выключение с сохранением состояния
 * - Таймер отложенного включения (для туалета)
 * - Контроль максимального времени работы (защита от зависания)
 *
 * Не содержит логики ШИМ — только дискретное управление.
 */
class ActuatorBase {
 public:
  ActuatorBase();

  /**
   * @brief Инициализация пина и начального состояния
   * @param pin Номер GPIO для управления реле
   * @param relayOnLevel Уровень сигнала для включения (HIGH или LOW)
   * @param bootState true — включить при старте, false — выключить
   */
  void init(uint8_t pin, uint8_t relayOnLevel, bool bootState);

  /**
   * @brief Установить состояние вентилятора
   * @param on true — включить, false — выключить
   * @param manual true — команда от пользователя (MQTT/веб), false —
   * автоматическая (датчик/таймер)
   */
  void set(bool on, bool manual = true);

  /**
   * @brief Получить текущее состояние
   * @return true — включён, false — выключен
   */
  bool getState() const;

  /**
   * @brief Периодический вызов в loop()
   * Проверяет таймер отложенного включения и максимальное время работы
   */
  void update();

  /**
   * @brief Принудительно выключить при превышении maxOnTime
   * Переводит устройство в ручной режим (для TYPE 1)
   */
  void forceStop();

  // Методы для доступа (используются веб-интерфейсом для отображения таймера)
  unsigned long getStartTime() const { return _startTime; }
  bool isDelayActive() const { return _delayActive; }
  unsigned long getDelayTimer() const { return _delayTimer; }

  // Указатели на колбэки (экономия Flash вместо virtual функций)
  void (*onSetPhysicalCallback)(
      void*,
      bool);  // Вызов при изменении физического состояния пина
  void (*onForceStopCallback)(void*);  // Вызов при принудительной остановке
  void (*onManualCommandCallback)(
      void*);             // ВЫЗОВ ПРИ РУЧНОЙ КОМАНДЕ (manual=true)
  void* callbackContext;  // Контекст (this для производного класса)

  bool isEmergencyStop() const { return _emergencyStop; }
  // void clearEmergencyStop() { _emergencyStop = false; }

 protected:
  void checkMaxOnTime();  // Проверка превышения максимального времени работы
  bool delayTimer(bool start);  // Управление таймером отложенного включения

  uint8_t _pin;               // Номер GPIO
  uint8_t _relayOnLevel;      // Уровень включения (HIGH/LOW)
  bool _state;                // Текущее состояние (true=вкл)
  unsigned long _startTime;   // Время последнего включения (для maxOnTime)
  bool _delayActive;          // Активен ли таймер отложенного включения
  unsigned long _delayTimer;  // Время срабатывания таймера (millis)
  bool _emergencyStop = false;
};

#endif