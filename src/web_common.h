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
#include "common_types.h"

#if defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ МАКРОСЫ
// ============================================================================

/**
 * @brief Безопасное добавление строки в буфер с проверкой переполнения
 * @param dst Буфер назначения (должен быть char[] с известным размером)
 * @param src Добавляемая строка
 * @param size Размер буфера (обычно sizeof(dst))
 *
 * @warning Использует strcat, но только если есть место
 * @warning Требует, чтобы dst был инициализирован (buf[0] = '\0')
 */
#define SAFE_STRCAT(dst, src, size)               \
  do {                                            \
    if (strlen(dst) + strlen(src) < (size) - 1) { \
      strcat(dst, src);                           \
    }                                             \
  } while (0)

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
 * @param label Подпись поля
 * @param name Имя поля (атрибут name)
 * @param note Пояснение под полем
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
 * @param label Подпись поля
 * @param name Имя поля (атрибут name)
 * @param note Пояснение под полем
 * @param value Текущее значение поля
 * @param placeholder Плейсхолдер
 * @param hideInput true = скрывать ввод (password)
 * @param required true = поле обязательно для заполнения
 */
struct TextField : FieldBase {
  const char* value;
  const char* placeholder;
  bool hideInput;
  bool required;
};

/**
 * @brief Числовое поле (NUMBER / FLOAT)
 * @param label Подпись поля
 * @param name Имя поля (атрибут name)
 * @param note Пояснение под полем
 * @param value Текущее значение поля
 * @param placeholder Плейсхолдер
 * @param min Минимальное допустимое значение
 * @param max Максимальное допустимое значение
 * @param step Шаг изменения
 * @param required true = поле обязательно для заполнения
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
 * @param label Подпись поля
 * @param name Имя поля (атрибут name)
 * @param note Пояснение под полем
 * @param checked true = флажок установлен
 * @param required true = поле обязательно для заполнения
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
 * @param title Заголовок блока
 * @param value Отображаемое значение
 * @param unit Единица измерения (например, "°C", "%")
 * @param colorClass CSS-класс для стилизации (info, success, error)
 */
struct TextBlockParams {
  const char* title;
  const char* value;
  const char* unit;
  const char* colorClass;
};

/**
 * @brief Параметры блока статуса (ON / OFF)
 * @param isOn true = включено, false = выключено
 * @param label Текст статуса (если nullptr — "ON"/"OFF")
 */
struct StatusBlockParams {
  bool isOn;
  const char* label;
};

/**
 * @brief Параметры кнопки
 * @param label Текст на кнопке
 * @param url URL, на который ведёт кнопка
 * @param colorClass CSS-класс для стилизации (link-btn, danger)
 */
struct ButtonParams {
  const char* label;
  const char* url;
  const char* colorClass;
};

/**
 * @brief Параметры индикатора прогресса (шкала)
 * @param percent Значение в процентах (0-100)
 */
struct ProgressParams {
  int percent;
};

/**
 * @brief Параметры информационного блока (многострочный текст)
 * @param title Заголовок блока
 * @param text Текст блока (может содержать HTML)
 * @param colorClass CSS-класс для стилизации (info, success, error)
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
// РЕНДЕРИНГ БЛОКОВ (для композиции страниц) — ОБЩИЕ
// ============================================================================

/**
 * @brief Рендерить блок WiFi настроек
 * @details Используется на страницах /config и /savewifi (AP)
 * @param buf Буфер для записи HTML
 * @param size Размер буфера
 * @param config Конфигурация транспорта (для предзаполнения)
 * @param mode Режим: PAGE_MODE_NORMAL или PAGE_MODE_AP
 */
void renderWifiBlock(char* buf,
                     size_t size,
                     const TransportConfig* config,
                     PageMode mode);

/**
 * @brief Рендерить блок MQTT настроек
 * @details Используется на страницах /config и /savewifi (AP)
 * @param buf Буфер для записи HTML
 * @param size Размер буфера
 * @param config Конфигурация транспорта (для предзаполнения)
 */
void renderMQTTBlock(char* buf, size_t size, const TransportConfig* config);

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

#endif  // WEB_COMMON_H