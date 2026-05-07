#include <Arduino.h>
#include <unity.h>
#include "config.h"
#include "fan.h"
#include "sensor.h"
#include "mqtt.h"

// ==================== Config Tests ====================

void test_config_defaults_type1(void) {
  config_setDefaults();
  TEST_ASSERT_EQUAL(0x5A6B, config.magic);
  TEST_ASSERT_EQUAL(1883, config.mqttPort);
  #if DEVICE_TYPE == 1
    TEST_ASSERT_EQUAL(27.0, config.lowTemp);
    TEST_ASSERT_TRUE(config.lowTemp >= 26.99 && config.lowTemp <= 27.01);
    TEST_ASSERT_TRUE(config.highTemp >= 28.99 && config.highTemp <= 29.01);
    TEST_ASSERT_TRUE(config.lowHum >= 59.99 && config.lowHum <= 60.01);
    TEST_ASSERT_TRUE(config.highHum >= 69.99 && config.highHum <= 70.01);
    TEST_ASSERT_EQUAL(60, config.delaySeconds);
    TEST_ASSERT_EQUAL(false, config.slowModeEnabled);
    TEST_ASSERT_EQUAL(128, config.slowModeDuty);
    TEST_ASSERT_EQUAL(3600, config.maxOnTime);
    TEST_ASSERT_EQUAL(true, config.forceOffOnBoot);
    TEST_ASSERT_EQUAL(true, config.automaticMode);
  #endif
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  TEST_ASSERT_EQUAL(10, config.sensorInterval);
  #endif
  TEST_ASSERT_TRUE(true);
}

void test_crc16(void) {
  uint8_t data[] = {0x5A, 0x6B, 0x00, 0x00};
  uint16_t crc = crc16(data, sizeof(data));
  TEST_ASSERT_NOT_EQUAL(0, crc);
}

void test_config_write_read(void) {
  config_setDefaults();
  strcpy(config.wifiSsid, "test_ssid");
  config_write();
  config_read();
  TEST_ASSERT_TRUE(configValid);
  TEST_ASSERT_EQUAL_STRING("test_ssid", config.wifiSsid);
}

// ==================== Fan Tests ====================

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

void test_fan_init_forceOff(void) {
  config.forceOffOnBoot = true;
  fan_init();
  TEST_ASSERT_FALSE(fan_getState());
}

void test_fan_set_full(void) {
  config.slowModeEnabled = false;
  fan_set(true);
  TEST_ASSERT_TRUE(fan_getState());
  delay(50);
  TEST_ASSERT_TRUE(fan_getRealState());
  fan_set(false);
  TEST_ASSERT_FALSE(fan_getState());
}

void test_fan_override(void) {
  fan_setOverrideMode(true);
  TEST_ASSERT_TRUE(manualOverride);
  fan_setOverrideMode(false);
  TEST_ASSERT_FALSE(manualOverride);
}

void test_fan_delayTimer(void) {
  config.delaySeconds = 1;
  delayActive = false;
  fanOn = false;
  fan_delayTimer(true);
  TEST_ASSERT_TRUE(delayActive);
  TEST_ASSERT_GREATER_THAN(0, delayTimer);
  delay(1100);
  fan_delayTimer(false);
  TEST_ASSERT_TRUE(fan_getState());
  fan_set(false);
}

#endif

// ==================== Sensor Tests ====================

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

void test_sensor_not_ok(void) {
  sensorOk = false;
  TEST_ASSERT_FALSE(sensor_isOk());
}

void test_sensor_init(void) {
  sensor_init();
  TEST_ASSERT_TRUE(true);
}

#endif


// ==================== Runner ====================

void setup() {
  UNITY_BEGIN();
  
  // Config
  RUN_TEST(test_config_defaults_type1);
  RUN_TEST(test_crc16);
  EEPROM.begin(sizeof(Config) + 4);
  RUN_TEST(test_config_write_read);
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  RUN_TEST(test_fan_init_forceOff);
  RUN_TEST(test_fan_set_full);
  RUN_TEST(test_fan_override);
  RUN_TEST(test_fan_delayTimer);
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  RUN_TEST(test_sensor_not_ok);
  RUN_TEST(test_sensor_init);
  #endif
  
  
  
  UNITY_END();
}

void loop() {}