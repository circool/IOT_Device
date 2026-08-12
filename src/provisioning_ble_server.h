/**
 * @file provisioning_ble_server.h
 * @brief BLE-сервер для провизионинга
 */

#ifndef PROVISIONING_BLE_SERVER_H
#define PROVISIONING_BLE_SERVER_H

#include <Arduino.h>

#ifndef BLE_PROVISIONING_PIN
#define BLE_PROVISIONING_PIN "abcd1234"
#endif

#ifndef MAX_RETRIES
#define MAX_RETRIES 3
#endif

struct BleWifiConfig {
  char wifiSsid[32];
  char wifiPassword[64];
};

// Предобъявление класса
class ProvisioningManager;

#if defined(ESP32) && !defined(ESP8266)

class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr);
  ~BleProvisioningServer();

  bool begin(ProvisioningManager* manager);
  void stop();
  bool isActive() const { return _active; }

 private:
  bool _active = false;
  char _deviceName[32];
};

extern BleProvisioningServer* g_bleServer;

#else

class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr) {
    (void)deviceName;
  }
  ~BleProvisioningServer() {}

  bool begin(ProvisioningManager* manager) {
    (void)manager;
    return false;
  }
  void stop() {}
  bool isActive() const { return false; }
};

inline BleProvisioningServer* g_bleServer = nullptr;

#endif

#endif  // PROVISIONING_BLE_SERVER_H