#include "config.h"

Config config;
bool configValid = false;
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

// Внутренняя функция - загрузка из credentials.h
static void config_loadFromCredentials() {
  #if HAS_CREDENTIALS
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Loading from credentials.h");
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
      Serial.println("[CONFIG] No credentials.h found, using empty defaults");
    #endif
  #endif
}

void config_setDefaults() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Setting defaults");
  #endif
  
  // Полностью обнуляем структуру
  memset(&config, 0, sizeof(Config));
  
  config.magic = 0x5A6B;
  config.crc = 0;
  
  // MQTT порт по умолчанию
  config.mqttPort = MQTT_PORT;
  
  // Условные поля в зависимости от типа устройства
  #if DEVICE_TYPE == 1
    config.lowHum = DEFAULT_LOW_HUM;
    config.highHum = DEFAULT_HIGH_HUM;
    config.lowTemp = DEFAULT_LOW_TEMP;
    config.highTemp = DEFAULT_HIGH_TEMP;
    config.automaticMode = DEFAULT_AUTOMATIC_MODE;
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    config.delaySeconds = DEFAULT_DELAY_SECONDS;
    config.slowModeEnabled = DEFAULT_SLOW_MODE;
    config.slowModeDuty = SLOW_MODE_DUTY_CYCLE;
    config.maxOnTime = MAX_ON_TIME_SEC;
    config.forceOffOnBoot = DEFAULT_FORCE_OFF_ON_BOOT;
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    config.sensorInterval = SENSOR_DURATION;
  #endif
  
  memset(config.reserved, 0, sizeof(config.reserved));
  
  // Загружаем WiFi/MQTT из credentials.h
  config_loadFromCredentials();
  
    // Генерируем MQTT Client ID если пуст
  if (strlen(config.mqttClientId) == 0) {
    #if defined(ESP8266)
      uint32_t chipId = ESP.getChipId();
      uint16_t suffix = chipId & 0xFFFF;
    #elif defined(ESP32)
      uint64_t mac64 = ESP.getEfuseMac();
      uint8_t mac[6];
      mac[0] = mac64 & 0xFF;           
      mac[1] = (mac64 >> 8) & 0xFF;    
      mac[2] = (mac64 >> 16) & 0xFF;   
      mac[3] = (mac64 >> 24) & 0xFF;   
      mac[4] = (mac64 >> 32) & 0xFF;   
      mac[5] = (mac64 >> 40) & 0xFF;   
      uint16_t suffix = (mac[4] << 8) | mac[5];
    #endif
    snprintf(config.mqttClientId, sizeof(config.mqttClientId), 
             "%s_%04X", DEVICE_PREFIX, suffix);
    #ifdef DEBUG_ENABLE
      Serial.printf("[CONFIG] Generated MQTT Client ID: %s\n", config.mqttClientId);
    #endif

    
  }
  
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Defaults set");
  #endif
}

void config_clear() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Clearing configuration");
  #endif
  
  config_setDefaults();  // hardware defaults + credentials
  config_write();        // сохраняем в EEPROM
  configValid = false;
  
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Configuration cleared and saved");
  #endif
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
  
  // Проверка валидности всех полей
  bool dataValid = true;
  
  // MQTT порт - должен быть в диапазоне 1-65535
  if (config.mqttPort < 1 || config.mqttPort > 65535) {
    Serial.printf("[CONFIG] Invalid MQTT port: %d\n", config.mqttPort);
    dataValid = false;
  }
  
  // WiFi SSID - не пустой
  if (strlen(config.wifiSsid) == 0) {
    Serial.println("[CONFIG] WiFi SSID is empty");
    dataValid = false;
  }
  
  // MQTT Broker - не пустой
  if (strlen(config.mqttBroker) == 0) {
    Serial.println("[CONFIG] MQTT Broker is empty");
    dataValid = false;
  }
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    // slowModeDuty - должен быть 0-255
    if (config.slowModeDuty > 255) {
      Serial.printf("[CONFIG] Invalid slowModeDuty: %d\n", config.slowModeDuty);
      dataValid = false;
    }
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    // sensorInterval - должен быть > 0
    if (config.sensorInterval == 0) {
      Serial.printf("[CONFIG] Invalid sensorInterval: %d\n", config.sensorInterval);
      dataValid = false;
    }
  #endif
  
  #if DEVICE_TYPE == 1
    // lowTemp/highTemp - диапазон -40..85
    if (config.lowTemp < -40 || config.lowTemp > 85) {
      Serial.printf("[CONFIG] Invalid lowTemp: %.1f\n", config.lowTemp);
      dataValid = false;
    }
    if (config.highTemp < -40 || config.highTemp > 85) {
      Serial.printf("[CONFIG] Invalid highTemp: %.1f\n", config.highTemp);
      dataValid = false;
    }
    if (config.lowTemp >= config.highTemp) {
      Serial.printf("[CONFIG] lowTemp (%.1f) >= highTemp (%.1f)\n", config.lowTemp, config.highTemp);
      dataValid = false;
    }
    
    // lowHum/highHum - диапазон 0..100
    if (config.lowHum < 0 || config.lowHum > 100) {
      Serial.printf("[CONFIG] Invalid lowHum: %.1f\n", config.lowHum);
      dataValid = false;
    }
    if (config.highHum < 0 || config.highHum > 100) {
      Serial.printf("[CONFIG] Invalid highHum: %.1f\n", config.highHum);
      dataValid = false;
    }
    if (config.lowHum >= config.highHum) {
      Serial.printf("[CONFIG] lowHum (%.1f) >= highHum (%.1f)\n", config.lowHum, config.highHum);
      dataValid = false;
    }
  #endif
  
  // Решение: валиден ли конфиг?
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
    Serial.printf("[CONFIG] Verify magic: 0x%04X, WiFi: '%s', Calculated: 0x%04X\n", 
                  verify.magic, verify.wifiSsid, calcVerifyCrc);
  #endif
  
  if (verify.magic == config.magic && calcVerifyCrc == config.crc) {
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Verification PASSED");
    #endif
    configValid = true;
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
    Serial.printf("Auto mode: %s\n", config.automaticMode ? "ON" : "OFF");
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    Serial.printf("Delay: %d sec\n", config.delaySeconds);
    Serial.printf("Slow mode: %s, Duty: %d\n", 
                  config.slowModeEnabled ? "ON" : "OFF", 
                  config.slowModeDuty);
    Serial.printf("MaxOnTime: %d sec\n", config.maxOnTime);
    Serial.printf("Force OFF on boot: %s\n", config.forceOffOnBoot ? "ON" : "OFF");
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
  Serial.printf("AP mode: %s\n", apMode ? "YES" : "NO");
  
  if (!configValid && HAS_CREDENTIALS) {
    Serial.println("Credentials: loaded from credentials.h");
  } 
  Serial.println("=================");
}