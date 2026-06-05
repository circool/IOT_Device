#include "config.h"
#include "led.h"
#include "ansi.h"

#ifdef ESP32
#include <esp_mac.h>
#endif

char deviceId[12] = "";

Config config;

bool configValid = false;
bool apMode = false;
char configLastError[64] = "";   

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

static void config_loadFromCredentials() {
  #if HAS_CREDENTIALS
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Loading factory settings");
    #endif
    
    if (strlen(SSID_NAME) > 0) {
      strncpy(config.wifiSsid, SSID_NAME, sizeof(config.wifiSsid) - 1);
      config.wifiSsid[sizeof(config.wifiSsid) - 1] = '\0';
    } else {
      memset(config.wifiSsid, 0, sizeof(config.wifiSsid));
    }
    
    if (strlen(WIFI_PASSWORD) > 0) {
      strncpy(config.wifiPassword, WIFI_PASSWORD, sizeof(config.wifiPassword) - 1);
      config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
    } else {
      memset(config.wifiPassword, 0, sizeof(config.wifiPassword));
    }
  #if MQTT_ENABLED == 1
    if (strlen(MQTT_ADDRESS) > 0) {
      strncpy(config.mqttBroker, MQTT_ADDRESS, sizeof(config.mqttBroker) - 1);
      config.mqttBroker[sizeof(config.mqttBroker) - 1] = '\0';
    } else {
      memset(config.mqttBroker, 0, sizeof(config.mqttBroker));
    }
    
    config.mqttPort = MQTT_PORT;
    
    if (strlen(MQTT_USER) > 0) {
      strncpy(config.mqttUser, MQTT_USER, sizeof(config.mqttUser) - 1);
      config.mqttUser[sizeof(config.mqttUser) - 1] = '\0';
    } else {
      memset(config.mqttUser, 0, sizeof(config.mqttUser));
    }
    
    if (strlen(MQTT_PASSWORD) > 0) {
      strncpy(config.mqttPassword, MQTT_PASSWORD, sizeof(config.mqttPassword) - 1);
      config.mqttPassword[sizeof(config.mqttPassword) - 1] = '\0';
    } else {
      memset(config.mqttPassword, 0, sizeof(config.mqttPassword));
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
  
  memset(&config, 0, sizeof(Config));
  
  config.magic = MAGIC_VALUE;
  config.crc = 0;
  #if MQTT_ENABLED == 1
  config.mqttPort = MQTT_PORT;
  #endif
  
  #if DEVICE_TYPE == 1
    config.lowHum = DEFAULT_LOW_HUM;
    config.highHum = DEFAULT_HIGH_HUM;
    config.lowTemp = DEFAULT_LOW_TEMP;
    config.highTemp = DEFAULT_HIGH_TEMP;
    config.sensorControlMode = DEFAULT_SENSOR_CONTROL_MODE;
    config.speedPercent = DEFAULT_SPEED_PERCENT;  
    config.adaptiveMode = DEFAULT_ADAPTIVE_MODE;
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    config.delaySeconds = DEFAULT_DELAY_SECONDS;
    config.maxOnTime = MAX_ON_TIME_SEC;
    config.bootState = BOOT_SWITCH_STATE;
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    config.sensorInterval = SENSOR_DURATION;
  #endif
  
  initDeviceId();
  config_loadFromCredentials();
  
  #if MQTT_ENABLED == 1
    snprintf(config.mqttClientId, sizeof(config.mqttClientId), "%s", deviceId);
    
    #if DEBUG_ENABLED == 1
      Serial.printf("[CONFIG] Generated MQTT Client ID: %s\n", config.mqttClientId);
    #endif
  #endif

  #if WIFI_ENABLED == 1
    config.wifiOutputPower = WIFI_OUTPUT_POWER;
  #endif

  #if DEBUG_ENABLED == 1
    Serial.println("[CONFIG] Defaults set");
  #endif
}

bool config_clear() {
  #if STATUS_LED_PIN > 0
    led_setMode(LED_MODE_FAST_BLINK);  // Индикация сброса
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
  return ok;
}

bool config_validate() {
  configLastError[0] = '\0';  // Очистка массива
  bool valid = true;
  
  // === СЕТЕВЫЕ НАСТРОЙКИ ===
  #if MQTT_ENABLED == 1
  if (config.mqttPort < 1 || config.mqttPort > 65535) {
    snprintf(configLastError, sizeof(configLastError), "MQTT Port must be 1-65535");
    valid = false;
  }
  #endif

  if (strlen(config.wifiSsid) == 0) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "WiFi SSID cannot be empty");
    }
    valid = false;
  }
  
  #if MQTT_ENABLED == 1
  if (strlen(config.mqttBroker) == 0) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "MQTT Broker cannot be empty");
    }
    valid = false;
  }
  
  if (strlen(config.mqttClientId) == 0) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "MQTT Client ID cannot be empty");
    }
    valid = false;
  }
  #endif

  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  // === НАСТРОЙКИ УПРАВЛЕНИЯ ===
  if (config.delaySeconds < 0 || config.delaySeconds > 86400) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "Delay must be 0-86400 seconds");
    }
    valid = false;
  }
  
  if (config.maxOnTime < 0 || config.maxOnTime > 86400) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "MaxOnTime must be 0-86400 seconds");
    }
    valid = false;
  }
  
  #if DEVICE_TYPE == 1
  // === ЛОГИЧЕСКИЕ ПРОВЕРКИ ДЛЯ TYPE 1 ===
  if (config.speedPercent > 100) {  
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "Speed percent must be 0-100");
    }
    valid = false;
  }
  
  if (config.adaptiveMode && !config.sensorControlMode) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "Adaptive mode requires Sensor Control Mode ON");
    }
    valid = false;
  }
  
  if (config.adaptiveMode && config.speedPercent == 0) {  
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "Adaptive mode requires speed percent > 0%%");
    }
    valid = false;
  }
  #endif
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  // === НАСТРОЙКИ ДАТЧИКА ===
  if (config.sensorInterval < 1 || config.sensorInterval > 3600) {
    char buf[80];
    snprintf(buf, sizeof(buf), "Sensor interval must be 1-3600 seconds, but = %d", config.sensorInterval);
    strncpy(configLastError, buf, sizeof(configLastError) - 1);
    configLastError[sizeof(configLastError) - 1] = '\0';
    valid = false;
  }
  #endif
  
  #if DEVICE_TYPE == 1
  // === ПОРОГИ ТЕМПЕРАТУРЫ ===
  if (config.lowTemp < -40 || config.lowTemp > 85) {
    char buf[80];
    snprintf(buf, sizeof(buf), "Low Temp must be -40..85°C, but = %.1f", config.lowTemp);
    strncpy(configLastError, buf, sizeof(configLastError) - 1);
    configLastError[sizeof(configLastError) - 1] = '\0';
    valid = false;
  }
  if (config.highTemp < -40 || config.highTemp > 85) {
    char buf[80];
    snprintf(buf, sizeof(buf), "High Temp must be -40..85°C, but = %.1f", config.highTemp);
    strncpy(configLastError, buf, sizeof(configLastError) - 1);
    configLastError[sizeof(configLastError) - 1] = '\0';
    valid = false;
  }
  if (config.lowTemp >= config.highTemp) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "Low Temp must be < High Temp");
    }
    valid = false;
  }
  
  // === ПОРОГИ ВЛАЖНОСТИ ===
  if (config.lowHum < 0 || config.lowHum > 100) {
    char buf[80];
    snprintf(buf, sizeof(buf), "Low Hum must be 0..100%%, but = %.1f", config.lowHum);
    strncpy(configLastError, buf, sizeof(configLastError) - 1);
    configLastError[sizeof(configLastError) - 1] = '\0';
    valid = false;
  }
  if (config.highHum < 0 || config.highHum > 100) {
    char buf[80];
    snprintf(buf, sizeof(buf), "High Hum must be 0..100%%, but = %.1f", config.highHum);
    strncpy(configLastError, buf, sizeof(configLastError) - 1);
    configLastError[sizeof(configLastError) - 1] = '\0';
    valid = false;
  }
  if (config.lowHum >= config.highHum) {
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "Low Hum must be < High Hum");
    }
    valid = false;
  }
  
  // === ДОПОЛНИТЕЛЬНЫЕ ПРОВЕРКИ ДЛЯ TYPE 1 ===
  if (config.sensorControlMode && config.speedPercent == 0) {  
    if (configLastError[0] == '\0') {
      snprintf(configLastError, sizeof(configLastError), "Sensor Control Mode requires speed percent > 0%%");
    }
    valid = false;
  }
  #endif
  
  #if DEBUG_ENABLED == 1
    if (!valid) {
      Serial.print(ANSI_BRIGHT_RED);
      Serial.printf("[CONFIG] Validation failed: %s\n", configLastError);
      Serial.print(ANSI_RESET);
    }
  #endif
  
  return valid;
}


void config_read() {
  #if DEBUG_ENABLED == 1
    Serial.println("[CONFIG] Reading from EEPROM...");
  #endif
  
  memset(&config, 0, sizeof(Config));
  
  uint8_t* ptr = (uint8_t*)&config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    ptr[i] = EEPROM.read(i);
  }
  
  #if DEBUG_ENABLED == 1
    Serial.printf("[CONFIG] Read magic: 0x%04X (expected 0x%04X)\n", config.magic, MAGIC_VALUE);
    Serial.printf("[CONFIG] Read WiFi SSID: '%s'\n", config.wifiSsid);
    #if MQTT_ENABLED == 1
      Serial.printf("[CONFIG] Read MQTT Broker: '%s'\n", config.mqttBroker);
      Serial.printf("[CONFIG] Read MQTT User: '%s'\n", config.mqttUser);
      Serial.printf("[CONFIG] Read MQTT Client ID: '%s'\n", config.mqttClientId);
    #endif
    Serial.printf("[CONFIG] Read CRC from EEPROM: 0x%04X\n", config.crc);
  #endif
  
  bool eepromValid = false;
  
  if (config.magic == MAGIC_VALUE) {
    uint16_t savedCrc = config.crc;
    config.crc = 0;
    
    uint16_t calcCrc = crc16((uint8_t*)&config, sizeof(Config));
    
    #if DEBUG_ENABLED == 1
      Serial.printf("[CONFIG] Calculated CRC: 0x%04X\n", calcCrc);
    #endif
    
    if (calcCrc == savedCrc) {
      config.crc = savedCrc;
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
    configValid = true;
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Config is VALID (EEPROM)");
    #endif
  } else {
    configValid = false;
    config_setDefaults();
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Using defaults for setup mode");
    #endif
  }
}

void config_write() {
  #if LOG_CONFIG == 1
    Serial.println(ANSI_BRIGHT_RED "[CONFIG] Writing to EEPROM..." ANSI_RESET);
    Serial.printf("[CONFIG] WiFi SSID: '%s'\n", config.wifiSsid);
    #if MQTT_ENABLED == 1
    Serial.printf("[CONFIG] MQTT Broker: '%s:%d'\n", config.mqttBroker, config.mqttPort);
    Serial.printf("[CONFIG] MQTT Client ID: '%s'\n", config.mqttClientId);
    #endif
  #endif
  
  uint16_t oldCrc = config.crc;
  config.crc = 0;
  
  config.crc = crc16((uint8_t*)&config, sizeof(Config));
  
  #if DEBUG_ENABLED == 1
    Serial.printf("[CONFIG] Calculated CRC: 0x%04X\n", config.crc);
  #endif
  
  uint8_t* ptr = (uint8_t*)&config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, ptr[i]);
  }
  
  EEPROM.commit();
  
  #if DEBUG_ENABLED == 1
    Serial.println(ANSI_MAGENTA "[CONFIG] Write completed!" ANSI_RESET);
    Serial.println("[CONFIG] Verifying...");
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
  
  if (verify.magic == config.magic && calcVerifyCrc == config.crc) {
    #if DEBUG_ENABLED == 1
      Serial.println("[CONFIG] Verification PASSED");
    #endif
    configValid = true;
    
    
  } else {
    #if DEBUG_ENABLED == 1
      Serial.print(ANSI_BRIGHT_RED);
      Serial.println("[CONFIG] Verification FAILED!");
      Serial.print(ANSI_RESET);
    #endif
    config.crc = oldCrc;
    configValid = false;
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
}

Config config_getSaved() {
    Config saved;
    memset(&saved, 0, sizeof(Config));
    
    // Читаем весь блок EEPROM во временную структуру
    uint8_t* ptr = (uint8_t*)&saved;
    for (size_t i = 0; i < sizeof(Config); i++) {
        ptr[i] = EEPROM.read(i);
    }
    
    // Проверяем валидность магического числа
    if (saved.magic != MAGIC_VALUE) {
        // Если невалидно, возвращаем defaults (но не сохраняем в EEPROM)
        #if DEBUG_ENABLED == 1
            Serial.println("[CONFIG] config_getSaved: invalid magic, returning defaults");
        #endif
        config_setDefaults();
        memcpy(&saved, &config, sizeof(Config));
        saved.crc = 0;  // сбросим CRC, т.к. это временные данные
    }
    
    return saved;
}


#if DEBUG_ENABLED == 1
void config_print() {
  #if LOG_CONFIG == 1
    Serial.println("=== Config ===");
    Serial.printf("WiFi SSID: '%s'\n", config.wifiSsid);
    Serial.printf("WiFi Password: %s\n", config.wifiPassword[0] ? "***" : "(empty)");
    #if MQTT_ENABLED == 1
    Serial.printf("MQTT Broker: '%s:%d'\n", config.mqttBroker, config.mqttPort);
    Serial.printf("MQTT User: '%s'\n", config.mqttUser);
    Serial.printf("MQTT Client ID: '%s'\n", config.mqttClientId);
    #endif
  
    #if DEVICE_TYPE == 1
      Serial.printf("Temp range: %.1f - %.1f\n", config.lowTemp, config.highTemp);
      Serial.printf("Hum range: %.1f - %.1f\n", config.lowHum, config.highHum);
      Serial.printf("Sensor control mode: %s\n", config.sensorControlMode ? "ON" : "OFF");
      Serial.printf("Speed percent: %d%% (%s)\n", 
                    config.speedPercent,  
                    config.speedPercent == 100 ? "full power" : "slow mode");
      Serial.printf("Adaptive mode: %s\n", config.adaptiveMode ? "ON" : "OFF");
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
      Serial.printf("Delay: %d sec\n", config.delaySeconds);
      Serial.printf("MaxOnTime: %d sec\n", config.maxOnTime);
      Serial.printf("Boot state: %s\n", config.bootState ? "ON" : "OFF");
    #endif
  
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
      Serial.printf("Sensor interval: %d sec\n", config.sensorInterval);
      Serial.printf("Sensor type: %d\n", SENSOR_TYPE);
      #if SENSOR_TYPE == 2
        Serial.printf("Sensor pin: %d\n", SENSOR_PIN);
      #endif
    #endif
  #endif //LOG_CONFIG == 1
  Serial.printf("CRC: 0x%04X\n", config.crc);
  Serial.printf("Config valid: %s\n", configValid ? "YES" : "NO");
  
  if (!configValid || strlen(config.wifiSsid) == 0) {
    Serial.println("Mode: SETUP (AP will be started)");
  } else {
    Serial.println("Mode: NORMAL");
  }
  
  if (!configValid && HAS_CREDENTIALS) {
    Serial.print(ANSI_BRIGHT_RED);
    Serial.println("Loaded factory settings.");
    Serial.print(ANSI_RESET);
  }
  Serial.println("=================");
}
#endif