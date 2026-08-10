/**
 * @file state_provider.cpp
 * @brief Реализация провайдера состояния
 * @version 0.11
 * @date 10.08.2026
 */

#include "state_provider.h"
#include "logger.h"


bool StateProvider::init() {
    // Установка начальных значений
    link_ok = false;
    gateway_ok = false;
    setup_mode = false;
    XLOG_INFO(CAT_STATE, "StateProvider initialized");
    return true;
}