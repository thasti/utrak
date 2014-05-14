/*
 * helper functions - string conversion
 *
 * Stefan Biereigel
 *
 */

#include <stdio.h>
#include <inttypes.h>

void atoid32(char *string, uint8_t len, uint32_t *integer, uint32_t *decimal) {
	uint8_t j;
	uint32_t mult = 1;
	uint8_t int_port = 0;

	*integer = 0;
	*decimal = 0;
	for (j = 0; j < len; j++) {
		if (*(string + len - j - 1) == '.') {
			mult = 1;
			int_port = 1;
		} else {
			if (int_port) {
				*integer += mult * (*(string + len - j - 1) - 48);
			} else {
				*decimal += mult * (*(string + len - j - 1) - 48);
			}
			mult *= 10;
		}
	}
}

void atod32(char *string, uint8_t len, uint32_t *decimal) {
	uint8_t j;
	uint32_t mult = 1;

	*decimal = 0;
	for (j = 0; j < len; j++) {
		if (*(string + len - j - 1) == '.') {
			return;
		} else {
			*decimal += mult * (*(string + len - j - 1) - 48);
			mult *= 10;
		}
	}
}

void atoi32(char *string, uint8_t len, uint32_t *integer) {
	uint8_t j;
	uint32_t mult = 1;
	uint8_t start = 0;

	*integer = 0;
	for (j = 0; j < len; j++) {
		if (*(string + len - j - 1) == '.') {
			start = 1;
		} else {
			if (start) {
				*integer += mult * (*(string + len - j - 1) - 48);
				mult *= 10;
			}
		}
	}
}

void i32toa(uint32_t in, uint8_t len, char *out) {
	uint8_t i;
	uint32_t mult = 1;
	for (i = len; i > 0; i--) {
		*(out + i - 1) = ((in % (mult*10)) / mult) + 48;
		mult *= 10;
	}
}

