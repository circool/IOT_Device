/**
 * @file transport_wifi_web.h
 * @version 0.12
 * @brief Web-менеджер — HTTP-сервер для управления и настройки
 */

#ifndef transport_wifi_web_H
#define transport_wifi_web_H

#include <Arduino.h>
#include "common_types.h"
#include "logger.h"
#include "settings.h"
#include "transport_types.h"
#include "web_common.h"
#include "web_ota_manager.h"

// ============================================================================
// Web-МЕНЕДЖЕР
// ============================================================================

#ifdef USE_WEB

/**
 * @brief Web-менеджер — HTTP-сервер для управления и настройки
 * @details WebManager НЕ хранит кэш данных. Все данные читаются по указателям,
 *          переданным при инициализации. RSSI обновляется через setRSSI().
 */
class WebManager {
 public:
  WebManager();

  // ===== УПРАВЛЕНИЕ =====
  /**
   * @brief Инициализация WebManager с указателями на данные
   * @param transportConfig Указатель на TransportConfig
   * @param deviceConfig Указатель на DeviceConfig
   * @param deviceState Указатель на DeviceState
   * @param transportState Указатель на TransportState
   * @return true — успех, false — ошибка
   */
  bool begin(const TransportConfig* transportConfig,
             const DeviceConfig* deviceConfig,
             const DeviceState* deviceState,
             const TransportState* transportState);
  void update();
  bool isConnected() const;
  void disconnect();
  const char* getName() const;

  // ===== ЗАПУСК СЕРВЕРА =====
  void startServer();
  bool isServerStarted() const { return _serverStarted; }

  // ===== WiFi READY =====
  void setWifiReady(bool ready) { _wifiReady = ready; }
  bool isWifiReady() const { return _wifiReady; }

  // ===== РЕЖИМ AP (провизионинг) =====
  void enableProvisioningMode();
  void disableProvisioningMode();
  bool isProvisioningMode() const;

  // ===== RSSI (только для отображения) =====
  /**
   * @brief Обновить RSSI для отображения на странице
   * @param rssi Уровень сигнала в dBm
   * @note RSSI не хранится в транспорте, читается на лету WiFi.RSSI()
   */
  void setRSSI(int rssi);

  // ===== КОЛБЭКИ =====
  /**
   * @brief Регистрация колбэка для событий
   * @param callback Функция обратного вызова
   * @param context Контекст для колбэка
   */
  void onEvent(TransportEventCallback callback, void* context);

  // ===== ФЛАГИ ДЛЯ ОРКЕСТРАТОРА =====
  volatile bool configPending;
  volatile bool restartPending;
  volatile bool commandPending;
  TransportConfig pendingConfig;
  DeviceConfig pendingDeviceConfig;

  // ===== ДОСТУП К СЕРВЕРУ ДЛЯ AP =====
  WebServerClass* getServer() { return _server; }

 private:
  // ===== ВНУТРЕННИЕ ОБРАБОТЧИКИ =====
  void handleRoot();
  void handleConfig();
  void handleSave();
  void handleSet();
  void handleNotFound();
  void handleSaveWifi();
  void sendConfigPage(const char* errorMsg = nullptr,
                      const char* successMsg = nullptr);
  void sendApProvisioningPage();

  // ===== РЕНДЕРИНГ БЛОКОВ =====
  /**
   * @brief Рендеринг блока настроек устройства
   * @param buf Буфер для записи HTML
   * @param size Размер буфера
   * @note Данные читаются из _deviceConfig
   */
  void renderDeviceSettingsBlock(char* buf, size_t size);

  /**
   * @brief Рендеринг блока подтверждения
   * @param buf Буфер для записи HTML
   * @param size Размер буфера
   */
  void renderConfirmBlock(char* buf, size_t size);

  // ===== ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ =====
  /**
   * @brief Построить страницу состояния
   * @param buf Буфер для записи HTML
   * @param size Размер буфера
   * @note Данные читаются из _deviceState, _transportState
   */
  void buildStatusPage(char* buf, size_t size);

  /**
   * @brief Получить ID устройства
   * @return Строка с ID устройства
   */
  const char* getDeviceId() const;

  /**
   * @brief Настройка маршрутов HTTP-сервера
   */
  void setupRoutes();

  // ===== ДАННЫЕ =====
  WebServerClass* _server;

  // ===== УКАЗАТЕЛИ НА ДАННЫЕ (получаются при инициализации) =====
  const TransportConfig*
      _transportConfig;                   ///< Настройки транспорта (WiFi/MQTT)
  const DeviceConfig* _deviceConfig;      ///< Настройки устройства
  const DeviceState* _deviceState;        ///< Состояние устройства
  const TransportState* _transportState;  ///< Состояние транспорта

  // ===== RSSI (обновляется через setRSSI) =====
  int _rssi;  ///< Уровень сигнала WiFi
  const char* _resetReason;
  // ===== СОСТОЯНИЕ СЕРВЕРА =====
  bool _running;
  bool _connected;
  bool _provisioningMode;
  bool _serverStarted;
  bool _wifiReady;

  // ===== КОЛБЭКИ =====
  TransportEventCallback _eventCallback;
  void* _eventContext;
};

#else  // USE_WEB == 0

// ============================================================================
// ЗАГЛУШКИ (USE_WEB == 0)
// ============================================================================

/**
 * @brief Заглушка WebManager — Web-интерфейс отключён
 * @details Все методы — пустые заглушки. Используется при FEATURE_WEB_ENABLED
 * == 0
 */
class WebManager {
 public:
  WebManager() {}

  // ===== УПРАВЛЕНИЕ =====
  inline bool begin(const TransportConfig* transportConfig,
                    const DeviceConfig* deviceConfig,
                    const DeviceState* deviceState,
                    const TransportState* transportState) {
    (void)transportConfig;
    (void)deviceConfig;
    (void)deviceState;
    (void)transportState;
    return true;
  }
  inline void update() {}
  inline bool isConnected() const { return false; }
  inline void disconnect() {}
  inline const char* getName() const { return "Web (stub)"; }
  const char* _resetReason;
  // ===== ЗАПУСК СЕРВЕРА =====
  inline void startServer() {}
  inline bool isServerStarted() const { return false; }

  // ===== WiFi READY =====
  inline void setWifiReady(bool ready) { (void)ready; }
  inline bool isWifiReady() const { return false; }

  // ===== РЕЖИМ AP =====
  inline void enableProvisioningMode() {}
  inline void disableProvisioningMode() {}
  inline bool isProvisioningMode() const { return false; }

  // ===== RSSI =====
  inline void setRSSI(int rssi) { (void)rssi; }

  // ===== КОЛБЭКИ =====
  inline void onEvent(TransportEventCallback callback, void* context) {
    (void)callback;
    (void)context;
  }

  // ===== ФЛАГИ =====
  volatile bool configPending = false;
  volatile bool restartPending = false;
  volatile bool commandPending = false;
  TransportConfig pendingConfig;
  DeviceConfig pendingDeviceConfig;

  // ===== ДОСТУП К СЕРВЕРУ =====
  inline void* getServer() { return nullptr; }
};

#endif  // USE_WEB

#endif  // transport_wifi_web_H