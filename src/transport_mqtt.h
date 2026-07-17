#ifndef MQTT_TRANSPORT_H
#define MQTT_TRANSPORT_H

#include "mqtt.h"
#include "transport.h"

#if FEATURE_MQTT_ENABLED == 1
// ============================================================================
// MQTT ТРАНСПОРТ — РЕАЛИЗАЦИЯ ITRANSPORT ЧЕРЕЗ УКАЗАТЕЛИ НА ФУНКЦИИ
// ============================================================================

/**
 * @brief Получить глобальный экземпляр MQTT транспорта
 * @return Указатель на структуру Transport с заполненными методами
 */
Transport* getMQTTTransport();
#else  // FEATURE_MQTT_ENABLED == 0

/**
 * @brief Заглушка — MQTT отключён
 * @return nullptr
 */
inline Transport* getMQTTTransport() {
  return nullptr;
}

#endif  // FEATURE_MQTT_ENABLED

#endif  // MQTT_TRANSPORT_H