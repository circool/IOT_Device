/**
 * @file device_controller_switch_actuator.h
 * @brief Управление выключателем (только ON/OFF)
 * @note Используется только при DEVICE_TYPE == 3
 */

#ifndef DEVICE_CONTROLLER_SWITCH_ACTUATOR_H
#define DEVICE_CONTROLLER_SWITCH_ACTUATOR_H

#include "device_controller_actuator_base.h"

#if DEVICE_TYPE == 3

// ===== КЛАСС =====

/**
 * @brief Управление выключателем без ШИМ (только вкл/выкл)
 * 
 * Прокси-класс над ActuatorBase.
 * Используется при DEVICE_TYPE == 3 (управляемый выключатель)
 */
class SwitchActuator {
public:
    SwitchActuator();

    /**
     * @brief Инициализация
     * @param pin GPIO для управления реле
     * @param relayOnLevel Уровень включения (HIGH/LOW)
     * @param bootState Состояние при старте
     * @param delaySeconds Задержка отложенного включения (сек)
     * @param maxOnTime Аварийное отключение (сек)
     */
    void init(uint8_t pin, uint8_t relayOnLevel, bool bootState,
              int delaySeconds, uint32_t maxOnTime);

    /**
     * @brief Периодическая обработка
     */
    void update(int delaySeconds, uint32_t maxOnTime);

    /**
     * @brief Установить состояние
     */
    void set(bool on, bool manual = true);

    /**
     * @brief Получить состояние
     */
    bool getState() const;

    /**
     * @brief Обновить конфигурацию
     */
    void updateConfig(int delaySeconds, uint32_t maxOnTime);

    // ===== ПРОКСИ-МЕТОДЫ =====
    unsigned long getStartTime() const { return _base.getStartTime(); }
    bool isDelayActive() const { return _base.isDelayActive(); }
    unsigned long getDelayTimer() const { return _base.getDelayTimer(); }
    bool isEmergencyStop() const { return _base.isEmergencyStop(); }

    // ===== СТАТИЧЕСКИЕ КОЛБЭКИ =====
    static void onSetPhysicalCallback(void* context, bool on);
    static void onForceStopCallback(void* context);

private:
    ActuatorBase _base;
    uint8_t _pin;
    uint8_t _relayOnLevel;
    int _delaySeconds;
    uint32_t _maxOnTime;
};

#else
// ===== ЗАГЛУШКА ДЛЯ TYPE 1 и 2 =====
class SwitchActuator {
public:
    SwitchActuator() {}

    void init(uint8_t, uint8_t, bool, int, uint32_t) {}
    void update(int, uint32_t) {}
    void set(bool, bool = true) {}
    bool getState() const { return false; }
    void updateConfig(int, uint32_t) {}

    bool isEmergencyStop() const { return false; }
    unsigned long getStartTime() const { return 0; }
    bool isDelayActive() const { return false; }
    unsigned long getDelayTimer() const { return 0; }
};
#endif

#endif // DEVICE_CONTROLLER_SWITCH_ACTUATOR_H