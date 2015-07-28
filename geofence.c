#include "geofence.h"
#include "si4060.h"
#include "fix.h"

/* 
 * geofence_aprs_frequency
 *
 * sets the si4060 APRS frequency according to the current position. uses the 
 * most simple approach currently, will be refined if needed.
 *
 * three zones are relevant, only longitude is looked at
 *   USA from -180 to -40 deg
 *   China from +90 to +180
 *   Europe from -40 deg to +90 (or: everywhere else, fallback)
 *
 */
void geofence_aprs_frequency(struct gps_fix *fix) {
	if (fix->lon < COORD_UBX(-40.0)) {
		si4060_freq_aprs_us();
	} else if (fix->lon > COORD_UBX(+90.0)) {
		si4060_freq_aprs_cn();
	} else {
		si4060_freq_aprs_eu();
	}
}

/* 
 * geofence_slow_tlm_altitude
 *
 * if altitude is lower than 4km, RTTY shall be transmitted. this ensures that
 * recoveries are still possible, while conserving power in higher altitudes.
 * the RTTY telemetry needs less link margin (payload on the ground) and has
 * longer transmission times (allowing for direction finding).
 *
 * returns: 1 if slow telemetry should be sent
 *
 */
uint16_t geofence_slow_tlm_altitude(struct gps_fix *fix) {
	if (fix->alt > 4000) 	return 0;
	else			return 1;
}


