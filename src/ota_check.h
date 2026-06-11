
#ifndef OTA_CHECK_H
#define OTA_CHECK_H
#if OTA_ENABLED == 1
#include <Arduino.h>  

// Флаг для принудительного отключения OTA при отладке
#ifndef TEST_DISABLE_OTA
    #define TEST_DISABLE_OTA 0
#endif

/**
 * @brief Проверить, доступно ли обновление по воздуху (OTA)
 * 
 * OTA требует:
 * - ESP8266: Flash >= 2MB (иначе не хватит места)
 * - ESP32: Flash >= 2MB
 * 
 * @return true — OTA возможно, false — недоступно (мало Flash)
 */
static inline bool isOtaAvailable() {
    LOG_INFO(CAT_OTA, "Checking OTA capable...");
    
    #if TEST_DISABLE_OTA == 1
    LOG_INFO(CAT_OTA, "OTA disabled for testing");
    return false;
    #endif

    #ifdef ESP8266
    uint32_t flashSize = ESP.getFlashChipRealSize();
    uint32_t freeSketchSpace = ESP.getFreeSketchSpace();
    uint32_t currentSketchSize = ESP.getSketchSize();
    
    bool flashEnough = (flashSize >= (2 * 1024 * 1024));
    bool spaceEnough = (freeSketchSpace >= currentSketchSize);
        
    LOG_DEBUG(CAT_OTA, " Flash: %u MB, Free: %u KB, Sketch: %u KB, FlashOK: %d, SpaceOK: %s",
        flashSize / (1024 * 1024),
        freeSketchSpace / 1024,
        currentSketchSize / 1024,
        flashEnough ? "YES" : "NO",
        spaceEnough ? "YES" : "NO");

        
    return (flashEnough && spaceEnough);
        
    #elif defined(ESP32)
        uint32_t flashSize = ESP.getFlashChipSize();
        bool flashEnough = (flashSize >= (2 * 1024 * 1024));
        
        LOG_DEBUG(CAT_OTA, "Flash: %u MB, OK: %s",
                          flashSize / (1024 * 1024),
                          flashEnough ? "YES" : "NO");
        
        return flashEnough;
        
    #else
        #warning "Unknown platform - OTA disabled"
        return false;
    #endif
}
#endif // OTA_ENABLED == 1
#endif // OTA_CHECK_H