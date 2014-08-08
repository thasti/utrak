#ifndef NMEA_H_
#define NMEA_H_

#include <inttypes.h>

uint8_t NMEA_sentence_is_GPGGA(volatile char *sentence);
uint8_t GPGGA_has_fix(volatile char *sentence);
uint8_t GPGGA_get_data(	volatile char *sentence,
			volatile char *lat,
			volatile char *lon,
			volatile char *alt,
			volatile char *sat,
			volatile char *time);

/*
 * GPGGA field numbers
 */
#define TIME_FIELD	1
#define LAT_FIELD	2
#define NS_FIELD	3
#define LON_FIELD	4
#define EW_FIELD	5
#define FIX_FIELD	6
#define SAT_FIELD	7
#define ALT_FIELD	9

/*
 * GPGGA field lengths (where fixed)
 */
#define LAT_LENGTH	9
#define LON_LENGTH	10
#define TIME_LENGTH	6
#define SAT_LENGTH	2
#define ALT_LENGTH	5

#endif
