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
 * @brief Тип поля ввода для унифицированного рендеринга
 */
typedef enum {
  FIELD_TYPE_TEXT,     /**< Текстовое поле (<input type="text">) */
  FIELD_TYPE_PASSWORD, /**< Поле пароля (<input type="password">) */
  FIELD_TYPE_NUMBER,   /**< Числовое поле (<input type="number">) */
  FIELD_TYPE_CHECKBOX, /**< Флажок (<input type="checkbox">) */
  FIELD_TYPE_LABEL,    /**< Информационная метка (<div class="info">) */
  FIELD_TYPE_BUTTON,   /**< Кнопка с ссылкой */
  FIELD_TYPE_LINK,     /**< Обычная ссылка */
  FIELD_TYPE_CARD,     /**< Карточка статуса */
} FieldType;

/**
 * @brief Структура описания поля для унифицированного рендеринга
 */
typedef struct {
  FieldType type;          /**< Тип поля */
  const char* label;       /**< Подпись */
  const char* name;        /**< Имя поля (для input) */
  const char* value;       /**< Текущее значение */
  const char* placeholder; /**< Подсказка */
  const char* note;        /**< Примечание */
  const char* min;         /**< Минимальное значение (для number) */
  const char* max;         /**< Максимальное значение (для number) */
  const char* step;        /**< Шаг (для number) */
  const char* link;        /**< URL ссылки */
  const char* buttonText;  /**< Текст кнопки */
  bool checked;            /**< Для checkbox */
  bool required;           /**< Обязательное поле */
} FieldDef;

/**
 * @brief Колбэк для отправки HTML-контента
 * @param chunk Строка для отправки (null-terminated)
 * @param context Контекст (указатель на WebServerClass)
 */
typedef void (*WebSendCallback)(const char* chunk, void* context);

// ============================================================================
// УНИВЕРСАЛЬНЫЙ РЕНДЕРИНГ
// ============================================================================

/**
 * @brief Сгенерировать HTML-блок поля в буфер
 * @param buf Буфер для записи
 * @param size Размер буфера
 * @param field Описание поля
 */
void web_renderField(char* buf, size_t size, const FieldDef* field);

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
void web_sendResultPage(WebSendCallback send,
                        void* context,
                        const char* action,
                        bool success);

/**
 * @brief Отправить страницу AP-провизионинга (настройка WiFi)
 * @param send Колбэк для отправки
 * @param context Контекст
 * @todo Перенести этот функционал в слой провизионинга
 */
void web_sendApProvisioningPage(WebSendCallback send, void* context);



#endif  // WEB_COMMON_H