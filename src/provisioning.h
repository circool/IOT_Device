/**
 * @file provisioning.h
 * @brief Управление процессом первоначальной настройки (комиссионинга)
 * @details Менеджер провизионинга управляет BLE и AP методами настройки.
 *          При ошибках считает попытки и при исчерпании переходит в FAILED.
 * @note Для embedded: колбэки — указатели на функции вместо std::function
 *       (экономия RAM ~32 байта)
 */

#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>

#ifndef USE_BLE_PROVISIONING
#endif

/**
 * @brief Режим провизионинга
 * @values
 *   - 0: Нет (отключён) — для ZigBee или заводской прошивки
 *   - 1: Только BLE
 *   - 2: Только AP (WiFi точка доступа + веб-интерфейс)
 *   - 3: BLE + AP (комбинированный)
 * @details
 *   - ESP8266 (без BLE) → автоматически 2 (AP)
 *   - Остальные платформы → автоматически 3 (BLE+AP)
 *   - При TRANSPORT_TYPE=1 (ZigBee) принудительно устанавливается в 0
 * @note Производные флаги USE_BLE_PROVISIONING и USE_AP_PROVISIONING
 *       вычисляются в provisioning.h на основе этого значения
 */
#ifndef PROVISIONING_METHOD
#if PLATFORM_ESP8266
#define PROVISIONING_METHOD 2
#else
#define PROVISIONING_METHOD 3
#endif
#endif

#if PROVISIONING_METHOD == 0 
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 0

#elif PROVISIONING_METHOD == 1
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 0

#elif PROVISIONING_METHOD == 2
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1

#elif PROVISIONING_METHOD == 3
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 1

#endif

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
// КЛАСС МЕНЕДЖЕРА ПРОВИЗИОНИНГА
// ============================================================================

/**
 * @brief Колбэк при завершении провизионинга
 * @param method Метод, которым была выполнена настройка
 * @param context Пользовательский контекст
 */
typedef void (*ProvisioningCallback)(ProvisioningMethod method, void* context);

/**
 * @class ProvisioningManager
 * @brief Синглтон-менеджер процесса провизионинга
 * @details Управляет BLE и AP методами настройки.
 *          Использует легковесные колбэки (указатели на функции).
 * @note Синглтон с прямым доступом — осознанное отступление для embedded
 */
class ProvisioningManager {
 public:
  /**
   * @brief Получить экземпляр синглтона
   * @return Ссылка на единственный экземпляр
   */
  static ProvisioningManager& getInstance();

  /**
   * @brief Запустить процесс провизионинга
   * @param callback Колбэк при завершении настройки
   * @param context Пользовательский контекст (передаётся в колбэк)
   * @return true — процесс запущен, false — уже запущен
   */
  bool begin(ProvisioningCallback callback = nullptr, void* context = nullptr);

  /**
   * @brief Обновить состояние процесса
   * @details Должна вызываться в loop()
   */
  void update();

  /**
   * @brief Проверить, активен ли процесс настройки
   * @return true — активен
   * @deprecated Не используется
   */
  // bool isActive() const;

  /**
   * @brief Проверить, завершён ли процесс
   * @return true — завершён (успешно или с ошибкой)
   */
  bool isCompleted() const;

  /**
   * @brief Получить метод завершения провизионинга
   * @return Метод (BLE/AP/FAILED/NONE)
   */
  ProvisioningMethod getCompletedBy() const;

  /**
   * @brief Получить текущее состояние процесса
   * @return Состояние (IDLE/ACTIVE/COMPLETED/FAILED)
   */
  ProvisioningState getState() const;

  /**
   * @brief Получить количество неудачных попыток
   * @return Количество попыток (0..MAX_RETRIES)
   */
  int getRetryCount() const;

  /**
   * @brief Получить полученные данные настройки
   * @return Указатель на структуру с данными
   */
  const ProvisioningData* getData() const;

  /**
   * @brief Обработчик получения данных от BLE
   * @param data Полученные данные (SSID и пароль)
   */
  void onDataReceived(const ProvisioningData& data);

  /**
   * @brief Обработчик статуса BLE-событий
   * @param status Код статуса (из ARDUINO_EVENT_PROV_*)
   */
  void onBleStatus(uint8_t status);

 private:
  // ========================================================================
  // КОНСТРУКТОРЫ (закрытые для синглтона)
  // ========================================================================

  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;

  // ========================================================================
  // ВНУТРЕННИЕ МЕТОДЫ
  // ========================================================================

  void selectProvisioningMethod();  ///< Выбор метода (BLE или AP)
  void startBleProvisioning();      ///< Запуск BLE-комиссионинга
  void startApProvisioning();       ///< Запуск AP-комиссионинга (веб)
  bool isApComplete();              ///< Проверка завершения AP-настройки

  static constexpr int MAX_RETRIES = 3;  ///< Максимум попыток BLE

  // ========================================================================
  // ДАННЫЕ
  // ========================================================================

  ProvisioningState _state = ProvisioningState::IDLE;
  ProvisioningMethod _completedBy = ProvisioningMethod::NONE;
  ProvisioningCallback _callback = nullptr;  ///< Колбэк (указатель на функцию)
  void* _context = nullptr;                  ///< Контекст для колбэка
  bool _started = false;                     ///< Флаг запуска
  bool _apStarted = false;                   ///< Флаг запуска AP
  int _retryCount = 0;                       ///< Счётчик неудачных попыток
  ProvisioningData _data;                    ///< Полученные данные
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ-ОБЁРТКИ
// ============================================================================

/**
 * @brief Запустить процесс провизионинга
 * @details Использует синглтон ProvisioningManager
 */
void startProvisioning();

/**
 * @brief Проверить, завершён ли провизионинг
 * @return true — завершён (успешно или с ошибкой)
 */
bool isProvisioningComplete();

/**
 * @brief Получить метод завершения провизионинга
 * @return Метод (BLE/AP/FAILED/NONE)
 */
ProvisioningMethod getProvisioningMethod();

/**
 * @brief Получить состояние провизионинга
 * @return Текущее состояние
 */
ProvisioningState getProvisioningState();

/**
 * @brief Получить количество неудачных попыток
 * @return Количество попыток
 */
int getProvisioningRetryCount();

#endif  // PROVISIONING_H