/**
 * @file transport_factory.h
 * @version 0.12
 * @brief Фабрика транспортов — выбор реализации на этапе компиляции
 */

#ifndef TRANSPORT_FACTORY_H
#define TRANSPORT_FACTORY_H

#include "settings.h"
#include "transport.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
#include "transport_wifi.h"
#elif TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
#include "transport_zigbee.h"
#endif

#include "transport_null.h"

// ============================================================================
// ФАБРИКА
// ============================================================================

/**
 * @brief Создать транспорт в зависимости от TRANSPORT_TYPE
 * @return Указатель на структуру Transport или nullptr
 */
inline Transport* createTransport() {
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
  return getWiFiTransport();
#elif TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
  return getZigbeeTransport();
#elif TRANSPORT_TYPE == TRANSPORT_TYPE_THREAD
#warning "Thread transport not implemented yet"
  return nullptr;
#else
  return getNullTransport();
#endif
}

#endif  // TRANSPORT_FACTORY_H