/**
 * @file web_manager.h
 * @brief Веб-интерфейс устройства
 * @details HTTP-сервер для управления и настройки устройства.
 *          Работает только при TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI.
 * @date 2026-07-28
 */

#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config_manager.h"
#include "web_common.h"
#include "web_status_provider.h"

// ============================================================================
// НАСТРОЙКИ
// ============================================================================

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

/**
 * @brief Показывать RSSI на странице состояния
 */
#ifndef WEB_SHOW_RSSI
#define WEB_SHOW_RSSI 1
#endif

/**
 * @brief Интервал автообновления страницы по умолчанию (сек)
 */
#ifndef DEFAULT_WEB_REFRESH
#define DEFAULT_WEB_REFRESH 5
#endif

/**
 * @brief Включить сброс настроек через веб-интерфейс
 */
#ifndef WEB_RESET_ENABLED
#define WEB_RESET_ENABLED 0
#endif

#endif

// ============================================================================
// ПУБЛИЧНЫЙ API
// ============================================================================

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI && FEATURE_WEB_STATUS_ENABLED == 1

/** @brief Глобальный экземпляр веб-сервера */
extern WebServerClass server;

/**
 * @brief Зарегистрировать провайдер статуса для Web
 * @param provider Указатель на реализацию IWebStatusProvider
 */
void web_register_status_provider(IWebStatusProvider* provider);

/**
 * @brief Инициализация веб-сервера
 * @details Регистрирует все маршруты: /, /config, /save, /set, /update
 * @note Вызывается один раз после подключения WiFi
 */
void web_init(void);

/**
 * @brief Периодическая обработка HTTP-запросов
 * @details Вызывается в loop(), обрабатывает входящие запросы
 */
void web_update(void);

/**
 * @brief Построить HTML-код страницы состояния
 * @return HTML-строка с данными устройства
 */
String web_build_status_html(void);

/**
 * @brief Отправить страницу состояния
 * @param refreshInterval Интервал автообновления (сек)
 */
void web_send_status_page(int refreshInterval);

/**
 * @brief Отправить страницу конфигурации
 * @param send Колбэк для отправки контента
 * @param context Контекст для колбэка
 * @param cfg Указатель на структуру ConfigData
 * @param currentMode Текущий режим (Client WiFi / Access Point)
 * @param currentSsid Текущий SSID
 * @param currentIp Текущий IP-адрес
 * @param refreshSeconds Интервал автообновления (0 = отключено)
 * @param errorMsg Текст ошибки (NULL если нет)
 * @param successMsg Текст успеха (NULL если нет)
 */
void web_send_config_page(WebSendCallback send,
                          void* context,
                          const ConfigData* cfg,
                          const char* currentMode,
                          const char* currentSsid,
                          const char* currentIp,
                          int refreshSeconds,
                          const char* errorMsg,
                          const char* successMsg);

/**
 * @brief Обработчик сохранения конфигурации
 * @details Обрабатывает POST-запрос на /save.
 *          Парсит и валидирует параметры формы.
 *          Устанавливает флаг g_webConfigPending для оркестратора.
 */
void web_handle_save(void);

/**
 * @brief Обработчик оперативных команд
 * @details Обрабатывает GET-запрос на /set.
 *          Поддерживает команды: state, speed, manualMode.
 *          Устанавливает флаг g_webCommandPending для оркестратора.
 */
void web_handle_set(void);

// ============================================================================
// ГЛОБАЛЬНЫЕ ФЛАГИ ДЛЯ ОРКЕСТРАТОРА
// ============================================================================

/**
 * @brief Флаг: есть новые настройки от Web (/save)
 * @details Устанавливается в web_handle_save(), сбрасывается в main.cpp
 */
extern volatile bool g_webConfigPending;

/**
 * @brief Флаг: запрошена перезагрузка от Web (/resetall)
 * @details Устанавливается в обработчике /resetall, сбрасывается в main.cpp
 */
extern volatile bool g_webRestartPending;

/**
 * @brief Флаг: есть команда от Web (/set)
 * @details Устанавливается в web_handle_set(), сбрасывается в main.cpp
 */
extern volatile bool g_webCommandPending;

/**
 * @brief Временный буфер с новыми настройками
 * @details Заполняется в web_handle_save(), применяется в main.cpp
 */
extern ConfigData g_webPendingConfig;

// ============================================================================
// КОМАНДЫ /set
// ============================================================================

/**
 * @brief Типы команд от Web
 */
typedef enum {
  CMD_STATE,       /**< Включить/выключить устройство */
  CMD_SPEED,       /**< Установить скорость (TYPE 1) */
  CMD_MANUAL_MODE, /**< Включить/выключить ручной режим (TYPE 1) */
} WebCommandType;

/**
 * @brief Структура команды от Web
 */
typedef struct {
  WebCommandType type; /**< Тип команды */
  union {
    bool boolVal; /**< Для команд ON/OFF */
    int intVal;   /**< Для команд скорости */
  } value;
} WebCommand;

/**
 * @brief Буфер команды от Web
 */
extern WebCommand g_webCommand;

#else  // TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI || FEATURE_WEB_STATUS_ENABLED ==
       // 0

// ============================================================================
// ЗАГЛУШКИ
// ============================================================================

/**
 * @brief Заглушка — Web отключён
 */
inline void web_register_status_provider(IWebStatusProvider* provider) {
  (void)provider;
}

/**
 * @brief Заглушка — Web отключён
 */
inline void web_init(void) {}

/**
 * @brief Заглушка — Web отключён
 */
inline void web_update(void) {}

/**
 * @brief Заглушка — Web отключён
 */
inline String web_build_status_html(void) {
  return String();
}

/**
 * @brief Заглушка — Web отключён
 */
inline void web_send_status_page(int) {}

/**
 * @brief Заглушка — Web отключён
 */
inline void web_send_config_page(WebSendCallback send,
                                 void* context,
                                 const ConfigData* cfg,
                                 const char* currentMode,
                                 const char* currentSsid,
                                 const char* currentIp,
                                 int refreshSeconds,
                                 const char* errorMsg,
                                 const char* successMsg) {
  (void)send;
  (void)context;
  (void)cfg;
  (void)currentMode;
  (void)currentSsid;
  (void)currentIp;
  (void)refreshSeconds;
  (void)errorMsg;
  (void)successMsg;
}

/**
 * @brief Заглушка — Web отключён
 */
inline void web_handle_save(void) {}

/**
 * @brief Заглушка — Web отключён
 */
inline void web_handle_set(void) {}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI && FEATURE_WEB_STATUS_ENABLED
        // == 1

#endif  // WEB_H