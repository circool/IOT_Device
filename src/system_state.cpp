/**
 * @file system_state.cpp
 * @brief Определение глобального состояния системы
 */

#include "system_state.h"
#include "logger.h"

SystemState g_systemState = SystemState::INIT;
static SystemState _lastLoggedState = SystemState::INIT;

void system_state_set(SystemState state) {
  if (g_systemState != state) {
    g_systemState = state;

    // Логируем изменение состояния
    const char* stateNames[] = {"INIT",   "NORMAL", "MODE_1",         "MODE_2",
                                "MODE_3", "MODE_4", "RESTART_PENDING"};
    XLOG_INFO(CAT_SYSTEM, "State: %s", stateNames[(uint8_t)state]);
  }
}

SystemState system_state_get() {
  return g_systemState;
}