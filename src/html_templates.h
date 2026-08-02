/**
 * @file html_templates.h
 * @brief HTML-шаблоны для веб-интерфейса
 * @details Содержит HTML-константы в PROGMEM.
 * @date 2026-07-28
 */

#ifndef HTML_TEMPLATES_H
#define HTML_TEMPLATES_H

#include <Arduino.h>

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

/**
 * @brief Начало HTML-страницы
 */
extern const char HTML_PAGE_START[];

/**
 * @brief CSS-стили для веб-интерфейса
 */
extern const char HTML_STYLE[];

/**
 * @brief Конец HTML-страницы
 */
extern const char HTML_PAGE_END[];

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

#endif  // HTML_TEMPLATES_H