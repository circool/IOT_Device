#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#ifndef MONITOR_SPEED
#define MONITOR_SPEED 115200
#endif

/**
 * @brief Уровни логирования
 */
enum LogLevel : uint8_t {
  LOG_LEVEL_NONE = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4
};

/**
 * @brief Категории (тэги) для фильтрации
 */
enum LogCategory : uint16_t {
  CAT_NONE = 0,
  CAT_CONFIG = 1 << 0,    // 1
  CAT_SENSOR = 1 << 1,    // 2
  CAT_FAN = 1 << 2,       // 4
  CAT_SWITCH = 1 << 3,    // 8
  CAT_ACTUATOR = 1 << 4,  // 16
  CAT_MQTT = 1 << 5,      // 32
  CAT_WIFI = 1 << 6,      // 64
  CAT_WEB = 1 << 7,       // 128
  CAT_OTA = 1 << 8,       // 256
  CAT_LED = 1 << 9,       // 512
  CAT_WDT = 1 << 10,      // 1024
  CAT_AP = 1 << 11,       // 2048
  CAT_MAIN = 1 << 12,     // 4096
  CAT_ALL = 0xFFFF
};

/**
 * @brief ANSI-коды для цветного вывода
 *
 * Для монохромных терминалов (PlatformIO, Arduino IDE) игнорируются
 */
// Цвета текста
#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"

// Яркие цвета
#define ANSI_BRIGHT_RED "\033[91m"
#define ANSI_BRIGHT_GREEN "\033[92m"
#define ANSI_BRIGHT_YELLOW "\033[93m"
#define ANSI_BRIGHT_BLUE "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN "\033[96m"
#define ANSI_BRIGHT_WHITE "\033[97m"

// Фоны
#define ANSI_BG_BLACK "\033[40m"
#define ANSI_BG_RED "\033[41m"
#define ANSI_BG_GREEN "\033[42m"
#define ANSI_BG_YELLOW "\033[43m"
#define ANSI_BG_BLUE "\033[44m"
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_ITALIC "\033[3m"
#define ANSI_UNDERLINE "\033[4m"

// Сброс
#define ANSI_RESET "\033[0m"

/**
 * @brief Единый логгер для всего проекта
 *
 * Особенности:
 * - Синглтон (один экземпляр)
 * - Поддержка уровней логирования
 * - Поддержка категорий (фильтрация по модулям)
 * - Возможность подключить несколько выводов (Serial, MQTT, файл)
 */
class Logger {
 public:
  /**
   * @brief Получить экземпляр логгера
   */
  static Logger& getInstance();

  /**
   * @brief Инициализация (вызывается один раз в setup)
   * @param level Максимальный уровень для вывода
   * @param categories Битовая маска разрешённых категорий
   * @param useColor Использовать ANSI-цвета (если терминал поддерживает)
   */
  void begin(LogLevel level = LOG_LEVEL_INFO,
             uint16_t categories = CAT_ALL,
             bool useColor = true);

  /**
   * @brief Установить уровень логирования
   */
  void setLevel(LogLevel level);

  /**
   * @brief Установить разрешённые категории
   */
  void setCategories(uint16_t categories);

  /**
   * @brief Включить/выключить цвета
   */
  void setColorEnabled(bool enabled);

  /**
   * @brief Основной метод логирования
   * @param level Уровень сообщения
   * @param category Категория (тэг)
   * @param format Форматная строка (printf-style)
   * @param ... Аргументы
   */
  void log(LogLevel level, LogCategory category, const char* format, ...);

  /**
   * @brief Логирование без формата (готовая строка)
   */
  void log(LogLevel level, LogCategory category, const String& message);

  /**
   * @brief Проверить, нужно ли логировать данный уровень/категорию
   */
  bool isEnabled(LogLevel level, LogCategory category) const;

 private:
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void output(const char* message);
  const char* levelToString(LogLevel level) const;
  const char* categoryToString(LogCategory category) const;
  const char* getColorForLevel(LogLevel level) const;

  LogLevel _currentLevel = LOG_LEVEL_INFO;
  uint16_t _enabledCategories = CAT_ALL;
  bool _useColor = true;
  bool _initialized = false;
};

// ============================================================================
// УДОБНЫЕ МАКРОСЫ
// ============================================================================

#define LOG_ERROR(cat, fmt, ...) \
  Logger::getInstance().log(LOG_LEVEL_ERROR, cat, fmt, ##__VA_ARGS__)
#define LOG_WARN(cat, fmt, ...) \
  Logger::getInstance().log(LOG_LEVEL_WARN, cat, fmt, ##__VA_ARGS__)
#define LOG_INFO(cat, fmt, ...) \
  Logger::getInstance().log(LOG_LEVEL_INFO, cat, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(cat, fmt, ...) \
  Logger::getInstance().log(LOG_LEVEL_DEBUG, cat, fmt, ##__VA_ARGS__)

// Проверка, нужно ли логировать (для дорогих операций)
#define LOG_ENABLED(level, cat) Logger::getInstance().isEnabled(level, cat)

#endif  // LOGGER_H