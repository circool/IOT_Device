#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>
#include <functional>

// Forward declaration
class BleProvisioningServer;

extern char deviceId[12];
extern bool apMode;

enum class ProvisioningMode : uint8_t {
  NONE = 0,
  BLE = 1,
  AP = 2,
  COMPLETED = 3
};

struct ProvisioningConfig {
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
  char deviceName[32];
  uint8_t protocolMode;
};

using ProvisioningCallback =
    std::function<void(const ProvisioningConfig* config, void* userData)>;

class ProvisioningManager {
 public:
  static ProvisioningManager& getInstance();

  bool begin(ProvisioningCallback callback = nullptr,
             void* userData = nullptr,
             uint32_t timeoutMs = 0);

  void process();
  bool isActive() const;
  ProvisioningMode getMode() const;
  bool complete(const ProvisioningConfig* config = nullptr);
  void reset();
  bool getSavedConfig(ProvisioningConfig* config) const;
  bool saveConfig(const ProvisioningConfig* config);

  void onConfigReceived(const ProvisioningConfig* config);

 private:
  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;

  enum class InternalState : uint8_t {
    IDLE,
    WAITING,
    RECEIVED,
    COMPLETED,
    ERROR
  };

  InternalState _state = InternalState::IDLE;
  ProvisioningMode _mode = ProvisioningMode::NONE;
  ProvisioningCallback _callback = nullptr;
  void* _userData = nullptr;
  uint32_t _timeoutMs = 0;
  unsigned long _startTime = 0;
  ProvisioningConfig _config;

  BleProvisioningServer* _bleServer = nullptr;  // <-- ДОБАВЛЕНО

  void selectProvisioningMethod();
  bool loadFromStorage();
  BleProvisioningServer* getBleServer() const;  // <-- ДОБАВЛЕНО
};

#endif  // PROVISIONING_H