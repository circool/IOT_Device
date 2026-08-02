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
// БАЗОВАЯ СТРУКТУРА ДЛЯ ВСЕХ ПОЛЕЙ
// ============================================================================

/**
 * @brief Базовая структура для всех полей ввода
 * @details Содержит общие поля, которые наследуются всеми типами полей
 */
struct FieldBase {
  const char* label; /**< Подпись поля */
  const char* name;  /**< Имя поля (атрибут name) */
  const char* note;  /**< Примечание (отображается под полем) */
};

// ============================================================================
// СПЕЦИАЛИЗИРОВАННЫЕ СТРУКТУРЫ (наследуют FieldBase)
// ============================================================================

/**
 * @brief Текстовое поле (TEXT / PASSWORD)
 * @details Используется для SSID, паролей, MQTT Broker, User, Client ID
 */
struct TextField : FieldBase {
  const char* value;       /**< Текущее значение */
  const char* placeholder; /**< Подсказка (placeholder) */
  bool hideInput;          /**< true → type='password', false → type='text' */
  bool required;           /**< Обязательное поле */
};

/**
 * @brief Числовое поле (NUMBER / FLOAT)
 * @details Используется для портов, интервалов, скорости, температуры,
 *          влажности, таймаутов и задержек
 */
struct NumberField : FieldBase {
  const char* value;       /**< Текущее значение */
  const char* placeholder; /**< Подсказка (placeholder) */
  const char* min;         /**< Минимальное значение */
  const char* max;         /**< Максимальное значение */
  const char* step;        /**< Шаг изменения */
  bool required;           /**< Обязательное поле */
};

/**
 * @brief Чекбокс (CHECKBOX)
 * @details Используется для адаптивного режима, состояния при старте,
 *          сенсорного управления и подтверждения сохранения
 */
struct CheckboxField : FieldBase {
  bool checked;  /**< Состояние: true — отмечен, false — не отмечен */
  bool required; /**< Обязательное поле (требует отметки) */
};

// ============================================================================
// ОДНА ФУНКЦИЯ С ПЕРЕГРУЗКАМИ
// ============================================================================

/**
 * @brief Рендеринг текстового поля
 * @param buf Буфер для записи HTML
 * @param size Размер буфера
 * @param field Структура с параметрами поля
 */
void render_field(char* buf, size_t size, const TextField& field);

/**
 * @brief Рендеринг числового поля
 * @param buf Буфер для записи HTML
 * @param size Размер буфера
 * @param field Структура с параметрами поля
 */
void render_field(char* buf, size_t size, const NumberField& field);

/**
 * @brief Рендеринг чекбокса
 * @param buf Буфер для записи HTML
 * @param size Размер буфера
 * @param field Структура с параметрами поля
 */
void render_field(char* buf, size_t size, const CheckboxField& field);

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