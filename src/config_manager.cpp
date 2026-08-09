/**
 * @file config_manager.cpp
 * @brief Реализация менеджера конфигурации
 * @version 0.11
 * @date 08.08.2026
 */

#include "config_manager.h"
#include "logger.h"

#include <string.h>
#include <algorithm>

// Выбор бэкенда хранения
#ifdef ESP8266
#include <EEPROM.h>
#elif defined(ESP32)
#include <Preferences.h>
#endif

// ================================================================
// КОНСТАНТЫ СМЕЩЕНИЙ (согласно разделу 8)
// ================================================================

static constexpr size_t MAGIC_OFFSET = 0;
static constexpr size_t MAGIC_SIZE = sizeof(uint16_t);
static constexpr size_t TRANSPORT_OFFSET = MAGIC_OFFSET + MAGIC_SIZE;
static constexpr size_t TRANSPORT_CRC_OFFSET = TRANSPORT_OFFSET;
static constexpr size_t TRANSPORT_DATA_OFFSET =
    TRANSPORT_CRC_OFFSET + sizeof(uint16_t);
static constexpr size_t DEVICE_OFFSET =
    TRANSPORT_DATA_OFFSET + sizeof(TransportConfig);
static constexpr size_t DEVICE_CRC_OFFSET = DEVICE_OFFSET;
static constexpr size_t DEVICE_DATA_OFFSET =
    DEVICE_CRC_OFFSET + sizeof(uint16_t);

// Общий размер EEPROM
static constexpr size_t EEPROM_SIZE = DEVICE_DATA_OFFSET + sizeof(DeviceConfig);

// ================================================================
// DEFAULT ЗНАЧЕНИЯ ДЛЯ CONFIG MANAGER
// ================================================================

// TransportConfig defaults
static constexpr const char* DEFAULT_DEVICE_ID = "unnamed_device";

#ifdef USE_WIFI
#ifdef WIFI_SSID
static constexpr const char* DEFAULT_WIFI_SSID = WIFI_SSID;
#else
static constexpr const char* DEFAULT_WIFI_SSID = "";
#endif

#ifdef WIFI_PASSWORD
static constexpr const char* DEFAULT_WIFI_PASSWORD = WIFI_PASSWORD;
#else
static constexpr const char* DEFAULT_WIFI_PASSWORD = "";
#endif
#endif

#ifdef USE_MQTT
#ifdef MQTT_BROKER
static constexpr const char* DEFAULT_MQTT_BROKER = MQTT_BROKER;
#else
static constexpr const char* DEFAULT_MQTT_BROKER = "";
#endif

#ifdef MQTT_PORT
static constexpr uint16_t DEFAULT_MQTT_PORT = MQTT_PORT;
#else
static constexpr uint16_t DEFAULT_MQTT_PORT = 1883;
#endif

#ifdef MQTT_USER
static constexpr const char* DEFAULT_MQTT_USER = MQTT_USER;
#else
static constexpr const char* DEFAULT_MQTT_USER = "";
#endif

#ifdef MQTT_PASSWORD
static constexpr const char* DEFAULT_MQTT_PASSWORD = MQTT_PASSWORD;
#else
static constexpr const char* DEFAULT_MQTT_PASSWORD = "";
#endif

#ifdef DEVICE_PREFIX
static constexpr const char* DEFAULT_MQTT_CLIENT_ID = DEVICE_PREFIX;
#else
static constexpr const char* DEFAULT_MQTT_CLIENT_ID = "unnamed_device";
#endif
#endif

#ifdef USE_ZIGBEE
static constexpr uint16_t DEFAULT_ZIGBEE_PAN_ID = 0x0000;
static constexpr uint8_t DEFAULT_ZIGBEE_CHANNEL = 11;
static constexpr const char* DEFAULT_ZIGBEE_NETWORK_KEY = "";
#endif

// DeviceConfig defaults
#if DEVICE_TYPE == 1
static constexpr bool DEFAULT_SENSOR_CONTROL_MODE = true;
static constexpr bool DEFAULT_ADAPTIVE_MODE = false;
static constexpr float DEFAULT_LOW_TEMP = 26.0f;
static constexpr float DEFAULT_HIGH_TEMP = 30.0f;
static constexpr float DEFAULT_LOW_HUM = 50.0f;
static constexpr float DEFAULT_HIGH_HUM = 65.0f;
static constexpr uint8_t DEFAULT_SPEED_PERCENT = 50;
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
static constexpr uint32_t DEFAULT_DELAY_SECONDS = 0;
static constexpr uint32_t DEFAULT_MAX_ON_TIME = 3600;
static constexpr bool DEFAULT_BOOT_STATE = false;
#endif

// ================================================================
// ПЛАТФОРМО-ЗАВИСИМЫЕ ОБЁРТКИ
// ================================================================

#ifdef ESP8266
// ESP8266 — EEPROM
static void storageInit() {
  EEPROM.begin(EEPROM_SIZE);
}

static void storageGet(size_t offset, void* data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    ((uint8_t*)data)[i] = EEPROM.read(offset + i);
  }
}

static void storagePut(size_t offset, const void* data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    EEPROM.write(offset + i, ((const uint8_t*)data)[i]);
  }
}

static bool storageCommit() {
  return EEPROM.commit();
}

#elif defined(ESP32)
// ESP32 — Preferences
static Preferences preferences;
static bool prefInitialized = false;

static void storageInit() {
  if (!prefInitialized) {
    preferences.begin("config", false);
    prefInitialized = true;
  }
}

static bool storageCommit() {
  return true;  // Preferences.commit() вызывается автоматически
}
#endif

// ================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ================================================================

bool ConfigManager::init() {
  if (m_initialized) {
    return true;
  }

#ifdef ESP8266
  storageInit();
  m_initialized = true;
  XLOG_INFO(CAT_CONFIG, "ConfigManager initialized (with EEPROM), size: %d",
            (int)EEPROM_SIZE);
  return true;

#elif defined(ESP32)
  storageInit();
  m_initialized = true;
  XLOG_INFO(CAT_CONFIG, "ConfigManager initialized (with Preferences)");
  return true;

#else
#error "Unsupported platform! Only ESP8266 and ESP32 are supported."
#endif
}

// ================================================================
// TRANSPORT CONFIG
// ================================================================

bool ConfigManager::set(const TransportConfig& config) {
  if (!m_initialized) {
    XLOG_ERROR(CAT_CONFIG, "ConfigManager not initialized");
    return false;
  }

  // Валидация (константную ссылку передаём в validate, который принимает
  // неконстантную)
  TransportConfig temp = config;  // ← копируем
  if (!validateTransportConfig(temp)) {
    XLOG_ERROR(CAT_CONFIG, "TransportConfig validation failed");
    return false;
  }

  // Читаем текущие данные
  TransportConfig current;
  bool hasCurrent = read(current);

  // Если данные не изменились — пропускаем запись
  if (hasCurrent && memcmp(&config, &current, sizeof(TransportConfig)) == 0) {
    XLOG_DEBUG(CAT_CONFIG, "TransportConfig unchanged, skip write");
    return true;
  }

  // Записываем
  return write(config);
}

bool ConfigManager::get(TransportConfig& config) {
  if (!m_initialized) {
    XLOG_ERROR(CAT_CONFIG, "ConfigManager not initialized");
    config = TransportConfig{};
    return false;
  }

  // 1. Проверяем MAGIC
  if (!checkMagic()) {
    XLOG_WARN(CAT_CONFIG, "MAGIC mismatch (config invalid), using defaults");
    config = TransportConfig{};
    return false;
  }

  // 2. Читаем данные
  TransportConfig temp;
  if (!read(temp)) {
    XLOG_WARN(CAT_CONFIG, "Failed to read TransportConfig");
    config = TransportConfig{};
    return false;
  }

  // 3. Проверяем CRC
  uint16_t storedCrc = 0;
#ifdef ESP8266
  storageGet(TRANSPORT_CRC_OFFSET, &storedCrc, sizeof(storedCrc));
#elif defined(ESP32)
  storedCrc = preferences.getUShort("t_crc", 0);
#endif

  uint16_t calculatedCrc = calculateCRC(reinterpret_cast<const uint8_t*>(&temp),
                                        sizeof(TransportConfig));

  if (storedCrc != calculatedCrc) {
    XLOG_WARN(CAT_CONFIG, "CRC mismatch for TransportConfig");
    config = TransportConfig{};
    return false;
  }

  // 4. Семантическая валидация
  bool allValid = validateTransportConfig(temp);
  config = temp;

  if (!allValid) {
    XLOG_WARN(CAT_CONFIG, "Some TransportConfig fields were invalid and fixed");
  }
  return true;
}

// DEVICE CONFIG
// ================================================================

bool ConfigManager::set(const DeviceConfig& settings) {
  if (!m_initialized) {
    XLOG_ERROR(CAT_CONFIG, "ConfigManager not initialized");
    return false;
  }

  // Валидация
  DeviceConfig temp = settings;  // ← копируем
  if (!validateDeviceConfig(temp)) {
    XLOG_ERROR(CAT_CONFIG, "DeviceConfig validation failed");
    return false;
  }

  // Читаем текущие данные
  DeviceConfig current;
  bool hasCurrent = read(current);

  // Если данные не изменились — пропускаем запись
  if (hasCurrent && memcmp(&settings, &current, sizeof(DeviceConfig)) == 0) {
    XLOG_DEBUG(CAT_CONFIG, "DeviceConfig unchanged, skip write");
    return true;
  }

  // Записываем
  return write(settings);
}

bool ConfigManager::get(DeviceConfig& settings) {
  if (!m_initialized) {
    XLOG_ERROR(CAT_CONFIG, "ConfigManager not initialized");
    settings = DeviceConfig{};
    return false;
  }

  // 1. Проверяем MAGIC
  if (!checkMagic()) {
    XLOG_WARN(CAT_CONFIG, "MAGIC mismatch (config invalid), using defaults");
    settings = DeviceConfig{};
    return false;
  }

  // 2. Читаем данные
  DeviceConfig temp;
  if (!read(temp)) {
    XLOG_WARN(CAT_CONFIG, "Failed to read DeviceConfig");
    settings = DeviceConfig{};
    return false;
  }

  // 3. Проверяем CRC
  uint16_t storedCrc = 0;
#ifdef ESP8266
  storageGet(DEVICE_CRC_OFFSET, &storedCrc, sizeof(storedCrc));
#elif defined(ESP32)
  storedCrc = preferences.getUShort("d_crc", 0);
#endif

  uint16_t calculatedCrc = calculateCRC(reinterpret_cast<const uint8_t*>(&temp),
                                        sizeof(DeviceConfig));

  if (storedCrc != calculatedCrc) {
    XLOG_WARN(CAT_CONFIG, "CRC mismatch for DeviceConfig");
    settings = DeviceConfig{};
    return false;
  }

  // 4. Семантическая валидация
  bool allValid = validateDeviceConfig(temp);
  settings = temp;

  if (!allValid) {
    XLOG_WARN(CAT_CONFIG, "Some DeviceConfig fields were invalid and fixed");
  }
  return true;
}

// ================================================================
// TRANSPORT CONFIG — RESET (void)
// ================================================================

void ConfigManager::reset(TransportConfig& config) {
  if (!m_initialized) {
    XLOG_ERROR(CAT_CONFIG, "ConfigManager not initialized");
    return;
  }

  // 1. Заполняем дефолтами
  TransportConfig defaults = {};

  strncpy(defaults.deviceId, DEFAULT_DEVICE_ID, sizeof(defaults.deviceId) - 1);
  defaults.deviceId[sizeof(defaults.deviceId) - 1] = '\0';

#ifdef USE_WIFI
  strncpy(defaults.wifiSsid, DEFAULT_WIFI_SSID, sizeof(defaults.wifiSsid) - 1);
  defaults.wifiSsid[sizeof(defaults.wifiSsid) - 1] = '\0';
  strncpy(defaults.wifiPassword, DEFAULT_WIFI_PASSWORD,
          sizeof(defaults.wifiPassword) - 1);
  defaults.wifiPassword[sizeof(defaults.wifiPassword) - 1] = '\0';
#endif

#ifdef USE_MQTT
  strncpy(defaults.mqttBroker, DEFAULT_MQTT_BROKER,
          sizeof(defaults.mqttBroker) - 1);
  defaults.mqttBroker[sizeof(defaults.mqttBroker) - 1] = '\0';
  defaults.mqttPort = DEFAULT_MQTT_PORT;
  strncpy(defaults.mqttUser, DEFAULT_MQTT_USER, sizeof(defaults.mqttUser) - 1);
  defaults.mqttUser[sizeof(defaults.mqttUser) - 1] = '\0';
  strncpy(defaults.mqttPassword, DEFAULT_MQTT_PASSWORD,
          sizeof(defaults.mqttPassword) - 1);
  defaults.mqttPassword[sizeof(defaults.mqttPassword) - 1] = '\0';
  strncpy(defaults.mqttClientId, DEFAULT_MQTT_CLIENT_ID,
          sizeof(defaults.mqttClientId) - 1);
  defaults.mqttClientId[sizeof(defaults.mqttClientId) - 1] = '\0';
#endif

#ifdef USE_ZIGBEE
  defaults.zigbeePanId = DEFAULT_ZIGBEE_PAN_ID;
  defaults.zigbeeChannel = DEFAULT_ZIGBEE_CHANNEL;
  strncpy(defaults.zigbeeNetworkKey, DEFAULT_ZIGBEE_NETWORK_KEY,
          sizeof(defaults.zigbeeNetworkKey) - 1);
  defaults.zigbeeNetworkKey[sizeof(defaults.zigbeeNetworkKey) - 1] = '\0';
#endif

  // 2. Читаем текущие данные
  TransportConfig current;
  bool hasCurrent = read(current);

  // 3. Если данных нет — пишем и выходим
  if (!hasCurrent) {
    XLOG_DEBUG(CAT_CONFIG, "No current data, writing defaults");
    config = defaults;
    if (!write(config)) {
      XLOG_ERROR(CAT_CONFIG, "Failed to write TransportConfig defaults");
    }
    return;
  }

  // 4. Сравниваем CRC дефолтов и текущих данных
  uint16_t defaultCrc = calculateCRC(
      reinterpret_cast<const uint8_t*>(&defaults), sizeof(TransportConfig));

  uint16_t currentCrc = calculateCRC(reinterpret_cast<const uint8_t*>(&current),
                                     sizeof(TransportConfig));

  // 5. Если CRC совпадают — данные уже дефолтные, пропускаем запись
  if (defaultCrc == currentCrc) {
    XLOG_DEBUG(CAT_CONFIG,
               "TransportConfig already at defaults (CRC match), skip write");
    config = defaults;
    return;
  }

  // 6. Пишем дефолты
  XLOG_DEBUG(CAT_CONFIG, "CRC mismatch (0x%04X vs 0x%04X), writing defaults",
             currentCrc, defaultCrc);
  config = defaults;
  if (!write(config)) {
    XLOG_ERROR(CAT_CONFIG, "Failed to write TransportConfig defaults");
  }
}

// ================================================================
// DEVICE CONFIG — RESET (void)
// ================================================================

void ConfigManager::reset(DeviceConfig& settings) {
  if (!m_initialized) {
    XLOG_ERROR(CAT_CONFIG, "ConfigManager not initialized");
    return;
  }

  // 1. Заполняем дефолтами
  DeviceConfig defaults = {};

#if DEVICE_TYPE == 1
  defaults.sensorControlMode = DEFAULT_SENSOR_CONTROL_MODE;
  defaults.adaptiveMode = DEFAULT_ADAPTIVE_MODE;
  defaults.lowTemp = DEFAULT_LOW_TEMP;
  defaults.highTemp = DEFAULT_HIGH_TEMP;
  defaults.lowHum = DEFAULT_LOW_HUM;
  defaults.highHum = DEFAULT_HIGH_HUM;
  defaults.speedPercent = DEFAULT_SPEED_PERCENT;
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  defaults.delaySeconds = DEFAULT_DELAY_SECONDS;
  defaults.maxOnTime = DEFAULT_MAX_ON_TIME;
  defaults.bootState = DEFAULT_BOOT_STATE;
#endif

  // 2. Читаем текущие данные
  DeviceConfig current;
  bool hasCurrent = read(current);

  // 3. Если данных нет — пишем и выходим
  if (!hasCurrent) {
    XLOG_DEBUG(CAT_CONFIG, "No current data, writing defaults");
    settings = defaults;
    if (!write(settings)) {
      XLOG_ERROR(CAT_CONFIG, "Failed to write DeviceConfig defaults");
    }
    return;
  }

  // 4. Сравниваем CRC
  uint16_t defaultCrc = calculateCRC(
      reinterpret_cast<const uint8_t*>(&defaults), sizeof(DeviceConfig));

  uint16_t currentCrc = calculateCRC(reinterpret_cast<const uint8_t*>(&current),
                                     sizeof(DeviceConfig));

  // 5. Если CRC совпадают — пропускаем запись
  if (defaultCrc == currentCrc) {
    XLOG_DEBUG(CAT_CONFIG,
               "DeviceConfig already at defaults (CRC match), skip write");
    settings = defaults;
    return;
  }

  // 6. Пишем дефолты
  XLOG_DEBUG(CAT_CONFIG, "CRC mismatch (0x%04X vs 0x%04X), writing defaults",
             currentCrc, defaultCrc);
  settings = defaults;
  if (!write(settings)) {
    XLOG_ERROR(CAT_CONFIG, "Failed to write DeviceConfig defaults");
  }
}

// ================================================================
// ПРИВАТНЫЕ МЕТОДЫ — CRC
// ================================================================

uint16_t ConfigManager::calculateCRC(const uint8_t* data, size_t len) {
  uint16_t crc = 0x0000;
  const uint16_t polynomial = 0x8005;

  for (size_t i = 0; i < len; i++) {
    crc ^= (data[i] << 8);
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ polynomial;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

// ================================================================
// ПРИВАТНЫЕ МЕТОДЫ — ВАЛИДАЦИЯ
// ================================================================

bool ConfigManager::validateTransportConfig(TransportConfig& config) const {
  // Проверка deviceId
  size_t len = strnlen(config.deviceId, sizeof(config.deviceId));
  if (len < DEVICE_ID_MIN_LEN || len > DEVICE_ID_MAX_LEN) {
    XLOG_WARN(CAT_CONFIG, "Invalid deviceId length: %d (min=%d, max=%d)",
              (int)len, DEVICE_ID_MIN_LEN, DEVICE_ID_MAX_LEN);
    return false;
  }

#ifdef USE_WIFI
  // Проверка wifiSsid
  len = strnlen(config.wifiSsid, sizeof(config.wifiSsid));
  if (len < WIFI_SSID_MIN_LEN || len > WIFI_SSID_MAX_LEN) {
    XLOG_WARN(CAT_CONFIG, "Invalid wifiSsid length: %d (min=%d, max=%d)",
              (int)len, WIFI_SSID_MIN_LEN, WIFI_SSID_MAX_LEN);
    return false;
  }

  // Проверка wifiPassword (опционально — может быть пустым)
  len = strnlen(config.wifiPassword, sizeof(config.wifiPassword));
  if (len > WIFI_PASSWORD_MAX_LEN) {
    XLOG_WARN(CAT_CONFIG, "wifiPassword too long: %d (max=%d)", (int)len,
              WIFI_PASSWORD_MAX_LEN);
    return false;
  }
#endif

#ifdef USE_MQTT
  // Проверка mqttBroker
  len = strnlen(config.mqttBroker, sizeof(config.mqttBroker));
  if (len < MQTT_BROKER_MIN_LEN || len > MQTT_BROKER_MAX_LEN) {
    XLOG_WARN(CAT_CONFIG, "Invalid mqttBroker length: %d (min=%d, max=%d)",
              (int)len, MQTT_BROKER_MIN_LEN, MQTT_BROKER_MAX_LEN);
    return false;
  }

  // Проверка mqttPort
  if (config.mqttPort < MQTT_PORT_MIN || config.mqttPort > MQTT_PORT_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid mqttPort: %d (min=%d, max=%d)",
              config.mqttPort, MQTT_PORT_MIN, MQTT_PORT_MAX);
    return false;
  }

  // Проверка mqttUser (опционально — может быть пустым)
  len = strnlen(config.mqttUser, sizeof(config.mqttUser));
  if (len > 31) {
    XLOG_WARN(CAT_CONFIG, "mqttUser too long: %d (max=31)", (int)len);
    return false;
  }

  // Проверка mqttPassword (опционально — может быть пустым)
  len = strnlen(config.mqttPassword, sizeof(config.mqttPassword));
  if (len > 63) {
    XLOG_WARN(CAT_CONFIG, "mqttPassword too long: %d (max=63)", (int)len);
    return false;
  }

  // Проверка mqttClientId
  len = strnlen(config.mqttClientId, sizeof(config.mqttClientId));
  if (len < MQTT_CLIENT_ID_MIN_LEN || len > MQTT_CLIENT_ID_MAX_LEN) {
    XLOG_WARN(CAT_CONFIG, "Invalid mqttClientId length: %d (min=%d, max=%d)",
              (int)len, MQTT_CLIENT_ID_MIN_LEN, MQTT_CLIENT_ID_MAX_LEN);
    return false;
  }
#endif

#ifdef USE_ZIGBEE
  // Проверка zigbeePanId
  if (config.zigbeePanId < ZIGBEE_PAN_ID_MIN ||
      config.zigbeePanId > ZIGBEE_PAN_ID_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid zigbeePanId: %d (min=%d, max=%d)",
              config.zigbeePanId, ZIGBEE_PAN_ID_MIN, ZIGBEE_PAN_ID_MAX);
    return false;
  }

  // Проверка zigbeeChannel
  if (config.zigbeeChannel < ZIGBEE_CHANNEL_MIN ||
      config.zigbeeChannel > ZIGBEE_CHANNEL_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid zigbeeChannel: %d (min=%d, max=%d)",
              config.zigbeeChannel, ZIGBEE_CHANNEL_MIN, ZIGBEE_CHANNEL_MAX);
    return false;
  }
#endif

  return true;
}

bool ConfigManager::validateDeviceConfig(DeviceConfig& settings) const {
#if DEVICE_TYPE == 1
  // Проверка lowTemp
  if (settings.lowTemp < LOW_TEMP_MIN || settings.lowTemp > LOW_TEMP_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid lowTemp: %.1f (min=%.1f, max=%.1f)",
              settings.lowTemp, LOW_TEMP_MIN, LOW_TEMP_MAX);
    return false;
  }

  // Проверка highTemp
  if (settings.highTemp < HIGH_TEMP_MIN || settings.highTemp > HIGH_TEMP_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid highTemp: %.1f (min=%.1f, max=%.1f)",
              settings.highTemp, HIGH_TEMP_MIN, HIGH_TEMP_MAX);
    return false;
  }

  // Проверка lowTemp < highTemp
  if (settings.lowTemp >= settings.highTemp) {
    XLOG_WARN(CAT_CONFIG, "lowTemp (%.1f) >= highTemp (%.1f)", settings.lowTemp,
              settings.highTemp);
    return false;
  }

  // Проверка lowHum
  if (settings.lowHum < LOW_HUM_MIN || settings.lowHum > LOW_HUM_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid lowHum: %.1f (min=%.1f, max=%.1f)",
              settings.lowHum, LOW_HUM_MIN, LOW_HUM_MAX);
    return false;
  }

  // Проверка highHum
  if (settings.highHum < HIGH_HUM_MIN || settings.highHum > HIGH_HUM_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid highHum: %.1f (min=%.1f, max=%.1f)",
              settings.highHum, HIGH_HUM_MIN, HIGH_HUM_MAX);
    return false;
  }

  // Проверка lowHum < highHum
  if (settings.lowHum >= settings.highHum) {
    XLOG_WARN(CAT_CONFIG, "lowHum (%.1f) >= highHum (%.1f)", settings.lowHum,
              settings.highHum);
    return false;
  }

  // Проверка speedPercent
  if (settings.speedPercent < SPEED_PERCENT_MIN ||
      settings.speedPercent > SPEED_PERCENT_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid speedPercent: %d (min=%d, max=%d)",
              settings.speedPercent, SPEED_PERCENT_MIN, SPEED_PERCENT_MAX);
    return false;
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  // Проверка delaySeconds
  if (settings.delaySeconds < DELAY_SECONDS_MIN ||
      settings.delaySeconds > DELAY_SECONDS_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid delaySeconds: %lu (min=%lu, max=%lu)",
              (unsigned long)settings.delaySeconds,
              (unsigned long)DELAY_SECONDS_MIN,
              (unsigned long)DELAY_SECONDS_MAX);
    return false;
  }

  // Проверка maxOnTime
  if (settings.maxOnTime < MAX_ON_TIME_MIN ||
      settings.maxOnTime > MAX_ON_TIME_MAX) {
    XLOG_WARN(CAT_CONFIG, "Invalid maxOnTime: %lu (min=%lu, max=%lu)",
              (unsigned long)settings.maxOnTime, (unsigned long)MAX_ON_TIME_MIN,
              (unsigned long)MAX_ON_TIME_MAX);
    return false;
  }

  // bootState не проверяем — это bool (0 или 1)
#endif

  return true;
}  

// ================================================================
// ПРИВАТНЫЕ МЕТОДЫ — MAGIC
// ================================================================

bool ConfigManager::checkMagic() const {
#ifdef ESP8266
  uint16_t magic = 0;
  storageGet(MAGIC_OFFSET, &magic, sizeof(magic));
  return (magic == MAGIC_VALUE);
#elif defined(ESP32)
  return preferences.isKey("magic") &&
         preferences.getUShort("magic", 0) == MAGIC_VALUE;
#endif
}

void ConfigManager::writeMagic() {
#ifdef ESP8266
  uint16_t magic = MAGIC_VALUE;
  storagePut(MAGIC_OFFSET, &magic, sizeof(magic));
  storageCommit();
#elif defined(ESP32)
  preferences.putUShort("magic", MAGIC_VALUE);
#endif
}

// ================================================================
// ПРИВАТНЫЕ МЕТОДЫ — ЧТЕНИЕ
// ================================================================

bool ConfigManager::read(TransportConfig& config) const {
#ifdef ESP8266
  uint8_t check = 0;
  storageGet(TRANSPORT_DATA_OFFSET, &check, 1);
  if (check == 0xFF) {
    uint8_t buffer[4];
    storageGet(TRANSPORT_DATA_OFFSET, buffer, sizeof(buffer));
    bool allFF = true;
    for (size_t i = 0; i < sizeof(buffer); i++) {
      if (buffer[i] != 0xFF) {
        allFF = false;
        break;
      }
    }
    if (allFF) {
      return false;
    }
  }

  storageGet(TRANSPORT_DATA_OFFSET, &config, sizeof(config));
  return true;

#elif defined(ESP32)
  // Проверяем наличие blob-ключа
  if (!preferences.isKey("t_data")) {
    XLOG_DEBUG(CAT_CONFIG, "t_data key not found");
    return false;
  }

  // Проверяем размер
  size_t len = preferences.getBytesLength("t_data");
  if (len != sizeof(TransportConfig)) {
    XLOG_WARN(CAT_CONFIG, "t_data size mismatch: expected %d, got %d",
              (int)sizeof(TransportConfig), (int)len);
    return false;
  }

  // Читаем ВСЮ структуру как blob
  preferences.getBytes("t_data", &config, sizeof(TransportConfig));

  XLOG_DEBUG(CAT_CONFIG, "TransportConfig read successfully (%d bytes)",
             (int)len);
  return true;
#endif
}

bool ConfigManager::read(DeviceConfig& settings) const {
#ifdef ESP8266
  uint8_t check = 0;
  storageGet(DEVICE_DATA_OFFSET, &check, 1);
  if (check == 0xFF) {
    uint8_t buffer[4];
    storageGet(DEVICE_DATA_OFFSET, buffer, sizeof(buffer));
    bool allFF = true;
    for (size_t i = 0; i < sizeof(buffer); i++) {
      if (buffer[i] != 0xFF) {
        allFF = false;
        break;
      }
    }
    if (allFF) {
      return false;
    }
  }

  storageGet(DEVICE_DATA_OFFSET, &settings, sizeof(settings));
  return true;

#elif defined(ESP32)
  // Проверяем наличие blob-ключа
  if (!preferences.isKey("d_data")) {
    XLOG_DEBUG(CAT_CONFIG, "d_data key not found");
    return false;
  }

  // Проверяем размер
  size_t len = preferences.getBytesLength("d_data");
  if (len != sizeof(DeviceConfig)) {
    XLOG_WARN(CAT_CONFIG, "d_data size mismatch: expected %d, got %d",
              (int)sizeof(DeviceConfig), (int)len);
    return false;
  }

  // Читаем ВСЮ структуру как blob
  preferences.getBytes("d_data", &settings, sizeof(DeviceConfig));

  XLOG_DEBUG(CAT_CONFIG, "DeviceConfig read successfully (%d bytes)", (int)len);
  return true;
#endif
}

// ================================================================
// ПРИВАТНЫЕ МЕТОДЫ — ЗАПИСЬ
// ================================================================

bool ConfigManager::write(const TransportConfig& config) {
  uint16_t crc = calculateCRC(reinterpret_cast<const uint8_t*>(&config),
                              sizeof(TransportConfig));

  // Проверяем MAGIC и записываем, если не совпадает
  if (!checkMagic()) {
    writeMagic();
    XLOG_DEBUG(CAT_CONFIG, "MAGIC written (or updated)");
  }

#ifdef ESP8266
  storagePut(TRANSPORT_CRC_OFFSET, &crc, sizeof(crc));
  storagePut(TRANSPORT_DATA_OFFSET, &config, sizeof(config));
  bool success = storageCommit();
  if (success) {
    XLOG_DEBUG(CAT_CONFIG, "TransportConfig written successfully");
  } else {
    XLOG_ERROR(CAT_CONFIG, "Failed to write TransportConfig");
  }
  return success;

#elif defined(ESP32)
  preferences.putUShort("t_crc", crc);
  preferences.putBytes("t_data", &config, sizeof(TransportConfig));
  XLOG_DEBUG(CAT_CONFIG, "TransportConfig written successfully (Preferences)");
  return true;
#endif
}

bool ConfigManager::write(const DeviceConfig& settings) {
  uint16_t crc = calculateCRC(reinterpret_cast<const uint8_t*>(&settings),
                              sizeof(DeviceConfig));

  // Проверяем MAGIC и записываем, если не совпадает
  if (!checkMagic()) {
    writeMagic();
    XLOG_DEBUG(CAT_CONFIG, "MAGIC written (or updated)");
  }

#ifdef ESP8266
  storagePut(DEVICE_CRC_OFFSET, &crc, sizeof(crc));
  storagePut(DEVICE_DATA_OFFSET, &settings, sizeof(settings));
  bool success = storageCommit();
  if (success) {
    XLOG_DEBUG(CAT_CONFIG, "DeviceConfig written successfully");
  } else {
    XLOG_ERROR(CAT_CONFIG, "Failed to write DeviceConfig");
  }
  return success;

#elif defined(ESP32)
  preferences.putUShort("d_crc", crc);
  preferences.putBytes("d_data", &settings, sizeof(DeviceConfig));
  XLOG_DEBUG(CAT_CONFIG, "DeviceConfig written successfully (Preferences)");
  return true;
#endif
}