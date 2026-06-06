#include "config.h"
#include "led.h"
#include "ansi.h"

#ifdef ESP32
#include <esp_mac.h>
#endif


static Config _config;
static bool _configValid = false;
static char _configLastError[64] = "";
static bool _configInitialized = false;

char deviceId[12] = "";

bool apMode = false;

uint16_t crc16(const uint8_t* data, size_t len) {
  
  uint16_t crc = 0x0000;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x8005;
      else crc <<= 1;
    }
  }
  return crc;
}

const Config* config_get() {
    if (!_configInitialized) {
        // Не инициализирован — возвращаем указатель на статическую структуру
        // (она может быть не заполнена, но это лучше чем null)
        return &_config;
    }
    return &_config;
}

bool config_isValid() {
    return _configValid;
}

const char* config_getLastError() {
    return _configLastError;
}

// --- Сеттеры с валидацией ---

bool config_setWifiSsid(const char* ssid) {
    if (!ssid || strlen(ssid) == 0) {
        snprintf(_configLastError, sizeof(_configLastError), "WiFi SSID cannot be empty");
        return false;
    }
    if (strlen(ssid) >= sizeof(_config.wifiSsid)) {
        snprintf(_configLastError, sizeof(_configLastError), "WiFi SSID too long (max %d)", 
                 (int)sizeof(_config.wifiSsid) - 1);
        return false;
    }
    strncpy(_config.wifiSsid, ssid, sizeof(_config.wifiSsid) - 1);
    _config.wifiSsid[sizeof(_config.wifiSsid) - 1] = '\0';
    return true;
}

bool config_setWifiPassword(const char* password) {
    if (!password) return false;
    if (strlen(password) >= sizeof(_config.wifiPassword)) {
        snprintf(_configLastError, sizeof(_configLastError), "WiFi password too long (max %d)",
                 (int)sizeof(_config.wifiPassword) - 1);
        return false;
    }
    if (strlen(password) > 0) {
        strncpy(_config.wifiPassword, password, sizeof(_config.wifiPassword) - 1);
        _config.wifiPassword[sizeof(_config.wifiPassword) - 1] = '\0';
    } else {
        _config.wifiPassword[0] = '\0';
    }
    return true;
}

bool config_setWifiOutputPower(float power) {
    if (power < 0 || power > 20.5) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "WiFi output power must be 0-20.5 dBm (got %.1f)", power);
        return false;
    }
    _config.wifiOutputPower = power;
    return true;
}

#if MQTT_ENABLED == 1
bool config_setMqttBroker(const char* broker) {
    if (!broker || strlen(broker) == 0) {
        snprintf(_configLastError, sizeof(_configLastError), "MQTT Broker cannot be empty");
        return false;
    }
    if (strlen(broker) >= sizeof(_config.mqttBroker)) {
        snprintf(_configLastError, sizeof(_configLastError), "MQTT Broker too long (max %d)",
                 (int)sizeof(_config.mqttBroker) - 1);
        return false;
    }
    strncpy(_config.mqttBroker, broker, sizeof(_config.mqttBroker) - 1);
    _config.mqttBroker[sizeof(_config.mqttBroker) - 1] = '\0';
    return true;
}

bool config_setMqttPort(uint16_t port) {
    if (port < 1 || port > 65535) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "MQTT Port must be 1-65535 (got %d)", port);
        return false;
    }
    _config.mqttPort = port;
    return true;
}

bool config_setMqttUser(const char* user) {
    if (!user) return false;
    if (strlen(user) >= sizeof(_config.mqttUser)) {
        snprintf(_configLastError, sizeof(_configLastError), "MQTT User too long (max %d)",
                 (int)sizeof(_config.mqttUser) - 1);
        return false;
    }
    strncpy(_config.mqttUser, user, sizeof(_config.mqttUser) - 1);
    _config.mqttUser[sizeof(_config.mqttUser) - 1] = '\0';
    return true;
}

bool config_setMqttPassword(const char* password) {
    if (!password) return false;
    if (strlen(password) >= sizeof(_config.mqttPassword)) {
        snprintf(_configLastError, sizeof(_configLastError), "MQTT Password too long (max %d)",
                 (int)sizeof(_config.mqttPassword) - 1);
        return false;
    }
    if (strlen(password) > 0) {
        strncpy(_config.mqttPassword, password, sizeof(_config.mqttPassword) - 1);
        _config.mqttPassword[sizeof(_config.mqttPassword) - 1] = '\0';
    } else {
        _config.mqttPassword[0] = '\0';
    }
    return true;
}

bool config_setMqttClientId(const char* clientId) {
    if (!clientId || strlen(clientId) == 0) {
        snprintf(_configLastError, sizeof(_configLastError), "MQTT Client ID cannot be empty");
        return false;
    }
    if (strlen(clientId) >= sizeof(_config.mqttClientId)) {
        snprintf(_configLastError, sizeof(_configLastError), "MQTT Client ID too long (max %d)",
                 (int)sizeof(_config.mqttClientId) - 1);
        return false;
    }
    // Валидация символов
    for (size_t i = 0; i < strlen(clientId); i++) {
        char c = clientId[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            snprintf(_configLastError, sizeof(_configLastError), 
                     "MQTT Client ID invalid char '%c' (use a-z, A-Z, 0-9, _, -)", c);
            return false;
        }
    }
    strncpy(_config.mqttClientId, clientId, sizeof(_config.mqttClientId) - 1);
    _config.mqttClientId[sizeof(_config.mqttClientId) - 1] = '\0';
    return true;
}
#endif // MQTT_ENABLED

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
bool config_setDelaySeconds(int seconds) {
    if (seconds < 0 || seconds > 86400) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Delay seconds must be 0-86400 (got %d)", seconds);
        return false;
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
    _config.maxOnTime = seconds;
    return true;
}

bool config_setBootState(bool state) {
    _config.bootState = state;
    return true;
}
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
bool config_setSensorInterval(uint16_t interval) {
    if (interval < 1 || interval > 3600) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Sensor interval must be 1-3600 seconds (got %d)", interval);
        return false;
    }
    _config.sensorInterval = interval;
    return true;
}
#endif

#if DEVICE_TYPE == 1
bool config_setLowTemp(double temp) {
    if (temp < -40 || temp > 85) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Low Temp must be -40..85°C (got %.1f)", temp);
        return false;
    }
    if (temp >= _config.highTemp && _config.highTemp != 0) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Low Temp (%.1f) must be < High Temp (%.1f)", temp, _config.highTemp);
        return false;
    }
    _config.lowTemp = temp;
    return true;
}

bool config_setHighTemp(double temp) {
    if (temp < -40 || temp > 85) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "High Temp must be -40..85°C (got %.1f)", temp);
        return false;
    }
    if (temp <= _config.lowTemp && _config.lowTemp != 0) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "High Temp (%.1f) must be > Low Temp (%.1f)", temp, _config.lowTemp);
        return false;
    }
    _config.highTemp = temp;
    return true;
}

bool config_setLowHum(double hum) {
    if (hum < 0 || hum > 100) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Low Hum must be 0..100%% (got %.1f)", hum);
        return false;
    }
    if (hum >= _config.highHum && _config.highHum != 0) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Low Hum (%.1f) must be < High Hum (%.1f)", hum, _config.highHum);
        return false;
    }
    _config.lowHum = hum;
    return true;
}

bool config_setHighHum(double hum) {
    if (hum < 0 || hum > 100) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "High Hum must be 0..100%% (got %.1f)", hum);
        return false;
    }
    if (hum <= _config.lowHum && _config.lowHum != 0) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "High Hum (%.1f) must be > Low Hum (%.1f)", hum, _config.lowHum);
        return false;
    }
    _config.highHum = hum;
    return true;
}

bool config_setSensorControlMode(bool enabled) {
    // Дополнительная бизнес-логика
    if (enabled && _config.speedPercent == 0) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Sensor Control Mode requires speed percent > 0%%");
        return false;
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
    _config.adaptiveMode = enabled;
    return true;
}
#endif // DEVICE_TYPE == 1


static void config_loadFromCredentials() {
  #if HAS_CREDENTIALS
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Loading factory settings");
    #endif
    
    if (strlen(SSID_NAME) > 0) {
      strncpy(_config.wifiSsid, SSID_NAME, sizeof(_config.wifiSsid) - 1);
      _config.wifiSsid[sizeof(_config.wifiSsid) - 1] = '\0';
    } else {
      memset(_config.wifiSsid, 0, sizeof(_config.wifiSsid));
    }
    
    if (strlen(WIFI_PASSWORD) > 0) {
      strncpy(_config.wifiPassword, WIFI_PASSWORD, sizeof(_config.wifiPassword) - 1);
      _config.wifiPassword[sizeof(_config.wifiPassword) - 1] = '\0';
    } else {
      memset(_config.wifiPassword, 0, sizeof(_config.wifiPassword));
    }
  #if MQTT_ENABLED == 1
    if (strlen(MQTT_ADDRESS) > 0) {
      strncpy(_config.mqttBroker, MQTT_ADDRESS, sizeof(_config.mqttBroker) - 1);
      _config.mqttBroker[sizeof(_config.mqttBroker) - 1] = '\0';
    } else {
      memset(_config.mqttBroker, 0, sizeof(_config.mqttBroker));
    }
    
    _config.mqttPort = MQTT_PORT;
    
    if (strlen(MQTT_USER) > 0) {
      strncpy(_config.mqttUser, MQTT_USER, sizeof(_config.mqttUser) - 1);
      _config.mqttUser[sizeof(_config.mqttUser) - 1] = '\0';
    } else {
      memset(_config.mqttUser, 0, sizeof(_config.mqttUser));
    }
    
    if (strlen(MQTT_PASSWORD) > 0) {
      strncpy(_config.mqttPassword, MQTT_PASSWORD, sizeof(_config.mqttPassword) - 1);
      _config.mqttPassword[sizeof(_config.mqttPassword) - 1] = '\0';
    } else {
      memset(_config.mqttPassword, 0, sizeof(_config.mqttPassword));
    }
  #endif

  #else
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] No factory settings found, using empty defaults");
    #endif
  #endif
}

void config_setDefaults() {
  #if DEBUG_ENABLED == 1
    Serial.println("[CONFIG] Setting defaults");
  #endif
  
  memset(&_config, 0, sizeof(Config));
  
  _config.magic = MAGIC_VALUE;
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
  
  initDeviceId();
  config_loadFromCredentials();
  
  #if MQTT_ENABLED == 1
    snprintf(_config.mqttClientId, sizeof(_config.mqttClientId), "%s", deviceId);
    
    #if DEBUG_ENABLED == 1
      Serial.printf("[CONFIG] Generated MQTT Client ID: %s\n", _config.mqttClientId);
    #endif
  #endif

  #if WIFI_ENABLED == 1
    _config.wifiOutputPower = WIFI_OUTPUT_POWER;
  #endif
  

  #if DEBUG_ENABLED == 1
    Serial.println("[CONFIG] Defaults set");
  #endif
}

bool config_clear() {
  #if STATUS_LED_PIN > 0
    led_setMode(LED_MODE_FAST_BLINK);
  #endif

  #if DEBUG_ENABLED == 1
    Serial.print(ANSI_BRIGHT_RED);
    Serial.println("[CONFIG] Erasing EEPROM...");
    Serial.print(ANSI_RESET);
  #endif

  EEPROM.end();
  delay(50);
  EEPROM.begin(sizeof(Config) + 4);
  
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, 0);
  }
  
  bool ok = EEPROM.commit();
  
  #if DEBUG_ENABLED == 1
    Serial.printf("[CONFIG] Clear %s\n", ok ? "SUCCESSFUL" : "FAILED");
  #endif
  
  EEPROM.end();
  
  if (ok) {
    // Сбросить структуры
    memset(&_config, 0, sizeof(Config));
    _configValid = false;

  }
  
  return ok;
}

bool config_validate() {
  _configLastError[0] = '\0';
  bool valid = true;
  
  // === СЕТЕВЫЕ НАСТРОЙКИ ===
  #if MQTT_ENABLED == 1
  if (_config.mqttPort < 1 || _config.mqttPort > 65535) {
    snprintf(_configLastError, sizeof(_configLastError), "MQTT Port must be 1-65535");
    valid = false;
  }
  #endif

  if (strlen(_config.wifiSsid) == 0) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "WiFi SSID cannot be empty");
    }
    valid = false;
  }
  
  #if MQTT_ENABLED == 1
  if (strlen(_config.mqttBroker) == 0) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "MQTT Broker cannot be empty");
    }
    valid = false;
  } 
  
  if (strlen(_config.mqttClientId) == 0) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "MQTT Client ID cannot be empty");
    }
    valid = false;
  } else {
    for (size_t i = 0; i < strlen(_config.mqttClientId); i++) {
      char c = _config.mqttClientId[i];
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || c == '_' || c == '-')) {
        snprintf(_configLastError, sizeof(_configLastError), 
         "MQTT Client ID invalid char '%c' (use a-z, A-Z, 0-9, _, -)", c);
        valid = false;
        break;
      }
    }
  }
  #endif

  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (_config.delaySeconds < 0 || _config.delaySeconds > 86400) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "Delay must be 0-86400 seconds");
    }
    valid = false;
  }
  
  if (_config.maxOnTime < 0 || _config.maxOnTime > 86400) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "MaxOnTime must be 0-86400 seconds");
    }
    valid = false;
  }
  
  #if DEVICE_TYPE == 1
  if (_config.speedPercent > 100) {  
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "Speed percent must be 0-100");
    }
    valid = false;
  }
  
  if (_config.adaptiveMode && !_config.sensorControlMode) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "Adaptive mode requires Sensor Control Mode ON");
    }
    valid = false;
  }
  
  if (_config.adaptiveMode && _config.speedPercent == 0) {  
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "Adaptive mode requires speed percent > 0%%");
    }
    valid = false;
  }
  #endif
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (_config.sensorInterval < 1 || _config.sensorInterval > 3600) {
    char buf[80];
    snprintf(buf, sizeof(buf), "Sensor interval must be 1-3600 seconds, but = %d", _config.sensorInterval);
    strncpy(_configLastError, buf, sizeof(_configLastError) - 1);
    _configLastError[sizeof(_configLastError) - 1] = '\0';
    valid = false;
  }
  #endif
  
  #if DEVICE_TYPE == 1
  if (_config.lowTemp < -40 || _config.lowTemp > 85) {
    char buf[80];
    snprintf(buf, sizeof(buf), "Low Temp must be -40..85°C, but = %.1f", _config.lowTemp);
    strncpy(_configLastError, buf, sizeof(_configLastError) - 1);
    _configLastError[sizeof(_configLastError) - 1] = '\0';
    valid = false;
  }
  if (_config.highTemp < -40 || _config.highTemp > 85) {
    char buf[80];
    snprintf(buf, sizeof(buf), "High Temp must be -40..85°C, but = %.1f", _config.highTemp);
    strncpy(_configLastError, buf, sizeof(_configLastError) - 1);
    _configLastError[sizeof(_configLastError) - 1] = '\0';
    valid = false;
  }
  if (_config.lowTemp >= _config.highTemp) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "Low Temp must be < High Temp");
    }
    valid = false;
  }
  
  if (_config.lowHum < 0 || _config.lowHum > 100) {
    char buf[80];
    snprintf(buf, sizeof(buf), "Low Hum must be 0..100%%, but = %.1f", _config.lowHum);
    strncpy(_configLastError, buf, sizeof(_configLastError) - 1);
    _configLastError[sizeof(_configLastError) - 1] = '\0';
    valid = false;
  }
  if (_config.highHum < 0 || _config.highHum > 100) {
    char buf[80];
    snprintf(buf, sizeof(buf), "High Hum must be 0..100%%, but = %.1f", _config.highHum);
    strncpy(_configLastError, buf, sizeof(_configLastError) - 1);
    _configLastError[sizeof(_configLastError) - 1] = '\0';
    valid = false;
  }
  if (_config.lowHum >= _config.highHum) {
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "Low Hum must be < High Hum");
    }
    valid = false;
  }
  
  if (_config.sensorControlMode && _config.speedPercent == 0) {  
    if (_configLastError[0] == '\0') {
      snprintf(_configLastError, sizeof(_configLastError), "Sensor Control Mode requires speed percent > 0%%");
    }
    valid = false;
  }
  #endif
  
  #if DEBUG_ENABLED == 1
    if (!valid) {
      Serial.print(ANSI_BRIGHT_RED);
      Serial.printf("[CONFIG] Validation failed: %s\n", _configLastError);
      Serial.print(ANSI_RESET);
    }
  #endif
  
  return valid;
}

void config_read() {
  #if DEBUG_ENABLED == 1
    Serial.println("[CONFIG] Reading from EEPROM...");
  #endif
  
  memset(&_config, 0, sizeof(Config));
  
  uint8_t* ptr = (uint8_t*)&_config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    ptr[i] = EEPROM.read(i);
  }
  
  #if DEBUG_ENABLED == 1
    Serial.printf("[CONFIG] Read magic: 0x%04X (expected 0x%04X)\n", _config.magic, MAGIC_VALUE);
    Serial.printf("[CONFIG] Read WiFi SSID: '%s'\n", _config.wifiSsid);
    #if MQTT_ENABLED == 1
      Serial.printf("[CONFIG] Read MQTT Broker: '%s'\n", _config.mqttBroker);
      Serial.printf("[CONFIG] Read MQTT User: '%s'\n", _config.mqttUser);
      Serial.printf("[CONFIG] Read MQTT Client ID: '%s'\n", _config.mqttClientId);
    #endif
    Serial.printf("[CONFIG] Read CRC from EEPROM: 0x%04X\n", _config.crc);
  #endif
  
  bool eepromValid = false;
  
  if (_config.magic == MAGIC_VALUE) {
    uint16_t savedCrc = _config.crc;
    _config.crc = 0;
    
    uint16_t calcCrc = crc16((uint8_t*)&_config, sizeof(Config));
    
    #if DEBUG_ENABLED == 1
      Serial.printf("[CONFIG] Calculated CRC: 0x%04X\n", calcCrc);
    #endif
    
    if (calcCrc == savedCrc) {
      _config.crc = savedCrc;
      eepromValid = true;
      #if DEBUG_ENABLED == 1
        Serial.println("[CONFIG] CRC is VALID");
      #endif
    } else {  
      #if DEBUG_ENABLED == 1
        Serial.print(ANSI_BRIGHT_RED);    
        Serial.printf("[CONFIG] CRC mismatch! EEPROM: 0x%04X, Calculated: 0x%04X\n", savedCrc, calcCrc);
        Serial.print(ANSI_RESET);
      #endif
    }
  } else {
      #if DEBUG_ENABLED == 1
        Serial.print(ANSI_BRIGHT_RED);
        Serial.println("[CONFIG] Magic mismatch! Config is INVALID");
        Serial.print(ANSI_RESET);
      #endif
  }
  
  bool dataValid = config_validate();
  
  if (eepromValid && dataValid) {
    _configValid = true;
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Config is VALID (EEPROM)");
    #endif
  } else {
    _configValid = false;
    config_setDefaults();
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Using defaults for setup mode");
    #endif
  }
  
  
}

bool config_write() {
  #if LOG_CONFIG == 1
    Serial.println(ANSI_BRIGHT_RED "[CONFIG] Writing to EEPROM..." ANSI_RESET);
    Serial.printf("[CONFIG] WiFi SSID: '%s'\n", _config.wifiSsid);
    #if MQTT_ENABLED == 1
    Serial.printf("[CONFIG] MQTT Broker: '%s:%d'\n", _config.mqttBroker, _config.mqttPort);
    Serial.printf("[CONFIG] MQTT Client ID: '%s'\n", _config.mqttClientId);
    #endif
  #endif
  
  uint16_t oldCrc = _config.crc;
  _config.crc = 0;
  
  _config.crc = crc16((uint8_t*)&_config, sizeof(Config));
  
  #if DEBUG_ENABLED == 1
    Serial.printf("[CONFIG] Calculated CRC: 0x%04X\n", _config.crc);
  #endif
  
  //@TODO: Убрать после отладки
  #if SIMULATE_EEPROM_MALFUNCTION == 1
    Serial.println(ANSI_BRIGHT_RED "[CONFIG] SIMULATE: EEPROM commit FAILED" ANSI_RESET);
    _config.crc = oldCrc;
    return false;
  #endif

  uint8_t* ptr = (uint8_t*)&_config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, ptr[i]);
  }
  
  if (!EEPROM.commit()) {
    #if DEBUG_ENABLED == 1
      Serial.println(ANSI_BRIGHT_RED "[CONFIG] EEPROM commit FAILED! Configuration NOT saved." ANSI_RESET);
    #endif
    return false;
  }
  
  #if DEBUG_ENABLED == 1
    Serial.println(ANSI_MAGENTA "[CONFIG] Write completed!" ANSI_RESET);
    Serial.println("[CONFIG] Verifying saved data...");
  #endif
  
  Config verify;
  memset(&verify, 0, sizeof(Config));
  
  uint8_t* vptr = (uint8_t*)&verify;
  for (size_t i = 0; i < sizeof(Config); i++) {
    vptr[i] = EEPROM.read(i);
  }
  
  verify.crc = 0;
  uint16_t calcVerifyCrc = crc16((uint8_t*)&verify, sizeof(Config));
  
  #if DEBUG_ENABLED == 1
    #if MQTT_ENABLED == 1
    Serial.printf("[CONFIG] Verify magic: 0x%04X, WiFi: '%s', ClientID: '%s', Calculated: 0x%04X\n", 
                  verify.magic, verify.wifiSsid, verify.mqttClientId, calcVerifyCrc);
    #else
    Serial.printf("[CONFIG] Verify magic: 0x%04X, WiFi: '%s', Calculated: 0x%04X\n", 
                  verify.magic, verify.wifiSsid, calcVerifyCrc);
    #endif
  #endif
  
  if (verify.magic == _config.magic && calcVerifyCrc == _config.crc) {
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Verification saved config PASSED");
    #endif
    _configValid = true;
    
    return true;
    
  } else {
    #if DEBUG_ENABLED == 1
      Serial.print(ANSI_BRIGHT_RED);
      Serial.println("[CONFIG] Verification saved config FAILED!");
      Serial.print(ANSI_RESET);
    #endif
    _config.crc = oldCrc;
    _configValid = false;
    return false;
  }
}

void initDeviceId() {
  #ifdef ESP32
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(deviceId, sizeof(deviceId), "%s_%02X%02X", DEVICE_PREFIX, mac[4], mac[5]);
  #elif defined(ESP8266)
    uint32_t chipId = ESP.getChipId();
    snprintf(deviceId, sizeof(deviceId), "%s_%04X", DEVICE_PREFIX, chipId & 0xFFFF);
  #endif
  
  #if DEBUG_ENABLED == 1
    Serial.printf("[CONFIG] Device ID initialized: %s\n", deviceId);
  #endif
}

void config_init() {
  initDeviceId();

  #if DEBUG_ENABLED == 1
    Serial.println("[CONFIG] Initializing EEPROM...");
  #endif
  EEPROM.begin(sizeof(Config) + 4);
  #if DEBUG_ENABLED == 1
    Serial.printf("[CONFIG] EEPROM size: %d bytes\n", EEPROM.length());
  #endif
  config_read();
  _configInitialized = true;
}

Config config_getSaved() {
    Config saved;
    memset(&saved, 0, sizeof(Config));
    
    uint8_t* ptr = (uint8_t*)&saved;
    for (size_t i = 0; i < sizeof(Config); i++) {
        ptr[i] = EEPROM.read(i);
    }
    
    if (saved.magic != MAGIC_VALUE) {
        #if DEBUG_ENABLED == 1
            Serial.println("[CONFIG] config_getSaved: invalid magic, returning defaults");
        #endif
        config_setDefaults();
        memcpy(&saved, &_config, sizeof(Config));
        saved.crc = 0;
    }
    
    return saved;
}

#if DEBUG_ENABLED == 1
void config_print() {
  #if LOG_CONFIG == 1
    Serial.println("=== Config ===");
    Serial.printf("WiFi SSID: '%s'\n", _config.wifiSsid);
    Serial.printf("WiFi Password: %s\n", _config.wifiPassword[0] ? "***" : "(empty)");
    #if MQTT_ENABLED == 1
    Serial.printf("MQTT Broker: '%s:%d'\n", _config.mqttBroker, _config.mqttPort);
    Serial.printf("MQTT User: '%s'\n", _config.mqttUser);
    Serial.printf("MQTT Client ID: '%s'\n", _config.mqttClientId);
    #endif
  
    #if DEVICE_TYPE == 1
      Serial.printf("Temp range: %.1f - %.1f\n", _config.lowTemp, _config.highTemp);
      Serial.printf("Hum range: %.1f - %.1f\n", _config.lowHum, _config.highHum);
      Serial.printf("Sensor control mode: %s\n", _config.sensorControlMode ? "ON" : "OFF");
      Serial.printf("Speed percent: %d%% (%s)\n", 
                    _config.speedPercent,  
                    _config.speedPercent == 100 ? "full power" : "slow mode");
      Serial.printf("Adaptive mode: %s\n", _config.adaptiveMode ? "ON" : "OFF");
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
      Serial.printf("Delay: %d sec\n", _config.delaySeconds);
      Serial.printf("MaxOnTime: %d sec\n", _config.maxOnTime);
      Serial.printf("Boot state: %s\n", _config.bootState ? "ON" : "OFF");
    #endif
  
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
      Serial.printf("Sensor interval: %d sec\n", _config.sensorInterval);
      Serial.printf("Sensor type: %d\n", SENSOR_TYPE);
      #if SENSOR_TYPE == 2
        Serial.printf("Sensor pin: %d\n", SENSOR_PIN);
      #endif
    #endif
  #endif //LOG_CONFIG == 1
  Serial.printf("CRC: 0x%04X\n", _config.crc);
  Serial.printf("Config valid: %s\n", _configValid ? "YES" : "NO");
  
  if (!_configValid || strlen(_config.wifiSsid) == 0) {
    Serial.println("Mode: SETUP (AP will be started)");
  } else {
    Serial.println("Mode: NORMAL");
  }
  
  if (!_configValid && HAS_CREDENTIALS) {
    Serial.print(ANSI_BRIGHT_RED);
    Serial.println("Loaded factory settings.");
    Serial.print(ANSI_RESET);
  }
  Serial.println("=================");
}
#endif