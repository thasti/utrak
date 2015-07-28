#ifndef __hw_h__
#define __hw_h__

#include <inttypes.h>

void hw_init(void);
void enable_xt1(void);
void disable_xt1(void);
void aprs_timer_enable(void);
void aprs_timer_disable(void);
uint16_t get_battery_voltage(void);
int16_t get_die_temperature(void);

#endif
