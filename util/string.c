/*
 * helper functions - string conversion
 *
 * Stefan Biereigel
 *
 */

#include <stdio.h>
#include <inttypes.h>

void atoi32(char *string, uint8_t len, uint8_t *decimal_pos, uint32_t *i) {
	uint8_t j;
	uint32_t mult = 1;
	*i = 0;
	for (j = 0; j < len; j++) {
		if (*(string + len - j - 1) == '.') {
			*decimal_pos = j;
		} else {
			*i += mult * (*(string + len - j - 1) - 48);
			mult *= 10;
		}
	}
}
/* void atoi16(char *string, uint8_t len, uint8_t *decimal_pos, uint16_t *i) */
/* void atoi8(char *string, uint8_t len, uint8_t *decimal_pos, uint8_t *i) */

void i8toa(uint8_t i, uint8_t len, char *a); 
