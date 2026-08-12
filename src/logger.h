/**
 * @file logger.h
 * @brief Логирование
 * @details Единая система логирования с поддержкой уровней, категорий и цветов
 * @version 0.12
 * @date 10.08.2026
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>
#include "settings.h"

#ifndef MONITOR_SPEED
#define MONITOR_SPEED 115200
#endif

// ============================================================================
// НАСТРОЙКИ ЛОГИРОВАНИЯ
// ============================================================================

#ifndef XLOG_LEVEL
#define XLOG_LEVEL 4  // 0=NONE, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG
#endif

#ifndef XLOG_CATEGORIES
#define XLOG_CATEGORIES 0xFFFFFFFF  // Все категории
#endif

#ifndef XLOG_USE_COLOR
#define XLOG_USE_COLOR 1
#endif

// ============================================================================
// ANSI-ЦВЕТА
// ============================================================================

#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"

#define ANSI_BRIGHT_RED "\033[91m"
#define ANSI_BRIGHT_GREEN "\033[92m"
#define ANSI_BRIGHT_YELLOW "\033[93m"
#define ANSI_BRIGHT_BLUE "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN "\033[96m"
#define ANSI_BRIGHT_WHITE "\033[97m"

#define ANSI_BG_BLACK "\033[40m"
#define ANSI_BG_RED "\033[41m"
#define ANSI_BG_GREEN "\033[42m"
#define ANSI_BG_YELLOW "\033[43m"
#define ANSI_BG_BLUE "\033[44m"

#define ANSI_BOLD "\033[1m"
#define ANSI_BOLD_RESET "\033[22m"
#define ANSI_DIM "\033[2m"
#define ANSI_ITALIC "\033[3m"
#define ANSI_UNDERLINE "\033[4m"
#define ANSI_RESET "\033[0m"

// ============================================================================
// УРОВНИ ЛОГИРОВАНИЯ
// ============================================================================

enum LogLevel : uint8_t {
  XLOG_LEVEL_NONE = 0,
  XLOG_LEVEL_ERROR = 1,
  XLOG_LEVEL_WARN = 2,
  XLOG_LEVEL_INFO = 3,
  XLOG_LEVEL_DEBUG = 4
};

// ============================================================================
// КАТЕГОРИИ ЛОГИРОВАНИЯ
// ============================================================================

enum LogCategory : uint32_t {
  CAT_NONE = 0,
  CAT_CONFIG = 1 << 0,
  CAT_SENSOR = 1 << 1,
  CAT_FAN = 1 << 2,
  CAT_SWITCH = 1 << 3,
  CAT_ACTUATOR = 1 << 4,
  CAT_MQTT = 1 << 5,
  CAT_WIFI = 1 << 6,
  CAT_WEB = 1 << 7,
  CAT_OTA = 1 << 8,
  CAT_LED = 1 << 9,
  CAT_WDT = 1 << 10,
  CAT_AP = 1 << 11,
  CAT_MAIN = 1 << 12,
  CAT_PROVISIONING = 1 << 13,
  CAT_BLE = 1 << 14,
  CAT_BUTTON = 1 << 15,
  CAT_RESTART = 1 << 16,
  CAT_SYSTEM = 1 << 17,
  CAT_DEVICE = 1 << 18,
  CAT_TRANSPORT = 1 << 19,
  CAT_STATE = 1 << 20,
  CAT_ALL = 0xFFFFFFFF
};

// ============================================================================
// API
// ============================================================================

/**
 * @brief Инициализация логгера
 * @param level Максимальный уровень для вывода
 * @param categories Битовая маска разрешённых категорий
 * @param useColor Использовать ANSI-цвета
 */
void log_init(LogLevel level = (LogLevel)XLOG_LEVEL,
              uint32_t categories = XLOG_CATEGORIES,
              bool useColor = XLOG_USE_COLOR);

/**
 * @brief Установить уровень логирования
 */
void log_set_level(LogLevel level);

/**
 * @brief Установить разрешённые категории
 */
void log_set_categories(uint32_t categories);

/**
 * @brief Включить/выключить цвета
 */
void log_set_color(bool enabled);

/**
 * @brief Проверить, нужно ли логировать данный уровень/категорию
 */
bool log_is_enabled(LogLevel level, LogCategory category);

/**
 * @brief Основной метод логирования (printf-стиль)
 */
void log_message(LogLevel level, LogCategory category, const char* format, ...);

// ============================================================================
// УДОБНЫЕ МАКРОСЫ
// ============================================================================

#define XLOG_ERROR(cat, fmt, ...) \
  log_message(XLOG_LEVEL_ERROR, cat, fmt, ##__VA_ARGS__)

#define XLOG_WARN(cat, fmt, ...) \
  log_message(XLOG_LEVEL_WARN, cat, fmt, ##__VA_ARGS__)

#define XLOG_INFO(cat, fmt, ...) \
  log_message(XLOG_LEVEL_INFO, cat, fmt, ##__VA_ARGS__)

#define XLOG_DEBUG(cat, fmt, ...) \
  log_message(XLOG_LEVEL_DEBUG, cat, fmt, ##__VA_ARGS__)

#define XLOG_ENABLED(level, cat) log_is_enabled(level, cat)

#endif  // LOGGER_H