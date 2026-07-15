#ifndef MQTT_TRANSPORT_H
#define MQTT_TRANSPORT_H

#include "mqtt.h"
#include "transport.h"

// ============================================================================
// MQTT ТРАНСПОРТ — РЕАЛИЗАЦИЯ ITRANSPORT ЧЕРЕЗ УКАЗАТЕЛИ НА ФУНКЦИИ
// ============================================================================

/**
 * @brief Получить глобальный экземпляр MQTT транспорта
 * @return Указатель на структуру Transport с заполненными методами
 */
Transport* getMQTTTransport();

#endif  // MQTT_TRANSPORT_H