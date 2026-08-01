/**
 * @file web_manager.h
 * @brief Веб-интерфейс устройства
 * @details HTTP-сервер для управления и настройки устройства.
 *          Работает только при TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI.
 *          В AP-режиме делегирует провизионинг в слой provisioning.
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

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

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
void web_registerStatusProvider(IWebStatusProvider* provider);

/**
 * @brief Инициализация веб-сервера
 * @param setupMode true — AP-режим (настройка), false — нормальный режим
 */
void web_init(bool setupMode = false);

/**
 * @brief Периодическая обработка HTTP-запросов. Вызывается в loop()
 */
void web_update(void);

/**
 * @brief Построить HTML-код страницы состояния
 * @return HTML-строка
 */
String web_buildStatusHtml(void);

/**
 * @brief Отправить страницу состояния
 * @param refreshInterval Интервал автообновления (сек)
 */
void web_sendStatusPage(int refreshInterval);

/**
 * @brief Отправить страницу конфигурации
 * @param errorMsg Текст ошибки (NULL если нет)
 * @param successMsg Текст успеха (NULL если нет)
 */
void web_sendConfigPage(const char* errorMsg, const char* successMsg);

/**
 * @brief Обработчик сохранения конфигурации (POST /save)
 */
void web_saveConfig(void);

/**
 * @brief Обработчик оперативных команд (/set)
 */
void handleSetCommand(void);

// ============================================================================
// ГЛОБАЛЬНЫЕ ФЛАГИ ДЛЯ ОРКЕСТРАТОРА
// ============================================================================

/**
 * @brief Флаг: есть новые настройки от Web (/save)
 */
extern volatile bool g_webConfigPending;

/**
 * @brief Флаг: запрошена перезагрузка от Web (/resetall)
 */
extern volatile bool g_webRestartPending;

/**
 * @brief Временный буфер с новыми настройками
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
 * @brief Флаг: есть команда от Web (/set)
 */
extern volatile bool g_webCommandPending;

/**
 * @brief Буфер команды от Web
 */
extern WebCommand g_webCommand;

#else  // TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI || FEATURE_WEB_STATUS_ENABLED ==
       // 0

// ============================================================================
// ЗАГЛУШКИ
// ============================================================================

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline void web_registerStatusProvider(IWebStatusProvider* provider) {
  (void)provider;
}

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline void web_init(bool setupMode) {
  (void)setupMode;
}

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline void web_update(void) {}

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline String web_buildStatusHtml(void) {
  return String();
}

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline void web_sendStatusPage(int) {}

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline void web_sendConfigPage(const char*, const char*) {}

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline void web_saveConfig(void) {}

// Заглушка - Web отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI или
// FEATURE_WEB_STATUS_ENABLED == 0)
inline void handleSetCommand(void) {}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI && FEATURE_WEB_STATUS_ENABLED
        // == 1

#endif  // WEB_H