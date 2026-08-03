/**
 * @file state_provider.h
 * @brief Единый источник данных о состоянии устройства
 * @details Все слои читают состояние из одного места и обновляют его при
 * изменениях. StateProvider — синглтон, хранит оперативное состояние устройства
 * в RAM. Не содержит бизнес-логику и не хранит настройки (ConfigManager).
 */

#ifndef STATE_PROVIDER_H
#define STATE_PROVIDER_H

#include <Arduino.h>
#include "settings.h"

// ============================================================================
// СТРУКТУРА СОСТОЯНИЯ
// ============================================================================

/**
 * @brief Полное состояние устройства
 */
struct DeviceState {
  // ===== ОПЕРАТИВНОЕ СОСТОЯНИЕ =====
  bool is_on;                /**< Актуатор включён */
  uint8_t speed;             /**< Скорость 0-100% (TYPE 1) */
  bool manual_mode;          /**< Ручной режим */
  bool timer_mode;           /**< Режим таймера */
  uint32_t delay_remain_sec; /**< Остаток таймера отложенного включения */
  bool adaptive_active;      /**< Адаптивный режим активен */

  // ===== ДАННЫЕ ДАТЧИКА =====
  float temperature;        /**< Температура, °C */
  float humidity;           /**< Влажность, % */
  bool sensor_valid;        /**< Данные датчика валидны */
  const char* sensor_error; /**< Текст ошибки датчика (NULL если нет) */

  // ===== ПОДКЛЮЧЕНИЯ =====
  bool wifi_connected; /**< WiFi подключён */
  bool mqtt_connected; /**< MQTT подключён */
  int wifi_rssi;       /**< Уровень сигнала WiFi, dBm */

  // ===== СИСТЕМНЫЕ ФЛАГИ =====
  bool provisioning;    /**< Режим настройки (AP/BLE) */
  bool emergency;       /**< Аварийное отключение */
  bool restart_pending; /**< Ожидание перезагрузки */

  // ===== КНОПКА =====
  bool button_pressed; /**< Кнопка нажата */
  uint8_t button_stage; /**< Стадия нажатия: 0=RELEASED, 1=PRESSED, 2=STAGE_1S,
                           3=STAGE_2S, 4=STAGE_3S */

  // ===== ВРЕМЯ =====
  unsigned long uptime;     /**< Время работы устройства, мс */
  unsigned long start_time; /**< Время старта устройства, мс */
};

// ============================================================================
// STATE PROVIDER
// ============================================================================

/**
 * @brief Единый источник данных о состоянии устройства
 * @details Синглтон. Все слои читают и пишут состояние через этот класс.
 *          Доступ к состоянию — только для чтения через get_state().
 *          Обновление — через специализированные методы update_*().
 */
class StateProvider {
 public:
  /**
   * @brief Получить экземпляр синглтона
   * @return Ссылка на StateProvider
   */
  static StateProvider& getInstance();

  // Запрет копирования
  StateProvider(const StateProvider&) = delete;
  StateProvider& operator=(const StateProvider&) = delete;

  /**
   * @brief Получить текущее состояние (только для чтения)
   * @return Указатель на DeviceState
   */
  const DeviceState* get_state() const;

  // ========================================================================
  // ОБНОВЛЕНИЕ СОСТОЯНИЯ
  // ========================================================================

  /**
   * @brief Обновить показания датчика
   * @param temp Температура, °C
   * @param hum Влажность, %
   * @param valid true — данные валидны
   * @param error Текст ошибки (NULL если нет)
   */
  void update_sensor(float temp, float hum, bool valid, const char* error);

  /**
   * @brief Обновить оперативное состояние (от DeviceController)
   * @param on Состояние актуатора
   * @param speed Скорость 0-100%
   * @param manual Ручной режим
   * @param adaptive Адаптивный режим активен
   */
  void update_operational(bool on, uint8_t speed, bool manual, bool adaptive);

  /**
   * @brief Обновить состояние подключений
   * @param wifi WiFi подключён
   * @param mqtt MQTT подключён
   * @param rssi Уровень сигнала WiFi, dBm
   */
  void update_connection(bool wifi, bool mqtt, int rssi);

  /**
   * @brief Обновить режим настройки
   * @param active true — режим настройки активен
   */
  void update_provisioning(bool active);

  /**
   * @brief Обновить аварийное отключение
   * @param active true — авария активна
   */
  void update_emergency(bool active);

  /**
   * @brief Обновить флаг ожидания перезагрузки
   * @param pending true — перезагрузка запрошена
   */
  void update_restart(bool pending);

  /**
   * @brief Обновить состояние кнопки
   * @param pressed true — кнопка нажата
   * @param stage Стадия нажатия (0-4)
   */
  void update_button(bool pressed, uint8_t stage);

  /**
   * @brief Обновить время работы устройства
   * @param uptime Текущее время работы, мс
   */
  void update_uptime(unsigned long uptime);

 private:
  StateProvider();
  ~StateProvider() = default;

  DeviceState _state; /**< Текущее состояние устройства */
  bool _initialized;  /**< Флаг инициализации */
};

#endif  // STATE_PROVIDER_H