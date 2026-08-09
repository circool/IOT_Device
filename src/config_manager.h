/**
 * @file config_manager.h
 * @brief Управление хранением настроек в энергонезависимой памяти
 * @version 0.11
 * @date 08.08.2026
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include "common_types.h"
#include "settings.h"

/**
 * @brief Менеджер конфигурации
 *
 * Обеспечивает сохранение, загрузку и валидацию конфигурационных данных в
 * EEPROM. Работает с двумя независимыми структурами: TransportConfig и
 * DeviceConfig.
 *
 * @note Критический компонент — без него невозможна работа DeviceController и
 * транспорта
 */
class ConfigManager {
 public:
  /**
   * @brief Инициализация менеджера конфигурации
   * @return true при успешной инициализации, false при ошибке
   */
  bool init();

  /**
   * @brief Сохранение конфигурации транспорта
   * @param config Структура с параметрами транспорта
   * @return true при успешной записи, false при ошибке
   */
  bool set(const TransportConfig& config);

  /**
   * @brief Загрузка конфигурации транспорта
   * @param config Ссылка на структуру для заполнения
   * @return true при успешном чтении, false при ошибке
   * @note Выполняет полную валидацию данных: CRC и семантическую проверку
   *       Невалидные значения заполняются пусто/ноль/ложь
   */
  bool get(TransportConfig& config);

  /**
   * @brief Сохранение настроек устройства
   * @param settings Структура с настройками устройства
   * @return true при успешной записи, false при ошибке
   */
  bool set(const DeviceConfig& settings);

  /**
   * @brief Загрузка настроек устройства
   * @param settings Ссылка на структуру для заполнения
   * @return true при успешном чтении, false при ошибке
   * @note Выполняет полную валидацию данных: CRC и семантическую проверку
   *       Невалидные значения заполняются пусто/ноль/ложь
   */
  bool get(DeviceConfig& settings);

  /**
   * @brief Сброс конфигурации транспорта к значениям по умолчанию
   * @param config Ссылка на структуру для заполнения значениями по умолчанию
   */
  void reset(TransportConfig& config);

  /**
   * @brief Сброс настроек устройства к значениям по умолчанию
   * @param settings Ссылка на структуру для заполнения значениями по умолчанию
   */
  void reset(DeviceConfig& settings);

 private:
  // ================================================================
  // ВНУТРЕННИЕ МЕТОДЫ
  // ================================================================

  /**
   * @brief Вычисление 16-битной контрольной суммы CRC
   * @param data Указатель на данные
   * @param len Размер данных в байтах
   * @return 16-битное значение CRC
   */
  static uint16_t calculateCRC(const uint8_t* data, size_t len);

  /**
   * @brief Валидация конфигурации транспорта
   * @param config Структура для проверки и корректировки
   * @return true если все поля валидны, false если были исправления
   */
  bool validateTransportConfig(TransportConfig& config) const;

  /**
   * @brief Валидация настроек устройства
   * @param settings Структура для проверки и корректировки
   * @return true если все поля валидны, false если были исправления
   */
  bool validateDeviceConfig(DeviceConfig& settings) const;

  /**
   * @brief Проверка магического числа в EEPROM
   * @return true если MAGIC совпадает, false в противном случае
   */
  bool checkMagic() const;

  /**
   * @brief Запись магического числа в EEPROM
   */
  void writeMagic();

  /**
   * @brief Чтение данных из EEPROM без валидации
   * @param config Структура для заполнения
   * @return true при успешном чтении, false при ошибке
   */
  bool read(TransportConfig& config) const;

  /**
   * @brief Чтение данных из EEPROM без валидации
   * @param settings Структура для заполнения
   * @return true при успешном чтении, false при ошибке
   */
  bool read(DeviceConfig& settings) const;

  /**
   * @brief Запись данных в EEPROM с пересчётом CRC
   * @param config Структура для записи
   * @return true при успешной записи, false при ошибке
   */
  bool write(const TransportConfig& config);

  /**
   * @brief Запись данных в EEPROM с пересчётом CRC
   * @param settings Структура для записи
   * @return true при успешной записи, false при ошибке
   */
  bool write(const DeviceConfig& settings);

  
  // ================================================================
  // ПРИВАТНЫЕ ПОЛЯ
  // ================================================================

  bool m_initialized;  ///< Флаг инициализации

  // ================================================================
  // КОНСТАНТЫ СМЕЩЕНИЙ
  // ================================================================

  static constexpr size_t MAGIC_OFFSET = 0;
  static constexpr size_t TRANSPORT_OFFSET = sizeof(uint16_t);
};

#endif  // CONFIG_MANAGER_H