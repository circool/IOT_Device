#include "provisioning_storage.h"
#include <EEPROM.h>

// ============================================================================
// CRC16
// ============================================================================

static uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// ============================================================================
// РЕАЛИЗАЦИЯ
// ============================================================================

ProvisioningStorage& ProvisioningStorage::getInstance() {
  static ProvisioningStorage instance;
  return instance;
}

bool ProvisioningStorage::begin() {
  if (_initialized)
    return true;

  EEPROM.begin(sizeof(StorageBlock));
  _initialized = true;

  return load(&_cache.config);
}

bool ProvisioningStorage::save(const ProvisioningConfig* config) {
  if (!_initialized || !config)
    return false;

  StorageBlock block;
  memset(&block, 0, sizeof(block));

  block.header.magic = STORAGE_MAGIC;
  block.header.version = STORAGE_VERSION;
  memcpy(&block.config, config, sizeof(ProvisioningConfig));

  block.header.crc = crc16(reinterpret_cast<const uint8_t*>(&block.config),
                           sizeof(ProvisioningConfig));

  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&block);
  for (size_t i = 0; i < sizeof(StorageBlock); i++) {
    EEPROM.write(i, ptr[i]);
  }

  if (!EEPROM.commit()) {
    return false;
  }

  memcpy(&_cache, &block, sizeof(StorageBlock));
  return true;
}

bool ProvisioningStorage::load(ProvisioningConfig* config) {
  if (!_initialized || !config)
    return false;

  StorageBlock block;
  uint8_t* ptr = reinterpret_cast<uint8_t*>(&block);
  for (size_t i = 0; i < sizeof(StorageBlock); i++) {
    ptr[i] = EEPROM.read(i);
  }

  if (!validate(&block)) {
    return false;
  }

  memcpy(config, &block.config, sizeof(ProvisioningConfig));
  memcpy(&_cache, &block, sizeof(StorageBlock));
  return true;
}

bool ProvisioningStorage::hasConfig() const {
  if (!_initialized)
    return false;

  StorageBlock block;
  uint8_t* ptr = reinterpret_cast<uint8_t*>(&block);
  for (size_t i = 0; i < sizeof(StorageBlock); i++) {
    ptr[i] = EEPROM.read(i);
  }

  return validate(&block);
}

void ProvisioningStorage::clear() {
  if (!_initialized)
    return;

  for (size_t i = 0; i < sizeof(StorageBlock); i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();

  memset(&_cache, 0, sizeof(_cache));
}

bool ProvisioningStorage::validate(const StorageBlock* block) const {
  if (!block)
    return false;

  if (block->header.magic != STORAGE_MAGIC)
    return false;
  if (block->header.version != STORAGE_VERSION)
    return false;

  uint16_t calculated = crc16(reinterpret_cast<const uint8_t*>(&block->config),
                              sizeof(ProvisioningConfig));

  return calculated == block->header.crc;
}