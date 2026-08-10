/**
 * @file state_provider.h
 * @brief Единый источник статусной информации о состоянии устройства
 * @version 0.11
 * @date 10.08.2026
 */

#ifndef STATE_PROVIDER_H
#define STATE_PROVIDER_H

#include <cstdint>
#include "common_types.h"

/**
 * @class StateProvider
 * @brief Глобальная шина статусной информации
 * 
 * StateProvider обеспечивает обмен статусной информацией между слоями
 * без прямых связей. Хранит статусы транспорта и события кнопки.
 * 
 * @note DeviceController НЕ ИМЕЕТ ДОСТУПА к StateProvider
 * @note Оркестратор НЕ ПИШЕТ в StateProvider, только читает
 */
class StateProvider {
public:
    /**
     * @brief Инициализация провайдера состояния
     * @return true при успешной инициализации
     */
    bool init();
    
    // ===== СТАТУСЫ ТРАНСПОРТА =====
    bool link_ok;        ///< Соединение с точкой доступа (WiFi/Zigbee)
    bool gateway_ok;     ///< Соединение с брокером/шлюзом (MQTT/HTTP)
    bool setup_mode;     ///< Режим настройки (AP режим)

    // ===== СОБЫТИЯ КНОПКИ =====
    ButtonStage button_stage;  ///< Стадия нажатия
};

/**
 * @brief Глобальный экземпляр StateProvider
 * @note Объявлен в main.cpp, доступен для всех слоёв
 */
extern StateProvider g_stateProvider;

#endif // STATE_PROVIDER_H