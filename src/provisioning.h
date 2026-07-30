/**
 * @file provisioning.h
 * @brief Менеджер комиссионинга (первоначальная настройка)
 * @details Управляет процессом первоначальной настройки устройства:
 *          - AP-режим (WiFi точка доступа)
 *          - BLE-режим (ESP BLE Provisioning)
 *          - Приём WiFi-учётных данных от пользователя
 *          - Управление флагом STATE_PROVISIONING
 */

#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>
#include "settings.h"
#include "logger.h"

// ============================================================================
// НАСТРОЙКИ ПРОВИЗИОНИНГА
// ============================================================================

/**
 * @brief Метод провизионинга
 * @values:
 *   - 0: Нет
 *   - 1: Только BLE
 *   - 2: Только AP (точка доступа)
 *   - 3: BLE + AP (одновременно)
 */
#ifndef PROVISIONING_METHOD
#if PLATFORM_ESP8266
#define PROVISIONING_METHOD 2
#else
#define PROVISIONING_METHOD 3
#endif
#endif

/**
 * @brief Флаг: провизионинг включён
 */
#if PROVISIONING_METHOD == 0
#define FEATURE_PROVISIONING_ENABLED 0
#else
#define FEATURE_PROVISIONING_ENABLED 1
#endif

/**
 * @brief Флаги использования методов провизионинга
 */
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
// ТИПЫ ДАННЫХ (всегда определены)
// ============================================================================

/**
 * @brief Метод, которым был завершён провизионинг
 */
enum class ProvisioningMethod : uint8_t {
  NONE = 0, /**< Не завершён */
#if FEATURE_PROVISIONING_ENABLED == 1
  WIFI = 1, /**< WiFi (AP или BLE) */
#endif
#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
  ZIGBEE = 2, /**< Zigbee */
#endif
#if FEATURE_MATTER_ENABLED == 1
  MATTER = 3, /**< Matter */
#endif
  FAILED = 4 /**< Ошибка провизионинга */
};

/**
 * @brief Состояние процесса провизионинга
 */
enum class ProvisioningState : uint8_t {
  IDLE = 0,      /**< Не запущен */
  ACTIVE = 1,    /**< Активен (ожидает ввода) */
  COMPLETED = 2, /**< Завершён успешно */
  FAILED = 3     /**< Ошибка */
};

/**
 * @brief Данные, полученные от пользователя
 */
struct ProvisioningData {
  uint8_t type; /**< 0 = WiFi, 1 = Zigbee, 2 = Matter */

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  char wifiSsid[32];     /**< Имя WiFi сети */
  char wifiPassword[64]; /**< Пароль WiFi */
#endif

#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
  uint16_t zigbeePanId;      /**< PAN ID Zigbee сети */
  uint8_t zigbeeChannel;     /**< Канал Zigbee (11-26) */
  char zigbeeNetworkKey[32]; /**< Сетевой ключ Zigbee */
#endif

#if FEATURE_MATTER_ENABLED == 1
  char matterSetupCode[32]; /**< Код настройки Matter */
#endif
};

// ============================================================================
// ТИПЫ КОЛБЭКОВ
// ============================================================================

/**
 * @brief Колбэк при завершении провизионинга
 */
typedef void (*ProvisioningCallback)(ProvisioningMethod method, void* context);

// ============================================================================
// ПРОВИЗИОНИНГ (реализация или заглушки)
// ============================================================================

#if FEATURE_PROVISIONING_ENABLED == 1

// ============================================================================
// РЕАЛИЗАЦИЯ: PROVISIONING ВКЛЮЧЁН
// ============================================================================

/**
 * @brief Менеджер провизионинга (синглтон)
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
   * @param callback Колбэк при завершении (может быть nullptr)
   * @param context Контекст для колбэка
   * @return true — успешно запущен, false — уже запущен
   */
  bool begin(ProvisioningCallback callback = nullptr, void* context = nullptr);

  /**
   * @brief Периодическая обработка (вызывается в loop)
   */
  void update();

  /**
   * @brief Проверить, завершён ли провизионинг
   * @return true — завершён (успешно или с ошибкой)
   */
  bool isCompleted() const;

  /**
   * @brief Получить метод, которым завершён провизионинг
   */
  ProvisioningMethod getCompletedBy() const;

  /**
   * @brief Получить текущее состояние провизионинга
   */
  ProvisioningState getState() const;

  /**
   * @brief Получить количество попыток провизионинга
   */
  int getRetryCount() const;

  /**
   * @brief Получить данные, полученные от пользователя
   * @return Указатель на ProvisioningData или nullptr
   */
  const ProvisioningData* getData() const;

  /**
   * @brief Обработать полученные от пользователя данные
   * @param data Данные провизионинга
   */
  void onDataReceived(const ProvisioningData& data);

  /**
   * @brief Обработать статус BLE-провизионинга
   * @param status Статус из arduino_event_t
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

  void selectProvisioningMethod();

#if USE_BLE_PROVISIONING == 1
  void startBleProvisioning();
#endif

#if USE_AP_PROVISIONING == 1
  void startApProvisioning();
  bool isApComplete() const;
#endif

  // ========================================================================
  // ДАННЫЕ
  // ========================================================================

  static constexpr int MAX_RETRIES = 3;

  ProvisioningState _state = ProvisioningState::IDLE;
  ProvisioningMethod _completedBy = ProvisioningMethod::NONE;
  ProvisioningCallback _callback = nullptr;
  void* _context = nullptr;
  bool _started = false;

#if USE_AP_PROVISIONING == 1
  bool _apStarted = false;
#endif

  int _retryCount = 0;
  ProvisioningData _data;
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ (для удобства вызова из оркестратора)
// ============================================================================

/**
 * @brief Запустить провизионинг
 */
void startProvisioning();

/**
 * @brief Проверить, завершён ли провизионинг
 * @return true — завершён
 */
bool isProvisioningComplete();

/**
 * @brief Получить метод завершения провизионинга
 */
ProvisioningMethod getProvisioningMethod();

/**
 * @brief Получить текущее состояние провизионинга
 */
ProvisioningState getProvisioningState();

/**
 * @brief Получить количество попыток провизионинга
 */
int getProvisioningRetryCount();

/**
 * @brief Получить данные провизионинга
 */
const ProvisioningData* getProvisioningData();

// ============================================================================
// ФУНКЦИИ AP-ПРОВИЗИОНИНГА (вызываются из Web)
// ============================================================================

#if USE_AP_PROVISIONING == 1

// Предварительное объявление WebServerClass
#if defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif

/**
 * @brief Отправить HTML-страницу AP-провизионинга
 * @param server Указатель на веб-сервер
 */
void provisioning_send_ap_page(WebServerClass* server);

/**
 * @brief Обработать POST-запрос сохранения WiFi-настроек
 * @param server Указатель на веб-сервер
 */
void provisioning_handle_ap_save(WebServerClass* server);

#else  // USE_AP_PROVISIONING == 0

// ============================================================================
// ЗАГЛУШКИ AP-ФУНКЦИЙ
// ============================================================================

/**
 * @brief Заглушка — AP-провизионинг отключён
 */
inline void provisioning_send_ap_page(void* server) {
  (void)server;
}

/**
 * @brief Заглушка — AP-провизионинг отключён
 */
inline void provisioning_handle_ap_save(void* server) {
  (void)server;
}

#endif  // USE_AP_PROVISIONING

// ============================================================================
// ЗАГЛУШКИ: PROVISIONING ВЫКЛЮЧЁН
// ============================================================================

#else  // FEATURE_PROVISIONING_ENABLED == 0

// ============================================================================
// КЛАСС — ЗАГЛУШКА
// ============================================================================

/**
 * @brief Заглушка — менеджер провизионинга отключён
 */
class ProvisioningManager {
 public:
  /**
   * @brief Заглушка — провизионинг отключён
   * @return Ссылка на статический экземпляр-заглушку
   */
  static ProvisioningManager& getInstance() {
    static ProvisioningManager instance;
    return instance;
  }

  /**
   * @brief Заглушка — провизионинг отключён
   * @return false (запуск невозможен)
   */
  inline bool begin(ProvisioningCallback = nullptr, void* = nullptr) {
    return false;
  }

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline void update() {
    // Пусто
  }

  /**
   * @brief Заглушка — провизионинг отключён
   * @return true (считаем завершённым, чтобы оркестратор не ждал)
   */
  inline bool isCompleted() const { return true; }

  /**
   * @brief Заглушка — провизионинг отключён
   * @return ProvisioningMethod::NONE
   */
  inline ProvisioningMethod getCompletedBy() const {
    return ProvisioningMethod::NONE;
  }

  /**
   * @brief Заглушка — провизионинг отключён
   * @return ProvisioningState::IDLE
   */
  inline ProvisioningState getState() const { return ProvisioningState::IDLE; }

  /**
   * @brief Заглушка — провизионинг отключён
   * @return 0
   */
  inline int getRetryCount() const { return 0; }

  /**
   * @brief Заглушка — провизионинг отключён
   * @return nullptr
   */
  inline const ProvisioningData* getData() const { return nullptr; }

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline void onDataReceived(const ProvisioningData&) {
    // Пусто
  }

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline void onBleStatus(uint8_t) {
    // Пусто
  }

 private:
  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ — ЗАГЛУШКИ
// ============================================================================

/**
 * @brief Заглушка — провизионинг отключён
 * @note В реальной реализации: XLOG_INFO(CAT_PROVISIONING, "Starting AP + BLE
 * provisioning") и т.д.
 */
inline void startProvisioning() {
  XLOG_WARN(CAT_PROVISIONING,
            "Provisioning is disabled (PROVISIONING_METHOD=0)");
}

/**
 * @brief Заглушка — провизионинг отключён
 * @return true (считаем завершённым, чтобы оркестратор не ждал)
 */
inline bool isProvisioningComplete() {
  return true;
}

/**
 * @brief Заглушка — провизионинг отключён
 * @return ProvisioningMethod::NONE
 */
inline ProvisioningMethod getProvisioningMethod() {
  return ProvisioningMethod::NONE;
}

/**
 * @brief Заглушка — провизионинг отключён
 * @return ProvisioningState::IDLE
 */
inline ProvisioningState getProvisioningState() {
  return ProvisioningState::IDLE;
}

/**
 * @brief Заглушка — провизионинг отключён
 * @return 0
 */
inline int getProvisioningRetryCount() {
  return 0;
}

/**
 * @brief Заглушка — провизионинг отключён
 * @return nullptr
 */
inline const ProvisioningData* getProvisioningData() {
  return nullptr;
}

// ============================================================================
// AP-ФУНКЦИИ — ЗАГЛУШКИ
// ============================================================================

/**
 * @brief Заглушка — AP-провизионинг отключён
 */
inline void provisioning_send_ap_page(void* server) {
  (void)server;
}

/**
 * @brief Заглушка — AP-провизионинг отключён
 */
inline void provisioning_handle_ap_save(void* server) {
  (void)server;
}

#endif  // FEATURE_PROVISIONING_ENABLED

#endif  // PROVISIONING_H