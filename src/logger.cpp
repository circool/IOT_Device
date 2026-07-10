#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

Logger& Logger::getInstance() {
  static Logger instance;
  return instance;
}

void Logger::begin(LogLevel level, uint16_t categories, bool useColor) {
  _currentLevel = level;
  _enabledCategories = categories;
  _useColor = useColor;
  _initialized = true;

  Serial.begin(MONITOR_SPEED);
  
  delay(100);

  log(XLOG_LEVEL_INFO, CAT_CONFIG,
      "\n\n\nLogger initialized (level=%d, categories=0x%04X)", (int)level,
      categories);
}

void Logger::setLevel(LogLevel level) {
  _currentLevel = level;
}

void Logger::setCategories(uint16_t categories) {
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

// ========== ФУНКЦИЯ log() - ИСПРАВЛЕННАЯ ==========
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

  char output[320];
  const char* color = _useColor ? getColorForLevel(level) : "";
  const char* reset = _useColor ? ANSI_RESET : "";

  if (_useColor) {
    snprintf(output, sizeof(output), "%s[%s] [%s] %s%s%s\n",
             ANSI_RESET,  // сброс перед строкой
             levelToString(level), categoryToString(category), color, buffer,
             reset);
  } else {
    snprintf(output, sizeof(output), "[%s] [%s] %s\n", levelToString(level),
             categoryToString(category), buffer);
  }
  Serial.print(output);
}

void Logger::log(LogLevel level, LogCategory category, const String& message) {
  if (!isEnabled(level, category))
    return;

  const char* color = _useColor ? getColorForLevel(level) : "";
  const char* reset = _useColor ? ANSI_RESET : "";

  Serial.printf("[%s] [%s] %s%s%s\n", levelToString(level),
                categoryToString(category), color, message.c_str(), reset);
}

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