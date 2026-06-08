#ifndef FAN_ACTUATOR_H
#define FAN_ACTUATOR_H

#include "actuator_base.h"
#include "sensor.h"

/**
 * @brief Управление вентилятором с поддержкой ШИМ и адаптивного режима
 * 
 * Расширяет ActuatorBase:
 * - Регулировка скорости вращения (0-100%)
 * - Адаптивный тихий режим (автоматическое увеличение скорости при росте влажности/температуры)
 * - Стартовый импульс на полной мощности для раскрутки
 * 
 * Используется только при DEVICE_TYPE == 1
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
     */
    void init(uint8_t pin, uint8_t relayOnLevel, bool bootState, uint16_t defaultSpeed);
    
    /**
     * @brief Периодический вызов в loop()
     * Обрабатывает стартовый импульс, адаптивный режим, таймеры
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
     * @param manual true — ручная команда (отключает адаптивный режим)
     */
    void setSpeed(int percent, bool manual = true);
    
    /**
     * @brief Получить текущую скорость
     * @return 0-100%
     */
    int getSpeed() const;
    
    /**
     * @brief Включить/выключить адаптивный режим
     * При включении фиксирует базовые показания датчика
     */
    void setAdaptiveMode(bool enabled);
    
    /**
     * @brief Получить состояние адаптивного режима
     */
    bool getAdaptiveMode() const;
    
    // Прокси-методы для доступа к таймерам базового класса
    unsigned long getStartTime() const { return _base.getStartTime(); }
    bool isDelayActive() const { return _base.isDelayActive(); }
    unsigned long getDelayTimer() const { return _base.getDelayTimer(); }

    /**
     * @brief Включить поэтапное увеличение скорости (для туалета)
     * При срабатывании таймера отложенного включения
     */
    // void enableRampUp() { _rampUpActive = true; _lastRampUpTime = millis(); }
    
    
    
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
    
    /**
     * @brief Обновление адаптивного режима (вызывается в update)
     * Увеличивает скорость при росте температуры или влажности
     */
    void adaptiveUpdate();
    
    /**
     * @brief Рассчитать шаг увеличения скорости при адаптации
     * @param deltaTemp Изменение температуры от базового значения
     * @param deltaHum Изменение влажности от базового значения
     * @param humRate Скорость изменения влажности (%/сек)
     * @return Шаг в процентах (5-60)
     */
    int calculateAdaptiveStep(float deltaTemp, float deltaHum, float humRate);
    
    ActuatorBase _base;          // Базовый класс (дискретное управление)
    
    uint8_t _pin;                // GPIO для управления реле
    uint8_t _relayOnLevel;       // Уровень включения
    int _currentSpeed;           // Текущая скорость (0-100)
    bool _adaptiveMode;          // Включён ли адаптивный режим
    bool _pwmActive;             // Активен ли ШИМ в данный момент
    
    // Адаптивный режим
    bool _adaptiveActive;        // Адаптация активна в текущей сессии
    float _baseTemp;             // Температура при включении вентилятора
    float _baseHum;              // Влажность при включении вентилятора
    unsigned long _lastAdaptiveCheck;  // Время последней адаптации
    
    // Стартовый импульс
    bool _startingPulseActive;   // Идёт ли стартовый импульс
    unsigned long _startingPulseStart;  // Время начала импульса
    
    // Поэтапное увеличение скорости (ramp-up)
    bool _rampUpActive;          // Активно ли поэтапное увеличение
    unsigned long _lastRampUpTime;      // Время последнего увеличения
};

#endif