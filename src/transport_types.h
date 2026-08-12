/**
 * @file transport_types.h
 * @brief Типы данных для транспортной абстракции
 */

#ifndef TRANSPORT_TYPES_H
#define TRANSPORT_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "common_types.h"

// ============================================================================
// КОМАНДЫ
// ============================================================================



// ============================================================================
// ТИПЫ КОЛБЭКОВ
// ============================================================================

/**
 * @brief Колбэк для событий транспорта
 * @param event Указатель на структуру события (TransportEventData)
 * @param context Контекст, переданный при регистрации
 */
typedef void (*TransportEventCallback)(const TransportEventData* event,
                                       void* context);



#endif  // TRANSPORT_TYPES_H