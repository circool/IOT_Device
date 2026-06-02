#if DEVICE_TYPE == 3

#ifndef SWITCH_H
#define SWITCH_H



extern bool switchOn;
extern unsigned long switchStartTime;
extern unsigned long delayTimer;
extern bool delayActive;

void switch_init();
void switch_update();
void switch_set(bool on, bool manual=false);
bool switch_getState();
void switch_checkMaxOnTime();
bool switch_delayTimer(bool start);

#endif
#endif