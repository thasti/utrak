#ifndef __GPS_H__
#define __GPS_H__

#include "fix.h"

uint8_t gps_set_gps_only(void);
uint8_t gps_disable_nmea_output(void);
uint8_t gps_set_airborne_model(void);
uint8_t gps_set_power_save(void);
uint8_t gps_power_save(int on);
uint8_t gps_save_settings(void);
void gps_get_fix(struct gps_fix *fix);
void gps_startup_delay(void);

#endif
