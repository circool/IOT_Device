#ifndef TRANSPORT_TYPES_H
#define TRANSPORT_TYPES_H

#include <stdint.h>

// ============================================================================
// ТИПЫ КОЛБЭКОВ (единые для всех транспортов)
// ============================================================================

/** @brief Колбэк с булевым параметром */
typedef void (*TransportBoolCallback)(bool value, void* context);

/** @brief Колбэк с целочисленным параметром */
typedef void (*TransportIntCallback)(int value, void* context);

/** @brief Колбэк с 32-битным беззнаковым параметром */
typedef void (*TransportUintCallback)(uint32_t value, void* context);

/** @brief Колбэк с float-параметром */
typedef void (*TransportFloatCallback)(float value, void* context);

/** @brief Колбэк без параметров */
typedef void (*TransportVoidCallback)(void* context);

// ============================================================================
// КОНТЕЙНЕР ДЛЯ КОЛБЭКА С КОНТЕКСТОМ
// ============================================================================

/**
 * @brief Простая структура для хранения колбэка и контекста
 * @note Без std::function — экономия RAM (~32 байта на колбэк)
 */
template <typename T>
struct TransportCallback {
  T func = nullptr;
  void* context = nullptr;
};

#endif  // TRANSPORT_TYPES_H