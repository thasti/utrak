#include <stdio.h>
#include <inttypes.h>

#include "../aprs.h"

extern volatile uint8_t finished;

uint16_t aprs_buf_len = APRS_BUF_LEN;
char aprs_buf[APRS_BUF_LEN] = "!xxxxxxxx/xxxxxxxxxO/A=xxxxxx " APRS_COMMENT;

int main(void) {
	aprs_init();
	while(!finished) {
		printf("%d ", get_next_bit());
	}

	return 0;
}

