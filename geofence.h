#ifndef GEOFENCE_H_
#define GEOFENCE_H_

#include <inttypes.h>

void geofence_aprs_frequency(char *lat, char *lon);
uint16_t geofence_slow_tlm_altitude(char *alt, int alt_len);

#endif
