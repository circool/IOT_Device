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

void config_loadFromCredentials() {
  #if HAS_CREDENTIALS
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Loading defaults from credentials.h");
    #endif
    
    if (strlen(SSID_NAME) > 0) {
      strncpy(config.wifiSsid, SSID_NAME, sizeof(config.wifiSsid) - 1);
      config.wifiSsid[sizeof(config.wifiSsid) - 1] = '\0';
      #ifdef DEBUG_ENABLE
        Serial.printf("[CONFIG] Loaded SSID: %s\n", config.wifiSsid);
      #endif
    }
    
    if (strlen(WIFI_PASSWORD) > 0) {
      strncpy(config.wifiPassword, WIFI_PASSWORD, sizeof(config.wifiPassword) - 1);
      config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
      #ifdef DEBUG_ENABLE
        Serial.println("[CONFIG] Loaded WiFi password");
      #endif
    }
    
    if (strlen(MQTT_ADDRESS) > 0) {
      strncpy(config.mqttBroker, MQTT_ADDRESS, sizeof(config.mqttBroker) - 1);
      config.mqttBroker[sizeof(config.mqttBroker) - 1] = '\0';
      #ifdef DEBUG_ENABLE
        Serial.printf("[CONFIG] Loaded MQTT Broker: %s\n", config.mqttBroker);
      #endif
    }
    
    config.mqttPort = MQTT_PORT;
    #ifdef DEBUG_ENABLE
      Serial.printf("[CONFIG] Loaded MQTT Port: %d\n", config.mqttPort);
    #endif
    
    if (strlen(MQTT_USER) > 0) {
      strncpy(config.mqttUser, MQTT_USER, sizeof(config.mqttUser) - 1);
      config.mqttUser[sizeof(config.mqttUser) - 1] = '\0';
      #ifdef DEBUG_ENABLE
        Serial.printf("[CONFIG] Loaded MQTT User: %s\n", config.mqttUser);
      #endif
    }
    
    if (strlen(MQTT_PASSWORD) > 0) {
      strncpy(config.mqttPassword, MQTT_PASSWORD, sizeof(config.mqttPassword) - 1);
      config.mqttPassword[sizeof(config.mqttPassword) - 1] = '\0';
      #ifdef DEBUG_ENABLE
        Serial.println("[CONFIG] Loaded MQTT password");
      #endif
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
  
  config.magic = 0x5A6B;
  config.crc = 0;
  
  memset(config.wifiSsid, 0, sizeof(config.wifiSsid));
  memset(config.wifiPassword, 0, sizeof(config.wifiPassword));
  memset(config.mqttBroker, 0, sizeof(config.mqttBroker));
  memset(config.mqttUser, 0, sizeof(config.mqttUser));
  memset(config.mqttPassword, 0, sizeof(config.mqttPassword));
  memset(config.mqttClientId, 0, sizeof(config.mqttClientId));
  
  config.mqttPort = 1883;
  config.lowHum = DEFAULT_LOW_HUM;
  config.highHum = DEFAULT_HIGH_HUM;
  config.lowTemp = DEFAULT_LOW_TEMP;
  config.highTemp = DEFAULT_HIGH_TEMP;
  config.delaySeconds = DEFAULT_DELAY_SECONDS;
  config.automaticMode = DEFAULT_AUTOMATIC_MODE;
  config.slowModeEnabled = DEFAULT_SLOW_MODE;
  config.slowModeDuty = SLOW_MODE_DUTY_CYCLE;
  config.sensorInterval = SENSOR_DURATION;
  config.maxOnTime = MAX_ON_TIME_SEC;
  config.setSwitchOff = DEFAULT_SET_SWITCH_OFF;
  config.scheduleCount = 0;
  memset(config.reserved, 0, sizeof(config.reserved));
  
  config_loadFromCredentials();
  
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Defaults set");
  #endif
}

void config_clear() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Clearing configuration");
  #endif
  config_setDefaults();
  config_write();
  configValid = false;
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Configuration cleared");
  #endif
}

void config_read() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Reading from EEPROM...");
  #endif
  
  uint8_t* ptr = (uint8_t*)&config;
  
  for (size_t i = 0; i < sizeof(Config); i++) {
    ptr[i] = EEPROM.read(i);
  }
  
  #ifdef DEBUG_ENABLE
    Serial.printf("[CONFIG] Read magic: 0x%04X (expected 0x5A6B)\n", config.magic);
    Serial.printf("[CONFIG] Read WiFi SSID: '%s'\n", config.wifiSsid);
    Serial.printf("[CONFIG] Read MQTT Broker: '%s'\n", config.mqttBroker);
    Serial.printf("[CONFIG] Read CRC from EEPROM: 0x%04X\n", config.crc);
  #endif
  
  if (config.magic == 0x5A6B) {
    uint16_t savedCrc = config.crc;
    config.crc = 0;
    uint16_t calcCrc = crc16((uint8_t*)&config, sizeof(Config));
    
    #ifdef DEBUG_ENABLE
      Serial.printf("[CONFIG] Calculated CRC: 0x%04X\n", calcCrc);
    #endif
    
    if (calcCrc == savedCrc) {
      config.crc = savedCrc;
      configValid = true;
      #ifdef DEBUG_ENABLE
        Serial.println("[CONFIG] Config is VALID");
      #endif
      return;
    } else {
      #ifdef DEBUG_ENABLE
        Serial.printf("[CONFIG] CRC mismatch! EEPROM: 0x%04X, Calculated: 0x%04X\n", savedCrc, calcCrc);
      #endif
    }
  } else {
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Magic mismatch! Config is INVALID");
    #endif
  }
  
  configValid = false;
  config_setDefaults();
}

void config_write() {
  #ifdef DEBUG_ENABLE
    Serial.println("[CONFIG] Writing to EEPROM...");
    Serial.printf("[CONFIG] WiFi SSID: '%s'\n", config.wifiSsid);
    Serial.printf("[CONFIG] MQTT Broker: '%s:%d'\n", config.mqttBroker, config.mqttPort);
  #endif
  
  uint16_t oldCrc = config.crc;
  config.crc = 0;
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
  uint8_t* vptr = (uint8_t*)&verify;
  for (size_t i = 0; i < sizeof(Config); i++) {
    vptr[i] = EEPROM.read(i);
  }
  
  // Убрана переменная verifyCrc
  verify.crc = 0;
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
    #ifdef DEBUG_ENABLE
      Serial.println("[CONFIG] Verification FAILED!");
    #endif
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
  Serial.printf("Temp range: %.1f - %.1f\n", config.lowTemp, config.highTemp);
  Serial.printf("Hum range: %.1f - %.1f\n", config.lowHum, config.highHum);
  Serial.printf("Delay: %d sec, Auto: %d, Slow: %d, Duty: %d\n", 
    config.delaySeconds, config.automaticMode, config.slowModeEnabled, config.slowModeDuty);
  Serial.printf("SET_SWITCH_OFF: %s\n", config.setSwitchOff ? "ON (выключаем при старте)" : "OFF (сохраняем состояние)");
  Serial.printf("CRC: 0x%04X\n", config.crc);
  Serial.printf("Config valid: %s\n", configValid ? "YES" : "NO");
  #if HAS_CREDENTIALS
    Serial.println("Credentials: loaded from credentials.h");
  #else
    Serial.println("Credentials: no credentials.h found");
  #endif
}