// ===== ФАЙЛ: src/provisioning/ble/ble_server.h =====
#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>
#include <functional>

struct BleConfigData {
  char wifiSsid[32];
  char wifiPassword[64];
  char mqttBroker[64];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPassword[64];
  char mqttClientId[24];
  char zigbeeNetworkKey[32];
  uint16_t zigbeePanId;
  uint8_t zigbeeChannel;
  uint8_t protocolMode;
};

using ProvConfigCallback = std::function<void(const BleConfigData* config)>;
using ProvStatusCallback = std::function<void(uint8_t status)>;
using ProvConnCallback = std::function<bool()>;
using ProvIpCallback = std::function<const char*()>;

#if defined(ESP32) && !defined(ESP8266)

class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr);
  ~BleProvisioningServer();

  bool begin(ProvConfigCallback configCallback,
             ProvStatusCallback statusCallback = nullptr,
             ProvConnCallback connCallback = nullptr,
             ProvIpCallback ipCallback = nullptr);

  void stop();
  bool isActive() const;
  const char* getDeviceName() const;
  void setDeviceName(const char* name);
  void sendStatus(uint8_t status);
  void sendConnectionStatus(bool connected, const char* ip = nullptr);
  bool hasConnectedClient() const { return false; }
  void sendGetStatus() {}

 private:
  bool _active = false;
  char _deviceName[32];
  BleConfigData _config;
  bool _configReceived = false;

  ProvConfigCallback _configCallback;
  ProvStatusCallback _statusCallback;
  ProvConnCallback _connCallback;
  ProvIpCallback _ipCallback;
};

#else

class BleProvisioningServer {
 public:
  explicit BleProvisioningServer(const char* deviceName = nullptr) {
    (void)deviceName;
  }
  ~BleProvisioningServer() {}

  bool begin(ProvConfigCallback,
             ProvStatusCallback = nullptr,
             ProvConnCallback = nullptr,
             ProvIpCallback = nullptr) {
    return false;
  }
  void stop() {}
  bool isActive() const { return false; }
  const char* getDeviceName() const { return "No BLE"; }
  void setDeviceName(const char* name) { (void)name; }
  void sendStatus(uint8_t status) { (void)status; }
  void sendConnectionStatus(bool connected, const char* ip = nullptr) {
    (void)connected;
    (void)ip;
  }
  bool hasConnectedClient() const { return false; }
  void sendGetStatus() {}
};

#endif

#endif  // BLE_SERVER_H