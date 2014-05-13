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

}
