/**
 * @file system_state.cpp
 * @brief Реализация состояния системы
 */

#include "system_state.h"
#include "logger.h"

static uint16_t _bits = 0;

void system_state_init() {
  _bits = 0;
  XLOG_INFO(CAT_SYSTEM, "System state initialized");
}

void system_state_set_bit(uint16_t bit) {
  if (!(_bits & bit)) {
    _bits |= bit;
    XLOG_DEBUG(CAT_SYSTEM, "Set bit: 0x%04X (bits: 0x%04X)", bit, _bits);
  }
}

void system_state_clear_bit(uint16_t bit) {
  if (_bits & bit) {
    _bits &= ~bit;
    XLOG_DEBUG(CAT_SYSTEM, "Clear bit: 0x%04X (bits: 0x%04X)", bit, _bits);
  }
}

bool system_state_has_bit(uint16_t bit) {
  return (_bits & bit) != 0;
}

uint16_t system_state_get_bits() {
  return _bits;
}