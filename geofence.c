#include "geofence.h"
#include "si4060.h"

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
void geofence_aprs_frequency(char *lat, char *lon) {
	/* 
	 * lat/lon [0] contain + or -
	 * lat [1,2] contain 00 .. 90
	 * lon [1,2,3] contains 000 .. 180
	 */

	if (lon[0] == '-' && ((lon[1] == '0' && lon[2] >= '4') || (lon[1] == '1'))) {
		si4060_freq_aprs_us();
	} else if (lon[0] == '+' && ((lon[1] == '0' && lon[2] >= '9') || (lon[1] == '1'))) {
		si4060_freq_aprs_cn();
	} else {
		/* also catches invalid fixes, where the first char does not match + or - or reads zeros*/
		si4060_freq_aprs_eu();
	}
}

/* 
 * geofence_slow_tlm_altitude
 *
 * if altitude is lower than 3km, RTTY shall be transmitted. this ensures that
 * recoveries are still possible, while conserving power in higher altitudes.
 * the RTTY telemetry needs less link margin (payload on the ground) and has
 * longer transmission times (allowing for direction finding).
 *
 * returns: 1 if slow telemetry should be sent
 *
 */
uint16_t geofence_slow_tlm_altitude(char *alt, int alt_len) {
	/* alt is left-aligned and alt_len characters long */
	if (alt_len <= 3) 	return 1;	/* lower than 1000m */
	if (alt_len >= 5) 	return 0;	/* at least 10000m */
	
	/* 
	 * altitude string is 4 characters long (implicit from checks above) 
	 * altitude is between 1000 and 9999m
	 */
	if (alt[0] >= '4') 	return 0;	/* at least 4000m */
	else 			return 1;	/* lower than 4000m */
}


