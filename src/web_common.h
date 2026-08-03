/**
 * @file web_common.h
 * @brief Общие функции для всех Web-страниц
 * @details Содержит унифицированные функции рендеринга HTML-элементов
 *          и отправки страниц. Используется как в обычном режиме,
 *          так и в AP-режиме (провизионинг).
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
typedef enum { PAGE_MODE_NORMAL, PAGE_MODE_AP, PAGE_MODE_RESULT } PageMode;

/**
 * @brief Колбэк для отправки HTML-контента
 * @param chunk Строка для отправки
 * @param context Контекст (указатель на WebServerClass)
 */
typedef void (*WebSendCallback)(const char* chunk, void* context);

// ============================================================================
// БАЗОВАЯ СТРУКТУРА ДЛЯ ПОЛЕЙ
// ============================================================================

/**
 * @brief Базовая структура для полей ввода
 */
struct FieldBase {
  const char* label;
  const char* name;
  const char* note;
};

// ============================================================================
// СТРУКТУРЫ ПОЛЕЙ
// ============================================================================

/**
 * @brief Текстовое поле (TEXT / PASSWORD)
 */
struct TextField : FieldBase {
  const char* value;
  const char* placeholder;
  bool hideInput;
  bool required;
};

/**
 * @brief Числовое поле (NUMBER / FLOAT)
 */
struct NumberField : FieldBase {
  const char* value;
  const char* placeholder;
  const char* min;
  const char* max;
  const char* step;
  bool required;
};

/**
 * @brief Чекбокс
 */
struct CheckboxField : FieldBase {
  bool checked;
  bool required;
};

// ============================================================================
// СТРУКТУРЫ БЛОКОВ (по внешнему виду)
// ============================================================================

/**
 * @brief Параметры текстового блока (заголовок + значение + единица измерения)
 */
struct TextBlockParams {
  const char* title;
  const char* value;
  const char* unit;
  const char* colorClass;
};

/**
 * @brief Параметры блока статуса (ON / OFF)
 */
struct StatusBlockParams {
  bool isOn;
  const char* label;
};

/**
 * @brief Параметры кнопки
 */
struct ButtonParams {
  const char* label;
  const char* url;
  const char* colorClass;
};

/**
 * @brief Параметры индикатора прогресса (шкала)
 */
struct ProgressParams {
  int percent;
};

/**
 * @brief Параметры информационного блока (многострочный текст)
 */
struct InfoBlockParams {
  const char* title;
  const char* text;
  const char* colorClass;
};

// ============================================================================
// ЕДИНАЯ ФУНКЦИЯ РЕНДЕРИНГА (перегрузки)
// ============================================================================

// ---- ПОЛЯ ----
void render(char* buf, size_t size, const TextField& field);
void render(char* buf, size_t size, const NumberField& field);
void render(char* buf, size_t size, const CheckboxField& field);

// ---- БЛОКИ ----
void render(char* buf, size_t size, const TextBlockParams& params);
void render(char* buf, size_t size, const StatusBlockParams& params);
void render(char* buf, size_t size, const ButtonParams& params);
void render(char* buf, size_t size, const ProgressParams& params);
void render(char* buf, size_t size, const InfoBlockParams& params);

// ============================================================================
// ОТПРАВКА СТРАНИЦ
// ============================================================================

/**
 * @brief Отправить HTML-контент через WebServer
 * @param chunk Строка для отправки
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
 * @param url URL для перехода
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
 * @brief Отправить страницу AP-провизионинга
 * @param send Колбэк для отправки
 * @param context Контекст
 */
void web_sendApProvisioningPage(WebSendCallback send, void* context);

#endif  // WEB_COMMON_H