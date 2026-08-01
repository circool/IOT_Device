/**
 * @file zigbee_manager.cpp
 * @brief 
 */

#include "zigbee_manager.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
ZigBeeManager zigbeeManager;
#else
ZigBeeManager zigbeeManager;
#endif