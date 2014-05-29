#ifndef NMEA_H_
#define NMEA_H_

#include <inttypes.h>

uint8_t NMEAsentenceIsGPGGA(char *sentence);
uint8_t GPGGAGetData(char *sentence, char *lat, char *lon, char *alt);

#define LAT_FIELD	2
#define NS_FIELD	3
#define LON_FIELD	4
#define EW_FIELD	5
#define FIX_FIELD	6
#define ALT_FIELD	9

#endif
