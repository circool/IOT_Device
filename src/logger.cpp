/**
 * @file logger.cpp
 * @brief Реализация логгера
 * @version 0.12
 * @date 10.08.2026
 */

#include "logger.h"

// ============================================================================
// ГЛОБАЛЬНОЕ СОСТОЯНИЕ
// ============================================================================

static struct {
  LogLevel level;
  uint32_t categories;
  bool useColor;
  bool initialized;
} s_log = {.level = XLOG_LEVEL_INFO,
           .categories = CAT_ALL,
           .useColor = true,
           .initialized = false};

// ============================================================================
// ПРИВАТНЫЕ ФУНКЦИИ
// ============================================================================

static const char* _level_to_string(LogLevel level) {
  switch (level) {
    case XLOG_LEVEL_ERROR:
      return "ERROR";
    case XLOG_LEVEL_WARN:
      return "WARN ";
    case XLOG_LEVEL_INFO:
      return "INFO ";
    case XLOG_LEVEL_DEBUG:
      return "DEBUG";
    default:
      return "?????";
  }
}

static const char* _category_to_string(LogCategory category) {
  switch (category) {
    case CAT_CONFIG:
      return "CONFIG";
    case CAT_SENSOR:
      return "SENSOR";
    case CAT_FAN:
      return "FAN";
    case CAT_SWITCH:
      return "SWITCH";
    case CAT_ACTUATOR:
      return "ACTUATOR";
    case CAT_MQTT:
      return "MQTT";
    case CAT_WIFI:
      return "WIFI";
    case CAT_WEB:
      return "WEB";
    case CAT_OTA:
      return "OTA";
    case CAT_LED:
      return "LED";
    case CAT_WDT:
      return "WDT";
    case CAT_AP:
      return "AP";
    case CAT_MAIN:
      return "MAIN";
    case CAT_PROVISIONING:
      return "PROV";
    case CAT_BLE:
      return "BLE";
    case CAT_BUTTON:
      return "BUTTON";
    case CAT_RESTART:
      return "RESTART";
    case CAT_SYSTEM:
      return "SYSTEM";
    case CAT_DEVICE:
      return "DEVCTRL";
    case CAT_TRANSPORT:
      return "TRANSPORT";
    case CAT_STATE:
      return "STATE";
    default:
      return "???";
  }
}

static const char* _color_for_level(LogLevel level) {
  switch (level) {
    case XLOG_LEVEL_ERROR:
      return ANSI_BRIGHT_RED;
    case XLOG_LEVEL_WARN:
      return ANSI_BRIGHT_MAGENTA;
    case XLOG_LEVEL_INFO:
      return ANSI_RESET;
    case XLOG_LEVEL_DEBUG:
      return ANSI_BLUE;
    default:
      return ANSI_RESET;
  }
}

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

void log_init(LogLevel level, uint32_t categories, bool useColor) {
  s_log.level = level;
  s_log.categories = categories;
  s_log.useColor = useColor;
  s_log.initialized = true;

  Serial.begin(MONITOR_SPEED);
  delay(100);

  log_message(XLOG_LEVEL_INFO, CAT_CONFIG,
              "Logger initialized (level=%d, categories=0x%08X)", (int)level,
              categories);
}

void log_set_level(LogLevel level) {
  s_log.level = level;
}

void log_set_categories(uint32_t categories) {
  s_log.categories = categories;
}

void log_set_color(bool enabled) {
  s_log.useColor = enabled;
}

bool log_is_enabled(LogLevel level, LogCategory category) {
  if (!s_log.initialized)
    return false;
  if (level > s_log.level)
    return false;
  if (!(s_log.categories & category))
    return false;
  return true;
}

void log_message(LogLevel level,
                 LogCategory category,
                 const char* format,
                 ...) {
  if (!log_is_enabled(level, category))
    return;

  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  unsigned long uptime = millis();

  const char* color = s_log.useColor ? _color_for_level(level) : "";
  const char* reset = s_log.useColor ? ANSI_RESET : "";

  if (s_log.useColor) {
    Serial.printf("%s[%6lu] [%s] [%s] %s%s\n", color, uptime,
                  _level_to_string(level), _category_to_string(category),
                  buffer, reset);
  } else {
    Serial.printf("[%6lu] [%s] [%s] %s\n", uptime, _level_to_string(level),
                  _category_to_string(category), buffer);
  }
}