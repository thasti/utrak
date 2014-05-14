/* 
 * NMEA-0183 related functionality
 *
 * Stefan Biereigel 
 *
 */

#include <inttypes.h>

#include "nmea.h"
#include "../util/string.h"

uint8_t NMEAsentenceIsGPGGA(char *sentence) {
	uint8_t i;
	const char pattern[] = "$GPGGA";

	/* match first 6 characters of NMEA sentence to pattern */
	for (i = 0; i < 6; i++) {
		if (*(sentence + i) != pattern[i]) {
			return 0;
		}
	}

	return 1;
	
}

uint8_t GPGGAGetData(char *sentence, char *lat, char *lon, char *alt) {
	uint8_t i, tmp;
	uint8_t field = 0;
	uint8_t len = 0;
	uint32_t alt_i = 0;

	/* one sentence is max. 83 characters */
	for (i = 0; i < 83 && (*(sentence + i) != '\n'); i++) {
		len++;

		/* process preceeding field if field separator is reached */
		if (*(sentence + i) == ',') {
			switch (field) {
				case LAT_FIELD:
					for (tmp = 0; tmp < 7; tmp++) {
						*(lat++) = *(sentence + i - len + 1 + tmp);
					}
					break;
				case NS_FIELD:
					*lat = *(sentence + i - 1);
					break;
				case LON_FIELD:
					for (tmp = 0; tmp < 8; tmp++) {
						*(lon++) = *(sentence + i - len + 1 + tmp);
					}
					break;
				case EW_FIELD:
					*lon = *(sentence + i - 1);
					break;
				case FIX_FIELD:
					if (*(sentence + i - 1) == '0') 
						return 0;
					break;
				case ALT_FIELD:
					atoi32(sentence + i - len + 1, len - 1, &alt_i);
					/* alt is < 16bit, so we can safely multiply * 100 */
					alt_i = alt_i * 328 / 100;
					/* 20000 m ~ 65600 ft */
					i32toa(alt_i, 6, alt);
					break;
				default:
					break;
			}
			field++;
			len = 0;
		}
	}
	return 1;
}
