/**
 * @file logger.cpp
 * @brief Реализация логгера
 */

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

// ============================================================================
// СИНГЛТОН
// ============================================================================

Logger& Logger::getInstance() {
  static Logger instance;
  return instance;
}

// ============================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ============================================================================

void Logger::begin(LogLevel level, uint32_t categories, bool useColor) {
  _currentLevel = level;
  _enabledCategories = categories;
  _useColor = useColor;
  _initialized = true;

  Serial.begin(MONITOR_SPEED);
  delay(100);

  log(XLOG_LEVEL_INFO, CAT_CONFIG,
      "\n\n\nLogger initialized (level=%d, categories=0x%08X)", (int)level,
      categories);
}

void Logger::setLevel(LogLevel level) {
  _currentLevel = level;
}

void Logger::setCategories(uint32_t categories) {
  _enabledCategories = categories;
}

void Logger::setColorEnabled(bool enabled) {
  _useColor = enabled;
}

bool Logger::isEnabled(LogLevel level, LogCategory category) const {
  if (!_initialized)
    return false;
  if (level > _currentLevel)
    return false;
  if (!(_enabledCategories & category))
    return false;
  return true;
}

void Logger::log(LogLevel level,
                 LogCategory category,
                 const char* format,
                 ...) {
  if (!isEnabled(level, category))
    return;

  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  char output[340];
  const char* color = _useColor ? getColorForLevel(level) : "";
  const char* reset = _useColor ? ANSI_RESET : "";

  unsigned long uptime = millis();

  if (_useColor) {
    snprintf(output, sizeof(output), "%s [%6lu] [%s] [%s] %s%s\n", color,uptime, 
             levelToString(level), categoryToString(category), buffer, reset);
  } else {
    snprintf(output, sizeof(output), "[%6lu] [%s] [%s] %s\n", uptime,
             levelToString(level), categoryToString(category), buffer);
  }
  Serial.print(output);
}

void Logger::log(LogLevel level, LogCategory category, const String& message) {
  if (!isEnabled(level, category))
    return;

  const char* color = _useColor ? getColorForLevel(level) : "";
  const char* reset = _useColor ? ANSI_RESET : "";

  if (_useColor) {
    Serial.printf("%s[%s] [%s] %s%s\n", color, levelToString(level),
                  categoryToString(category), message.c_str(), reset);
  } else {
    Serial.printf("[%s] [%s] %s\n", levelToString(level),
                  categoryToString(category), message.c_str());
  }
}

// ============================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ============================================================================

const char* Logger::levelToString(LogLevel level) const {
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

const char* Logger::categoryToString(LogCategory category) const {
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
    case CAT_RESET_BTN:
      return "RESET_BTN";
    case CAT_RESTART:
      return "RESTART";
    case CAT_SYSTEM:
      return "SYSTEM";
    default:
      return "???";
  }
}

const char* Logger::getColorForLevel(LogLevel level) const {
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