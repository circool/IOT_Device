#include "zigbee.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
ZigBeeManager zigbeeManager;
#else
ZigBeeManager zigbeeManager;
#endif