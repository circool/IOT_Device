#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>
#include "settings.h"

#ifndef PROVISIONING_METHOD
#if PLATFORM_ESP8266
#define PROVISIONING_METHOD 2
#else
#define PROVISIONING_METHOD 3
#endif
#endif

#if PROVISIONING_METHOD == 0
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 0
#elif PROVISIONING_METHOD == 1
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 0
#elif PROVISIONING_METHOD == 2
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1
#elif PROVISIONING_METHOD == 3
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 1
#endif

enum class ProvisioningMethod : uint8_t {
  NONE = 0,
#if USE_BLE_PROVISIONING == 1 || USE_AP_PROVISIONING == 1
  WIFI = 1,
#endif
#if TRANSPORT_TYPE == 2
  ZIGBEE = 2,
#endif
#if FEATURE_MATTER_ENABLED == 1
  MATTER = 3,
#endif
  FAILED = 4
};

enum class ProvisioningState : uint8_t {
  IDLE = 0,
  ACTIVE = 1,
  COMPLETED = 2,
  FAILED = 3
};

struct ProvisioningData {
  uint8_t type;

#if TRANSPORT_TYPE == 1
  char wifiSsid[32];
  char wifiPassword[64];
#endif

#if TRANSPORT_TYPE == 2
  uint16_t zigbeePanId;
  uint8_t zigbeeChannel;
  char zigbeeNetworkKey[32];
#endif

#if FEATURE_MATTER_ENABLED == 1
  char matterSetupCode[32];
#endif
};

typedef void (*ProvisioningCallback)(ProvisioningMethod method, void* context);

class ProvisioningManager {
 public:
  static ProvisioningManager& getInstance();

  bool begin(ProvisioningCallback callback = nullptr, void* context = nullptr);
  void update();

  bool isCompleted() const;
  ProvisioningMethod getCompletedBy() const;
  ProvisioningState getState() const;
  int getRetryCount() const;
  const ProvisioningData* getData() const;

  void onDataReceived(const ProvisioningData& data);
  void onBleStatus(uint8_t status);

 private:
  ProvisioningManager() = default;
  ~ProvisioningManager() = default;
  ProvisioningManager(const ProvisioningManager&) = delete;
  ProvisioningManager& operator=(const ProvisioningManager&) = delete;

  void selectProvisioningMethod();

#if USE_BLE_PROVISIONING == 1
  void startBleProvisioning();
#endif

#if USE_AP_PROVISIONING == 1
  void startApProvisioning();
  bool isApComplete();
#endif

  static constexpr int MAX_RETRIES = 3;

  ProvisioningState _state = ProvisioningState::IDLE;
  ProvisioningMethod _completedBy = ProvisioningMethod::NONE;
  ProvisioningCallback _callback = nullptr;
  void* _context = nullptr;
  bool _started = false;

#if USE_AP_PROVISIONING == 1
  bool _apStarted = false;
#endif

  int _retryCount = 0;
  ProvisioningData _data;
};

void startProvisioning();
bool isProvisioningComplete();
ProvisioningMethod getProvisioningMethod();
ProvisioningState getProvisioningState();
int getProvisioningRetryCount();
const ProvisioningData* getProvisioningData();

#endif