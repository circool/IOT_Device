#ifndef DEBUG_TOOLS_H
#define DEBUG_TOOLS_H

#include "config_manager.h"

void print_system_info();
const char* getResetReason();
float getChipTemperature();

/**
 * @brief Вывод конфигурации в лог
 * @param transport Конфигурация транспорта (уже прочитанная)
 * @param device Настройки устройства (уже прочитанные)
 */
void printConfig(const TransportConfig& transport,
                 const DeviceConfig& device);

/**
 * @brief Генерация случайного имени устройства
 * @param buffer Буфер для записи ID (должен быть минимум 32 байта)
 * @param size Размер буфера
 * @details Имя формируется как "прилагательное-существительное-число"
 *          Например: "brave-panda-7341", "calm-tiger-2198"
 *          Использует esp_random() на ESP32 или random() на ESP8266
 */
void generateRandomDeviceId(char* buffer, size_t size);

#endif  // DEBUG_TOOLS_H