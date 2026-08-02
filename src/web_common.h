/**
 * @file web_common.h
 * @brief Общие функции для всех Web-страниц
 * @details Содержит унифицированные функции рендеринга HTML-элементов
 *          и отправки страниц. Используется как в обычном режиме,
 *          так и в AP-режиме (провизионинг).
 * @date 2026-07-28
 */

#ifndef WEB_COMMON_H
#define WEB_COMMON_H

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif

#include "config_manager.h"

// ============================================================================
// ТИПЫ
// ============================================================================

/**
 * @brief Режим отображения страницы
 */
typedef enum {
  PAGE_MODE_NORMAL, /**< Обычный режим (полный функционал) */
  PAGE_MODE_AP,     /**< AP-режим (упрощённая страница) */
  PAGE_MODE_RESULT  /**< Страница результата операции */
} PageMode;

/**
 * @brief Колбэк для отправки HTML-контента
 * @param chunk Строка для отправки (null-terminated)
 * @param context Контекст (указатель на WebServerClass)
 */
typedef void (*WebSendCallback)(const char* chunk, void* context);

// ============================================================================
// СПЕЦИАЛИЗИРОВАННЫЕ СТРУКТУРЫ ДЛЯ ПОЛЕЙ
// ============================================================================

/**
 * @brief Текстовое поле (TEXT / PASSWORD)
 */
typedef struct {
  const char* label;
  const char* name;
  const char* value;
  const char* placeholder;
  const char* note;
  bool hideInput;  // true: type='password', false: type='text'
  bool required;
} FieldText;

/**
 * @brief Числовое поле (NUMBER) — целые числа
 */
typedef struct {
  const char* label;
  const char* name;
  const char* value;
  const char* placeholder;
  const char* note;
  const char* min;
  const char* max;
  const char* step;
  bool required;
} FieldNumber;

/**
 * @brief Числовое поле (FLOAT) — числа с плавающей точкой
 */
typedef struct {
  const char* label;
  const char* name;
  const char* value;
  const char* placeholder;
  const char* note;
  const char* min;
  const char* max;
  const char* step;
  bool required;
} FieldFloat;

/**
 * @brief Чекбокс (CHECKBOX)
 */
typedef struct {
  const char* label;
  const char* name;
  const char* note;
  bool checked;
  bool required;
} FieldCheckbox;

// ============================================================================
// РЕНДЕРИНГ ПОЛЕЙ
// ============================================================================

void render_text(char* buf, size_t size, const FieldText* field);
void render_number(char* buf, size_t size, const FieldNumber* field);
void render_float(char* buf, size_t size, const FieldFloat* field);
void render_checkbox(char* buf, size_t size, const FieldCheckbox* field);

// ============================================================================
// ОБЩИЕ ФУНКЦИИ РЕНДЕРИНГА
// ============================================================================

/**
 * @brief Сгенерировать карточку датчика
 * @param buf Буфер для записи
 * @param size Размер буфера
 * @param value Значение датчика
 * @param label Подпись
 * @param unit Единица измерения
 * @param colorClass CSS-класс цвета (info/error/success)
 * @param note Примечание (опционально)
 */
void web_renderSensorCard(char* buf,
                          size_t size,
                          float value,
                          const char* label,
                          const char* unit,
                          const char* colorClass,
                          const char* note);

/**
 * @brief Сгенерировать статусную карточку
 * @param buf Буфер для записи
 * @param size Размер буфера
 * @param status Текст статуса (ON/OFF)
 * @param isOn true = включено (error), false = выключено (info)
 */
void web_renderStatusCard(char* buf,
                          size_t size,
                          const char* status,
                          bool isOn);

/**
 * @brief Сгенерировать кнопку
 * @param buf Буфер для записи
 * @param size Размер буфера
 * @param text Текст кнопки
 * @param url Ссылка
 * @param style CSS-класс кнопки (опционально, по умолчанию "link-btn")
 */
void web_renderButton(char* buf,
                      size_t size,
                      const char* text,
                      const char* url,
                      const char* style);

/**
 * @brief Сгенерировать шкалу скорости
 * @param buf Буфер для записи
 * @param size Размер буфера
 * @param speed Скорость (0-100)
 */
void web_renderSpeedBar(char* buf, size_t size, int speed);

// ============================================================================
// ОТПРАВКА СТРАНИЦ
// ============================================================================

/**
 * @brief Отправить HTML-контент через WebServer
 * @param chunk Строка для отправки (null-terminated)
 * @param context Контекст (указатель на WebServerClass)
 */
void webSendContent(const char* chunk, void* context);

/**
 * @brief Отправить начало HTML-страницы
 * @param send Колбэк для отправки
 * @param context Контекст
 * @param title Заголовок страницы
 * @param mode Режим страницы
 */
void web_sendPageStart(WebSendCallback send,
                       void* context,
                       const char* title,
                       PageMode mode);

/**
 * @brief Отправить конец HTML-страницы
 * @param send Колбэк для отправки
 * @param context Контекст
 */
void web_sendPageEnd(WebSendCallback send, void* context);

/**
 * @brief Отправить meta-тег автоматического обновления
 * @param send Колбэк для отправки
 * @param context Контекст
 * @param refreshSeconds Интервал обновления (сек)
 * @param url URL для перехода (NULL = обновить текущую страницу)
 */
void web_sendRefreshMeta(WebSendCallback send,
                         void* context,
                         int refreshSeconds,
                         const char* url);

/**
 * @brief Отправить страницу результата операции
 * @param send Колбэк для отправки
 * @param context Контекст
 * @param action Название действия
 * @param success true = успех, false = ошибка
 */
void web_send_result_page(WebSendCallback send,
                          void* context,
                          const char* action,
                          bool success);

/**
 * @brief Отправить страницу AP-провизионинга (настройка WiFi)
 * @param send Колбэк для отправки
 * @param context Контекст
 * @note Только рендеринг HTML. Логика сохранения — в provisioning.
 */
void web_sendApProvisioningPage(WebSendCallback send, void* context);

#endif  // WEB_COMMON_H