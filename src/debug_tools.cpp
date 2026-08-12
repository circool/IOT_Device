/**
 * @file debug_tools.cpp
 * @brief Различные процедуры для отладки
 * @version 0.12
 * @date 10.08.2026
 */

#include "debug_tools.h"
#include "common_types.h"
#include "config_manager.h"
#include "logger.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <user_interface.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <esp_chip_info.h>
#include <esp_system.h>
#endif

// ============ RTC-память для сохранения контекста ============

#ifdef ESP32
static RTC_NOINIT_ATTR PersistentResetInfo s_resetInfo;
#else
static PersistentResetInfo s_resetInfo;
#endif

#define RESET_INFO_MAGIC 0xDEADBEEF

// ============ Реализация ============

float getChipTemperature() {
#ifdef ESP32
  return temperatureRead();
#else
  return -273.15f;
#endif
}

ResetReason getResetReasonEnum() {
#ifdef ESP8266
  struct rst_info* resetInfo = system_get_rst_info();
  uint8_t reason = resetInfo->reason;
  switch (reason) {
    case REASON_DEFAULT_RST:
      return RESET_REASON_POWER_ON;
    case REASON_WDT_RST:
      return RESET_REASON_WATCHDOG;
    case REASON_EXCEPTION_RST:
      return RESET_REASON_EXCEPTION;
    case REASON_SOFT_WDT_RST:
      return RESET_REASON_WATCHDOG;
    case REASON_SOFT_RESTART:
      return RESET_REASON_SOFT_RESET;
    case REASON_DEEP_SLEEP_AWAKE:
      return RESET_REASON_DEEP_SLEEP_AWAKE;
    case REASON_EXT_SYS_RST:
      return RESET_REASON_EXT_SYS_RST;
    default:
      return RESET_REASON_NONE;
  }
#elif defined(ESP32)
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON:
      return RESET_REASON_POWER_ON;
    case ESP_RST_EXT:
      return RESET_REASON_EXT_SYS_RST;
    case ESP_RST_SW:
      return RESET_REASON_SOFT_RESET;
    case ESP_RST_PANIC:
      return RESET_REASON_EXCEPTION;
    case ESP_RST_INT_WDT:
      return RESET_REASON_WATCHDOG;
    case ESP_RST_TASK_WDT:
      return RESET_REASON_SW_CPU_RESET;
    case ESP_RST_WDT:
      return RESET_REASON_WATCHDOG;
    case ESP_RST_DEEPSLEEP:
      return RESET_REASON_DEEP_SLEEP_AWAKE;
    case ESP_RST_BROWNOUT:
      return RESET_REASON_BROWNOUT;
    case ESP_RST_SDIO:
      return RESET_REASON_EXT_SYS_RST;
#ifdef ESP_RST_USB
    case ESP_RST_USB:
      return RESET_REASON_EXT_SYS_RST;
#endif
#ifdef ESP_RST_EFUSE
    case ESP_RST_EFUSE:
      return RESET_REASON_EXT_SYS_RST;
#endif
    default:
      return RESET_REASON_NONE;
  }
#else
  return RESET_REASON_NONE;
#endif
}

const char* getResetReason() {
  ResetReason reason = getResetReasonEnum();
  switch (reason) {
    case RESET_REASON_NONE:
      return "NONE";
    case RESET_REASON_POWER_ON:
      return "POWER_ON";
    case RESET_REASON_WATCHDOG:
      return "WATCHDOG";
    case RESET_REASON_EXCEPTION:
      return "EXCEPTION";
    case RESET_REASON_SOFT_RESET:
      return "SOFT_RESET";
    case RESET_REASON_FACTORY_RESET:
      return "FACTORY_RESET";
    case RESET_REASON_BROWNOUT:
      return "BROWNOUT";
    case RESET_REASON_SW_CPU_RESET:
      return "SW_CPU_RESET";
    case RESET_REASON_DEEP_SLEEP_AWAKE:
      return "DEEP_SLEEP_AWAKE";
    case RESET_REASON_EXT_SYS_RST:
      return "EXT_SYS_RST";
    default:
      return "UNKNOWN";
  }
}

uint32_t getUptimeSeconds() {
  return millis() / 1000;
}

bool isRtcAvailable() {
#ifdef ESP32
  return true;
#else
  return false;
#endif
}

bool isRtcPersistent() {
  return (s_resetInfo.magic == RESET_INFO_MAGIC);
}

void saveResetContext(ResetReason reason, uint32_t uptime) {
  s_resetInfo.reason = reason;
  s_resetInfo.uptimeSeconds = uptime;
  s_resetInfo.magic = RESET_INFO_MAGIC;
}

bool readResetContext(PersistentResetInfo* info) {
  if (s_resetInfo.magic != RESET_INFO_MAGIC) {
    return false;
  }
  if (info) {
    *info = s_resetInfo;
  }
  return true;
}

void clearResetContext() {
  s_resetInfo.magic = 0;
  s_resetInfo.reason = RESET_REASON_NONE;
  s_resetInfo.uptimeSeconds = 0;
}

// ============ printSystemInfo() ============

void printSystemInfo() {
#ifdef ESP32
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  const char* chip_name = "Unknown";

  switch (chip_info.model) {
    case CHIP_ESP32:
      chip_name = "ESP32";
      break;
    case CHIP_ESP32S2:
      chip_name = "ESP32-S2";
      break;
    case CHIP_ESP32S3:
      chip_name = "ESP32-S3";
      break;
    case CHIP_ESP32C3:
      chip_name = "ESP32-C3";
      break;
#ifdef CHIP_ESP32C6
    case CHIP_ESP32C6:
      chip_name = "ESP32-C6";
      break;
#endif
#ifdef CHIP_ESP32H2
    case CHIP_ESP32H2:
      chip_name = "ESP32-H2";
      break;
#endif
    default:
#if defined(CONFIG_IDF_TARGET_ESP32C6)
      chip_name = "ESP32-C6";
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
      chip_name = "ESP32-H2";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
      chip_name = "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
      chip_name = "ESP32-S3";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
      chip_name = "ESP32-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32)
      chip_name = "ESP32";
#elif defined(ESP32C6_DEV) || defined(ARDUINO_ESP32C6_DEV)
      chip_name = "ESP32-C6";
#elif defined(ESP32H2_DEV) || defined(ARDUINO_ESP32H2_DEV)
      chip_name = "ESP32-H2";
#endif
      break;
  }

  XLOG_INFO(CAT_ALL, "Chip: %s (revision v%d.%d)", chip_name,
            chip_info.revision / 100, chip_info.revision % 100);
  XLOG_INFO(CAT_ALL, "Cores: %d, Frequency: %d MHz", chip_info.cores,
            getCpuFrequencyMhz());

  uint64_t mac = ESP.getEfuseMac();
  XLOG_INFO(CAT_ALL, "Chip ID: %08llX", mac);

  uint32_t flashSize = ESP.getFlashChipSize();
  XLOG_INFO(CAT_ALL, "Flash: %u MB (%d MHz, mode %d)",
            flashSize / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000,
            ESP.getFlashChipMode());

#ifdef CONFIG_SPIRAM_SUPPORT
  XLOG_INFO(CAT_ALL, "PSRAM: %u bytes (free: %u)", ESP.getPsramSize(),
            ESP.getFreePsram());
#else
  XLOG_INFO(CAT_ALL, "PSRAM: not supported");
#endif

  XLOG_INFO(CAT_ALL, "Heap: %u bytes free (min: %u, max alloc: %u)",
            ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
  XLOG_INFO(CAT_ALL, "ESP-IDF: %s", esp_get_idf_version());

  const char* arch = "Unknown";
#if defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C3)
  arch = "RISC-V";
#else
  arch = "Xtensa";
#endif
  XLOG_INFO(CAT_ALL, "Architecture: %s", arch);

  bool hasWifi = (chip_info.features & CHIP_FEATURE_WIFI_BGN) != 0;
  bool hasBle = (chip_info.features & CHIP_FEATURE_BLE) != 0;

#ifdef CHIP_FEATURE_IEEE802154
  bool has802154 = (chip_info.features & CHIP_FEATURE_IEEE802154) != 0;
#else
  bool has802154 = false;
#endif

#ifdef CHIP_FEATURE_BT
  bool hasBt = (chip_info.features & CHIP_FEATURE_BT) != 0;
#else
  bool hasBt = false;
#endif

  String protocols = "";
  protocols += hasWifi ? "WiFi" : "";
  if (hasBle) {
    if (protocols.length() > 0)
      protocols += ", ";
    protocols += "BLE";
  }
  if (has802154) {
    if (protocols.length() > 0)
      protocols += ", ";
    protocols += "IEEE 802.15.4";
  }
  if (hasBt) {
    if (protocols.length() > 0)
      protocols += ", ";
    protocols += "BT Classic";
  }

  XLOG_INFO(CAT_ALL, "Protocols: %s", protocols.c_str());
  XLOG_INFO(CAT_ALL, "  WiFi: " ANSI_BOLD "%s",
            hasWifi ? ANSI_GREEN "Yes" ANSI_RESET : ANSI_RED "No" ANSI_RESET);
  XLOG_INFO(CAT_ALL, "  BLE: " ANSI_BOLD "%s",
            hasBle ? ANSI_GREEN "Yes" ANSI_RESET : ANSI_RED "No" ANSI_RESET);
  XLOG_INFO(CAT_ALL, "  IEEE 802.15.4: " ANSI_BOLD "%s",
            has802154 ? ANSI_GREEN "Yes" ANSI_RESET : ANSI_RED "No" ANSI_RESET);
  if (hasBt) {
    XLOG_INFO(CAT_ALL, "  BT Classic: " ANSI_GREEN "Yes" ANSI_RESET);
  }

  float temp = getChipTemperature();
  if (temp > -50.0f && temp < 150.0f) {
    XLOG_INFO(CAT_ALL, "Chip temperature: %.1f °C / %.1f °F", temp,
              (temp * 9.0f / 5.0f) + 32.0f);
  } else {
    XLOG_INFO(CAT_ALL, "Chip temperature: Not available");
  }

#elif defined(ESP8266)
  XLOG_INFO(CAT_ALL, "Platform: ESP8266");
  XLOG_INFO(CAT_ALL, "Chip ID: %08X", ESP.getChipId());
  XLOG_INFO(CAT_ALL, "Core: %s, CPU: %d MHz", ESP.getCoreVersion().c_str(),
            ESP.getCpuFreqMHz());
  XLOG_INFO(CAT_ALL, "Free heap: %u bytes", ESP.getFreeHeap());

  uint32_t flashSize = ESP.getFlashChipRealSize();
  XLOG_INFO(CAT_ALL, "Flash: %u MB (%d MHz, mode %d)",
            flashSize / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000,
            ESP.getFlashChipMode());

  XLOG_INFO(CAT_ALL, "SDK: %s", system_get_sdk_version());
  XLOG_INFO(CAT_ALL, "Chip temperature: Not available (ESP8266)");
#endif

  XLOG_INFO(CAT_ALL, "==========================================");
  XLOG_INFO(CAT_ALL, "Sketch size: %u bytes", ESP.getSketchSize());
  XLOG_INFO(CAT_ALL, "Free sketch space: %u bytes", ESP.getFreeSketchSpace());
  XLOG_INFO(CAT_ALL, "Free heap: %u bytes", ESP.getFreeHeap());
  XLOG_INFO(CAT_ALL, "Flash chip size: %u bytes",
#ifdef ESP32
            ESP.getFlashChipSize()
#else
            flashSize
#endif
  );
  XLOG_INFO(CAT_ALL, "Firmware ver. %s", VERSION);
  XLOG_INFO(CAT_ALL, "Reset reason: %s", getResetReason());

#ifdef ESP32
  XLOG_INFO(CAT_ALL, "RTC memory: " ANSI_GREEN "Available" ANSI_RESET);
  if (isRtcPersistent()) {
    XLOG_INFO(CAT_ALL, "  - Status: " ANSI_GREEN "Data stored" ANSI_RESET);
  } else {
    ResetReason reason = getResetReasonEnum();
    const char* note = "first boot or no data";
    if (reason == RESET_REASON_EXT_SYS_RST) {
      note = "hardware reset (RESET button)";
    } else if (reason == RESET_REASON_POWER_ON) {
      note = "power-on reset (data lost)";
    }
    XLOG_INFO(CAT_ALL, "  - Status: " ANSI_YELLOW "No data (%s)" ANSI_RESET,
              note);
  }
#else
  XLOG_INFO(CAT_ALL, "RTC memory: " ANSI_RED "Not supported" ANSI_RESET);
#endif

  PersistentResetInfo savedInfo;
  if (readResetContext(&savedInfo)) {
    const char* reasonStr = "UNKNOWN";
    switch (savedInfo.reason) {
      case RESET_REASON_FACTORY_RESET:
        reasonStr = "FACTORY_RESET";
        break;
      case RESET_REASON_SOFT_RESET:
        reasonStr = "SOFT_RESET";
        break;
      case RESET_REASON_WATCHDOG:
        reasonStr = "WATCHDOG";
        break;
      case RESET_REASON_EXCEPTION:
        reasonStr = "EXCEPTION";
        break;
      case RESET_REASON_BROWNOUT:
        reasonStr = "BROWNOUT";
        break;
      default:
        break;
    }

    XLOG_INFO(CAT_ALL, "Previous reset context:");
    XLOG_INFO(CAT_ALL, "  - Reason: %s", reasonStr);
    XLOG_INFO(CAT_ALL, "  - Uptime: %lu seconds (%.2f hours)",
              savedInfo.uptimeSeconds, savedInfo.uptimeSeconds / 3600.0f);

    if (savedInfo.reason == RESET_REASON_FACTORY_RESET) {
      XLOG_INFO(CAT_ALL,
                ANSI_YELLOW "  *** FACTORY RESET COMPLETED ***" ANSI_RESET);
    }

    clearResetContext();
  } else {
    XLOG_INFO(CAT_ALL, "Previous reset context: " ANSI_BOLD "none" ANSI_RESET);
  }
}

// ============ printConfig() ============

void printConfig(const TransportConfig& transport, const DeviceConfig& device) {
  XLOG_INFO(CAT_CONFIG, "========== CONFIG DUMP ==========");

  XLOG_INFO(CAT_CONFIG, ANSI_BOLD "TRANSPORT CONFIG" ANSI_RESET);
  XLOG_INFO(CAT_CONFIG, "  Device ID: '%s'", transport.deviceId);

#ifdef USE_WIFI
  XLOG_INFO(CAT_CONFIG, "  WiFi SSID: '%s'", transport.wifiSsid);
  XLOG_INFO(CAT_CONFIG, "  WiFi Password: %d chars",
            strnlen(transport.wifiPassword, sizeof(transport.wifiPassword)));
#endif

#ifdef USE_MQTT
  XLOG_INFO(CAT_CONFIG, "  MQTT Broker: '%s'", transport.mqttBroker);
  XLOG_INFO(CAT_CONFIG, "  MQTT Port: %d", transport.mqttPort);
  XLOG_INFO(CAT_CONFIG, "  MQTT Client ID: '%s'", transport.mqttClientId);
#endif

#ifdef USE_ZIGBEE
  XLOG_INFO(CAT_CONFIG, "  ZigBee PAN ID: 0x%04X", transport.zigbeePanId);
  XLOG_INFO(CAT_CONFIG, "  ZigBee Channel: %d", transport.zigbeeChannel);
#endif

  XLOG_INFO(CAT_CONFIG, ANSI_BOLD "DEVICE CONFIG" ANSI_RESET);

#if DEVICE_TYPE == 1
  XLOG_INFO(CAT_CONFIG, "  Sensor Control Mode: %s",
            device.sensorMode ? "ON" : "OFF");
  XLOG_INFO(CAT_CONFIG, "  Adaptive Mode: %s",
            device.adaptiveMode ? "ON" : "OFF");
  XLOG_INFO(CAT_CONFIG, "  Low Temp: %.1f°C", device.lowTemp);
  XLOG_INFO(CAT_CONFIG, "  High Temp: %.1f°C", device.highTemp);
  XLOG_INFO(CAT_CONFIG, "  Low Hum: %.1f%%", device.lowHum);
  XLOG_INFO(CAT_CONFIG, "  High Hum: %.1f%%", device.highHum);
  XLOG_INFO(CAT_CONFIG, "  Speed Percent: %d%%", device.speedPercent);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  XLOG_INFO(CAT_CONFIG, "  Delay Seconds: %lu", device.delaySeconds);
  XLOG_INFO(CAT_CONFIG, "  Max On Time: %lu", device.maxOnTime);
  XLOG_INFO(CAT_CONFIG, "  Boot State: %s", device.bootState ? "ON" : "OFF");
#endif

  XLOG_INFO(CAT_CONFIG, "==================================");
}

// ============ generateRandomDeviceId() ============

void generateRandomDeviceId(char* buffer, size_t size) {
  if (buffer == nullptr || size < 16) {
    if (buffer && size > 0) {
      strncpy(buffer, "device", size - 1);
      buffer[size - 1] = '\0';
    }
    return;
  }

  static const char* adjectives[] = {
      "brave", "calm",   "eager", "fierce", "gentle",  "happy", "jolly",
      "kind",  "lively", "noble", "proud",  "quick",   "rapid", "swift",
      "wise",  "bold",   "cool",  "daring", "elegant", "fancy"};

  static const char* nouns[] = {"panda", "tiger", "eagle", "dolphin", "falcon",
                                "raven", "wolf",  "lynx",  "fox",     "bear",
                                "owl",   "hawk",  "lion",  "deer",    "snake",
                                "whale", "shark", "mouse", "rabbit",  "otter"};

#ifdef ESP8266
  randomSeed(analogRead(A0) + micros());
  uint8_t adjIdx = random(0, sizeof(adjectives) / sizeof(adjectives[0]));
  uint8_t nounIdx = random(0, sizeof(nouns) / sizeof(nouns[0]));
  uint16_t suffix = random(1000, 9999);
#elif defined(ESP32)
  uint32_t randVal = esp_random();
  uint8_t adjIdx = randVal % (sizeof(adjectives) / sizeof(adjectives[0]));
  uint8_t nounIdx = (randVal >> 8) % (sizeof(nouns) / sizeof(nouns[0]));
  uint16_t suffix = (randVal >> 16) % 9000 + 1000;
#else
  uint8_t adjIdx = 0;
  uint8_t nounIdx = 0;
  uint16_t suffix = 0;
#endif

  snprintf(buffer, size, "%s-%s-%04d", adjectives[adjIdx], nouns[nounIdx],
           suffix);
}