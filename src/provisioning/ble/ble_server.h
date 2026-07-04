/**
 * @file ble_server.h
 * @brief BLE Provisioning Server для ESP32
 *
 * Реализует BLE-сервер для первоначальной настройки устройства через
 * протокол ESP BLE Provisioning от Espressif.
 *
 * @note Доступно только на ESP32 (не ESP8266)
 * @see
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/
 */

#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>
#include <functional>

/**
 * @brief Структура конфигурации, передаваемая через BLE
 *
 * Содержит все необходимые настройки для подключения устройства:
 * - WiFi (SSID, пароль)
 * - MQTT (брокер, порт, пользователь, пароль, client ID)
 * - Zigbee (ключ, PAN ID, канал)
 * - Режим протокола (MQTT или Zigbee)
 */
struct BleConfigData {
  char wifiSsid[32];          //!< Имя WiFi сети (SSID)
  char wifiPassword[64];      //!< Пароль WiFi
  char mqttBroker[64];        //!< Адрес MQTT брокера
  uint16_t mqttPort;          //!< Порт MQTT брокера
  char mqttUser[32];          //!< Имя пользователя MQTT
  char mqttPassword[64];      //!< Пароль MQTT
  char mqttClientId[24];      //!< Client ID для MQTT
  char zigbeeNetworkKey[32];  //!< Ключ Zigbee сети (16 байт в hex)
  uint16_t zigbeePanId;       //!< PAN ID для Zigbee
  uint8_t zigbeeChannel;      //!< Канал Zigbee (11-26)
  uint8_t protocolMode;       //!< 0 = MQTT, 1 = Zigbee
};

/**
 * @brief Колбэк при получении конфигурации через BLE
 * @param config Указатель на полученную конфигурацию (nullptr при ошибке)
 */
using ProvConfigCallback = std::function<void(const BleConfigData* config)>;

/**
 * @brief Колбэк при изменении статуса провизионинга
 * @param status Код статуса
 */
using ProvStatusCallback = std::function<void(uint8_t status)>;

/**
 * @brief Колбэк проверки подключения клиента
 * @return true если клиент подключён
 */
using ProvConnCallback = std::function<bool()>;

/**
 * @brief Колбэк получения IP-адреса
 * @return Строка с IP-адресом
 */
using ProvIpCallback = std::function<const char*()>;

#if defined(ESP32) && !defined(ESP8266)

/**
 * @brief Класс BLE-сервера для провизионинга ESP32
 *
 * Инкапсулирует настройку и управление BLE-сервером для
 * первоначальной конфигурации устройства через ESP BLE Provisioning.
 *
 * @details Использует библиотеку WiFiProv от Espressif.
 *          Поддерживает передачу WiFi, MQTT и Zigbee настроек.
 *
 * @note Только для ESP32 (ESP8266 не поддерживает BLE)
 *
 * @example
 * @code
 * BleProvisioningServer server("my_device");
 * server.begin([](const BleConfigData* config) {
 *     if (config) {
 *         // Сохранить конфигурацию
 *     }
 * });
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
   * @param configCallback Колбэк при получении конфигурации
   * @param statusCallback Колбэк при изменении статуса (опционально)
   * @param connCallback Колбэк проверки подключения (опционально)
   * @param ipCallback Колбэк получения IP (опционально)
   *
   * @return true — сервер запущен, false — ошибка
   *
   * @note Использует PIN из BLE_PROVISIONING_PIN (по умолчанию "12345678")
   * @note При успешном запуске устройство становится видимым в BLE
   */
  bool begin(ProvConfigCallback configCallback,
             ProvStatusCallback statusCallback = nullptr,
             ProvConnCallback connCallback = nullptr,
             ProvIpCallback ipCallback = nullptr);

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
   * @return Строка с именем устройства
   */
  const char* getDeviceName() const;

  /**
   * @brief Установить имя устройства
   * @param name Новое имя устройства (максимум 31 символ)
   */
  void setDeviceName(const char* name);

  /**
   * @brief Отправить статус клиенту (заглушка)
   * @param status Код статуса
   */
  void sendStatus(uint8_t status);

  /**
   * @brief Отправить статус подключения клиенту (заглушка)
   * @param connected true — подключён
   * @param ip IP-адрес (опционально)
   */
  void sendConnectionStatus(bool connected, const char* ip = nullptr);

  /**
   * @brief Проверить наличие подключённого клиента
   * @return всегда false (заглушка)
   */
  bool hasConnectedClient() const { return false; }

  /**
   * @brief Отправить запрос статуса (заглушка)
   */
  void sendGetStatus() {}

 private:
  bool _active = false;          //!< Флаг активности сервера
  char _deviceName[32];          //!< Имя устройства
  BleConfigData _config;         //!< Текущая конфигурация
  bool _configReceived = false;  //!< Флаг получения конфигурации

  ProvConfigCallback _configCallback;  //!< Колбэк конфигурации
  ProvStatusCallback _statusCallback;  //!< Колбэк статуса
  ProvConnCallback _connCallback;      //!< Колбэк подключения
  ProvIpCallback _ipCallback;          //!< Колбэк IP
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
             ProvConnCallback = nullptr,
             ProvIpCallback = nullptr) {
    return false;
  }
  void stop() {}
  bool isActive() const { return false; }
  const char* getDeviceName() const { return "No BLE"; }
  void setDeviceName(const char* name) { (void)name; }
  void sendStatus(uint8_t status) { (void)status; }
  void sendConnectionStatus(bool connected, const char* ip = nullptr) {
    (void)connected;
    (void)ip;
  }
  bool hasConnectedClient() const { return false; }
  void sendGetStatus() {}
};

#endif  // ESP32

#endif  // BLE_SERVER_H