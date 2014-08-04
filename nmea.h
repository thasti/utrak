#ifndef NMEA_H_
#define NMEA_H_

#include <inttypes.h>

uint8_t NMEA_sentence_is_GPGGA(char *sentence);
uint8_t GPGGA_has_fix(char *sentence);
uint8_t GPGGA_get_data(char *sentence, char *lat, char *lon, char *alt, char *sat);

/*
 * GPGGA field numbers
 */
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
#define LAT_LENGTH	7
#define LON_LENGTH	8

#endif
