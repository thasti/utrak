/*
 * NMEA-0183 related functionality
 *
 * Stefan Biereigel
 *
 */

#include <inttypes.h>

#include "nmea.h"
#include "string.h"

/*
 * NMEA_sentence_is_GPGGA
 *
 * tests, whether a full NMEA sentence contains is a GGA sentence
 * uBlox GNSS modules use GN as standard talker ID instead of GP
 *
 * sentence:	pointer to the sentence under test
 *
 * returns: 	1 if GPGGA,
 * 		0 if not GPGGA
 */
uint8_t NMEA_sentence_is_GGA(volatile char *sentence) {
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

/*
 * GPGGA_has_fix
 *
 * determines whether a GPGGA-message includes valid fix data
 *
 * sentence:	pointer to the sentence under test
 *
 * returns:	1 if fix is OK
 * 		0 if fix is not OK
 */
uint8_t GPGGA_has_fix(volatile char *sentence) {
	uint8_t field = 0;
	uint8_t len = 0;
	uint8_t i;
	/* one sentence is max. 83 characters */
	for (i = 0; i < 83 && (*(sentence + i) != '\n'); i++) {
		len++;
		/* process preceeding field if field separator is reached */
		if (*(sentence + i) == ',') {
			switch (field) {
				case FIX_FIELD:
					if (*(sentence + i - 1) == '0')
						return 0;
					else
						return 1;
					break;
				default:
					break;
			}
			field++;
		}
	}
	return 0;
}

uint8_t GPGGA_get_data(	volatile char *sentence,
			volatile char *lat,
			volatile char *lon,
			volatile char *alt,
			uint8_t *alt_length,
			volatile char *sat,
			volatile char *time) {
	uint8_t i, tmp;
	uint8_t field = 0;
	uint8_t len = 0;
	uint16_t alt_i = 0;

	volatile char *lat_cpy = lat;
	volatile char *lon_cpy = lon;

	/* one sentence is max. 83 characters */
	for (i = 0; i < 83 && (*(sentence + i) != '\n'); i++) {
		len++;

		/* process preceeding field if field separator is reached */
		if (*(sentence + i) == ',') {
			switch (field) {
				case TIME_FIELD:
					for (tmp = 0; tmp < TIME_LENGTH; tmp++) {
						*(time++) = *(sentence + i - len + 1 + tmp);
					}
					break;
				case LAT_FIELD:
					*(lat++) = '+';
					for (tmp = 0; tmp < LAT_LENGTH; tmp++) {
						*(lat++) = *(sentence + i - len + 1 + tmp);
					}
					break;
				case NS_FIELD:
					if (*(sentence + i - 1) == 'S') {
						*lat_cpy = '-';		/* overwrite the "+" */
					}
					break;
				case LON_FIELD:
					*(lon++) = '+';
					for (tmp = 0; tmp < LON_LENGTH; tmp++) {
						*(lon++) = *(sentence + i - len + 1 + tmp);
					}
					break;
				case EW_FIELD:
					if (*(sentence + i - 1) == 'W') {
						*lon_cpy = '-';		/* overwrite the "+" */
					}
					break;
				case FIX_FIELD:
					if (*(sentence + i - 1) == '0')
						return 0;
					break;
				case SAT_FIELD:
					*(sat++) = *(sentence + i - len + 1);
					*(sat) = *(sentence + i - len + 2);
					break;
				case ALT_FIELD:
					atoi16(sentence + i - len + 1, len - 1, &alt_i);
					*alt_length = i16toav(alt_i, alt);
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
