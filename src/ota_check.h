
#ifndef OTA_CHECK_H
#define OTA_CHECK_H
#if OTA_ENABLED == 1
#include <Arduino.h>  

// Флаг для принудительного отключения OTA при отладке
#ifndef DEBUG_OTA
    #define DEBUG_OTA 0
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
    #if DEBUG_OTA == 1
        #ifdef LOG_OTA
            Serial.println("[OTA] DEBUG_OTA=1 - OTA forcibly disabled");
        #endif
        return false;
    #endif

    #ifdef ESP8266
        uint32_t flashSize = ESP.getFlashChipRealSize();
        uint32_t freeSketchSpace = ESP.getFreeSketchSpace();
        uint32_t currentSketchSize = ESP.getSketchSize();
        
        bool flashEnough = (flashSize >= (2 * 1024 * 1024));
        bool spaceEnough = (freeSketchSpace >= currentSketchSize);
        
        #ifdef LOG_OTA
            Serial.printf("[OTA] Flash: %u MB, Free: %u KB, Sketch: %u KB, FlashOK: %d, SpaceOK: %d\n",
                          flashSize / (1024 * 1024),
                          freeSketchSpace / 1024,
                          currentSketchSize / 1024,
                          flashEnough ? 1 : 0,
                          spaceEnough ? 1 : 0);
        #endif
        
        return (flashEnough && spaceEnough);
        
    #elif defined(ESP32)
        uint32_t flashSize = ESP.getFlashChipSize();
        bool flashEnough = (flashSize >= (2 * 1024 * 1024));
        
        #ifdef LOG_OTA
            Serial.printf("[OTA] Flash: %u MB, OK: %d\n",
                          flashSize / (1024 * 1024),
                          flashEnough ? 1 : 0);
        #endif
        
        return flashEnough;
        
    #else
        #warning "Unknown platform - OTA disabled"
        return false;
    #endif
}
#endif // OTA_ENABLED == 1
#endif // OTA_CHECK_H