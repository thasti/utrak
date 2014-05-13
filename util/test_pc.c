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

	printf("Test String: %s\n", test);
	printf("Length: %d\n", len);
	atoi32(test, len, &decimal_pos, &result);
	printf("Result: %zu\n", result);
	printf("Decimal Position: %d\n", decimal_pos);

}
