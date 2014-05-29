/* 
 * string util unit test
 *
 * Stefan Biereigel
 */

#ifdef TEST
#include <stdio.h>
#include <inttypes.h>
#include "string.h"
#include "crc.h"

int main(void) {
	char test[] = "19123.45";
	char number[7] = { 0 };

	int len = sizeof(test) - 1;
	uint32_t result;
	uint8_t decimal_pos;

	uint32_t integer;
	uint32_t decimal;

	printf("** String to integer/decimal tests\n");
	printf("Test String: %s\n", test);
	printf("Length: %d\n", len);
	
	atoid32(test, len, &integer, &decimal);
	printf("Integer Portion: %zu\n", integer);
	printf("Decimal Portion: %zu\n", decimal);
	
	atod32(test, len, &decimal);
	printf("Decimal Portion: %zu\n", decimal);
	
	atoi32(test, len, &integer);
	printf("Integer Portion: %zu\n", integer);
	
	printf("\n** Integer to String tests\n");
	integer = 300;

	i32toa(integer, 6, number);
	printf("%d as String: %s\n", integer, number);

	printf("\n** Integer to Hex tests\n");
	char tohex[4] = "0000";
	uint16_t hex = 0x20af;
	uint8_t i;
	i16tox(hex, tohex);
	printf("%x to hex: ", hex);
	for (i = 0; i < 4; i++) {
		printf("%c", tohex[i]);
	}
	printf("\n");

	printf("\n** CRC tests\n");
	uint16_t crc = 0xffff;
	char testcheck[] = "hadie,181,10:42:10,54.422829,-6.741293,27799.3,1:10";
	for (i = 0; i < sizeof(testcheck)-1; i++) {
		crc = crc_xmodem_update(crc, testcheck[i]);
	}
	printf("CRC should be 0x002A");

	i16tox(crc, tohex);
	printf("\nCRC in hex: ", hex);
	for (i = 0; i < 4; i++) {
		printf("%c", tohex[i]);
	}
	printf("\n");
}
#endif
