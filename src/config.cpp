#include <Arduino.h>
#include "config.h"
#include "led.h"
#include "logger.h"

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
#if WIFI_ENABLED == 1
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
#endif // WIFI_ENABLED
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
    if (temp < TEMP_MIN || temp > TEMP_MAX) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "Low Temp must be %.1f..%.1f°C (got %.1f)", TEMP_MIN, TEMP_MAX, temp);
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
    if (temp < TEMP_MIN || temp > TEMP_MAX) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "High Temp must be %.1f..%.1f°C (got %.1f)", TEMP_MIN, TEMP_MAX, temp);
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
    _config.highHum = hum;
    return true;
}





bool config_setMaxOnTime(uint32_t seconds) {
    if (seconds > MAX_ON_TIME_MAX) {
        snprintf(_configLastError, sizeof(_configLastError), 
                 "MaxOnTime seconds must be %d-%d (got %u)", 
                 MAX_ON_TIME_MIN, MAX_ON_TIME_MAX, seconds);
        return false;
    }
    _config.maxOnTime = seconds;
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
    LOG_DEBUG(CAT_CONFIG, "Loading factory settings");
    #if WIFI_ENABLED == 1
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
  #endif // MQTT_ENABLED
  #endif // WIFI_ENABLED
  #else
    LOG_WARN(CAT_CONFIG, "No factory settings found, using empty defaults");
  #endif
}

void config_setDefaults() {
  LOG_DEBUG(CAT_CONFIG, "Setting defaults");

  
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
    
    LOG_DEBUG(CAT_CONFIG, "Generated MQTT Client ID: %s", _config.mqttClientId);
  #endif

  #if WIFI_ENABLED == 1
    _config.wifiOutputPower = WIFI_OUTPUT_POWER;
  #endif
  

  LOG_INFO(CAT_CONFIG, "Defaults set");

}

bool config_clear() {
  #if STATUS_LED_PIN > 0
    led_setMode(LED_MODE_MORZE_I);
  #endif

  LOG_INFO(CAT_CONFIG, "Erasing EEPROM...");

  EEPROM.end();
  delay(50);
  EEPROM.begin(sizeof(Config) + 4);
  
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, 0);
  }
  
  bool ok = EEPROM.commit();
  if(ok){
    LOG_DEBUG(CAT_CONFIG, "Clear %s", ok ? "SUCCESSFUL" : "FAILED");
  } else {
    LOG_WARN(CAT_CONFIG, "Clear %s", ok ? "SUCCESSFUL" : "FAILED");
  }
  

  
  EEPROM.end();
  
  if (ok) {
    // Сбросить структуры
    memset(&_config, 0, sizeof(Config));
    _configValid = false;

  }
  
  return ok;
}

void config_read() {
    LOG_DEBUG(CAT_CONFIG, "Reading from EEPROM...");


    // Читаем сырые данные во временную структуру
    Config raw;
    memset(&raw, 0, sizeof(Config));
    
    uint8_t* ptr = (uint8_t*)&raw;
    for (size_t i = 0; i < sizeof(Config); i++) {
        ptr[i] = EEPROM.read(i);
    }

    LOG_DEBUG(CAT_CONFIG, "Read magic: 0x%04X (expected 0x%04X)", raw.magic, MAGIC_VALUE);
    LOG_DEBUG(CAT_CONFIG, "Read CRC from EEPROM: 0x%04X", raw.crc);

    bool eepromValid = false;

    if (raw.magic == MAGIC_VALUE) {
        uint16_t savedCrc = raw.crc;
        raw.crc = 0;
        uint16_t calcCrc = crc16((uint8_t*)&raw, sizeof(Config));

        LOG_DEBUG(CAT_CONFIG, "Calculated CRC: 0x%04X", calcCrc);


        if (calcCrc == savedCrc) {
            eepromValid = true;
            LOG_INFO(CAT_CONFIG, "CRC is VALID");

        } else {
            LOG_WARN(CAT_CONFIG, "CRC mismatch! EEPROM: 0x%04X, Calculated: 0x%04X", savedCrc, calcCrc);

        }
    } else {
        LOG_WARN(CAT_CONFIG, "Magic mismatch! Config is INVALID");

    }

    // Сбрасываем на defaults
    config_setDefaults();

    if (eepromValid) {
        LOG_INFO(CAT_CONFIG, "EEPROM config valid, applying...");
        // Применяем прочитанные значения через сеттеры
        #if WIFI_ENABLED == 1
        if (strlen(raw.wifiSsid) > 0) {
            if (!config_setWifiSsid(raw.wifiSsid)) {
              LOG_WARN(CAT_CONFIG, "Failed to set WiFi SSID from EEPROM: %s", config_getLastError());
            }
        }
        if (strlen(raw.wifiPassword) > 0) {
            if (!config_setWifiPassword(raw.wifiPassword)) {
                LOG_WARN(CAT_CONFIG, "Failed to set WiFi password from EEPROM: %s", config_getLastError());
            }
        }

        #if MQTT_ENABLED == 1
        if (strlen(raw.mqttBroker) > 0) {
            if (!config_setMqttBroker(raw.mqttBroker)) {
              LOG_WARN(CAT_CONFIG, "Failed to set MQTT broker from EEPROM: %s", config_getLastError());

            }
        }
        if (!config_setMqttPort(raw.mqttPort)) {
          LOG_WARN(CAT_CONFIG, "Failed to set MQTT port from EEPROM: %s", config_getLastError());

        }
        if (strlen(raw.mqttUser) > 0) {
            if (!config_setMqttUser(raw.mqttUser)) {
              LOG_WARN(CAT_CONFIG, "Failed to set MQTT user from EEPROM: %s", config_getLastError());
            }
        }
        if (strlen(raw.mqttPassword) > 0) {
            if (!config_setMqttPassword(raw.mqttPassword)) {
              LOG_WARN(CAT_CONFIG, "Failed to set MQTT password from EEPROM: %s", config_getLastError());
            }
        }
        if (strlen(raw.mqttClientId) > 0) {
            if (!config_setMqttClientId(raw.mqttClientId)) {
              LOG_WARN(CAT_CONFIG, "Failed to set MQTT client ID from EEPROM: %s", config_getLastError());
            }
        }
        #endif // MQTT_ENABLED
        #endif // WIFI_ENABLED

        #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
        if (!config_setSensorInterval(raw.sensorInterval)) {
          LOG_WARN(CAT_CONFIG, "Failed to set sensor interval from EEPROM: %s", config_getLastError());
        }
        #endif

        #if DEVICE_TYPE == 1
        if (!config_setLowTemp(raw.lowTemp)) {
            LOG_WARN(CAT_CONFIG, "Failed to set low temp from EEPROM: %s", config_getLastError());
        }
        if (!config_setHighTemp(raw.highTemp)) {
          LOG_WARN(CAT_CONFIG, "Failed to set high temp from EEPROM: %s", config_getLastError());
        }
        if (!config_setLowHum(raw.lowHum)) {
          LOG_WARN(CAT_CONFIG, "Failed to set low hum from EEPROM: %s", config_getLastError());
        }
        if (!config_setHighHum(raw.highHum)) {
          LOG_WARN(CAT_CONFIG, "Failed to set high hum from EEPROM: %s", config_getLastError());
        }
        if (!config_setSpeedPercent(raw.speedPercent)) {
          LOG_WARN(CAT_CONFIG, "Failed to set speed percent from EEPROM: %s", config_getLastError());
        }
        config_setAdaptiveMode(raw.adaptiveMode);
        config_setSensorControlMode(raw.sensorControlMode);
        #endif

        #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
        if (!config_setDelaySeconds(raw.delaySeconds)) {
          LOG_WARN(CAT_CONFIG, "Failed to set delay seconds from EEPROM: %s", config_getLastError());
        }
        if (!config_setMaxOnTime(raw.maxOnTime)) {
          LOG_WARN(CAT_CONFIG, "Failed to set max on time from EEPROM: %s", config_getLastError());
        }
        config_setBootState(raw.bootState);
        #endif

        #if WIFI_ENABLED == 1
            if (strlen(_config.wifiSsid) == 0) {
                _configValid = false;
                LOG_INFO(CAT_CONFIG, "Config has empty WiFi SSID — marking as INVALID");
                
            } else {
                _configValid = true;
            }
        #else
            _configValid = true;
        #endif

        
            if (_configValid) {
              LOG_INFO(CAT_CONFIG, "Config loaded from EEPROM");
            } else {
              LOG_WARN(CAT_CONFIG, "Config loaded but invalid (empty SSID)");
            }
      
    } else {
        _configValid = false;
        LOG_WARN(CAT_CONFIG, "Using defaults (EEPROM invalid)");
      
    }
}

bool config_write() {
  LOG_INFO(CAT_CONFIG, "Writing to EEPROM...");
  #if WIFI_ENABLED == 1
  LOG_DEBUG(CAT_CONFIG, "WiFi SSID: '%s'", _config.wifiSsid);
  #if MQTT_ENABLED == 1
  LOG_DEBUG(CAT_CONFIG, "MQTT Broker: '%s:%d'", _config.mqttBroker, _config.mqttPort);
  LOG_DEBUG(CAT_CONFIG, "MQTT Client ID: '%s'", _config.mqttClientId);
  #endif // MQTT_ENABLED
  #endif // WIFI_ENABLED
  
  
  uint16_t oldCrc = _config.crc;
  _config.crc = 0;
  
  _config.crc = crc16((uint8_t*)&_config, sizeof(Config));
  
  LOG_DEBUG(CAT_CONFIG, "Calculated CRC: 0x%04X", _config.crc);
  
  
  //@TODO: Убрать после отладки
  #if SIMULATE_EEPROM_MALFUNCTION == 1
    LOG_DEBUG(CAT_CONFIG, "SIMULATE: EEPROM commit FAILED" ANSI_RESET);
    _config.crc = oldCrc;
    return false;
  #endif

  uint8_t* ptr = (uint8_t*)&_config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, ptr[i]);
  }
  
  if (!EEPROM.commit()) {
    LOG_ERROR(CAT_CONFIG, "EEPROM commit FAILED! Configuration NOT saved.");
    
    return false;
  }
  
  LOG_INFO(CAT_CONFIG, "Write completed!");
  LOG_INFO(CAT_CONFIG, "Verifying saved data...");

  
  Config verify;
  memset(&verify, 0, sizeof(Config));
  
  uint8_t* vptr = (uint8_t*)&verify;
  for (size_t i = 0; i < sizeof(Config); i++) {
    vptr[i] = EEPROM.read(i);
  }
  
  verify.crc = 0;
  uint16_t calcVerifyCrc = crc16((uint8_t*)&verify, sizeof(Config));
  
  #if DEBUG_ENABLED == 1
    #if MQTT_ENABLED
    #if MQTT_ENABLED == 1
    LOG_DEBUG(CAT_CONFIG, "Verify magic: 0x%04X, WiFi: '%s', ClientID: '%s', Calculated: 0x%04X", 
                  verify.magic, verify.wifiSsid, verify.mqttClientId, calcVerifyCrc);
    #else
    LOG_DEBUG(CAT_CONFIG, "Verify magic: 0x%04X, WiFi: '%s', Calculated: 0x%04X", 
                  verify.magic, verify.wifiSsid, calcVerifyCrc);
    #endif // MQTT_ENABLED
    #endif // WIFI_ENABLED
  #endif
  
  if (verify.magic == _config.magic && calcVerifyCrc == _config.crc) {
    LOG_INFO(CAT_CONFIG, "Verification saved config PASSED");

    _configValid = true;
    
    return true;
    
  } else {
    LOG_ERROR(CAT_CONFIG, "Verification saved config FAILED!");

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
  
  LOG_DEBUG(CAT_CONFIG, "Device ID initialized: %s", deviceId);

}

void config_init() {
  initDeviceId();

  LOG_INFO(CAT_CONFIG, "Initializing EEPROM...");

  EEPROM.begin(sizeof(Config) + 4);
  LOG_DEBUG(CAT_CONFIG, "EEPROM size: %d bytes", EEPROM.length());

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
        LOG_WARN(CAT_CONFIG, "config_getSaved: invalid magic, returning defaults");

        config_setDefaults();
        memcpy(&saved, &_config, sizeof(Config));
        saved.crc = 0;
    }
    
    return saved;
}

#if DEBUG_ENABLED == 1
void config_print() {
  LOG_DEBUG(CAT_CONFIG, "=== Config ===");
    #if WIFI_ENABLED == 1
    LOG_DEBUG(CAT_CONFIG, "WiFi SSID: '%s'", _config.wifiSsid);
    LOG_DEBUG(CAT_CONFIG, "WiFi Password: %s", _config.wifiPassword[0] ? "***" : "(empty)");

    #if MQTT_ENABLED == 1
      LOG_DEBUG(CAT_CONFIG, "MQTT Broker: '%s:%d'", _config.mqttBroker, _config.mqttPort);
      LOG_DEBUG(CAT_CONFIG, "MQTT User: '%s'", _config.mqttUser);
      LOG_DEBUG(CAT_CONFIG, "MQTT Client ID: '%s'", _config.mqttClientId);
    #endif
    #endif // WIFI_ENABLED == 1
    #if DEVICE_TYPE == 1
      LOG_DEBUG(CAT_CONFIG, "Temp range: %.1f - %.1f", _config.lowTemp, _config.highTemp);
      LOG_DEBUG(CAT_CONFIG, "Hum range: %.1f - %.1f", _config.lowHum, _config.highHum);
      LOG_DEBUG(CAT_CONFIG, "Sensor control mode: %s", _config.sensorControlMode ? "ON" : "OFF");
      LOG_DEBUG(CAT_CONFIG, "Speed percent: %d%% (%s)", 
                    _config.speedPercent,  
                    _config.speedPercent == 100 ? "full power" : "slow mode");
      LOG_DEBUG(CAT_CONFIG, "Adaptive mode: %s", _config.adaptiveMode ? "ON" : "OFF");
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
      LOG_DEBUG(CAT_CONFIG, "Delay: %d sec", _config.delaySeconds);
      LOG_DEBUG(CAT_CONFIG, "MaxOnTime: %d sec", _config.maxOnTime);
      LOG_DEBUG(CAT_CONFIG, "Boot state: %s", _config.bootState ? "ON" : "OFF");
    #endif
  
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
      LOG_DEBUG(CAT_CONFIG, "Sensor interval: %d sec", _config.sensorInterval);
      LOG_INFO(CAT_CONFIG, "Sensor type: %d", SENSOR_TYPE);
      #if SENSOR_TYPE == 2
        LOG_INFO(CAT_CONFIG, "Sensor pin: %d", SENSOR_PIN);
      #endif
    #endif

  LOG_DEBUG(CAT_CONFIG, "CRC: 0x%04X", _config.crc);
  LOG_INFO(CAT_CONFIG, "Config valid: %s", _configValid ? "YES" : "NO");
  #if WIFI_ENABLED == 1
  if (!_configValid || strlen(_config.wifiSsid) == 0) {
    LOG_INFO(CAT_CONFIG, "Mode: SETUP (AP will be started)");
  } else {
    LOG_INFO(CAT_CONFIG, "Mode: NORMAL");
  }
  #endif
  
  if (!_configValid && HAS_CREDENTIALS) {
    LOG_INFO(CAT_CONFIG, "Loaded factory settings.");
    
  }
  LOG_DEBUG(CAT_CONFIG, "=================");
}
#endif