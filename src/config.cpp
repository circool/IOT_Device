#include "config.h"
#include <Arduino.h>
#include "logger.h"

#ifdef ESP32
#include <esp_mac.h>
#endif

// ============================================================================
// СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ
// ============================================================================

static Config _config;
static bool _configValid = false;
static char _configLastError[64] = "";
static bool _configInitialized = false;



// ============================================================================
// CRC16
// ============================================================================

uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0x0000;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x8005;
      else
        crc <<= 1;
    }
  }
  return crc;
}

// ============================================================================
// ГЕНЕРАЦИЯ DEVICE ID
// ============================================================================

static void generateDeviceIdFromMac(char* out, size_t outSize) {
  if (!out || outSize == 0)
    return;

#ifdef ESP32
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(out, outSize, "%s_%02X%02X", DEVICE_PREFIX, mac[4], mac[5]);
#elif defined(ESP8266)
  uint32_t chipId = ESP.getChipId();
  snprintf(out, outSize, "%s_%04X", DEVICE_PREFIX, chipId & 0xFFFF);
#else
  snprintf(out, outSize, "%s_0000", DEVICE_PREFIX);
#endif
  out[outSize - 1] = '\0';
}

// ============================================================================
// ЗАГРУЗКА ФАБРИЧНЫХ НАСТРОЕК
// ============================================================================

static void loadFactoryCredentials(Config& cfg) {
#if HAS_CREDENTIALS == 1
  LOG_DEBUG(CAT_CONFIG, "Loading factory credentials");

#if WIFI_ENABLED == 1
  if (strlen(SSID_NAME) > 0) {
    strncpy(cfg.wifiSsid, SSID_NAME, sizeof(cfg.wifiSsid) - 1);
    cfg.wifiSsid[sizeof(cfg.wifiSsid) - 1] = '\0';
  }
  if (strlen(WIFI_PASSWORD) > 0) {
    strncpy(cfg.wifiPassword, WIFI_PASSWORD, sizeof(cfg.wifiPassword) - 1);
    cfg.wifiPassword[sizeof(cfg.wifiPassword) - 1] = '\0';
  }
#endif

#if MQTT_ENABLED == 1
  if (strlen(MQTT_ADDRESS) > 0) {
    strncpy(cfg.mqttBroker, MQTT_ADDRESS, sizeof(cfg.mqttBroker) - 1);
    cfg.mqttBroker[sizeof(cfg.mqttBroker) - 1] = '\0';
  }
  cfg.mqttPort = MQTT_PORT;
  if (strlen(MQTT_USER) > 0) {
    strncpy(cfg.mqttUser, MQTT_USER, sizeof(cfg.mqttUser) - 1);
    cfg.mqttUser[sizeof(cfg.mqttUser) - 1] = '\0';
  }
  if (strlen(MQTT_PASSWORD) > 0) {
    strncpy(cfg.mqttPassword, MQTT_PASSWORD, sizeof(cfg.mqttPassword) - 1);
    cfg.mqttPassword[sizeof(cfg.mqttPassword) - 1] = '\0';
  }
#endif

#else
  (void)cfg;
#endif
}

// ============================================================================
// УСТАНОВКА ЗНАЧЕНИЙ ПО УМОЛЧАНИЮ
// ============================================================================

void config_setDefaults() {
  LOG_DEBUG(CAT_CONFIG, "Setting defaults");

  memset(&_config, 0, sizeof(Config));

  _config.magic = MAGIC_VALUE;
  _config.crc = 0;
  generateDeviceIdFromMac(_config.deviceId, sizeof(_config.deviceId));

#if WIFI_ENABLED == 1
  strncpy(_config.wifiSsid, DEFAULT_WIFI_SSID, sizeof(_config.wifiSsid) - 1);
  strncpy(_config.wifiPassword, DEFAULT_WIFI_PASSWORD,
          sizeof(_config.wifiPassword) - 1);
#endif

#if MQTT_ENABLED == 1
  strncpy(_config.mqttBroker, DEFAULT_MQTT_BROKER,
          sizeof(_config.mqttBroker) - 1);
  _config.mqttPort = DEFAULT_MQTT_PORT;
  strncpy(_config.mqttUser, DEFAULT_MQTT_USER, sizeof(_config.mqttUser) - 1);
  strncpy(_config.mqttPassword, DEFAULT_MQTT_PASSWORD,
          sizeof(_config.mqttPassword) - 1);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  _config.sensorInterval = DEFAULT_SENSOR_INTERVAL;
#endif

#if DEVICE_TYPE == 1
  _config.lowHum = DEFAULT_LOW_HUM;
  _config.highHum = DEFAULT_HIGH_HUM;
  _config.lowTemp = DEFAULT_LOW_TEMP;
  _config.highTemp = DEFAULT_HIGH_TEMP;
  _config.sensorControlMode = DEFAULT_SENSOR_CONTROL_MODE;
  _config.speedPercent = DEFAULT_SPEED_PERCENT;
  _config.adaptiveMode = DEFAULT_ADAPTIVE_MODE;
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  _config.delaySeconds = DEFAULT_DELAY_SECONDS;
  _config.maxOnTime = DEFAULT_MAX_ON_TIME;
  _config.bootState = DEFAULT_BOOT_STATE;
#endif

  loadFactoryCredentials(_config);

  _config.valid = false;
  _config.error[0] = '\0';
}

// ============================================================================
// СЕТТЕРЫ С ВАЛИДАЦИЕЙ И ЛОГИРОВАНИЕМ
// ============================================================================

bool config_setDeviceId(const char* deviceId) {
  if (!deviceId || strlen(deviceId) == 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Device ID cannot be empty");
    return false;
  }
  if (strlen(deviceId) >= sizeof(_config.deviceId)) {
    snprintf(_configLastError, sizeof(_configLastError), "Device ID too long");
    return false;
  }

  for (size_t i = 0; i < strlen(deviceId); i++) {
    char c = deviceId[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) {
      snprintf(_configLastError, sizeof(_configLastError),
               "Invalid char '%c' in Device ID", c);
      return false;
    }
  }

  char oldValue[sizeof(_config.deviceId)];
  strncpy(oldValue, _config.deviceId, sizeof(oldValue));
  oldValue[sizeof(oldValue) - 1] = '\0';

  strncpy(_config.deviceId, deviceId, sizeof(_config.deviceId) - 1);
  _config.deviceId[sizeof(_config.deviceId) - 1] = '\0';

  if (strcmp(oldValue, _config.deviceId) != 0) {
    LOG_INFO(CAT_CONFIG, "Device ID: '%s' -> '%s'", oldValue, _config.deviceId);
  }

  return true;
}

#if WIFI_ENABLED == 1

bool config_setWifiSsid(const char* ssid) {
  if (!ssid || strlen(ssid) == 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "WiFi SSID cannot be empty");
    return false;
  }
  if (strlen(ssid) >= sizeof(_config.wifiSsid)) {
    snprintf(_configLastError, sizeof(_configLastError), "WiFi SSID too long");
    return false;
  }

  char oldValue[sizeof(_config.wifiSsid)];
  strncpy(oldValue, _config.wifiSsid, sizeof(oldValue));
  oldValue[sizeof(oldValue) - 1] = '\0';

  strncpy(_config.wifiSsid, ssid, sizeof(_config.wifiSsid) - 1);
  _config.wifiSsid[sizeof(_config.wifiSsid) - 1] = '\0';

  if (strcmp(oldValue, _config.wifiSsid) != 0) {
    LOG_INFO(CAT_CONFIG, "WiFi SSID: '%s' -> '%s'", oldValue, _config.wifiSsid);
  }

  return true;
}

bool config_setWifiPassword(const char* password) {
  if (!password)
    return false;

  if (strlen(password) >= sizeof(_config.wifiPassword)) {
    snprintf(_configLastError, sizeof(_configLastError),
             "WiFi password too long");
    return false;
  }

  char oldValue[sizeof(_config.wifiPassword)];
  strncpy(oldValue, _config.wifiPassword, sizeof(oldValue));
  oldValue[sizeof(oldValue) - 1] = '\0';

  if (strlen(password) > 0) {
    strncpy(_config.wifiPassword, password, sizeof(_config.wifiPassword) - 1);
    _config.wifiPassword[sizeof(_config.wifiPassword) - 1] = '\0';
  } else {
    _config.wifiPassword[0] = '\0';
  }

  if (strcmp(oldValue, _config.wifiPassword) != 0) {
    LOG_INFO(CAT_CONFIG, "WiFi password changed");
  }

  return true;
}

#endif  // WIFI_ENABLED

#if MQTT_ENABLED == 1

bool config_setMqttBroker(const char* broker) {
  if (!broker || strlen(broker) == 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "MQTT Broker cannot be empty");
    return false;
  }
  if (strlen(broker) >= sizeof(_config.mqttBroker)) {
    snprintf(_configLastError, sizeof(_configLastError),
             "MQTT Broker too long");
    return false;
  }

  char oldValue[sizeof(_config.mqttBroker)];
  strncpy(oldValue, _config.mqttBroker, sizeof(oldValue));
  oldValue[sizeof(oldValue) - 1] = '\0';

  strncpy(_config.mqttBroker, broker, sizeof(_config.mqttBroker) - 1);
  _config.mqttBroker[sizeof(_config.mqttBroker) - 1] = '\0';

  if (strcmp(oldValue, _config.mqttBroker) != 0) {
    LOG_INFO(CAT_CONFIG, "MQTT Broker: '%s' -> '%s'", oldValue,
             _config.mqttBroker);
  }

  return true;
}

bool config_setMqttPort(uint16_t port) {
  if (port < 1 || port > 65535) {
    snprintf(_configLastError, sizeof(_configLastError),
             "MQTT Port must be 1-65535");
    return false;
  }

  uint16_t oldValue = _config.mqttPort;

  if (port != oldValue) {
    LOG_INFO(CAT_CONFIG, "MQTT Port: %d -> %d", oldValue, port);
  }

  _config.mqttPort = port;
  return true;
}

bool config_setMqttUser(const char* user) {
  if (!user)
    return false;

  if (strlen(user) >= sizeof(_config.mqttUser)) {
    snprintf(_configLastError, sizeof(_configLastError), "MQTT User too long");
    return false;
  }

  char oldValue[sizeof(_config.mqttUser)];
  strncpy(oldValue, _config.mqttUser, sizeof(oldValue));
  oldValue[sizeof(oldValue) - 1] = '\0';

  strncpy(_config.mqttUser, user, sizeof(_config.mqttUser) - 1);
  _config.mqttUser[sizeof(_config.mqttUser) - 1] = '\0';

  if (strcmp(oldValue, _config.mqttUser) != 0) {
    LOG_INFO(CAT_CONFIG, "MQTT User: '%s' -> '%s'", oldValue, _config.mqttUser);
  }

  return true;
}

bool config_setMqttPassword(const char* password) {
  if (!password)
    return false;

  if (strlen(password) >= sizeof(_config.mqttPassword)) {
    snprintf(_configLastError, sizeof(_configLastError),
             "MQTT Password too long");
    return false;
  }

  char oldValue[sizeof(_config.mqttPassword)];
  strncpy(oldValue, _config.mqttPassword, sizeof(oldValue));
  oldValue[sizeof(oldValue) - 1] = '\0';

  if (strlen(password) > 0) {
    strncpy(_config.mqttPassword, password, sizeof(_config.mqttPassword) - 1);
    _config.mqttPassword[sizeof(_config.mqttPassword) - 1] = '\0';
  } else {
    _config.mqttPassword[0] = '\0';
  }

  if (strcmp(oldValue, _config.mqttPassword) != 0) {
    LOG_INFO(CAT_CONFIG, "MQTT password changed");
  }

  return true;
}

#endif  // MQTT_ENABLED

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

bool config_setDelaySeconds(int seconds) {
  if (seconds < 0 || seconds > 86400) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Delay seconds must be 0-86400 (got %d)", seconds);
    return false;
  }

  int oldValue = _config.delaySeconds;

  if (seconds != oldValue) {
    LOG_INFO(CAT_CONFIG, "Delay seconds: %d -> %d", oldValue, seconds);
  }

  _config.delaySeconds = seconds;
  return true;
}

bool config_setMaxOnTime(uint32_t seconds) {
  if (seconds > 86400) {
    snprintf(_configLastError, sizeof(_configLastError),
             "MaxOnTime seconds must be 0-86400 (got %u)", seconds);
    return false;
  }

  uint32_t oldValue = _config.maxOnTime;

  if (seconds != oldValue) {
    LOG_INFO(CAT_CONFIG, "Max on time: %lu -> %lu sec", oldValue, seconds);
  }

  _config.maxOnTime = seconds;
  return true;
}

bool config_setBootState(bool state) {
  bool oldValue = _config.bootState;

  if (state != oldValue) {
    LOG_INFO(CAT_CONFIG, "Boot state: %s -> %s", oldValue ? "ON" : "OFF",
             state ? "ON" : "OFF");
  }

  _config.bootState = state;
  return true;
}

#endif  // DEVICE_TYPE == 1 || DEVICE_TYPE == 3

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

bool config_setSensorInterval(uint16_t interval) {
  if (interval < 1 || interval > 3600) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Sensor interval must be 1-3600 seconds (got %d)", interval);
    return false;
  }

  uint16_t oldValue = _config.sensorInterval;

  if (interval != oldValue) {
    LOG_INFO(CAT_CONFIG, "Sensor interval: %d -> %d sec", oldValue, interval);
  }

  _config.sensorInterval = interval;
  return true;
}

#endif  // DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#if DEVICE_TYPE == 1

bool config_setLowTemp(double temp) {
  if (temp < TEMP_MIN || temp > TEMP_MAX) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Low Temp must be %.1f..%.1f°C (got %.1f)", TEMP_MIN, TEMP_MAX,
             temp);
    return false;
  }
  if (temp >= _config.highTemp && _config.highTemp != 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Low Temp (%.1f) must be < High Temp (%.1f)", temp,
             _config.highTemp);
    return false;
  }

  double oldValue = _config.lowTemp;

  if (fabs(oldValue - temp) > 0.01) {
    LOG_INFO(CAT_CONFIG, "Low temperature: %.1f -> %.1f°C", oldValue, temp);
  }

  _config.lowTemp = temp;
  return true;
}

bool config_setHighTemp(double temp) {
  if (temp < TEMP_MIN || temp > TEMP_MAX) {
    snprintf(_configLastError, sizeof(_configLastError),
             "High Temp must be %.1f..%.1f°C (got %.1f)", TEMP_MIN, TEMP_MAX,
             temp);
    return false;
  }
  if (temp <= _config.lowTemp && _config.lowTemp != 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "High Temp (%.1f) must be > Low Temp (%.1f)", temp,
             _config.lowTemp);
    return false;
  }

  double oldValue = _config.highTemp;

  if (fabs(oldValue - temp) > 0.01) {
    LOG_INFO(CAT_CONFIG, "High temperature: %.1f -> %.1f°C", oldValue, temp);
  }

  _config.highTemp = temp;
  return true;
}

bool config_setLowHum(double hum) {
  if (hum < HUM_MIN || hum > HUM_MAX) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Low Hum must be %.1f..%.1f%% (got %.1f)", HUM_MIN, HUM_MAX, hum);
    return false;
  }
  if (hum >= _config.highHum && _config.highHum != 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Low Hum (%.1f) must be < High Hum (%.1f)", hum, _config.highHum);
    return false;
  }

  double oldValue = _config.lowHum;

  if (fabs(oldValue - hum) > 0.01) {
    LOG_INFO(CAT_CONFIG, "Low humidity: %.1f -> %.1f%%", oldValue, hum);
  }

  _config.lowHum = hum;
  return true;
}

bool config_setHighHum(double hum) {
  if (hum < HUM_MIN || hum > HUM_MAX) {
    snprintf(_configLastError, sizeof(_configLastError),
             "High Hum must be %.1f..%.1f%% (got %.1f)", HUM_MIN, HUM_MAX, hum);
    return false;
  }
  if (hum <= _config.lowHum && _config.lowHum != 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "High Hum (%.1f) must be > Low Hum (%.1f)", hum, _config.lowHum);
    return false;
  }

  double oldValue = _config.highHum;

  if (fabs(oldValue - hum) > 0.01) {
    LOG_INFO(CAT_CONFIG, "High humidity: %.1f -> %.1f%%", oldValue, hum);
  }

  _config.highHum = hum;
  return true;
}

bool config_setSensorControlMode(bool enabled) {
  if (enabled && _config.speedPercent == 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Sensor Control Mode requires speed percent > 0%%");
    return false;
  }

  bool oldValue = _config.sensorControlMode;

  if (enabled != oldValue) {
    LOG_INFO(CAT_CONFIG, "Sensor control mode: %s -> %s",
             oldValue ? "ON" : "OFF", enabled ? "ON" : "OFF");
  }

  _config.sensorControlMode = enabled;
  return true;
}

bool config_setSpeedPercent(uint16_t percent) {
  if (percent > 100) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Speed percent must be 0-100 (got %d)", percent);
    return false;
  }

  uint16_t oldValue = _config.speedPercent;

  if (percent != oldValue) {
    LOG_INFO(CAT_CONFIG, "Speed percent: %d%% -> %d%%", oldValue, percent);
  }

  _config.speedPercent = percent;
  return true;
}

bool config_setAdaptiveMode(bool enabled) {
  if (enabled && !_config.sensorControlMode) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Adaptive mode requires Sensor Control Mode ON");
    return false;
  }
  if (enabled && _config.speedPercent == 0) {
    snprintf(_configLastError, sizeof(_configLastError),
             "Adaptive mode requires speed percent > 0%%");
    return false;
  }

  bool oldValue = _config.adaptiveMode;

  if (enabled != oldValue) {
    LOG_INFO(CAT_CONFIG, "Adaptive mode: %s -> %s", oldValue ? "ON" : "OFF",
             enabled ? "ON" : "OFF");
  }

  _config.adaptiveMode = enabled;
  return true;
}

#endif  // DEVICE_TYPE == 1

// ============================================================================
// ЧТЕНИЕ ИЗ EEPROM
// ============================================================================

static void readRawFromEeprom(Config& cfg) {
  uint8_t* ptr = (uint8_t*)&cfg;
  for (size_t i = 0; i < sizeof(Config); i++) {
    ptr[i] = EEPROM.read(i);
  }
}

static void writeRawToEeprom(const Config& cfg) {
  const uint8_t* ptr = (const uint8_t*)&cfg;
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, ptr[i]);
  }
}

// ============================================================================
// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ ДЛЯ ВАЛИДАЦИИ СТРОК
// ============================================================================

static bool isValidString(const char* str, bool allowEmpty) {
  if (!str)
    return false;
  if (!allowEmpty && str[0] == '\0')
    return false;

  for (size_t i = 0; str[i] != '\0'; i++) {
    char c = str[i];
    // Разрешены: латиница, цифры, пробел, _, -, ., :, / (для IP, доменов,
    // путей)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == ' ' || c == '_' || c == '-' ||
          c == '.' || c == ':' || c == '/')) {
      return false;
    }
  }
  return true;
}

bool config_validate(Config& cfg) {
  cfg.valid = false;
  cfg.error[0] = '\0';

  if (!isValidString(cfg.deviceId, false)) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid deviceId");
    return false;
  }

#if WIFI_ENABLED == 1
  if (!isValidString(cfg.wifiSsid, false)) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid wifiSsid");
    return false;
  }
  if (!isValidString(cfg.wifiPassword, true)) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid wifiPassword");
    return false;
  }
#endif

#if MQTT_ENABLED == 1
  if (!isValidString(cfg.mqttBroker, false)) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid mqttBroker");
    return false;
  }
  if (cfg.mqttPort < 1 || cfg.mqttPort > 65535) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid mqttPort: %d",
             cfg.mqttPort);
    return false;
  }
  if (!isValidString(cfg.mqttUser, true)) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid mqttUser");
    return false;
  }
  if (!isValidString(cfg.mqttPassword, true)) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid mqttPassword");
    return false;
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (cfg.sensorInterval < SENSOR_INTERVAL_MIN ||
      cfg.sensorInterval > SENSOR_INTERVAL_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid sensorInterval: %d",
             cfg.sensorInterval);
    return false;
  }
#endif

#if DEVICE_TYPE == 1
  if (cfg.lowTemp < TEMP_MIN || cfg.lowTemp > TEMP_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid lowTemp: %.1f",
             cfg.lowTemp);
    return false;
  }
  if (cfg.highTemp < TEMP_MIN || cfg.highTemp > TEMP_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid highTemp: %.1f",
             cfg.highTemp);
    return false;
  }
  if (cfg.lowTemp >= cfg.highTemp) {
    snprintf(cfg.error, sizeof(cfg.error), "lowTemp >= highTemp");
    return false;
  }
  if (cfg.lowHum < HUM_MIN || cfg.lowHum > HUM_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid lowHum: %.1f", cfg.lowHum);
    return false;
  }
  if (cfg.highHum < HUM_MIN || cfg.highHum > HUM_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid highHum: %.1f",
             cfg.highHum);
    return false;
  }
  if (cfg.lowHum >= cfg.highHum) {
    snprintf(cfg.error, sizeof(cfg.error), "lowHum >= highHum");
    return false;
  }
  if (cfg.speedPercent < SPEED_PERCENT_MIN ||
      cfg.speedPercent > SPEED_PERCENT_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid speedPercent: %d",
             cfg.speedPercent);
    return false;
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (cfg.delaySeconds < DELAY_SECONDS_MIN ||
      cfg.delaySeconds > DELAY_SECONDS_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid delaySeconds: %d",
             cfg.delaySeconds);
    return false;
  }
  if (cfg.maxOnTime < MAX_ON_TIME_MIN || cfg.maxOnTime > MAX_ON_TIME_MAX) {
    snprintf(cfg.error, sizeof(cfg.error), "Invalid maxOnTime: %lu",
             cfg.maxOnTime);
    return false;
  }
#endif

  cfg.valid = true;
  return true;
}

Config config_load() {
  Config cfg;
  Config raw;

  readRawFromEeprom(raw);

  LOG_DEBUG(CAT_CONFIG, "Read magic: 0x%04X (expected 0x%04X)", raw.magic,
            MAGIC_VALUE);
  LOG_DEBUG(CAT_CONFIG, "Read CRC from EEPROM: 0x%04X", raw.crc);

  if (raw.magic != MAGIC_VALUE) {
    LOG_WARN(CAT_CONFIG, "Magic mismatch! Using defaults");
    cfg = config_getDefaults();
    cfg.valid = false;
    snprintf(cfg.error, sizeof(cfg.error), "Magic mismatch");
    return cfg;
  }

  uint16_t savedCrc = raw.crc;
  raw.crc = 0;
  uint16_t calcCrc = crc16((uint8_t*)&raw, sizeof(Config));

  if (calcCrc != savedCrc) {
    LOG_WARN(CAT_CONFIG, "CRC mismatch! saved=0x%04X, calc=0x%04X", savedCrc,
             calcCrc);
    cfg = config_getDefaults();
    cfg.valid = false;
    snprintf(cfg.error, sizeof(cfg.error), "CRC mismatch");
    return cfg;
  }

  LOG_DEBUG(CAT_CONFIG, "CRC is VALID");

  config_setDefaults();
  cfg = raw;

  if (!config_validate(cfg)) {
    LOG_WARN(CAT_CONFIG, "Semantic validation failed: %s", cfg.error);
    cfg = config_getDefaults();
    cfg.valid = false;
    snprintf(cfg.error, sizeof(cfg.error), "Semantic validation failed");
    return cfg;
  }

  LOG_INFO(CAT_CONFIG, "Config loaded successfully");
  cfg.valid = true;
  cfg.error[0] = '\0';

  return cfg;
}

bool config_save(const Config& cfg) {
  LOG_INFO(CAT_CONFIG, "Saving config to EEPROM...");

  Config temp = cfg;
  if (!config_validate(temp)) {
    LOG_ERROR(CAT_CONFIG, "Cannot save invalid config: %s", temp.error);
    return false;
  }

  Config toSave = cfg;
  toSave.crc = 0;
  toSave.crc = crc16((uint8_t*)&toSave, sizeof(Config));
  LOG_DEBUG(CAT_CONFIG, "Calculated CRC: 0x%04X", toSave.crc);

  writeRawToEeprom(toSave);

  if (!EEPROM.commit()) {
    LOG_ERROR(CAT_CONFIG, "EEPROM commit FAILED!");
    return false;
  }

  Config verify;
  readRawFromEeprom(verify);
  verify.crc = 0;
  uint16_t verifyCrc = crc16((uint8_t*)&verify, sizeof(Config));

  if (verify.magic == MAGIC_VALUE && verifyCrc == toSave.crc) {
    LOG_INFO(CAT_CONFIG, "Save successful, verification PASSED");
    return true;
  } else {
    LOG_ERROR(CAT_CONFIG, "Save verification FAILED!");
    return false;
  }
}

Config config_getDefaults() {
  Config cfg;
  memset(&cfg, 0, sizeof(Config));

  cfg.magic = MAGIC_VALUE;
  cfg.crc = 0;
  generateDeviceIdFromMac(cfg.deviceId, sizeof(cfg.deviceId));

  LOG_DEBUG(CAT_CONFIG, "Generated Device ID: %s", cfg.deviceId);

#if WIFI_ENABLED == 1
  strncpy(cfg.wifiSsid, DEFAULT_WIFI_SSID, sizeof(cfg.wifiSsid) - 1);
  strncpy(cfg.wifiPassword, DEFAULT_WIFI_PASSWORD,
          sizeof(cfg.wifiPassword) - 1);
#endif

#if MQTT_ENABLED == 1
  strncpy(cfg.mqttBroker, DEFAULT_MQTT_BROKER, sizeof(cfg.mqttBroker) - 1);
  cfg.mqttPort = DEFAULT_MQTT_PORT;
  strncpy(cfg.mqttUser, DEFAULT_MQTT_USER, sizeof(cfg.mqttUser) - 1);
  strncpy(cfg.mqttPassword, DEFAULT_MQTT_PASSWORD,
          sizeof(cfg.mqttPassword) - 1);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  cfg.sensorInterval = DEFAULT_SENSOR_INTERVAL;
#endif

#if DEVICE_TYPE == 1
  cfg.lowHum = DEFAULT_LOW_HUM;
  cfg.highHum = DEFAULT_HIGH_HUM;
  cfg.lowTemp = DEFAULT_LOW_TEMP;
  cfg.highTemp = DEFAULT_HIGH_TEMP;
  cfg.sensorControlMode = DEFAULT_SENSOR_CONTROL_MODE;
  cfg.speedPercent = DEFAULT_SPEED_PERCENT;
  cfg.adaptiveMode = DEFAULT_ADAPTIVE_MODE;
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  cfg.delaySeconds = DEFAULT_DELAY_SECONDS;
  cfg.maxOnTime = DEFAULT_MAX_ON_TIME;
  cfg.bootState = DEFAULT_BOOT_STATE;
#endif

  loadFactoryCredentials(cfg);

  cfg.valid = config_validate(cfg);
  return cfg;
}

void config_init() {
  LOG_INFO(CAT_CONFIG, "Initializing EEPROM...");
  EEPROM.begin(sizeof(Config));
  LOG_DEBUG(CAT_CONFIG, "EEPROM size: %d bytes", EEPROM.length());
}

void config_print(const Config& cfg) {
  LOG_DEBUG(CAT_CONFIG, "=== Config ===");
  LOG_DEBUG(CAT_CONFIG, "magic: 0x%04X", cfg.magic);
  LOG_DEBUG(CAT_CONFIG, "crc: 0x%04X", cfg.crc);
  LOG_DEBUG(CAT_CONFIG, "deviceId: '%s'", cfg.deviceId);

#if WIFI_ENABLED == 1
  LOG_DEBUG(CAT_CONFIG, "wifiSsid: '%s'", cfg.wifiSsid);
  LOG_DEBUG(CAT_CONFIG, "wifiPassword: %s",
            cfg.wifiPassword[0] ? "***" : "(empty)");
#endif

#if MQTT_ENABLED == 1
  LOG_DEBUG(CAT_CONFIG, "mqttBroker: '%s:%d'", cfg.mqttBroker, cfg.mqttPort);
  LOG_DEBUG(CAT_CONFIG, "mqttUser: '%s'", cfg.mqttUser);
  LOG_DEBUG(CAT_CONFIG, "mqttPassword: %s",
            cfg.mqttPassword[0] ? "***" : "(empty)");
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  LOG_DEBUG(CAT_CONFIG, "sensorInterval: %d sec", cfg.sensorInterval);
#endif

#if DEVICE_TYPE == 1
  LOG_DEBUG(CAT_CONFIG, "lowTemp: %.1f, highTemp: %.1f", cfg.lowTemp,
            cfg.highTemp);
  LOG_DEBUG(CAT_CONFIG, "lowHum: %.1f, highHum: %.1f", cfg.lowHum, cfg.highHum);
  LOG_DEBUG(CAT_CONFIG, "sensorControlMode: %s",
            cfg.sensorControlMode ? "ON" : "OFF");
  LOG_DEBUG(CAT_CONFIG, "speedPercent: %d%%", cfg.speedPercent);
  LOG_DEBUG(CAT_CONFIG, "adaptiveMode: %s", cfg.adaptiveMode ? "ON" : "OFF");
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  LOG_DEBUG(CAT_CONFIG, "delaySeconds: %d sec", cfg.delaySeconds);
  LOG_DEBUG(CAT_CONFIG, "maxOnTime: %lu sec", cfg.maxOnTime);
  LOG_DEBUG(CAT_CONFIG, "bootState: %s", cfg.bootState ? "ON" : "OFF");
#endif

  LOG_DEBUG(CAT_CONFIG, "valid: %s", cfg.valid ? "true" : "false");
  if (!cfg.valid && cfg.error[0] != '\0') {
    LOG_DEBUG(CAT_CONFIG, "error: %s", cfg.error);
  }
  LOG_DEBUG(CAT_CONFIG, "=================");
}

// ============================================================================
// КОНСТРУКТОР
// ============================================================================

Config::Config() {
  memset(this, 0, sizeof(Config));
  valid = false;
  error[0] = '\0';
}