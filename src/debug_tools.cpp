#include "debug_tools.h"
#include "logger.h"

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



#if LOG_LEVEL > 0

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

  LOG_INFO(CAT_ALL, "Chip: %s (revision v%d.%d)", chip_name,
           chip_info.revision / 100, chip_info.revision % 100);
  LOG_INFO(CAT_ALL, "Cores: %d, Frequency: %d MHz", chip_info.cores,
           getCpuFrequencyMhz());
  LOG_INFO(CAT_ALL, "Chip ID: %08X", (uint32_t)ESP.getEfuseMac());

  uint32_t flashSize = ESP.getFlashChipSize();
  LOG_INFO(CAT_ALL, "Flash: %u MB (%d MHz, mode %d)", flashSize / (1024 * 1024),
           ESP.getFlashChipSpeed() / 1000000, ESP.getFlashChipMode());

#ifdef CONFIG_SPIRAM_SUPPORT
  LOG_INFO(CAT_ALL, "PSRAM: %u bytes (free: %u)", ESP.getPsramSize(),
           ESP.getFreePsram());
#else
  LOG_INFO(CAT_ALL, "PSRAM: not supported");
#endif

  LOG_INFO(CAT_ALL, "Heap: %u bytes free (min: %u, max alloc: %u)",
           ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
  LOG_INFO(CAT_ALL, "ESP-IDF: %s", esp_get_idf_version());

#ifdef CONFIG_IDF_TARGET_ESP32C3
  LOG_INFO(CAT_ALL, "Architecture: RISC-V");
  LOG_INFO(CAT_ALL, "WiFi: %s, BLE: %s",
           chip_info.features & CHIP_FEATURE_WIFI_BGN ? "Yes" : "No",
           chip_info.features & CHIP_FEATURE_BLE ? "Yes" : "No");
#endif

#elif defined(ESP8266)
  LOG_INFO(CAT_ALL, "Platform: ESP8266");
  LOG_INFO(CAT_ALL, "Chip ID: %08X", ESP.getChipId());
  LOG_INFO(CAT_ALL, "Core: %s, CPU: %d MHz", ESP.getCoreVersion().c_str(),
           ESP.getCpuFreqMHz());
  LOG_INFO(CAT_ALL, "Free heap: %u bytes", ESP.getFreeHeap());

  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t realFlashSize = ESP.getFlashChipRealSize();
  LOG_INFO(CAT_ALL, "Flash: %u MB (%d MHz, mode %d)",
           realFlashSize / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000,
           ESP.getFlashChipMode());

  LOG_INFO(CAT_ALL, "SDK: %s", system_get_sdk_version());
#endif

  LOG_INFO(CAT_ALL, "==========================================");
  LOG_INFO(CAT_ALL, "Sketch size: %u bytes", ESP.getSketchSize());
  LOG_INFO(CAT_ALL, "Free sketch space: %u bytes", ESP.getFreeSketchSpace());
  LOG_INFO(CAT_ALL, "Free heap: %u bytes", ESP.getFreeHeap());
  LOG_INFO(CAT_ALL, "Firmware ver. %s", VERSION);
  LOG_INFO(CAT_ALL, "Reset reason: %s", getResetReason());
}


#endif