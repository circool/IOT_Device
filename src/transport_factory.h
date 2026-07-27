#ifndef TRANSPORT_FACTORY_H
#define TRANSPORT_FACTORY_H

#include "settings.h"
#include "transport.h"

// ============================================================================
// ФАБРИКА ТРАНСПОРТОВ (выбор на этапе компиляции)
// ============================================================================

/**
 * @brief Создать транспорт в зависимости от TRANSPORT_TYPE
 * @return Указатель на структуру Transport или nullptr
 *
 * @note Выбор на этапе компиляции через #if — экономия Flash
 * @note В рантайме вызывается один раз при старте
 */
inline Transport* createTransport() {
#if TRANSPORT_TYPE == 0  // MQTT
#if FEATURE_MQTT_ENABLED == 1
#include "transport_mqtt.h"
  return getMQTTTransport();
#else
  return nullptr;
#endif

#elif TRANSPORT_TYPE == 1  // ZigBee
#if TRANSPORT_TYPE == 2
#include "transport_zigbee.h"
  return getZigbeeTransport();
#else
  return nullptr;
#endif

#elif TRANSPORT_TYPE == 2  // Matter
#warning "Matter transport not implemented yet"
  return nullptr;

#else
  return nullptr;
#endif
}

#endif  // TRANSPORT_FACTORY_H