// ============================================================================
// @file wifi_manager.h
// @brief Управление WiFi соединением
//
// Обеспечивает:
// - Подключение к WiFi сети в режиме клиента (STA)
// - Запуск точки доступа (AP) для настройки
// - Мониторинг состояния соединения
// - Получение информации о сети (IP, RSSI)
// - Автоматическое переподключение при потере соединения
//
// @note Все решения о том, в каком режиме работать (клиент/AP),
//       принимает оркестратор (main.cpp). WiFi слой только выполняет команды.
// ============================================================================

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

#if WIFI_ENABLED == 1

// ============================================================================
// ПАРАМЕТРЫ ПО УМОЛЧАНИЮ (можно переопределить в platformio.ini)
// ============================================================================

#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 30000  // Таймаут подключения (мс)
#endif

#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"  // IP адрес точки доступа
#endif

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Инициализация WiFi стека
 *
 * Вызывается один раз в setup(). Настраивает режим STA,
 * но не пытается подключиться к сети.
 */
void wifi_init();

/**
 * @brief Подключиться к WiFi сети
 *
 * @param ssid     Имя сети (SSID)
 * @param password Пароль (может быть пустым для открытой сети)
 * @return true    — подключение инициировано (асинхронно)
 * @return false   — неверные параметры (пустой ssid)
 *
 * @note Функция не блокирующая. Результат проверяется через wifi_isConnected()
 */
bool wifi_connect(const char* ssid, const char* password);

/**
 * @brief Проверить наличие соединения с WiFi
 * @return true — подключено, false — нет соединения
 */
bool wifi_isConnected();

/**
 * @brief Получить локальный IP адрес
 * @return IP адрес в виде строки (например, "192.168.1.100")
 */
String wifi_getLocalIP();

/**
 * @brief Получить уровень сигнала WiFi
 * @return RSSI в dBm (отрицательное значение, например -55)
 */
int wifi_getRSSI();

/**
 * @brief Получить статус подключения
 * @return WL_CONNECTED, WL_NO_SSID_AVAIL, WL_CONNECT_FAILED и т.д.
 */
wl_status_t wifi_getStatus();

/**
 * @brief Периодический вызов в loop()
 *
 * Обрабатывает асинхронное подключение:
 * - Проверяет статус подключения
 * - При успехе — логирует IP
 * - При таймауте — останавливает попытку
 *
 * @note Для автоматического переподключения при потере соединения
 *       оркестратор должен вызывать wifi_connect() повторно.
 */
void wifi_process();

/**
 * @brief Запустить точку доступа (AP режим)
 *
 * @param ssid     Имя сети (SSID)
 * @param password Пароль (может быть пустым для открытой сети)
 * @return true    — AP запущена
 * @return false   — ошибка
 */
bool wifi_startAP(const char* ssid, const char* password = nullptr);

/**
 * @brief Остановить точку доступа
 *
 * Отключает AP режим, возвращает устройство в режим клиента.
 */
void wifi_stopAP();

/**
 * @brief Проверить, активна ли точка доступа
 * @return true — AP активна, false — нет
 */
bool wifi_isAPActive();

/**
 * @brief Сканирование WiFi сетей (для отладки)
 *
 * @param targetSsid Если указан, будет отмечен в логе
 * @return Количество найденных сетей, -1 при ошибке
 */
int wifi_scan(const char* targetSsid = nullptr);

#else  // WIFI_ENABLED == 0

// ============================================================================
// ЗАГЛУШКИ ДЛЯ РЕЖИМА БЕЗ WIFI
// ============================================================================

inline void wifi_init() {}
inline bool wifi_connect(const char*, const char*) {
  return false;
}
inline bool wifi_isConnected() {
  return false;
}
inline String wifi_getLocalIP() {
  return "0.0.0.0";
}
inline int wifi_getRSSI() {
  return 0;
}
inline void wifi_process() {}
inline bool wifi_startAP(const char* ssid, const char* password = nullptr) {
  return false;
}
inline void wifi_stopAP() {}
inline bool wifi_isAPActive() {
  return false;
}
inline int wifi_scan(const char*) {
  return -1;
}

#endif  // WIFI_ENABLED == 1

#endif  // WIFI_MANAGER_H