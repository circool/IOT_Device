#include "config.h"

Config config;
Config staticConfig;            // Копия конфигурации для веб-интерфейса
bool configValid = false;
bool apMode = false;
String configLastError = "";

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
    #ifdef DEBUG_ENABLE
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
  #else
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] No factory settings found, using empty defaults");
    #endif
  #endif
}

void config_setDefaults() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Setting defaults");
  #endif
  
  memset(&config, 0, sizeof(Config));
  
  config.magic = 0x5A6B;
  config.crc = 0;
  config.mqttPort = MQTT_PORT;
  
  #if DEVICE_TYPE == 1
    config.lowHum = DEFAULT_LOW_HUM;
    config.highHum = DEFAULT_HIGH_HUM;
    config.lowTemp = DEFAULT_LOW_TEMP;
    config.highTemp = DEFAULT_HIGH_TEMP;
    config.sensorControlMode = DEFAULT_SENSOR_CONTROL_MODE;
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    config.delaySeconds = DEFAULT_DELAY_SECONDS;
    config.pwmDutyPercent = DEFAULT_PWM_DUTY_PERCENT;
    config.adaptiveMode = DEFAULT_ADAPTIVE_MODE;
    config.maxOnTime = MAX_ON_TIME_SEC;
    config.bootState = BOOT_SWITCH_STATE;
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    config.sensorInterval = SENSOR_DURATION;
  #endif
  
  memset(config.reserved, 0, sizeof(config.reserved));
  
  config_loadFromCredentials();
  
  #ifdef ESP32
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint16_t suffix = (mac[4] << 8) | mac[5];
  #elif defined(ESP8266)
    uint32_t chipId = ESP.getChipId();
    uint16_t suffix = chipId & 0xFFFF;
  #endif
  snprintf(config.mqttClientId, sizeof(config.mqttClientId), 
           "%s_%04X", DEVICE_PREFIX, suffix);
  #ifdef DEBUG_ENABLE
    Serial.printf("[CONFIG] Generated MQTT Client ID: %s\n", config.mqttClientId);
  #endif
  
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Defaults set");
  #endif
}

bool config_clear() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Erasing EEPROM...");
  #endif

  EEPROM.end();
  delay(50);
  EEPROM.begin(sizeof(Config) + 4);
  
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, 0);
  }
  
  bool ok = EEPROM.commit();
  
  #ifdef DEBUG_ENABLE
    Serial.printf("[CONFIG] Clear %s\n", ok ? "SUCCESSFUL" : "FAILED");
  #endif
  
  EEPROM.end();
  return ok;
}

bool config_validate() {
  configLastError = "";
  bool valid = true;
  
  // === СЕТЕВЫЕ НАСТРОЙКИ ===
  if (config.mqttPort < 1 || config.mqttPort > 65535) {
    configLastError = "MQTT Port must be 1-65535";
    valid = false;
  }
  
  if (strlen(config.wifiSsid) == 0) {
    if (configLastError.length() == 0) configLastError = "WiFi SSID cannot be empty";
    valid = false;
  }
  
  if (strlen(config.mqttBroker) == 0) {
    if (configLastError.length() == 0) configLastError = "MQTT Broker cannot be empty";
    valid = false;
  }
  
  if (strlen(config.mqttClientId) == 0) {
    if (configLastError.length() == 0) configLastError = "MQTT Client ID cannot be empty";
    valid = false;
  }
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  // === НАСТРОЙКИ УПРАВЛЕНИЯ ===
  if (config.delaySeconds < 0 || config.delaySeconds > 86400) {
    if (configLastError.length() == 0) configLastError = "Delay must be 0-86400 seconds";
    valid = false;
  }
  
  if (config.maxOnTime < 0 || config.maxOnTime > 86400) {
    if (configLastError.length() == 0) configLastError = "MaxOnTime must be 0-86400 seconds";
    valid = false;
  }
  
  if (config.pwmDutyPercent > 100) {
    if (configLastError.length() == 0) configLastError = "PWM duty must be 0-100";
    valid = false;
  }
  
  #if DEVICE_TYPE == 1
  // === ЛОГИЧЕСКИЕ ПРОВЕРКИ ДЛЯ TYPE 1 ===
  if (config.adaptiveMode && !config.sensorControlMode) {
    if (configLastError.length() == 0) configLastError = "Adaptive mode requires Sensor Control Mode ON";
    valid = false;
  }
  
  if (config.adaptiveMode && config.pwmDutyPercent == 0) {
    if (configLastError.length() == 0) configLastError = "Adaptive mode requires PWM duty > 0%";
    valid = false;
  }
  #endif
  
  #if DEVICE_TYPE == 3
  // === ЛОГИЧЕСКИЕ ПРОВЕРКИ ДЛЯ TYPE 3 ===
  if (config.sensorControlMode) {
    if (configLastError.length() == 0) configLastError = "Sensor Control Mode not available for Switch";
    valid = false;
  }
  #endif
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  // === НАСТРОЙКИ ДАТЧИКА ===
  if (config.sensorInterval < 1 || config.sensorInterval > 3600) {
    if (configLastError.length() == 0) configLastError = "Sensor interval must be 1-3600 seconds";
    valid = false;
  }
  #endif
  
  #if DEVICE_TYPE == 1
  // === ПОРОГИ ТЕМПЕРАТУРЫ ===
  if (config.lowTemp < -40 || config.lowTemp > 85) {
    if (configLastError.length() == 0) configLastError = "Low Temp must be -40..85°C";
    valid = false;
  }
  if (config.highTemp < -40 || config.highTemp > 85) {
    if (configLastError.length() == 0) configLastError = "High Temp must be -40..85°C";
    valid = false;
  }
  if (config.lowTemp >= config.highTemp) {
    if (configLastError.length() == 0) configLastError = "Low Temp must be < High Temp";
    valid = false;
  }
  
  // === ПОРОГИ ВЛАЖНОСТИ ===
  if (config.lowHum < 0 || config.lowHum > 100) {
    if (configLastError.length() == 0) configLastError = "Low Hum must be 0..100%";
    valid = false;
  }
  if (config.highHum < 0 || config.highHum > 100) {
    if (configLastError.length() == 0) configLastError = "High Hum must be 0..100%";
    valid = false;
  }
  if (config.lowHum >= config.highHum) {
    if (configLastError.length() == 0) configLastError = "Low Hum must be < High Hum";
    valid = false;
  }
  
  // === ДОПОЛНИТЕЛЬНЫЕ ПРОВЕРКИ ДЛЯ TYPE 1 ===
  if (config.sensorControlMode && config.pwmDutyPercent == 0) {
    if (configLastError.length() == 0) configLastError = "Sensor Control Mode requires PWM duty > 0%";
    valid = false;
  }
  #endif
  
  #ifdef DEBUG_ENABLE
    if (!valid) {
      Serial.printf("[CONFIG] Validation failed: %s\n", configLastError.c_str());
    }
  #endif
  
  return valid;
}
void config_read() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Reading from EEPROM...");
  #endif
  
  memset(&config, 0, sizeof(Config));
  
  uint8_t* ptr = (uint8_t*)&config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    ptr[i] = EEPROM.read(i);
  }
  
  #ifdef DEBUG_ENABLE
    Serial.printf("[CONFIG] Read magic: 0x%04X (expected 0x5A6B)\n", config.magic);
    Serial.printf("[CONFIG] Read WiFi SSID: '%s'\n", config.wifiSsid);
    Serial.printf("[CONFIG] Read MQTT Broker: '%s'\n", config.mqttBroker);
    Serial.printf("[CONFIG] Read MQTT User: '%s'\n", config.mqttUser);
    Serial.printf("[CONFIG] Read MQTT Client ID: '%s'\n", config.mqttClientId);
    Serial.printf("[CONFIG] Read CRC from EEPROM: 0x%04X\n", config.crc);
  #endif
  
  bool eepromValid = false;
  
  if (config.magic == 0x5A6B) {
    uint16_t savedCrc = config.crc;
    config.crc = 0;
    
    memset(config.reserved, 0, sizeof(config.reserved));
    
    uint16_t calcCrc = crc16((uint8_t*)&config, sizeof(Config));
    
    #ifdef DEBUG_ENABLE
      Serial.printf("[CONFIG] Calculated CRC: 0x%04X\n", calcCrc);
    #endif
    
    if (calcCrc == savedCrc) {
      config.crc = savedCrc;
      eepromValid = true;
      #ifdef DEBUG_ENABLE
        Serial.println("[CONFIG] CRC is VALID");
      #endif
    } else {      
        Serial.printf("[CONFIG] CRC mismatch! EEPROM: 0x%04X, Calculated: 0x%04X\n", savedCrc, calcCrc);
    }
  } else {
      Serial.println("[CONFIG] Magic mismatch! Config is INVALID");
  }
  
  bool dataValid = config_validate();
  
  if (eepromValid && dataValid) {
    configValid = true;
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Config is VALID (EEPROM)");
    #endif
  } else {
    configValid = false;
    config_setDefaults();
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Using defaults for setup mode");
    #endif
  }
}

void config_write() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Writing to EEPROM...");
    Serial.printf("[CONFIG] WiFi SSID: '%s'\n", config.wifiSsid);
    Serial.printf("[CONFIG] MQTT Broker: '%s:%d'\n", config.mqttBroker, config.mqttPort);
    Serial.printf("[CONFIG] MQTT Client ID: '%s'\n", config.mqttClientId);
  #endif
  
  uint16_t oldCrc = config.crc;
  config.crc = 0;
  
  memset(config.reserved, 0, sizeof(config.reserved));
  
  config.crc = crc16((uint8_t*)&config, sizeof(Config));
  
  #ifdef DEBUG_ENABLE
    Serial.printf("[CONFIG] Calculated CRC: 0x%04X\n", config.crc);
  #endif
  
  uint8_t* ptr = (uint8_t*)&config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, ptr[i]);
  }
  
  EEPROM.commit();
  
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Write completed");
    Serial.println("[CONFIG] Verifying...");
  #endif
  
  Config verify;
  memset(&verify, 0, sizeof(Config));
  
  uint8_t* vptr = (uint8_t*)&verify;
  for (size_t i = 0; i < sizeof(Config); i++) {
    vptr[i] = EEPROM.read(i);
  }
  
  verify.crc = 0;
  memset(verify.reserved, 0, sizeof(verify.reserved));
  uint16_t calcVerifyCrc = crc16((uint8_t*)&verify, sizeof(Config));
  
  #ifdef DEBUG_ENABLE
    Serial.printf("[CONFIG] Verify magic: 0x%04X, WiFi: '%s', ClientID: '%s', Calculated: 0x%04X\n", 
                  verify.magic, verify.wifiSsid, verify.mqttClientId, calcVerifyCrc);
  #endif
  
  if (verify.magic == config.magic && calcVerifyCrc == config.crc) {
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Verification PASSED");
    #endif
    configValid = true;
    
    // Обновляем статическую копию для веб-интерфейса
    memcpy(&staticConfig, &config, sizeof(Config));
  } else {
    Serial.println("[CONFIG] Verification FAILED!");
    config.crc = oldCrc;
    configValid = false;
  }
}

void config_init() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Initializing EEPROM...");
  #endif
  EEPROM.begin(sizeof(Config) + 4);
  #ifdef DEBUG_ENABLE
    Serial.printf("[CONFIG] EEPROM size: %d bytes\n", EEPROM.length());
  #endif
  config_read();
  
  // Создаём копию для веб-интерфейса
  memcpy(&staticConfig, &config, sizeof(Config));
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Static config copy created for web interface");
  #endif
}

void config_print() {
  Serial.println("=== Config ===");
  Serial.printf("WiFi SSID: '%s'\n", config.wifiSsid);
  Serial.printf("WiFi Password: %s\n", config.wifiPassword[0] ? "***" : "(empty)");
  Serial.printf("MQTT Broker: '%s:%d'\n", config.mqttBroker, config.mqttPort);
  Serial.printf("MQTT User: '%s'\n", config.mqttUser);
  Serial.printf("MQTT Client ID: '%s'\n", config.mqttClientId);
  
  #if DEVICE_TYPE == 1
    Serial.printf("Temp range: %.1f - %.1f\n", config.lowTemp, config.highTemp);
    Serial.printf("Hum range: %.1f - %.1f\n", config.lowHum, config.highHum);
    Serial.printf("Sensor control mode: %s\n", config.sensorControlMode ? "ON" : "OFF");
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    Serial.printf("Delay: %d sec\n", config.delaySeconds);
    Serial.printf("PWM duty: %d%% (%s)\n", 
                  config.pwmDutyPercent,
                  config.pwmDutyPercent == 100 ? "full power" : "slow mode");
    Serial.printf("Adaptive mode: %s\n", config.adaptiveMode ? "ON" : "OFF");
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
  
  Serial.printf("CRC: 0x%04X\n", config.crc);
  Serial.printf("Config valid: %s\n", configValid ? "YES" : "NO");
  
  if (!configValid || strlen(config.wifiSsid) == 0) {
    Serial.println("Mode: SETUP (AP will be started)");
  } else {
    Serial.println("Mode: NORMAL");
  }
  
  if (!configValid && HAS_CREDENTIALS) {
    Serial.println("Factory settings: loaded");
  }
  Serial.println("=================");
}