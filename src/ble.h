#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>

struct BleWifiConfig {
  char wifiSsid[32];
  char wifiPassword[64];
};

typedef void (*ble_config_callback_t)(const BleWifiConfig* config);

bool ble_start(ble_config_callback_t callback,
               const char* serviceName = "PROV_ESP32",
               const char* pop = "12345678");

void ble_stop();
bool ble_is_active();
bool ble_is_config_received();
bool ble_get_config(BleWifiConfig* outConfig);

#endif