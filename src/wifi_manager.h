#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// ============================================================================
// НАСТРОЙКИ WIFI
// ============================================================================

/** @brief Включить режим точки доступа (AP) для настройки */
#ifndef AP_ENABLED
#define AP_ENABLED 1
#endif

/** @brief Таймаут подключения к WiFi (миллисекунды) */
#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 6000
#endif

/** @brief Интервал проверки WiFi соединения (мс) */
#ifndef WIFI_CHECK_INTERVAL_MS
#define WIFI_CHECK_INTERVAL_MS 10000
#endif

// ============================================================================
// РЕЖИМ ТОЧКИ ДОСТУПА (AP)
// ============================================================================

#if AP_ENABLED == 1
/** @brief IP адрес точки доступа */
#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"
#endif

/**
 * @brief Время без WiFi до перехода в режим AP (мс)
 * Если устройство не может подключиться к WiFi дольше этого времени,
 * запускается собственная точка доступа для настройки.
 */
#ifndef AP_FALLBACK_TIMEOUT_MS
#define AP_FALLBACK_TIMEOUT_MS 12000
#endif
#endif

/** @brief Включить поддержку WiFi */
#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1
#endif

#if WIFI_ENABLED == 1

extern bool apMode;
bool wifi_is_ap_mode();

#ifndef SCANNING_WIFI_ENABLED
#define SCANNING_WIFI_ENABLED 0
#endif

/** @brief Включить веб-интерфейс */
#ifndef WEB_ENABLED
#define WEB_ENABLED 1
#endif

/**
 * @brief Флаг процесса подключения к WiFi
 * @note true — идёт подключение, false — не идёт
 */
extern bool wifi_is_connecting;

/**
 * @brief Инициализация подключения к WiFi
 * @note Запускает асинхронное подключение к сохранённой сети
 */
void wifi_begin();

/**
 * @brief Проверка статуса подключения к WiFi
 * @note Вызывается в loop() для отслеживания прогресса подключения
 */
void wifi_check();

/**
 * @brief Мониторинг и поддержание WiFi соединения
 * @note Обрабатывает потерю связи, переподключение и fallback в AP режим
 */
void wifi_monitor();

/**
 * @brief Получить локальный IP адрес
 * @return IP адрес в виде строки (например, "192.168.1.100")
 */
String wifi_get_local_ip();

/**
 * @brief Получить уровень сигнала WiFi
 * @return RSSI в dBm (отрицательное значение, например -55)
 */
int wifi_get_rssi();

/**
 * @brief Проверить наличие WiFi соединения
 * @return true — подключён к точке доступа, false — нет соединения
 */
bool wifi_is_connected();

/**
 * @brief Запустить режим точки доступа (AP)
 * @param ssid Имя WiFi сети (SSID) для точки доступа
 * @note IP адрес точки доступа задаётся макросом AP_IP_ADDRESS
 */
void wifi_start_ap(const char* ssid);

/**
 * @brief Выполнить сканирование WiFi сетей и вывести результат в лог
 * @param targetSsid SSID для отметки в логе (если nullptr или пустой — без
 * отметки)
 * @return количество найденных сетей, -1 при ошибке или если сканирование уже
 * выполняется
 *
 * @note Функция синхронная, блокирует выполнение до завершения сканирования
 * (2-5 секунд)
 * @note Защищена от реентерабельности
 */
int wifi_scan_and_log(const char* targetSsid);

void wifi_start_ap(const char* ssid);
void wifi_stop_ap();

#else  // WIFI_ENABLED == 0

/**
 * @brief Заглушка: инициализация WiFi (отключена)
 */
inline void wifi_begin() {}

/**
 * @brief Заглушка: проверка WiFi (отключена)
 */
inline void wifi_check() {}

/**
 * @brief Заглушка: мониторинг WiFi (отключён)
 */
inline void wifi_monitor() {}

/**
 * @brief Заглушка: получить локальный IP
 * @return "0.0.0.0" — нет соединения
 */
inline String wifi_get_local_ip() {
  return "0.0.0.0";
}

/**
 * @brief Заглушка: получить RSSI
 * @return 0 — нет сигнала
 */
inline int wifi_get_rssi() {
  return 0;
}

/**
 * @brief Заглушка: проверить соединение
 * @return false — WiFi отключён
 */
inline bool wifi_is_connected() {
  return false;
}

/**
 * @brief Заглушка: запустить точку доступа
 * @param ssid Не используется
 */
inline void wifi_start_ap(const char* ssid) {
  (void)ssid;
}

inline void wifi_stop_ap(){}

/**
 * @brief Заглушка: сканирование сетей
 * @return -1 — операция недоступна
 */
inline int wifi_scan_and_log(const char* /*targetSsid*/) {
  return -1;
}

/**
 * @brief Флаг подключения (заглушка)
 * @note Всегда false, так как WiFi отключён
 */
static bool wifi_is_connecting = false;

// void wifi_start_ap_mode() {};

#endif  // WIFI_ENABLED == 1

#endif  // WIFI_H