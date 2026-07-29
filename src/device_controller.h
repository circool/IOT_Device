/**
 * @file device_controller.h
 * @brief Бизнес-логика устройства
 *
 * Содержит всю логику принятия решений на основе показаний датчика,
 * порогов из Config, таймеров и режимов управления.
 * Не зависит от транспорта и инфраструктуры.
 */

#ifndef DEVICE_CONTROLLER_H
#define DEVICE_CONTROLLER_H

#include <stdint.h>
#include "config_manager.h"
#include "fan_actuator.h"
#include "switch_actuator.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
#include "sensor.h"
#endif

// ===== Типы команд от транспорта =====

/**
 * @brief Типы команд от транспорта
 * @note Набор команд зависит от DEVICE_TYPE
 */
typedef enum {
  CMD_DO_RESET,        /**< void — сброс к заводским */

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  CMD_SET_ACTUATOR,    /**< bool: true=включить, false=выключить */
  CMD_SET_DELAY_SEC,   /**< int: 0-86400 */
  CMD_SET_MAX_ON_TIME, /**< uint32_t: 0-86400 */
  CMD_SET_BOOT_STATE,  /**< bool: состояние при старте */
#endif

#if DEVICE_TYPE == 1
  CMD_SET_SPEED,               /**< uint8_t: 0-100 */
  CMD_SET_LOW_TEMP,            /**< float */
  CMD_SET_HIGH_TEMP,           /**< float */
  CMD_SET_LOW_HUM,             /**< float */
  CMD_SET_HIGH_HUM,            /**< float */
  CMD_SET_SENSOR_CONTROL_MODE, /**< bool: true=AUTO, false=MANUAL */
  CMD_SET_ADAPTIVE_MODE,       /**< bool */
#endif
} command_type_t;

// ===== Состояние устройства (RAM) =====

/**
 * @brief Оперативное состояние устройства
 */
typedef struct {
  bool is_on;    /**< Состояние актуатора */
  uint8_t speed; /**< Текущая скорость 0-100% */

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  bool manual_mode;          /**< Ручной режим */
  bool timer_mode;           /**< Работа по таймеру отложенного включения */
  uint32_t delay_remain_sec; /**< Остаток до срабатывания таймера */
#endif

#if DEVICE_TYPE == 1
  bool adaptive_mode_active; /**< Адаптивный режим включён */
#endif
} operational_state_t;

// ===== Колбэк для оркестратора =====

/**
 * @brief Колбэк при изменении состояния
 * @param state Указатель на текущее состояние
 * @param need_save true — нужно сохранить Config в EEPROM
 */
typedef void (*state_callback_t)(const operational_state_t* state,
                                 bool need_save);

// ===== Класс =====

/**
 * @brief Контроллер устройства — бизнес-логика
 */
class DeviceController {
 public:
  DeviceController();

  /**
   * @brief Инициализация контроллера
   * @param config Указатель на Config (не владеет)
   * @param actuator Указатель на актуатор (не владеет)
   */
  void init(const ConfigData* config,
#if DEVICE_TYPE == 1
            FanActuator* actuator
#elif DEVICE_TYPE == 3
            SwitchActuator* actuator
#endif
  );

  /**
   * @brief Периодическая обработка (вызывается в loop)
   */
  void update();

  /**
   * @brief Обработка команды от транспорта
   * @param type Тип команды
   * @param value Значение (интерпретируется в зависимости от типа)
   */
  void handle_command(command_type_t type, float value);

  /**
   * @brief Получение текущего состояния
   * @return Указатель на operational_state_t
   */
  const operational_state_t* get_state() const;

  /**
   * @brief Регистрация колбэка для оркестратора
   * @param callback Функция-колбэк
   */
  void set_state_callback(state_callback_t callback);

  // ===== Команды от Web =====
  void set_state(bool on);
  void set_speed(int percent);
  void set_manual_mode(bool enabled);

 private:
  // ===== Внутренние методы =====
#if DEVICE_TYPE == 1
  void update_fan();
#elif DEVICE_TYPE == 2
  void update_sensor();
#elif DEVICE_TYPE == 3
  void update_switch();
#endif

  void apply_state(); /**< Применить текущее состояние к Actuator */
  void notify_change(bool need_save); /**< Вызвать колбэк с флагом сохранения */

  // ===== Данные =====
  const ConfigData* _config;  /**< Указатель на Config (не владеет) */
  operational_state_t _state; /**< Текущее оперативное состояние */

#if DEVICE_TYPE == 1
  FanActuator* _actuator; /**< Указатель на актуатор вентилятора */
#elif DEVICE_TYPE == 3
  SwitchActuator* _actuator; /**< Указатель на актуатор выключателя */
#endif

  state_callback_t _callback; /**< Колбэк для оркестратора */
};

#endif  // DEVICE_CONTROLLER_H