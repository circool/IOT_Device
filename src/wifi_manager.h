/**
 * @file wifi_manager.h
 * @brief Менеджер соединения с WiFi
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include "settings.h"
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// ============================================================================
// НАСТРОЙКИ WIFI
// ============================================================================

/** @brief WiFi SSID по умолчанию (заводской) */
#ifndef SSID_NAME
#define SSID_NAME ""
#endif

/** @brief WiFi пароль по умолчанию (заводской) */
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif


/**
 * @brief Время без WiFi до перехода в режим AP (мс)
 * Если устройство потеряло соединение с WiFi дольше этого времени,
 * запускается провизионинг для настройки.
 */
#ifndef WIFI_FALLBACK_TIMEOUT_MS
#define WIFI_FALLBACK_TIMEOUT_MS 10000
#endif




#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

#ifndef SCANNING_WIFI_ENABLED
#define SCANNING_WIFI_ENABLED 0
#endif

/**
 * @brief Инициализация WiFi менеджера
 * @details Вызывается один раз в setup()
 *          Настраивает WiFi стек и запускает первое подключение
 */
void wifi_manager_init();

/**
 * @brief Подключение к WiFi
 * @note Запускает асинхронное подключение к сети
 * @deprecated see wifi_manager_init
 */
void wifi_manager_connect(const char* ssid, const char* password);

/**
 * @brief Периодический вызов в loop()
 * @details Обновляет состояние подключения и управляет STATE_WIFI_OK
 * @todo Продумать функциональные обязанности и решить что делать с
 * wifi_check/wifi_monitor
 */
void wifi_manager_update();

/**
 * @brief Получить локальный IP адрес
 * @return IP адрес в виде строки (например, "192.168.1.100")
 * @todo Продумать над целесообразностью наличия функции из одной строки
 */
String wifi_get_local_ip();

/**
 * @brief Получить уровень сигнала WiFi
 * @return RSSI в dBm (отрицательное значение, например -55)
 * @todo Продумать над целесообразностью наличия функции из одной строки
 */
int wifi_get_rssi();

/**
 * @brief Выполнить сканирование WiFi сетей и вывести результат в лог
 * @param targetSsid SSID для отметки в логе (если nullptr или пустой — без
 * отметки)
 * @return количество найденных сетей, -1 при ошибке или если сканирование уже
 * выполняется
 *
 * @note Функция синхронная, блокирует выполнение до завершения сканирования
 * (2-5 секунд)
 * @note Нужна только на этапе отладки, в продакшн удалить
 */
int wifi_scan_and_log(const char* targetSsid);



#else  // TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI

inline void wifi_manager_init() {};
inline void wifi_manager_connect(const char* ssid, const char* password) {};

inline void wifi_manager_update() {};
inline String wifi_get_local_ip() { return "0.0.0.0"; };
inline int wifi_get_rssi() {return 0;};
inline int wifi_scan_and_log(const char* /*targetSsid*/) { return -1; };


#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

#endif  // WIFI_MANAGER_H