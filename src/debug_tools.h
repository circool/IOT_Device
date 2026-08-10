/**
 * @file debug_tools.h
 * @brief Отладочные утилиты
 * @version 0.12
 * @date 10.08.2026
 */

#ifndef DEBUG_TOOLS_H
#define DEBUG_TOOLS_H

#include <stddef.h>
#include <stdint.h>
#include "common_types.h"

// ============ RTC-память для сохранения контекста ============

#ifdef ESP32
#define RTC_PERSISTENT RTC_DATA_ATTR
#else
#define RTC_PERSISTENT
#endif

/**
 * @brief Структура для сохранения контекста перезагрузки в RTC-памяти
 */
typedef struct {
  ResetReason reason;
  uint32_t uptimeSeconds;
  uint32_t magic;  // 0xDEADBEEF для проверки валидности
} PersistentResetInfo;

// ============ API ============

/**
 * @brief Получить температуру чипа
 * @return Температура в °C или -273.15 если недоступна
 */
float getChipTemperature();

/**
 * @brief Получить причину перезагрузки в виде строки
 * @return Строковое описание причины
 */
const char* getResetReason();

/**
 * @brief Получить причину перезагрузки в виде enum
 * @return ResetReason
 */
ResetReason getResetReasonEnum();

/**
 * @brief Получить время работы системы (uptime) в секундах
 * @return Количество секунд с момента старта
 */
uint32_t getUptimeSeconds();

/**
 * @brief Проверить, доступна ли RTC-память на аппаратном уровне
 * @return true если RTC-память доступна (ESP32)
 */
bool isRtcAvailable();

/**
 * @brief Проверить, сохранены ли данные в RTC-памяти
 * @return true если данные валидны (magic совпадает)
 */
bool isRtcPersistent();

/**
 * @brief Сохранить контекст перезагрузки в RTC-память
 * @param reason Причина перезагрузки
 * @param uptime Текущее время работы (сек)
 */
void saveResetContext(ResetReason reason, uint32_t uptime);

/**
 * @brief Прочитать сохранённый контекст перезагрузки
 * @param info Указатель на структуру для заполнения
 * @return true если данные валидны, false если нет
 */
bool readResetContext(PersistentResetInfo* info);

/**
 * @brief Очистить сохранённый контекст перезагрузки
 */
void clearResetContext();

/**
 * @brief Вывод информации о системе в лог
 */
void printSystemInfo();

/**
 * @brief Вывод конфигурации в лог
 * @param transport Структура транспортной конфигурации
 * @param device Структура конфигурации устройства
 */
void printConfig(const TransportConfig& transport, const DeviceConfig& device);

/**
 * @brief Генерация случайного имени устройства
 * @param buffer Буфер для записи ID (минимум 32 байта)
 * @param size Размер буфера
 */
void generateRandomDeviceId(char* buffer, size_t size);

#endif  // DEBUG_TOOLS_H