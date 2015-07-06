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
