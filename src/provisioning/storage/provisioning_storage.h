#ifndef PROVISIONING_STORAGE_H
#define PROVISIONING_STORAGE_H

#include <Arduino.h>
#include "../provisioning.h"

class ProvisioningStorage {
 public:
  static ProvisioningStorage& getInstance();

  bool begin();
  bool save(const ProvisioningConfig* config);
  bool load(ProvisioningConfig* config);
  bool hasConfig() const;
  void clear();

 private:
  ProvisioningStorage() = default;
  ~ProvisioningStorage() = default;
  ProvisioningStorage(const ProvisioningStorage&) = delete;
  ProvisioningStorage& operator=(const ProvisioningStorage&) = delete;

  static const uint16_t STORAGE_MAGIC = 0xA5C3;

  static const uint16_t STORAGE_VERSION = 1;

  struct StorageHeader {
    uint16_t magic;
    uint16_t version;
    uint16_t crc;
  };

  struct StorageBlock {
    StorageHeader header;
    ProvisioningConfig config;
  };

  bool _initialized = false;
  StorageBlock _cache;

  bool validate(const StorageBlock* block) const;
};

#endif  // PROVISIONING_STORAGE_H