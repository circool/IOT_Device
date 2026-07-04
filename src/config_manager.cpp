#include "config_manager.h"
#include "led.h"
#include "logger.h"

#ifdef ESP32
#include <esp_chip_info.h>
#include <esp_mac.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// ============================================================================
// РЕАЛИЗАЦИЯ CRC16
// ============================================================================

static uint16_t crc16_impl(const uint8_t* data, size_t len) {
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
// СИНГЛТОН
// ============================================================================

static ConfigManager* g_instance = nullptr;

ConfigManager& ConfigManager::getInstance() {
  if (!g_instance) {
    g_instance = new ConfigManager();
  }
  return *g_instance;
}

ConfigManager& g_configManager = ConfigManager::getInstance();

// ============================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ============================================================================

void ConfigManager::begin() {
  if (_initialized)
    return;

  LOG_INFO(CAT_CONFIG, "Initializing ConfigManager...");

  // Инициализация EEPROM
  EEPROM.begin(EEPROM_SIZE);
  LOG_DEBUG(CAT_CONFIG, "EEPROM size: %d bytes", EEPROM.length());

  // Генерация ID устройства
  initDeviceId();

  // Чтение из EEPROM
  readFromEEPROM();

  _initialized = true;
  LOG_INFO(CAT_CONFIG, "ConfigManager initialized (valid: %s)",
           _configValid ? "YES" : "NO");
}

const ConfigData* ConfigManager::get() const {
  return &_config;
}

bool ConfigManager::isValid() const {
  return _configValid;
}

const char* ConfigManager::getLastError() const {
  return _lastError;
}

bool ConfigManager::save() {
  LOG_INFO(CAT_CONFIG, "Saving configuration to EEPROM...");

  // Обновляем CRC
  _config.crc = 0;
  _config.crc = calculateCRC(_config);
  _config.magic = MAGIC;

  LOG_DEBUG(CAT_CONFIG, "Calculated CRC: 0x%04X", _config.crc);

  // Запись в EEPROM
  uint8_t* ptr = reinterpret_cast<uint8_t*>(&_config);
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, ptr[i]);
  }

  if (!EEPROM.commit()) {
    LOG_ERROR(CAT_CONFIG, "EEPROM commit FAILED!");
    return false;
  }

  // Верификация
  ConfigData verify;
  uint8_t* vptr = reinterpret_cast<uint8_t*>(&verify);
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    vptr[i] = EEPROM.read(i);
  }

  verify.crc = 0;
  uint16_t calcVerifyCrc = calculateCRC(verify);

  if (verify.magic == MAGIC && calcVerifyCrc == _config.crc) {
    LOG_INFO(CAT_CONFIG, "Verification PASSED");
    _configValid = true;

    // Уведомление об изменении
    if (_changeCallback) {
      _changeCallback();
    }

    return true;
  } else {
    LOG_ERROR(CAT_CONFIG, "Verification FAILED!");
    return false;
  }
}

bool ConfigManager::reset() {
  led_setMode(LED_MODE_MORZE_I);
  LOG_INFO(CAT_CONFIG, "Resetting configuration to defaults...");

  EEPROM.end();
  delay(50);
  EEPROM.begin(EEPROM_SIZE);

  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }

  bool ok = EEPROM.commit();
  EEPROM.end();

  if (ok) {
    setDefaults();
    _configValid = false;
    LOG_INFO(CAT_CONFIG, "Reset SUCCESSFUL");
  } else {
    LOG_WARN(CAT_CONFIG, "Reset FAILED");
  }

  return ok;
}

// ============================================================================
// ГЕТТЕРЫ
// ============================================================================

const char* ConfigManager::getWifiSsid() const {
#if WIFI_ENABLED == 1
  return _config.wifiSsid;
#else
  return "";
#endif
}

const char* ConfigManager::getWifiPassword() const {
#if WIFI_ENABLED == 1
  return _config.wifiPassword;
#else
  return "";
#endif
}

const char* ConfigManager::getMqttBroker() const {
#if MQTT_ENABLED == 1
  return _config.mqttBroker;
#else
  return "";
#endif
}

uint16_t ConfigManager::getMqttPort() const {
#if MQTT_ENABLED == 1
  return _config.mqttPort;
#else
  return 1883;
#endif
}

const char* ConfigManager::getMqttUser() const {
#if MQTT_ENABLED == 1
  return _config.mqttUser;
#else
  return "";
#endif
}

const char* ConfigManager::getMqttPassword() const {
#if MQTT_ENABLED == 1
  return _config.mqttPassword;
#else
  return "";
#endif
}

const char* ConfigManager::getMqttClientId() const {
#if MQTT_ENABLED == 1
  return _config.mqttClientId;
#else
  return "";
#endif
}

uint16_t ConfigManager::getSensorInterval() const {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  return _config.sensorInterval;
#else
  return SENSOR_DURATION;
#endif
}

double ConfigManager::getLowTemp() const {
#if DEVICE_TYPE == 1
  return _config.lowTemp;
#else
  return DEFAULT_LOW_TEMP;
#endif
}

double ConfigManager::getHighTemp() const {
#if DEVICE_TYPE == 1
  return _config.highTemp;
#else
  return DEFAULT_HIGH_TEMP;
#endif
}

double ConfigManager::getLowHum() const {
#if DEVICE_TYPE == 1
  return _config.lowHum;
#else
  return DEFAULT_LOW_HUM;
#endif
}

double ConfigManager::getHighHum() const {
#if DEVICE_TYPE == 1
  return _config.highHum;
#else
  return DEFAULT_HIGH_HUM;
#endif
}

bool ConfigManager::getSensorControlMode() const {
#if DEVICE_TYPE == 1
  return _config.sensorControlMode;
#else
  return false;
#endif
}

uint16_t ConfigManager::getSpeedPercent() const {
#if DEVICE_TYPE == 1
  return _config.speedPercent;
#else
  return DEFAULT_SPEED_PERCENT;
#endif
}

bool ConfigManager::getAdaptiveMode() const {
#if DEVICE_TYPE == 1
  return _config.adaptiveMode;
#else
  return false;
#endif
}

int ConfigManager::getDelaySeconds() const {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  return _config.delaySeconds;
#else
  return DEFAULT_DELAY_SECONDS;
#endif
}

uint32_t ConfigManager::getMaxOnTime() const {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  return _config.maxOnTime;
#else
  return MAX_ON_TIME_SEC;
#endif
}

bool ConfigManager::getBootState() const {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  return _config.bootState;
#else
  return BOOT_SWITCH_STATE;
#endif
}

const char* ConfigManager::getZigbeeNetworkKey() const {
#if ZIGBEE_ENABLED == 1
  return _config.zigbeeNetworkKey;
#else
  return "";
#endif
}

uint16_t ConfigManager::getZigbeePanId() const {
#if ZIGBEE_ENABLED == 1
  return _config.zigbeePanId;
#else
  return 0;
#endif
}

uint8_t ConfigManager::getZigbeeChannel() const {
#if ZIGBEE_ENABLED == 1
  return _config.zigbeeChannel;
#else
  return 15;
#endif
}

const char* ConfigManager::getDeviceId() const {
  return _deviceId;
}

// ============================================================================
// СЕТТЕРЫ С ВАЛИДАЦИЕЙ
// ============================================================================

bool ConfigManager::setWifiSsid(const char* ssid) {
#if WIFI_ENABLED == 1
  if (!ssid || strlen(ssid) == 0) {
    snprintf(_lastError, sizeof(_lastError), "WiFi SSID cannot be empty");
    return false;
  }
  if (strlen(ssid) >= sizeof(_config.wifiSsid)) {
    snprintf(_lastError, sizeof(_lastError), "WiFi SSID too long (max %d)",
             (int)sizeof(_config.wifiSsid) - 1);
    return false;
  }
  strncpy(_config.wifiSsid, ssid, sizeof(_config.wifiSsid) - 1);
  _config.wifiSsid[sizeof(_config.wifiSsid) - 1] = '\0';
  return true;
#else
  (void)ssid;
  return false;
#endif
}

bool ConfigManager::setWifiPassword(const char* password) {
#if WIFI_ENABLED == 1
  if (!password)
    return false;
  if (strlen(password) >= sizeof(_config.wifiPassword)) {
    snprintf(_lastError, sizeof(_lastError), "WiFi password too long (max %d)",
             (int)sizeof(_config.wifiPassword) - 1);
    return false;
  }
  strncpy(_config.wifiPassword, password, sizeof(_config.wifiPassword) - 1);
  _config.wifiPassword[sizeof(_config.wifiPassword) - 1] = '\0';
  return true;
#else
  (void)password;
  return false;
#endif
}

bool ConfigManager::setMqttBroker(const char* broker) {
#if MQTT_ENABLED == 1
  if (!broker || strlen(broker) == 0) {
    snprintf(_lastError, sizeof(_lastError), "MQTT Broker cannot be empty");
    return false;
  }
  if (strlen(broker) >= sizeof(_config.mqttBroker)) {
    snprintf(_lastError, sizeof(_lastError), "MQTT Broker too long (max %d)",
             (int)sizeof(_config.mqttBroker) - 1);
    return false;
  }
  strncpy(_config.mqttBroker, broker, sizeof(_config.mqttBroker) - 1);
  _config.mqttBroker[sizeof(_config.mqttBroker) - 1] = '\0';
  return true;
#else
  (void)broker;
  return false;
#endif
}

bool ConfigManager::setMqttPort(uint16_t port) {
#if MQTT_ENABLED == 1
  if (port < 1 || port > 65535) {
    snprintf(_lastError, sizeof(_lastError),
             "MQTT Port must be 1-65535 (got %d)", port);
    return false;
  }
  _config.mqttPort = port;
  return true;
#else
  (void)port;
  return false;
#endif
}

bool ConfigManager::setMqttUser(const char* user) {
#if MQTT_ENABLED == 1
  if (!user)
    return false;
  if (strlen(user) >= sizeof(_config.mqttUser)) {
    snprintf(_lastError, sizeof(_lastError), "MQTT User too long (max %d)",
             (int)sizeof(_config.mqttUser) - 1);
    return false;
  }
  strncpy(_config.mqttUser, user, sizeof(_config.mqttUser) - 1);
  _config.mqttUser[sizeof(_config.mqttUser) - 1] = '\0';
  return true;
#else
  (void)user;
  return false;
#endif
}

bool ConfigManager::setMqttPassword(const char* password) {
#if MQTT_ENABLED == 1
  if (!password)
    return false;
  if (strlen(password) >= sizeof(_config.mqttPassword)) {
    snprintf(_lastError, sizeof(_lastError), "MQTT Password too long (max %d)",
             (int)sizeof(_config.mqttPassword) - 1);
    return false;
  }
  strncpy(_config.mqttPassword, password, sizeof(_config.mqttPassword) - 1);
  _config.mqttPassword[sizeof(_config.mqttPassword) - 1] = '\0';
  return true;
#else
  (void)password;
  return false;
#endif
}

bool ConfigManager::setMqttClientId(const char* clientId) {
#if MQTT_ENABLED == 1
  if (!clientId || strlen(clientId) == 0) {
    snprintf(_lastError, sizeof(_lastError), "MQTT Client ID cannot be empty");
    return false;
  }
  if (strlen(clientId) >= sizeof(_config.mqttClientId)) {
    snprintf(_lastError, sizeof(_lastError), "MQTT Client ID too long (max %d)",
             (int)sizeof(_config.mqttClientId) - 1);
    return false;
  }
  // Проверка допустимых символов
  for (size_t i = 0; i < strlen(clientId); i++) {
    char c = clientId[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) {
      snprintf(_lastError, sizeof(_lastError),
               "MQTT Client ID invalid char '%c' (use a-z, A-Z, 0-9, _, -)", c);
      return false;
    }
  }
  strncpy(_config.mqttClientId, clientId, sizeof(_config.mqttClientId) - 1);
  _config.mqttClientId[sizeof(_config.mqttClientId) - 1] = '\0';
  return true;
#else
  (void)clientId;
  return false;
#endif
}

bool ConfigManager::setSensorInterval(uint16_t interval) {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (interval < SENSOR_INTERVAL_MIN || interval > SENSOR_INTERVAL_MAX) {
    snprintf(_lastError, sizeof(_lastError),
             "Sensor interval must be %d-%d seconds (got %d)",
             SENSOR_INTERVAL_MIN, SENSOR_INTERVAL_MAX, interval);
    return false;
  }
  _config.sensorInterval = interval;
  return true;
#else
  (void)interval;
  return false;
#endif
}

bool ConfigManager::setLowTemp(double temp) {
#if DEVICE_TYPE == 1
  if (temp < TEMP_MIN || temp > TEMP_MAX) {
    snprintf(_lastError, sizeof(_lastError),
             "Low Temp must be %.1f..%.1f°C (got %.1f)", TEMP_MIN, TEMP_MAX,
             temp);
    return false;
  }
  if (temp >= _config.highTemp && _config.highTemp != 0) {
    snprintf(_lastError, sizeof(_lastError),
             "Low Temp (%.1f) must be < High Temp (%.1f)", temp,
             _config.highTemp);
    return false;
  }
  _config.lowTemp = temp;
  return true;
#else
  (void)temp;
  return false;
#endif
}

bool ConfigManager::setHighTemp(double temp) {
#if DEVICE_TYPE == 1
  if (temp < TEMP_MIN || temp > TEMP_MAX) {
    snprintf(_lastError, sizeof(_lastError),
             "High Temp must be %.1f..%.1f°C (got %.1f)", TEMP_MIN, TEMP_MAX,
             temp);
    return false;
  }
  if (temp <= _config.lowTemp && _config.lowTemp != 0) {
    snprintf(_lastError, sizeof(_lastError),
             "High Temp (%.1f) must be > Low Temp (%.1f)", temp,
             _config.lowTemp);
    return false;
  }
  _config.highTemp = temp;
  return true;
#else
  (void)temp;
  return false;
#endif
}

bool ConfigManager::setLowHum(double hum) {
#if DEVICE_TYPE == 1
  if (hum < HUM_MIN || hum > HUM_MAX) {
    snprintf(_lastError, sizeof(_lastError),
             "Low Hum must be %.1f..%.1f%% (got %.1f)", HUM_MIN, HUM_MAX, hum);
    return false;
  }
  if (hum >= _config.highHum && _config.highHum != 0) {
    snprintf(_lastError, sizeof(_lastError),
             "Low Hum (%.1f) must be < High Hum (%.1f)", hum, _config.highHum);
    return false;
  }
  _config.lowHum = hum;
  return true;
#else
  (void)hum;
  return false;
#endif
}

bool ConfigManager::setHighHum(double hum) {
#if DEVICE_TYPE == 1
  if (hum < HUM_MIN || hum > HUM_MAX) {
    snprintf(_lastError, sizeof(_lastError),
             "High Hum must be %.1f..%.1f%% (got %.1f)", HUM_MIN, HUM_MAX, hum);
    return false;
  }
  if (hum <= _config.lowHum && _config.lowHum != 0) {
    snprintf(_lastError, sizeof(_lastError),
             "High Hum (%.1f) must be > Low Hum (%.1f)", hum, _config.lowHum);
    return false;
  }
  _config.highHum = hum;
  return true;
#else
  (void)hum;
  return false;
#endif
}

bool ConfigManager::setSensorControlMode(bool enabled) {
#if DEVICE_TYPE == 1
  if (enabled && _config.speedPercent == 0) {
    snprintf(_lastError, sizeof(_lastError),
             "Sensor Control Mode requires speed percent > 0%%");
    return false;
  }
  _config.sensorControlMode = enabled;
  return true;
#else
  (void)enabled;
  return false;
#endif
}

bool ConfigManager::setSpeedPercent(uint16_t percent) {
#if DEVICE_TYPE == 1
  if (percent > 100) {
    snprintf(_lastError, sizeof(_lastError),
             "Speed percent must be 0-100 (got %d)", percent);
    return false;
  }
  _config.speedPercent = percent;
  return true;
#else
  (void)percent;
  return false;
#endif
}

bool ConfigManager::setAdaptiveMode(bool enabled) {
#if DEVICE_TYPE == 1
  if (enabled && !_config.sensorControlMode) {
    snprintf(_lastError, sizeof(_lastError),
             "Adaptive mode requires Sensor Control Mode ON");
    return false;
  }
  if (enabled && _config.speedPercent == 0) {
    snprintf(_lastError, sizeof(_lastError),
             "Adaptive mode requires speed percent > 0%%");
    return false;
  }
  _config.adaptiveMode = enabled;
  return true;
#else
  (void)enabled;
  return false;
#endif
}

bool ConfigManager::setDelaySeconds(int seconds) {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (seconds < DELAY_SECONDS_MIN || seconds > DELAY_SECONDS_MAX) {
    snprintf(_lastError, sizeof(_lastError),
             "Delay seconds must be %d-%d (got %d)", DELAY_SECONDS_MIN,
             DELAY_SECONDS_MAX, seconds);
    return false;
  }
  _config.delaySeconds = seconds;
  return true;
#else
  (void)seconds;
  return false;
#endif
}

bool ConfigManager::setMaxOnTime(uint32_t seconds) {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (seconds > MAX_ON_TIME_MAX) {
    snprintf(_lastError, sizeof(_lastError),
             "MaxOnTime seconds must be %d-%d (got %u)", (int)MAX_ON_TIME_MIN,
             (int)MAX_ON_TIME_MAX, seconds);
    return false;
  }
  _config.maxOnTime = seconds;
  return true;
#else
  (void)seconds;
  return false;
#endif
}

bool ConfigManager::setBootState(bool state) {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  _config.bootState = state;
  return true;
#else
  (void)state;
  return false;
#endif
}

bool ConfigManager::setZigbeeNetworkKey(const char* key) {
#if ZIGBEE_ENABLED == 1
  if (!key)
    return false;
  if (strlen(key) >= sizeof(_config.zigbeeNetworkKey)) {
    snprintf(_lastError, sizeof(_lastError),
             "Zigbee network key too long (max %d)",
             (int)sizeof(_config.zigbeeNetworkKey) - 1);
    return false;
  }
  strncpy(_config.zigbeeNetworkKey, key, sizeof(_config.zigbeeNetworkKey) - 1);
  _config.zigbeeNetworkKey[sizeof(_config.zigbeeNetworkKey) - 1] = '\0';
  return true;
#else
  (void)key;
  return false;
#endif
}

bool ConfigManager::setZigbeePanId(uint16_t panId) {
#if ZIGBEE_ENABLED == 1
  _config.zigbeePanId = panId;
  return true;
#else
  (void)panId;
  return false;
#endif
}

bool ConfigManager::setZigbeeChannel(uint8_t channel) {
#if ZIGBEE_ENABLED == 1
  if (channel < 11 || channel > 26) {
    snprintf(_lastError, sizeof(_lastError),
             "Zigbee channel must be 11-26 (got %d)", channel);
    return false;
  }
  _config.zigbeeChannel = channel;
  return true;
#else
  (void)channel;
  return false;
#endif
}

// ============================================================================
// УВЕДОМЛЕНИЯ
// ============================================================================

void ConfigManager::onConfigChanged(ChangeCallback callback) {
  _changeCallback = callback;
}

// ============================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ============================================================================

void ConfigManager::setDefaults() {
  LOG_DEBUG(CAT_CONFIG, "Setting defaults");

  memset(&_config, 0, sizeof(ConfigData));

  _config.magic = MAGIC;
  _config.crc = 0;

#if MQTT_ENABLED == 1
  _config.mqttPort = MQTT_PORT;
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
  _config.maxOnTime = MAX_ON_TIME_SEC;
  _config.bootState = BOOT_SWITCH_STATE;
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  _config.sensorInterval = SENSOR_DURATION;
#endif

#if ZIGBEE_ENABLED == 1
  _config.zigbeeChannel = 15;
#endif

  // Загружаем заводские настройки (если есть)
  loadFromCredentials();

  // Генерируем MQTT Client ID
#if MQTT_ENABLED == 1
  snprintf(_config.mqttClientId, sizeof(_config.mqttClientId), "%s", _deviceId);
  LOG_DEBUG(CAT_CONFIG, "Generated MQTT Client ID: %s", _config.mqttClientId);
#endif

  LOG_DEBUG(CAT_CONFIG, "Defaults set");
}

void ConfigManager::loadFromCredentials() {
#if HAS_CREDENTIALS
  LOG_DEBUG(CAT_CONFIG, "Loading factory settings");
#if WIFI_ENABLED == 1
  if (strlen(SSID_NAME) > 0) {
    strncpy(_config.wifiSsid, SSID_NAME, sizeof(_config.wifiSsid) - 1);
    _config.wifiSsid[sizeof(_config.wifiSsid) - 1] = '\0';
  }
  if (strlen(WIFI_PASSWORD) > 0) {
    strncpy(_config.wifiPassword, WIFI_PASSWORD,
            sizeof(_config.wifiPassword) - 1);
    _config.wifiPassword[sizeof(_config.wifiPassword) - 1] = '\0';
  }
#endif
#if MQTT_ENABLED == 1
  if (strlen(MQTT_ADDRESS) > 0) {
    strncpy(_config.mqttBroker, MQTT_ADDRESS, sizeof(_config.mqttBroker) - 1);
    _config.mqttBroker[sizeof(_config.mqttBroker) - 1] = '\0';
  }
  if (strlen(MQTT_USER) > 0) {
    strncpy(_config.mqttUser, MQTT_USER, sizeof(_config.mqttUser) - 1);
    _config.mqttUser[sizeof(_config.mqttUser) - 1] = '\0';
  }
  if (strlen(MQTT_PASSWORD) > 0) {
    strncpy(_config.mqttPassword, MQTT_PASSWORD,
            sizeof(_config.mqttPassword) - 1);
    _config.mqttPassword[sizeof(_config.mqttPassword) - 1] = '\0';
  }
#endif
#endif
}

void ConfigManager::readFromEEPROM() {
  LOG_INFO(CAT_CONFIG, "Reading from EEPROM...");

  ConfigData raw;
  memset(&raw, 0, sizeof(ConfigData));

  uint8_t* ptr = reinterpret_cast<uint8_t*>(&raw);
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    ptr[i] = EEPROM.read(i);
  }

  LOG_DEBUG(CAT_CONFIG, "Read magic: 0x%04X (expected 0x%04X)", raw.magic,
            MAGIC);

  // Устанавливаем значения по умолчанию
  setDefaults();

  // Проверяем EEPROM
  if (raw.magic == MAGIC) {
    uint16_t savedCrc = raw.crc;
    raw.crc = 0;
    uint16_t calcCrc = calculateCRC(raw);

    LOG_DEBUG(CAT_CONFIG, "Calculated CRC: 0x%04X (saved: 0x%04X)", calcCrc,
              savedCrc);

    if (calcCrc == savedCrc) {
      LOG_DEBUG(CAT_CONFIG, "CRC is VALID, applying EEPROM config...");

      // Применяем все настройки из EEPROM через сеттеры
      validateAndApply(raw);

      _configValid = true;
      LOG_INFO(CAT_CONFIG, "Config loaded from EEPROM");
      return;
    } else {
      LOG_WARN(CAT_CONFIG, "CRC mismatch! Using defaults.");
    }
  } else {
    LOG_WARN(CAT_CONFIG, "Magic mismatch! Using defaults.");
  }

  _configValid = false;
  LOG_WARN(CAT_CONFIG, "Using defaults (EEPROM invalid)");
}

bool ConfigManager::validateAndApply(const ConfigData& raw) {
  bool ok = true;

#if WIFI_ENABLED == 1
  if (strlen(raw.wifiSsid) > 0) {
    if (!setWifiSsid(raw.wifiSsid)) {
      LOG_WARN(CAT_CONFIG, "Failed to set WiFi SSID: %s", _lastError);
      ok = false;
    }
  }
  if (strlen(raw.wifiPassword) > 0) {
    if (!setWifiPassword(raw.wifiPassword)) {
      LOG_WARN(CAT_CONFIG, "Failed to set WiFi password: %s", _lastError);
      ok = false;
    }
  }

#if MQTT_ENABLED == 1
  if (strlen(raw.mqttBroker) > 0) {
    if (!setMqttBroker(raw.mqttBroker)) {
      LOG_WARN(CAT_CONFIG, "Failed to set MQTT broker: %s", _lastError);
      ok = false;
    }
  }
  if (!setMqttPort(raw.mqttPort)) {
    LOG_WARN(CAT_CONFIG, "Failed to set MQTT port: %s", _lastError);
    ok = false;
  }
  if (strlen(raw.mqttUser) > 0) {
    if (!setMqttUser(raw.mqttUser)) {
      LOG_WARN(CAT_CONFIG, "Failed to set MQTT user: %s", _lastError);
      ok = false;
    }
  }
  if (strlen(raw.mqttPassword) > 0) {
    if (!setMqttPassword(raw.mqttPassword)) {
      LOG_WARN(CAT_CONFIG, "Failed to set MQTT password: %s", _lastError);
      ok = false;
    }
  }
  if (strlen(raw.mqttClientId) > 0) {
    if (!setMqttClientId(raw.mqttClientId)) {
      LOG_WARN(CAT_CONFIG, "Failed to set MQTT client ID: %s", _lastError);
      ok = false;
    }
  }
#endif  // MQTT_ENABLED
#endif  // WIFI_ENABLED

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (!setSensorInterval(raw.sensorInterval)) {
    LOG_WARN(CAT_CONFIG, "Failed to set sensor interval: %s", _lastError);
    ok = false;
  }
#endif

#if DEVICE_TYPE == 1
  if (!setLowTemp(raw.lowTemp)) {
    LOG_WARN(CAT_CONFIG, "Failed to set low temp: %s", _lastError);
    ok = false;
  }
  if (!setHighTemp(raw.highTemp)) {
    LOG_WARN(CAT_CONFIG, "Failed to set high temp: %s", _lastError);
    ok = false;
  }
  if (!setLowHum(raw.lowHum)) {
    LOG_WARN(CAT_CONFIG, "Failed to set low hum: %s", _lastError);
    ok = false;
  }
  if (!setHighHum(raw.highHum)) {
    LOG_WARN(CAT_CONFIG, "Failed to set high hum: %s", _lastError);
    ok = false;
  }
  if (!setSpeedPercent(raw.speedPercent)) {
    LOG_WARN(CAT_CONFIG, "Failed to set speed percent: %s", _lastError);
    ok = false;
  }
  setAdaptiveMode(raw.adaptiveMode);
  setSensorControlMode(raw.sensorControlMode);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (!setDelaySeconds(raw.delaySeconds)) {
    LOG_WARN(CAT_CONFIG, "Failed to set delay seconds: %s", _lastError);
    ok = false;
  }
  if (!setMaxOnTime(raw.maxOnTime)) {
    LOG_WARN(CAT_CONFIG, "Failed to set max on time: %s", _lastError);
    ok = false;
  }
  setBootState(raw.bootState);
#endif

#if ZIGBEE_ENABLED == 1
  if (strlen(raw.zigbeeNetworkKey) > 0) {
    if (!setZigbeeNetworkKey(raw.zigbeeNetworkKey)) {
      LOG_WARN(CAT_CONFIG, "Failed to set Zigbee network key: %s", _lastError);
      ok = false;
    }
  }
  setZigbeePanId(raw.zigbeePanId);
  setZigbeeChannel(raw.zigbeeChannel);
#endif

  return ok;
}

bool ConfigManager::isConfigValid(const ConfigData& config) const {
  // Проверяем, что есть WiFi SSID (если WiFi включён)
#if WIFI_ENABLED == 1
  if (strlen(config.wifiSsid) == 0) {
    return false;
  }
#endif

  // Проверяем, что есть MQTT брокер (если MQTT включён)
#if MQTT_ENABLED == 1
  if (strlen(config.mqttBroker) == 0) {
    return false;
  }
#endif

  return true;
}

uint16_t ConfigManager::calculateCRC(const ConfigData& config) const {
  ConfigData copy = config;
  copy.crc = 0;
  return crc16_impl(reinterpret_cast<const uint8_t*>(&copy),
                    sizeof(ConfigData));
}

void ConfigManager::initDeviceId() {
#if defined(ESP32)
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(_deviceId, sizeof(_deviceId), "%s_%02X%02X",
#if DEVICE_TYPE == 1
           "fan",
#elif DEVICE_TYPE == 2
           "sensor",
#elif DEVICE_TYPE == 3
           "switch",
#else
           "device",
#endif
           mac[4], mac[5]);
#elif defined(ESP8266)
  uint32_t chipId = ESP.getChipId();
  snprintf(_deviceId, sizeof(_deviceId), "%s_%04X",
#if DEVICE_TYPE == 1
           "fan",
#elif DEVICE_TYPE == 2
           "sensor",
#elif DEVICE_TYPE == 3
           "switch",
#else
           "device",
#endif
           chipId & 0xFFFF);
#endif

  LOG_DEBUG(CAT_CONFIG, "Device ID initialized: %s", _deviceId);
}

void ConfigManager::print() const {
  LOG_DEBUG(CAT_CONFIG, "=== Config ===");
#if WIFI_ENABLED == 1
  LOG_DEBUG(CAT_CONFIG, "WiFi SSID: '%s'", _config.wifiSsid);
  LOG_DEBUG(CAT_CONFIG, "WiFi Password: %s",
            _config.wifiPassword[0] ? "***" : "(empty)");
#if MQTT_ENABLED == 1
  LOG_DEBUG(CAT_CONFIG, "MQTT Broker: '%s:%d'", _config.mqttBroker,
            _config.mqttPort);
  LOG_DEBUG(CAT_CONFIG, "MQTT User: '%s'", _config.mqttUser);
  LOG_DEBUG(CAT_CONFIG, "MQTT Client ID: '%s'", _config.mqttClientId);
#endif
#endif

#if DEVICE_TYPE == 1
  LOG_DEBUG(CAT_CONFIG, "Temp range: %.1f - %.1f", _config.lowTemp,
            _config.highTemp);
  LOG_DEBUG(CAT_CONFIG, "Hum range: %.1f - %.1f", _config.lowHum,
            _config.highHum);
  LOG_DEBUG(CAT_CONFIG, "Sensor control mode: %s",
            _config.sensorControlMode ? "ON" : "OFF");
  LOG_DEBUG(CAT_CONFIG, "Speed percent: %d%%", _config.speedPercent);
  LOG_DEBUG(CAT_CONFIG, "Adaptive mode: %s",
            _config.adaptiveMode ? "ON" : "OFF");
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  LOG_DEBUG(CAT_CONFIG, "Delay: %d sec", _config.delaySeconds);
  LOG_DEBUG(CAT_CONFIG, "MaxOnTime: %u sec", _config.maxOnTime);
  LOG_DEBUG(CAT_CONFIG, "Boot state: %s", _config.bootState ? "ON" : "OFF");
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  LOG_DEBUG(CAT_CONFIG, "Sensor interval: %d sec", _config.sensorInterval);
#endif

#if ZIGBEE_ENABLED == 1
  LOG_DEBUG(CAT_CONFIG, "Zigbee PAN ID: 0x%04X", _config.zigbeePanId);
  LOG_DEBUG(CAT_CONFIG, "Zigbee Channel: %d", _config.zigbeeChannel);
#endif

  LOG_DEBUG(CAT_CONFIG, "CRC: 0x%04X", _config.crc);
  LOG_DEBUG(CAT_CONFIG, "Config valid: %s", _configValid ? "YES" : "NO");
  LOG_DEBUG(CAT_CONFIG, "=================");
}