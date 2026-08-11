/**
 * @file device_controller_fan_actuator.h
 * @brief Управление вентилятором с ШИМ (внутренняя подсистема DeviceController)
 * @note Используется только при DEVICE_TYPE == 1
 */

#ifndef DEVICE_CONTROLLER_FAN_ACTUATOR_H
#define DEVICE_CONTROLLER_FAN_ACTUATOR_H

#include "device_controller_actuator_base.h"

#if DEVICE_TYPE == 1

// ===== КОНСТАНТЫ =====
#ifndef PWM_FREQUENCY
// Частота ШИМ-сигнала
#define PWM_FREQUENCY 5
#endif

#ifndef PWM_RESOLUTION
// Разрешение ШИМ
#define PWM_RESOLUTION 8
#endif

#ifndef PWM_STARTING
// Длина стартового импульса при ШИМ
#define PWM_STARTING 1000
#endif

#ifndef MIN_SPEED_PERCENT
// Минимально разрешенная скорость вращения вентилятора
#define MIN_SPEED_PERCENT 10
#endif

// ===== КЛАСС =====

/**
 * @brief Управление вентилятором с поддержкой ШИМ
 * 
 * Расширяет ActuatorBase:
 * - Регулировка скорости вращения (0-100%)
 * - Стартовый импульс на полной мощности для раскрутки
 */
class FanActuator {
public:
    FanActuator();

    /**
     * @brief Инициализация
     * @param pin GPIO для управления реле
     * @param relayOnLevel Уровень включения (HIGH/LOW)
     * @param bootState Состояние при старте
     * @param defaultSpeed Скорость по умолчанию (0-100%)
     * @param adaptiveMode Флаг адаптивного режима (логика в DeviceController)
     * @param delaySeconds Задержка отложенного включения (сек)
     * @param maxOnTime Аварийное отключение (сек)
     */
    void init(uint8_t pin, uint8_t relayOnLevel, bool bootState,
              uint16_t defaultSpeed, bool adaptiveMode,
              int delaySeconds, uint32_t maxOnTime);

    /**
     * @brief Периодическая обработка
     */
    void update();

    /**
     * @brief Включить/выключить
     */
    void set(bool on, bool manual = true);

    /**
     * @brief Получить состояние
     */
    bool getState() const;

    /**
     * @brief Установить скорость
     * @param percent 0-100%
     * @param manual true — ручная команда
     */
    void setSpeed(int percent, bool manual = true);

    /**
     * @brief Получить текущую скорость
     */
    int getSpeed() const;

    /**
     * @brief Установить флаг адаптивного режима
     */
    void setAdaptiveMode(bool enabled);

    /**
     * @brief Получить флаг адаптивного режима
     */
    bool getAdaptiveMode() const;

    /**
     * @brief Обновить конфигурацию
     */
    void updateConfig(bool adaptiveMode, int delaySeconds, uint32_t maxOnTime);

    // ===== ПРОКСИ-МЕТОДЫ =====
    unsigned long getStartTime() const { return _base.getStartTime(); }
    bool isDelayActive() const { return _base.isDelayActive(); }
    unsigned long getDelayTimer() const { return _base.getDelayTimer(); }
    bool isEmergencyStop() const { return _base.isEmergencyStop(); }

    // ===== СТАТИЧЕСКИЕ КОЛБЭКИ =====
    static void onSetPhysicalCallback(void* context, bool on);
    static void onForceStopCallback(void* context);
    static void onManualCommandCallback(void* context);

private:
    void applySpeed(int percent);
    void enablePWM();
    void disablePWM();

    ActuatorBase _base;
    uint8_t _pin;
    uint8_t _relayOnLevel;
    int _currentSpeed;
    bool _adaptiveMode;
    bool _pwmActive;
    int _delaySeconds;
    uint32_t _maxOnTime;
    bool _startingPulseActive;
    unsigned long _startingPulseStart;
};

#else
// ===== ЗАГЛУШКА ДЛЯ TYPE 2 и 3 =====
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

    bool isEmergencyStop() const { return false; }
    unsigned long getStartTime() const { return 0; }
    bool isDelayActive() const { return false; }
    unsigned long getDelayTimer() const { return 0; }
};
#endif

#endif // DEVICE_CONTROLLER_FAN_ACTUATOR_H