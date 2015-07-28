#ifndef GEOFENCE_H_
#define GEOFENCE_H_

#include <inttypes.h>
#include "fix.h"

void geofence_aprs_frequency(struct gps_fix *fix);
uint16_t geofence_slow_tlm_altitude(struct gps_fix *fix);

#endif
