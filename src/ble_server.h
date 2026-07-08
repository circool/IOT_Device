/**
 * @file ble_server.h
 * @brief BLE Provisioning Server для ESP32
 *
 * Реализует BLE-сервер для первоначальной настройки WiFi через
 * протокол ESP BLE Provisioning от Espressif.
 *
 * @note Доступно только на ESP32 (не ESP8266)
 * @note BLE может передать только WiFi SSID и пароль
 * @see
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/
 */

#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>
#include <functional>

/**
 * @brief Структура WiFi-настроек, получаемых через BLE
 * @note BLE Provisioning передаёт только SSID и пароль
 */
struct BleWifiConfig {
  char wifiSsid[32];      //!< Имя WiFi сети (SSID)
  char wifiPassword[64];  //!< Пароль WiFi
};

/**
 * @brief Колбэк при получении конфигурации через BLE
 * @param config Указатель на полученный WiFi конфиг (nullptr при ошибке)
 */
using ProvConfigCallback = std::function<void(const BleWifiConfig* config)>;

/**
 * @brief Колбэк при изменении статуса провизионинга
 * @param status Код статуса
 */
using ProvStatusCallback = std::function<void(uint8_t status)>;

/**
 * @brief Колбэк при изменении состояния BLE-соединения
 * @param connected true — клиент подключился, false — отключился
 */
using BleConnectionCallback = std::function<void(bool connected)>;

#if defined(ESP32) && !defined(ESP8266)

/**
 * @brief Класс BLE-сервера для провизионинга ESP32
 *
 * @details Использует библиотеку WiFiProv от Espressif.
 *          Передаёт только WiFi SSID и пароль.
 *          Не знает про AP — только события через колбэк.
 *
 * @note Только для ESP32 (ESP8266 не поддерживает BLE)
 *
 * @example
 * @code
 * BleProvisioningServer server("my_device");
 * server.begin(
 *     [](const BleWifiConfig* config) {
 *         // Сохранить WiFi настройки
 *     },
 *     nullptr,
 *     [](bool connected) {
 *         // BLE клиент подключился/отключился
 *     }
 * );
 * @endcode
 */
class BleProvisioningServer {
 public:
  /**
   * @brief Конструктор
   * @param deviceName Имя устройства (отображается в BLE)
   * @note Если deviceName не указан, используется "PROV_123"
   */
  explicit BleProvisioningServer(const char* deviceName = nullptr);

  /**
   * @brief Деструктор
   * @note Автоматически останавливает BLE-сервер
   */
  ~BleProvisioningServer();

  /**
   * @brief Запустить BLE-сервер провизионинга
   *
   * @param configCallback Колбэк при получении WiFi конфигурации
   * @param statusCallback Колбэк при изменении статуса (опционально)
   * @param connCallback Колбэк при подключении/отключении клиента (опционально)
   *
   * @return true — сервер запущен, false — ошибка
   *
   * @note Использует PIN из BLE_PROVISIONING_PIN (по умолчанию "12345678")
   * @note При успешном запуске устройство становится видимым в BLE
   */
  bool begin(ProvConfigCallback configCallback,
             ProvStatusCallback statusCallback = nullptr,
             BleConnectionCallback connCallback = nullptr);

  /**
   * @brief Остановить BLE-сервер
   * @note Освобождает ресурсы и удаляет обработчики событий
   */
  void stop();

  /**
   * @brief Проверить активность BLE-сервера
   * @return true — сервер активен
   */
  bool isActive() const;

  /**
   * @brief Получить имя устройства
   * @deprecated Не испольхуется
   * @return Строка с именем устройства
   */
  const char* getDeviceName() const;

  /**
   * @brief Установить имя устройства
   * @deprecated Не используется
   * @param name Новое имя устройства (максимум 31 символ)
   */
  void setDeviceName(const char* name);

 private:
  bool _active = false;  //!< Флаг активности сервера
  char _deviceName[32];  //!< Имя устройства
};

#else  // ESP8266 или другая платформа

/**
 * @brief Заглушка BLE-сервера для платформ без BLE
 * @note Все методы возвращают false или пустые значения
 */
class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr) {
    (void)deviceName;
  }
  ~BleProvisioningServer() {}

  bool begin(ProvConfigCallback,
             ProvStatusCallback = nullptr,
             BleConnectionCallback = nullptr) {
    return false;
  }
  void stop() {}
  bool isActive() const { return false; }
  const char* getDeviceName() const { return "No BLE"; }
  void setDeviceName(const char* name) { (void)name; }
};

#endif  // ESP32

#endif  // BLE_SERVER_H