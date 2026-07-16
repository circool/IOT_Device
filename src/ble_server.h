#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>

#ifndef BLE_PROVISIONING_PIN
#define BLE_PROVISIONING_PIN "abcd1234"
#endif

struct BleWifiConfig {
  char wifiSsid[32];
  char wifiPassword[64];
};

#if defined(ESP32) && !defined(ESP8266)

class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr);
  ~BleProvisioningServer();

  bool begin();
  void stop();

 private:
  bool _active = false;
  char _deviceName[32];
};

#else

class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr) {
    (void)deviceName;
  }
  ~BleProvisioningServer() {}

  bool begin() { return false; }
  void stop() {}
};

#endif

#endif