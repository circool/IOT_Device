#include "debug_tools.h"
#include "logger.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <user_interface.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <esp_chip_info.h>
#endif

float getChipTemperature() {
#ifdef ESP32
  // Простая функция из Arduino Core для ESP32-C3
  return temperatureRead();
#else
  return -273.15f;
#endif
}

const char* getResetReason() {
#ifdef ESP8266
  struct rst_info* resetInfo = system_get_rst_info();
  uint8_t reason = resetInfo->reason;
  switch (reason) {
    case REASON_DEFAULT_RST:
      return "POWER_ON";
    case REASON_WDT_RST:
      return "WATCHDOG_CRASH";
    case REASON_EXCEPTION_RST:
      return "EXCEPTION_CRASH";
    case REASON_SOFT_WDT_RST:
      return "SOFT_WDT_CRASH";
    case REASON_SOFT_RESTART:
      return "SOFT_RESTART";
    case REASON_DEEP_SLEEP_AWAKE:
      return "DEEP_SLEEP_WAKE";
    case REASON_EXT_SYS_RST:
      return "EXT_RESET";
    default:
      return "UNKNOWN";
  }
#elif defined(ESP32)
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON:
      return "POWER_ON";
    case ESP_RST_EXT:
      return "EXT_RESET";
    case ESP_RST_SW:
      return "SOFT_RESTART";
    case ESP_RST_PANIC:
      return "PANIC_CRASH";
    case ESP_RST_INT_WDT:
      return "INT_WDT_CRASH";
    case ESP_RST_TASK_WDT:
      return "TASK_WDT_CRASH";
    case ESP_RST_WDT:
      return "WDT_CRASH";
    case ESP_RST_DEEPSLEEP:
      return "DEEP_SLEEP_WAKE";
    default:
      return "UNKNOWN";
  }
#else
  return "UNKNOWN_PLATFORM";
#endif
}

void print_system_info() {
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
      chip_name = "Unknown";
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
  XLOG_INFO(CAT_ALL, "  WiFi: %s", hasWifi ? "Yes" : "No");
  XLOG_INFO(CAT_ALL, "  BLE: %s", hasBle ? "Yes" : "No");
  XLOG_INFO(CAT_ALL, "  IEEE 802.15.4: %s", has802154 ? "Yes" : "No");
  if (hasBt) {
    XLOG_INFO(CAT_ALL, "  BT Classic: Yes");
  }

  float temp = getChipTemperature();
  if (temp > -50.0f && temp < 150.0f) {
    XLOG_INFO(CAT_ALL, "Chip temperature: %.1f °C / %.1f °F", temp,
              (temp * 9.0 / 5.0) + 32.0);
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
  XLOG_INFO(CAT_ALL, "Flash chip size: %u bytes", flashSize);
  XLOG_INFO(CAT_ALL, "Firmware ver. %s", VERSION);
  XLOG_INFO(CAT_ALL, "Reset reason: %s", getResetReason());
}