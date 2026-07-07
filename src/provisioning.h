/**
 * @file provisioning.h
 * @brief Управление процессом первоначальной настройки (комиссионинга)
 * @details Менеджер провизионинга управляет BLE и AP методами настройки.
 *          При ошибках считает попытки и при исчерпании переходит в FAILED.
 */

#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>
#include <functional>

// ============================================================================
// ПЕРЕЧИСЛЕНИЯ
// ============================================================================

/**
 * @brief Метод завершения провизионинга
 */
enum class ProvisioningMethod : uint8_t {
  NONE = 0,  /**< Провизионинг не выполнялся (конфиг уже был) */
  BLE = 1,   /**< Настройка выполнена через BLE */
  AP = 2,    /**< Настройка выполнена через AP (веб-интерфейс) */
  FAILED = 3 /**< Провизионинг завершился с критической ошибкой */
};

/**
 * @brief Состояние процесса провизионинга
 */
enum class ProvisioningState : uint8_t {
  IDLE = 0,      /**< Менеджер не инициализирован */
  ACTIVE = 1,    /**< Процесс настройки активен, ожидание данных */
  COMPLETED = 3, /**< Настройка успешно завершена */
  FAILED = 4     /**< Критическая ошибка, процесс остановлен */
};

// ============================================================================
// СТРУКТУРЫ ДАННЫХ
// ============================================================================

/**
 * @brief Данные, полученные от методов провизионинга
 */
struct ProvisioningData {
  char wifiSsid[32];     /**< Имя WiFi сети (SSID) */
  char wifiPassword[64]; /**< Пароль WiFi сети */
};

// ============================================================================
// ОБЪЯВЛЕНИЕ КОЛБЭКОВ
// ============================================================================

/**
 * @brief Колбэк при завершении провизионинга
 * @param method Метод, которым была выполнена настройка
 */
using ProvisioningCallback = std::function<void(ProvisioningMethod method)>;

// ============================================================================
// КЛАСС МЕНЕДЖЕРА ПРОВИЗИОНИНГА
// ============================================================================

/**
 * @class ProvisioningManager
 * @brief Синглтон-менеджер процесса провизионинга
 */
class ProvisioningManager {
 public:
  /**
   * @brief Получить экземпляр синглтона
   */
  static ProvisioningManager& getInstance();

  /**
   * @brief Запустить процесс провизионинга
   * @param callback Колбэк при завершении настройки
   * @param userData Пользовательские данные (не используется)
   * @return true — процесс запущен, false — уже запущен
   */
  bool begin(ProvisioningCallback callback = nullptr, void* userData = nullptr);

  /**
   * @brief Обновить состояние процесса
   * @details Должна вызываться в loop()
   */
  void update();

  /**
   * @brief Проверить, активен ли процесс настройки
   */
  bool isActive() const;

  /**
   * @brief Проверить, завершён ли процесс
   */
  bool isCompleted() const;

  /**
   * @brief Получить метод завершения провизионинга
   */
  ProvisioningMethod getCompletedBy() const;

  /**
   * @brief Получить текущее состояние процесса
   */
  ProvisioningState getState() const;

  /**
   * @brief Получить количество неудачных попыток
   */
  int getRetryCount() const;

  /**
   * @brief Получить полученные данные настройки
   */
  const ProvisioningData* getData() const;

  /**
   * @brief Сбросить состояние менеджера
   */
  // void reset();

  /**
   * @brief Обработчик получения данных от BLE
   */
  void onDataReceived(const ProvisioningData& data);

  /**
   * @brief Обработчик статуса BLE-событий
   */
  void onBleStatus(uint8_t status);

 private:
  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;

  void selectProvisioningMethod();
  void startBleProvisioning();
  void startApProvisioning();
  bool isApComplete();

  static constexpr int MAX_RETRIES = 3;

  ProvisioningState _state = ProvisioningState::IDLE;
  ProvisioningMethod _completedBy = ProvisioningMethod::NONE;
  ProvisioningCallback _callback = nullptr;
  void* _userData = nullptr;
  bool _started = false;
  bool _apStarted = false;
  int _retryCount = 0;
  ProvisioningData _data;
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ-ОБЁРТКИ
// ============================================================================

void startProvisioning();
bool isProvisioningComplete();
ProvisioningMethod getProvisioningMethod();
ProvisioningState getProvisioningState();
int getProvisioningRetryCount();

#endif  // PROVISIONING_H