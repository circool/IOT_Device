/**
 * @file transport_null.h
 * @brief NullTransport — заглушка для отсутствующего транспорта
 * @version 0.12
 * @details Используется когда TRANSPORT_TYPE == NONE
 *          Все методы — пустые заглушки, возвращают безопасные значения.
 */

#ifndef TRANSPORT_NULL_H
#define TRANSPORT_NULL_H

#include "transport.h"
#include "transport_types.h"

// ============================================================================
// NULL TRANSPORT (заглушка)
// ============================================================================

/**
 * @brief Получить экземпляр NullTransport
 * @return Указатель на структуру Transport с пустыми методами
 */
Transport* getNullTransport();

#endif  // TRANSPORT_NULL_H