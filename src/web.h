// ============================================================================
// @file web.h
// @brief Веб-сервер для отображения статуса и настройки
// ============================================================================

#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config.h"



#if defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif

// ============================================================================
// ФУНКЦИИ УСТАНОВКИ КОНТЕКСТА (вызываются из оркестратора)
// ============================================================================

/**
 * @brief Установить тип активного транспорта
 * @param type TRANSPORT_MQTT, TRANSPORT_ZIGBEE, TRANSPORT_MATTER или
 * TRANSPORT_NONE
 */
void web_setTransport(TransportType type);

/**
 * @brief Установить тип устройства
 * @param type DEVICE_TYPE (1=fan, 2=sensor, 3=switch)
 */
void web_setDeviceType(uint8_t type);

/**
 * @brief Установить указатель на активный конфиг
 * @param cfg Указатель на Config (из оркестратора)
 */
void web_setConfig(const Config* cfg);

/**
 * @brief Установить флаг режима AP
 * @param isApMode true = AP режим, false = STA режим
 */
void web_setApMode(bool isApMode);

/**
 * @brief Включить/выключить страницу статуса в корне
 * @param enabled true — корень ведёт на статус, false — на конфиг
 */
void web_enableStatusPage(bool enabled);

/**
 * @brief Обновить данные датчика для отображения
 */
void web_setSensorData(float temp, float hum);

/**
 * @brief Обновить состояние актуатора для отображения
 */
void web_setActuatorState(bool on, int speed = -1);

/**
 * @brief Инициализация веб-сервера
 */
void web_init();

/**
 * @brief Периодический вызов в loop()
 */
void web_update();

// #else  // WEB_ENABLED == 1

// // Заглушки
// inline void web_setTransport(TransportType) {}
// inline void web_setDeviceType(uint8_t) {}
// inline void web_setConfig(const Config*) {}
// inline void web_setApMode(bool) {}
// inline void web_enableStatusPage(bool) {}
// inline void web_setSensorData(float, float) {}
// inline void web_setActuatorState(bool, int) {}
// inline void web_init() {}
// inline void web_update() {}

// #endif  // WEB_ENABLED == 1

#endif  // WEB_H