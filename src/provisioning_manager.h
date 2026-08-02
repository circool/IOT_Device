/**
 * @file provisioning_manager.h
 * @brief Менеджер провизионинга (BLE + AP)
 * @details Управляет процессом первоначальной настройки WiFi.
 *          Поддерживает BLE и AP методы.
 */

#ifndef PROVISIONING_MANAGER_H
#define PROVISIONING_MANAGER_H

#include <Arduino.h>
#include "settings.h"

// ============================================================================
// ТИПЫ ДАННЫХ
// ============================================================================

/**
 * @brief Структура данных, полученных от провизионинга
 */
typedef struct {
  uint8_t type;          /**< 0 — WiFi */
  char wifiSsid[32];     /**< SSID WiFi сети */
  char wifiPassword[64]; /**< Пароль WiFi сети */
} ProvisioningData;

/**
 * @brief Колбэк завершения провизионинга
 * @param success true — успех, false — ошибка
 * @param context Контекст (пользовательские данные)
 */
typedef void (*ProvisioningCallback)(bool success, void* context);

// ============================================================================
// API
// ============================================================================

#if PROVISIONING_METHOD != 0

class ProvisioningManager {
 public:
  /**
   * @brief Получить экземпляр менеджера (синглтон)
   */
  static ProvisioningManager& getInstance();

  /**
   * @brief Запустить процесс провизионинга
   * @param callback Колбэк по завершении
   * @param context Контекст для колбэка
   * @return true при успешном запуске
   */
  bool begin(ProvisioningCallback callback = nullptr, void* context = nullptr);

  /**
   * @brief Периодическая обработка (вызывается в loop)
   */
  void update();

  /**
   * @brief Получить данные провизионинга
   * @return Указатель на ProvisioningData или nullptr
   */
  const ProvisioningData* getProvisioningData();

  /**
   * @brief Обработчик получения данных
   * @param data Полученные данные
   */
  void onDataReceived(const ProvisioningData& data);

  /**
   * @brief Обработчик статуса BLE
   * @param status Код статуса
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

  ProvisioningCallback _callback = nullptr;
  void* _context = nullptr;
  ProvisioningData _data;
  bool _started = false;
  int _retryCount = 0;
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ФУНКЦИИ (обёртки для оркестратора)
// ============================================================================

/**
 * @brief Запустить провизионинг
 * @details Обёртка для ProvisioningManager::begin()
 */
void startProvisioning();

/**
 * @brief Обновление состояния провизионинга
 * @details Обёртка для ProvisioningManager::update()
 */
void provisioning_update();

/**
 * @brief Получить данные провизионинга
 * @details Обёртка для ProvisioningManager::getProvisioningData()
 */
const ProvisioningData* getProvisioningData();

#else  // PROVISIONING_METHOD == 0

// ============================================================================
// ЗАГЛУШКИ
// ============================================================================

// Заглушка — провизионинг отключён (PROVISIONING_METHOD=0)
inline void startProvisioning() {}

// Заглушка — провизионинг отключён (PROVISIONING_METHOD=0)
inline void provisioning_update() {}

// Заглушка — провизионинг отключён (PROVISIONING_METHOD=0)
inline const ProvisioningData* getProvisioningData() {
  return nullptr;
}

#endif  // PROVISIONING_METHOD != 0

#endif  // PROVISIONING_MANAGER_H