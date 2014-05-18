/*
 * CRC-XMODEM checksum routine
 *
 * from http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html#gaca726c22a1900f9bad52594c8846115f
 *
 */

#include <inttypes.h>

static __inline__ uint16_t crc_xmodem_update(uint16_t crc, uint8_t data) {
	int i;

	crc = crc ^ ((uint16_t)data << 8);
	for (i=0; i<8; i++) {
		if (crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}
	return crc;
}
