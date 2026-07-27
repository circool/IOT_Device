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
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
#if FEATURE_MQTT_ENABLED == 1
#include "transport_mqtt.h"
  return getMQTTTransport();
#else
  return nullptr;
#endif

#elif TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
#include "transport_zigbee.h"
  return getZigbeeTransport();

#elif TRANSPORT_TYPE == TRANSPORT_TYPE_THREAD
#warning "Thread transport not implemented yet"
  return nullptr;

#else  // TRANSPORT_TYPE_NONE или неизвестный
  return nullptr;
#endif
}

#endif  // TRANSPORT_FACTORY_H