/**
 * @file provisioning.h
 * @brief Управление процессом первоначальной настройки (комиссионинга)
 *
 * Поддерживает два метода:
 * - BLE (только WiFi) — через ESP BLE Provisioning
 * - AP + Web (полная настройка) — через точку доступа и веб-интерфейс
 *
 * @note Вся логика провизионинга инкапсулирована в этом модуле
 */

#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>
#include <functional>

// Forward declaration
class BleProvisioningServer;

// ============================================================================
// ТИПЫ
// ============================================================================

/**
 * @brief Режим провизионинга
 */
enum class ProvisioningMode : uint8_t {
  NONE = 0,      ///< Не выбран
  BLE = 1,       ///< BLE (только WiFi)
  AP = 2,        ///< AP + Web (полная настройка)
  COMPLETED = 3  ///< Завершён
};

/**
 * @brief Колбэк при завершении провизионинга
 * @param userData Пользовательские данные
 */
using ProvisioningCallback = std::function<void(void* userData)>;

// ============================================================================
// КЛАСС PROVISIONING MANAGER
// ============================================================================

/**
 * @brief Менеджер первоначальной настройки устройства
 *
 * @details Управляет процессом комиссионинга:
 *          - Выбор метода (BLE или AP) на основе PROVISIONING_METHOD
 *          - Запуск и остановка провизионинга
 *          - Обработка полученных данных
 *
 * @note BLE метод передаёт только WiFi (SSID + пароль)
 * @note AP метод передаёт полную конфигурацию через веб-интерфейс
 */
class ProvisioningManager {
 public:
  /**
   * @brief Получить экземпляр (синглтон)
   * @return Ссылка на единственный экземпляр
   */
  static ProvisioningManager& getInstance();

  /**
   * @brief Запустить процесс провизионинга
   * @param callback Колбэк при завершении
   * @param userData Пользовательские данные (передаётся в колбэк)
   * @param timeoutMs Таймаут в миллисекундах (0 = бесконечно)
   * @return true — успешно запущен, false — ошибка
   */
  bool begin(ProvisioningCallback callback = nullptr,
             void* userData = nullptr,
             uint32_t timeoutMs = 0);

  /**
   * @brief Периодическая обработка провизионинга
   * @note Вызывается в loop() для обработки BLE/AP событий
   */
  void update();

  /**
   * @brief Проверить, активен ли провизионинг
   * @return true — процесс идёт, false — не активен или завершён
   */
  bool isActive() const;

  /**
   * @brief Проверить, завершён ли процесс
   * @return true — завершён успешно или с ошибкой
   */
  bool isCompleted() const;

  /**
   * @brief Получить текущий режим провизионинга
   * @return Текущий режим (BLE, AP или NONE)
   */
  ProvisioningMode getMode() const;

  /**
   * @brief Завершить провизионинг (принудительно)
   * @return true — успешно завершён
   */
  bool complete();

  /**
   * @brief Сбросить состояние провизионинга
   */
  void reset();

 private:
  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;

  /**
   * @brief Внутренние состояния
   */
  enum class InternalState : uint8_t {
    IDLE,       ///< Ожидание запуска
    WAITING,    ///< Процесс идёт
    RECEIVED,   ///< Данные получены
    COMPLETED,  ///< Завершён успешно
    ERROR       ///< Ошибка
  };

  /**
   * @brief Выбрать метод провизионинга на основе PROVISIONING_METHOD
   */
  void selectProvisioningMethod();

  /**
   * @brief Запустить AP-провизионинг (точка доступа + веб)
   */
  void startApProvisioning();

  /**
   * @brief Проверить, завершён ли BLE-провизионинг
   */
  bool isBleProvisioningComplete();

  // ========================================================================
  // ДАННЫЕ
  // ========================================================================

  InternalState _state = InternalState::IDLE;
  ProvisioningMode _mode = ProvisioningMode::NONE;
  ProvisioningCallback _callback = nullptr;
  void* _userData = nullptr;
  uint32_t _timeoutMs = 0;
  unsigned long _startTime = 0;
  bool _started = false;
  bool _completed = false;
};

// ============================================================================
// ПРОСТЫЕ ФУНКЦИИ-ОБЁРТКИ ДЛЯ MAIN
// ============================================================================

/**
 * @brief Запустить процесс провизионинга
 * @details Автоматически выбирает метод (BLE или AP) на основе
 *          PROVISIONING_METHOD и запускает процесс настройки.
 *          При ошибке BLE автоматически переключается на AP.
 */
void startProvisioning();

/**
 * @brief Проверить, завершён ли процесс настройки
 * @return true — настройка завершена (успешно или с ошибкой)
 */
bool isProvisioningComplete();

#endif  // PROVISIONING_H