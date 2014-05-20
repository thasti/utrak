/* 
 * string util unit test
 *
 * Stefan Biereigel
 */

#include <stdio.h>
#include <inttypes.h>
#include "string.h"

int main(void) {
	char test[] = "19123.45";
	char number[7] = { 0 };

	int len = sizeof(test) - 1;
	uint32_t result;
	uint8_t decimal_pos;

	uint32_t integer;
	uint32_t decimal;


	printf("Test String: %s\n", test);
	printf("Length: %d\n", len);
	
	atoid32(test, len, &integer, &decimal);
	printf("Integer Portion: %zu\n", integer);
	printf("Decimal Portion: %zu\n", decimal);
	
	atod32(test, len, &decimal);
	printf("Decimal Portion: %zu\n", decimal);
	
	atoi32(test, len, &integer);
	printf("Integer Portion: %zu\n", integer);
	
	integer = 300;

	i32toa(integer, 6, number);
	printf("Number %s\n", number);

	char tohex[4] = "0000";
	uint16_t hex = 0x20af;
	uint8_t i;
	i16tox(hex, tohex);
	printf("%x to hex: ", hex);
	for (i = 0; i < 4; i++) {
		printf("%c", tohex[i]);
	}
	printf("\n");

}
