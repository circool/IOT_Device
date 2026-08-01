/**
 * @file wifi_manager.h
 * @brief 
 */
 
#ifndef WIFI_H
#define WIFI_H

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

#if PROVISIONING_METHOD == 2 || PROVISIONING_METHOD == 3

/** @brief IP адрес точки доступа */
#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"
#endif

/** @brief Веб-интерфейс нужен для провизионинга
 * @todo: найти место где можно безопасно переиниировать эту константу
 */
#ifndef FEATURE_WEB_ENABLED
#define FEATURE_WEB_ENABLED 1
#endif

/**
 * @brief Запустить режим точки доступа (AP)
 * @param ssid Имя WiFi сети (SSID) для точки доступа
 * @note IP адрес точки доступа задаётся макросом AP_IP_ADDRESS
 */
void wifi_start_ap(const char* ssid);

/**
 * @brief Остановить режим точки доступа (AP)
 * @note Оставлен для симметрии с wifi_start_ap
 */
void wifi_stop_ap();

#endif  // PROVISIONING_METHOD == 2 || PROVISIONING_METHOD == 3

#else  // TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI

inline void wifi_manager_init() {};
inline void wifi_manager_connect(const char* ssid, const char* password) {};

inline void wifi_manager_update() {};
inline String wifi_get_local_ip() { return "0.0.0.0"; };
inline int wifi_get_rssi() {return 0;};
inline int wifi_scan_and_log(const char* /*targetSsid*/) { return -1; };
inline void wifi_start_ap(const char* ssid) {};
inline void wifi_stop_ap(){};
#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

#endif  // WIFI_H