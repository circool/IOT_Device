/**
 * @file restart_manager.h
 * @brief Менеджер перезагрузок
 * @details Управляет перезагрузкой устройства с задержкой.
 *          Позволяет запросить перезагрузку из любого места кода,
 *          а выполнение происходит в loop() с заданной задержкой.
 *          Пишет состояние в StateProvider.
 */

#ifndef RESTART_MANAGER_H
#define RESTART_MANAGER_H

#include <Arduino.h>

/**
 * @brief Запросить перезагрузку устройства
 * @param delayMs Задержка перед перезагрузкой в миллисекундах (по умолчанию
 * 500)
 *
 * @details Запрос сохраняется во внутреннем флаге, а реальная перезагрузка
 *          происходит в restart_update() после истечения задержки.
 *          Повторный вызов до выполнения перезагрузки игнорируется.
 *          Устанавливает флаг restart_pending в StateProvider.
 */
void restart_request(unsigned long delayMs = 500);

/**
 * @brief Периодическая проверка необходимости перезагрузки
 * @details Вызывается в loop(). Если перезагрузка запрошена и задержка истекла
 *          — выполняет ESP.restart().
 */
void restart_update();

#endif  // RESTART_MANAGER_H