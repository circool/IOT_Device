#ifndef DEBUG_TOOLS_H
#define DEBUG_TOOLS_H

const char* getResetReason();


#if LOG_LEVEL > 0
void print_system_info();
#else
inline void print_system_info() {};

#endif  // LOG_LEVEL > 0
#endif  // DEBUG_TOOLS_H