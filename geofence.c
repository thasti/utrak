#include "geofence.h"
#include "si4060.h"
#include "fix.h"

/* 
 * geofence_aprs_frequency
 *
 * sets the si4060 APRS frequency according to the current position. uses a very
 * simple approach currently, will be refined if needed.
 *
 * the coordinates for this approach were taken from Thomas Krahn, KT5TK/DL4MDW
 */
void geofence_aprs_frequency(struct gps_fix *fix) {
	int match = 0;

	if(COORD_UBX(-168.0f) < fix->lon && fix->lon < COORD_UBX(-34.0f)) {
		si4060_freq_aprs_reg2();
		match = 1;
	} 
	if(COORD_UBX(-34) <  fix->lon && fix->lon < COORD_UBX(71)) {
		si4060_freq_aprs_reg1();
		match = 1;
	} 
	if(COORD_UBX(-34.95f) < fix->lat && fix->lat < COORD_UBX(7.18f) && 
		  COORD_UBX(-73.13f) < fix->lon && fix->lon < COORD_UBX(-26.46f)) {
		si4060_freq_aprs_brazil();
		match = 1;
	}
	if(COORD_UBX(29.38f) < fix->lat && fix->lat < COORD_UBX(47.10f) && 
		  COORD_UBX(127.16f) < fix->lon && fix->lon < COORD_UBX(153.61f)) {
		si4060_freq_aprs_jp();
		match = 1;
	}
	if(COORD_UBX(19.06f) < fix->lat && fix->lat < COORD_UBX(53.74f) && 
		  COORD_UBX(72.05f) < fix->lon && fix->lon < COORD_UBX(127.16f)) {
		si4060_freq_aprs_cn();
		match = 1;
	}
	if(COORD_UBX(-0.30f) < fix->lat && fix->lat < COORD_UBX(20.42f) && 
		  COORD_UBX(93.06f) < fix->lon && fix->lon < COORD_UBX(105.15f)) {
		si4060_freq_aprs_thai();
		match = 1;
	}
	if(COORD_UBX(-54.54f) < fix->lat && fix->lat < COORD_UBX(-32.43f) &&
		  COORD_UBX(161.62f) < fix->lon && fix->lon < COORD_UBX(179.99f)) {
		si4060_freq_aprs_nz();
		match = 1;
	}
	if(COORD_UBX(-50.17f) < fix->lat && fix->lat < COORD_UBX(-8.66f) && 
		  COORD_UBX(105.80f) < fix->lon && fix->lon < COORD_UBX(161.62f)) {
		si4060_freq_aprs_aus();
		match = 1;
	}
	if (match == 0) {
		si4060_freq_aprs_reg1();
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


