/**
 * @file provisioning_manager.h
 * @brief Менеджер комиссионинга (первоначальная настройка)
 * @details Управляет процессом первоначальной настройки устройства:
 *          - AP-режим (WiFi точка доступа) — со своим HTTP-сервером
 *          - BLE-режим (ESP BLE Provisioning)
 *          - Приём WiFi-учётных данных от пользователя
 */

#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>
#include "logger.h"
#include "settings.h"

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
// ТИПЫ ДАННЫХ
// ============================================================================

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

/**
 * @brief Колбэк при завершении провизионинга
 * @param success true — данные получены, false — ошибка
 * @param context Контекст, переданный при регистрации
 */
typedef void (*ProvisioningCallback)(bool success, void* context);

// ============================================================================
// ПРОВИЗИОНИНГ (реализация или заглушки)
// ============================================================================

#if FEATURE_PROVISIONING_ENABLED == 1

// ============================================================================
// РЕАЛИЗАЦИЯ: PROVISIONING ВКЛЮЧЁН
// ============================================================================

/**
 * @brief Менеджер провизионинга (синглтон)
 * @details Управляет процессом первоначальной настройки:
 *          - Устанавливает STATE_PROVISIONING при запуске
 *          - Принимает данные через AP (HTTP) или BLE (события)
 *          - Предоставляет данные через getProvisioningData()
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
   * @param callback Колбэк при получении данных (success=true) или ошибке
   * @param context Контекст для колбэка
   * @return true — успешно запущен, false — уже запущен
   */
  bool begin(ProvisioningCallback callback = nullptr, void* context = nullptr);

  /**
   * @brief Периодическая обработка (вызывается в loop)
   * @details Для AP: обновляет HTTP-сервер (обрабатывает запросы)
   *          Для BLE: ничего не делает (работает через события)
   */
  void update();

  /**
   * @brief Получить данные провизионинга
   * @return Указатель на данные или nullptr, если данные не получены
   * @note После вызова состояние сбрасывается (данные возвращаются один раз)
   */
  const ProvisioningData* getProvisioningData();

  /**
   * @brief Обработать полученные от пользователя данные
   * @param data Данные провизионинга
   * @details Вызывается из AP-обработчика или BLE-колбэка
   */
  void onDataReceived(const ProvisioningData& data);

  /**
   * @brief Обработать статус BLE-провизионинга
   * @param status Статус из arduino_event_t
   */
  void onBleStatus(uint8_t status);

 private:
  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;

  /**
   * @brief Выбрать метод провизионинга на основе PROVISIONING_METHOD
   */
  void selectProvisioningMethod();

#if USE_BLE_PROVISIONING == 1
  /**
   * @brief Запустить BLE-провизионинг
   */
  void startBleProvisioning();
#endif

#if USE_AP_PROVISIONING == 1
  /**
   * @brief Запустить AP-провизионинг (точка доступа + HTTP-сервер)
   */
  void startApProvisioning();

  /**
   * @brief Проверить, завершён ли AP-провизионинг (есть ли данные)
   * @return true — данные получены
   */
  bool isApComplete() const;
#endif

  static constexpr int MAX_RETRIES = 3; /**< Максимум попыток BLE */

  bool _started = false;                    /**< Флаг: провизионинг запущен */
  int _retryCount = 0;                      /**< Счётчик попыток BLE */
  ProvisioningData _data;                   /**< Данные от пользователя */
  ProvisioningCallback _callback = nullptr; /**< Колбэк завершения */
  void* _context = nullptr;                 /**< Контекст колбэка */

#if USE_AP_PROVISIONING == 1
  bool _apStarted = false; /**< Флаг: AP-режим запущен */
#endif
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ (обёртки для оркестратора)
// ============================================================================

/**
 * @brief Запустить провизионинг
 * @details Удобная обёртка вокруг ProvisioningManager::begin()
 */
void startProvisioning();

/**
 * @brief Обновить состояние провизионинга
 * @details Удобная обёртка вокруг ProvisioningManager::update()
 *          Вызывается из оркестратора в loop()
 */
void provisioning_update();

/**
 * @brief Получить данные провизионинга
 * @return Указатель на данные или nullptr
 * @details Удобная обёртка вокруг ProvisioningManager::getProvisioningData()
 */
const ProvisioningData* getProvisioningData();

// ============================================================================
// ФУНКЦИИ AP-ПРОВИЗИОНИНГА (HTTP-обработчики)
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
 * @brief Заглушка — AP-провизионинг отключён (USE_AP_PROVISIONING == 0)
 */
inline void provisioning_send_ap_page(void* server) {
  (void)server;
}

/**
 * @brief Заглушка — AP-провизионинг отключён (USE_AP_PROVISIONING == 0)
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
 * (FEATURE_PROVISIONING_ENABLED == 0)
 */
class ProvisioningManager {
 public:
  /**
   * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
   * @return Ссылка на статический экземпляр-заглушку
   */
  static ProvisioningManager& getInstance() {
    static ProvisioningManager instance;
    return instance;
  }

  /**
   * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
   * @return false (запуск невозможен)
   */
  inline bool begin(ProvisioningCallback = nullptr, void* = nullptr) {
    return false;
  }

  /**
   * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
   */
  inline void update() {
    // Пусто
  }

  /**
   * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
   * @return nullptr
   */
  inline const ProvisioningData* getProvisioningData() { return nullptr; }

  /**
   * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
   */
  inline void onDataReceived(const ProvisioningData&) {
    // Пусто
  }

  /**
   * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
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
 * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
 */
inline void startProvisioning() {
  XLOG_WARN(CAT_PROVISIONING,
            "Provisioning is disabled (PROVISIONING_METHOD=0)");
}

/**
 * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
 */
inline void provisioning_update() {
  // Пусто
}

/**
 * @brief Заглушка — провизионинг отключён (FEATURE_PROVISIONING_ENABLED == 0)
 * @return nullptr
 */
inline const ProvisioningData* getProvisioningData() {
  return nullptr;
}

#endif  // FEATURE_PROVISIONING_ENABLED

#endif  // PROVISIONING_H