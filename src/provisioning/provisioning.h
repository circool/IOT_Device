/**
 * @file provisioning.h
 * @brief Управление процессом первоначальной настройки (комиссионинга)
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
 * @brief Структура конфигурации для провизионинга
 * @note Передаётся из оркестратора, не содержит зависимостей
 */
struct ProvisioningConfig {
  const char* deviceName;   // Имя устройства (для BLE и AP)
  const char* apIpAddress;  // IP адрес AP
  const char* blePin;       // PIN для BLE
};

/**
 * @brief Режим провизионинга
 */
enum class ProvisioningMode : uint8_t {
  NONE = 0,
  BLE = 1,
  AP = 2,
  BOTH = 3,
  COMPLETED = 4
};

/**
 * @brief Результат провизионинга
 *
 * @note BLE через WiFiProv передаёт ТОЛЬКО WiFi креденшелы
 * @note MQTT и Zigbee настраиваются через AP + Web
 */
struct ProvisioningResult {
  bool success;           // Успешно ли
  char wifiSsid[32];      // Полученный SSID (из BLE)
  char wifiPassword[64];  // Полученный пароль (из BLE)
};

/**
 * @brief Колбэк при завершении провизионинга
 * @param result Результат провизионинга
 * @param userData Пользовательские данные
 */
using ProvisioningCallback =
    std::function<void(const ProvisioningResult* result, void* userData)>;

/**
 * @brief Колбэк для обновления web сервера (AP режим)
 * @note Передаётся из оркестратора
 */
using WebUpdateCallback = std::function<void()>;

// ============================================================================
// КЛАСС PROVISIONING MANAGER
// ============================================================================

/**
 * @brief Менеджер провизионинга
 *
 * @details Только управляет BLE/AP. Не знает о ConfigManager.
 *          Получает все данные через параметры.
 *          Возвращает результат через колбэк.
 */
class ProvisioningManager {
 public:
  static ProvisioningManager& getInstance();

  /**
   * @brief Запустить провизионинг
   * @param mode Режим (BLE, AP или BOTH)
   * @param config Конфигурация провизионинга
   * @param webUpdateCallback Колбэк для обновления web (AP режим)
   * @param callback Колбэк при завершении
   * @param userData Пользовательские данные
   * @return true — успешно запущен
   */
  bool begin(ProvisioningMode mode,
             const ProvisioningConfig& config,
             WebUpdateCallback webUpdateCallback,
             ProvisioningCallback callback = nullptr,
             void* userData = nullptr);

  /**
   * @brief Периодическая обработка провизионинга
   * @note Вызывается из loop() оркестратором
   */
  void update();

  bool isActive() const;
  bool isCompleted() const;
  ProvisioningMode getMode() const;
  void reset();

 private:
  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;

  enum class InternalState : uint8_t { IDLE, WAITING, COMPLETED, ERROR };

  void startBleProvisioning();
  void startApProvisioning();
  void onBleConfigReceived(const void* data, size_t size);

  InternalState _state = InternalState::IDLE;
  ProvisioningMode _mode = ProvisioningMode::NONE;
  ProvisioningCallback _callback = nullptr;
  WebUpdateCallback _webUpdateCallback = nullptr;
  void* _userData = nullptr;
  bool _completed = false;
  bool _bleCompleted = false;
  bool _apCompleted = false;

  ProvisioningConfig _config;
  ProvisioningResult _result;

  void* _bleServer = nullptr;  // Opaque pointer
};

// ============================================================================
// ФУНКЦИИ-ОБЁРТКИ ДЛЯ MAIN
// ============================================================================

void startProvisioning(const ProvisioningConfig& config,
                       WebUpdateCallback webUpdateCallback,
                       ProvisioningCallback callback = nullptr,
                       void* userData = nullptr);

bool isProvisioningComplete();

#endif  // PROVISIONING_H