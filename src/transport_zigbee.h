/**
 * @file transport_zigbee.h
 * @brief Zigbee-транспорт — заглушка
 */

#ifndef TRANSPORT_ZIGBEE_H
#define TRANSPORT_ZIGBEE_H

#include "config_manager.h"
#include "settings.h"
#include "transport.h"
#include "transport_types.h"

// ============================================================================
// ZIGBEE-ТРАНСПОРТ (с заглушками)
// ============================================================================

#ifdef USE_ZIGBEE

/**
 * @brief Zigbee-транспорт (заглушка)
 * @details Все методы только логируют вызовы. Используется как заглушка
 *          до реализации реального Zigbee-стека.
 */
typedef struct ZigbeeTransport {
  Transport base;

  // ===== СОСТОЯНИЕ =====
  bool initialized;
  bool connected;
  const TransportConfig* config;

  // ===== ВНУТРЕННЕЕ СОСТОЯНИЕ ТРАНСПОРТА =====
  TransportState _state;
  TransportEventCallback _eventCallback;
  void* _eventContext;
} ZigbeeTransport;

/**
 * @brief Получить глобальный экземпляр Zigbee-транспорта
 * @return Указатель на структуру Transport
 */
Transport* getZigbeeTransport();

#else  // USE_ZIGBEE == 0

// ============================================================================
// ЗАГЛУШКИ (USE_ZIGBEE == 0)
// ============================================================================

/**
 * @brief Заглушка ZigbeeTransport — Zigbee-транспорт отключён
 * @details Используется при TRANSPORT_TYPE != ZIGBEE
 */
typedef struct ZigbeeTransport {
  Transport base;
} ZigbeeTransport;

/**
 * @brief Заглушка — Zigbee-транспорт отключён
 * @return Всегда возвращает nullptr
 */
inline Transport* getZigbeeTransport() {
  return nullptr;
}

#endif  // USE_ZIGBEE

#endif  // TRANSPORT_ZIGBEE_H