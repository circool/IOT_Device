#ifndef FAN_ACTUATOR_H
#define FAN_ACTUATOR_H

#include "actuator_base.h"

// ============================================================================
// ШИМ (PWM) — для TYPE 1
// ============================================================================

#if DEVICE_TYPE == 1

// Для доступа к константам адаптивного режима
#ifndef ADAPTIVE_STEP_SIZE
#define ADAPTIVE_STEP_SIZE 10
#endif

#ifndef ADAPTIVE_EPSILON_TEMP
#define ADAPTIVE_EPSILON_TEMP 0.5
#endif

#ifndef ADAPTIVE_EPSILON_HUM
#define ADAPTIVE_EPSILON_HUM 2.0
#endif

#ifndef ADAPTIVE_SPEED_SENSITIVITY
#define ADAPTIVE_SPEED_SENSITIVITY 0.7
#endif

/** @brief Включить ШИМ управление скоростью */
#ifndef PWM_ENABLED
#define PWM_ENABLED 1
#endif

#if PWM_ENABLED == 1
/** @brief Частота ШИМ в Герцах */
#ifndef PWM_FREQUENCY
#define PWM_FREQUENCY 500
#endif

/** @brief Разрешение ШИМ (бит) */
#ifndef PWM_RESOLUTION
#define PWM_RESOLUTION 8
#endif

/** @brief Длина стартового импульса для раскрутки вентилятора (мс) */
#ifndef PWM_STARTING
#define PWM_STARTING 200
#endif
/** @brief Минимально допустимая скость (определяется особенностями мотора) */
#ifndef MIN_SPEED_PERCENT
#define MIN_SPEED_PERCENT 10  
#endif

/** @brief Скорость по умолчанию (%) */
#ifndef DEFAULT_SPEED_PERCENT
#define DEFAULT_SPEED_PERCENT 50
#endif
#endif

#else
#ifndef PWM_ENABLED
#define PWM_ENABLED 0
#endif
#endif
#if DEVICE_TYPE == 1
/**
 * @brief Управление вентилятором с поддержкой ШИМ
 *
 * Расширяет ActuatorBase:
 * - Регулировка скорости вращения (0-100%)
 * - Стартовый импульс на полной мощности для раскрутки
 *
 * Используется только при DEVICE_TYPE == 1
 *
 * @note Адаптивный режим и логика управления вынесены в оркестратор (main.cpp)
 */
class FanActuator {
 public:
  FanActuator();

  /**
   * @brief Инициализация вентилятора
   * @param pin GPIO для управления реле
   * @param relayOnLevel Уровень включения (HIGH/LOW)
   * @param bootState Состояние при старте (true=вкл)
   * @param defaultSpeed Скорость по умолчанию (0-100%)
   * @param adaptiveMode Включить адаптивный режим (только флаг, логика в main)
   * @param delaySeconds Задержка отложенного включения (сек)
   * @param maxOnTime Таймер аварийного отключения (сек)
   */
  void init(uint8_t pin,
            uint8_t relayOnLevel,
            bool bootState,
            uint16_t defaultSpeed,
            bool adaptiveMode,
            int delaySeconds,
            uint32_t maxOnTime);

  /**
   * @brief Периодический вызов в loop()
   * Обрабатывает только стартовый импульс и базовые таймеры
   */
  void update();

  /**
   * @brief Включить/выключить вентилятор
   * @param on true — включить, false — выключить
   * @param manual true — ручная команда (отключает адаптивный режим)
   */
  void set(bool on, bool manual = true);

  /**
   * @brief Получить текущее состояние
   */
  bool getState() const;

  /**
   * @brief Установить скорость вращения
   * @param percent 0-100%
   * @param manual true — ручная команда
   */
  void setSpeed(int percent, bool manual = true);

  /**
   * @brief Получить текущую скорость
   * @return 0-100%
   */
  int getSpeed() const;

  /**
   * @brief Включить/выключить адаптивный режим (только флаг)
   * @note Реальная адаптивная логика выполняется в main.cpp
   */
  void setAdaptiveMode(bool enabled);

  /**
   * @brief Получить состояние адаптивного режима
   */
  bool getAdaptiveMode() const;

  /**
   * @brief Обновить конфигурацию во время работы
   * @param adaptiveMode Новое состояние адаптивного режима (флаг)
   * @param delaySeconds Новая задержка отложенного включения (сек)
   * @param maxOnTime Новое время аварийного отключения (сек)
   */
  void updateConfig(bool adaptiveMode, int delaySeconds, uint32_t maxOnTime);

  // Прокси-методы для доступа к таймерам базового класса
  unsigned long getStartTime() const { return _base.getStartTime(); }
  bool isDelayActive() const { return _base.isDelayActive(); }
  unsigned long getDelayTimer() const { return _base.getDelayTimer(); }
  bool isEmergencyStop() const { return _base.isEmergencyStop(); }

  // Статические колбэки для ActuatorBase
  static void onSetPhysicalCallback(void* context, bool on);
  static void onForceStopCallback(void* context);
  static void onManualCommandCallback(void* context);

 private:
  /**
   * @brief Применить скорость к физическому выходу (ШИМ или дискретно)
   */
  void applySpeed(int percent);

  /**
   * @brief Включить режим ШИМ на пине
   */
  void enablePWM();

  /**
   * @brief Выключить режим ШИМ (возврат к дискретному управлению)
   */
  void disablePWM();

  ActuatorBase _base;  // Базовый класс (дискретное управление)

  uint8_t _pin;           // GPIO для управления реле
  uint8_t _relayOnLevel;  // Уровень включения
  int _currentSpeed;      // Текущая скорость (0-100)
  bool _adaptiveMode;     // Флаг адаптивного режима (логика в main)
  bool _pwmActive;        // Активен ли ШИМ в данный момент

  // Настройки (хранятся для передачи в базовый класс)
  int _delaySeconds;
  uint32_t _maxOnTime;

  // Стартовый импульс
  bool _startingPulseActive;          // Идёт ли стартовый импульс
  unsigned long _startingPulseStart;  // Время начала импульса
};
#else
// Заглушка
class FanActuator {
 public:
  FanActuator() {}

  void init(uint8_t, uint8_t, bool, uint16_t, bool, int, uint32_t) {}
  void update() {}
  void set(bool, bool = true) {}
  bool getState() const { return false; }
  void setSpeed(int, bool = true) {}
  int getSpeed() const { return 0; }
  void setAdaptiveMode(bool) {}
  bool getAdaptiveMode() const { return false; }
  void updateConfig(bool, int, uint32_t) {}

  unsigned long getStartTime() const { return 0; }
  bool isDelayActive() const { return false; }
  unsigned long getDelayTimer() const { return 0; }
  bool isEmergencyStop() const { return false; }
};
#endif
#endif