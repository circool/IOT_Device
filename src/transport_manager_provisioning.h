/**
 * @file transport_manager_provisioning.h
 * @brief Провизионинг — внутренний компонент WiFi-транспорта
 */

#ifndef TRANSPORT_MANAGER_PROVISIONING_H
#define TRANSPORT_MANAGER_PROVISIONING_H

#include <Arduino.h>
#include "config_manager.h"
#include "logger.h"
#include "settings.h"
#include "transport_types.h"

// Предобъявление класса WebManager
class WebManager;

// ============================================================================
// PROVISIONING MANAGER (с заглушками)
// ============================================================================

#if defined(USE_AP) || defined(USE_BLE)

// ===== РЕАЛЬНАЯ РЕАЛИЗАЦИЯ (USE_AP или USE_BLE определён) =====

/**
 * @brief Менеджер провизионинга (внутренний компонент транспорта)
 * @details Управляет AP и BLE серверами для получения WiFi учётных данных.
 *          Автоматически переключается между методами при ошибках.
 */
class ProvisioningManager {
 public:
  ProvisioningManager();

  /**
   * @brief Запустить провизионинг
   * @param deviceId ID устройства (используется как SSID для AP)
   * @param callback Колбэк для событий (TransportEventCallback)
   * @param context Контекст для колбэка
   * @param webManager Указатель на WebManager (для управления режимом AP)
   * @return true — успешно запущен
   */
  bool begin(const char* deviceId,
             TransportEventCallback callback,
             void* context,
             WebManager* webManager = nullptr);

  /**
   * @brief Периодическая обработка (вызывается в loop)
   */
  void update();

  /**
   * @brief Остановить провизионинг
   */
  void stop();

  /**
   * @brief Активен ли провизионинг
   */
  bool isActive() const;

  /**
   * @brief Получить последнюю ошибку
   */
  const char* getLastError() const;

  /**
   * @brief Обработчик получения данных от BLE/AP серверов
   * @param ssid WiFi SSID
   * @param password WiFi пароль
   */
  void onDataReceived(const char* ssid, const char* password);

  /**
   * @brief Обработчик ошибки BLE
   */
  void onBleError();

 private:
  // ===== ВНУТРЕННИЕ МЕТОДЫ =====
  bool startAP();
  bool startBLE();
  void stopAP();
  void stopBLE();
  void switchToNextMethod();
  void onError();

  /**
   * @brief Проверить, активен ли AP-режим
   */
  bool isApRunning() const { return _apRunning; }

  // ===== ДАННЫЕ =====
  char _deviceId[32];
  TransportEventCallback _callback;
  void* _context;

  bool _active;
  bool _apRunning;
  bool _bleRunning;
  int _retryCount;
  int _currentMethod;  // 0 = AP, 1 = BLE

  bool _resultReceived;
  char _lastError[64];

  // ===== УКАЗАТЕЛЬ НА WEB MANAGER =====
  WebManager* _webManager; /**< Для переключения режима AP */
};

#else  // USE_AP и USE_BLE не определены

// ============================================================================
// ЗАГЛУШКИ (провизионинг полностью отключён)
// ============================================================================

/**
 * @brief Заглушка ProvisioningManager — провизионинг отключён
 * @details Все методы — пустые заглушки. Используется при PROVISIONING_METHOD
 * == 0
 */
class ProvisioningManager {
 public:
  ProvisioningManager() {}

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline bool begin(const char* deviceId,
                    TransportEventCallback callback,
                    void* context,
                    WebManager* webManager = nullptr) {
    (void)deviceId;
    (void)callback;
    (void)context;
    (void)webManager;
    return false;
  }

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline void update() {}

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline void stop() {}

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline bool isActive() const { return false; }

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline const char* getLastError() const { return "Provisioning disabled"; }

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline void onDataReceived(const char* ssid, const char* password) {
    (void)ssid;
    (void)password;
  }

  /**
   * @brief Заглушка — провизионинг отключён
   */
  inline void onBleError() {}

 private:
  inline bool isApRunning() const { return false; }
};

#endif  // defined(USE_AP) || defined(USE_BLE)

#endif  // TRANSPORT_MANAGER_PROVISIONING_H