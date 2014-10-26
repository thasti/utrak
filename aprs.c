#ifndef TEST
#include <msp430.h>
#include "hw.h"
#include "si4060.h"
#include "sin_table.h"
#include "main.h"
#endif

#include <inttypes.h>
#include "aprs.h"

extern volatile uint16_t aprs_tick;
extern uint16_t aprs_buf_len;
extern char aprs_buf[APRS_BUF_LEN];
extern char tlm_lat[];
extern char tlm_lon[];
extern char tlm_alt_ft[];

const unsigned char aprs_header[APRS_HEADER_LEN] = {
	'A'*2, 'P'*2, 'R'*2, 'S'*2, ' '*2, ' '*2, SSID_RESC + (DST_SSID << 1),
	'D'*2, 'K'*2, '3'*2, 'S'*2, 'B'*2, ' '*2, SSID_RESC + (SRC_SSID << 1),
	'W'*2, 'I'*2, 'D'*2, 'E'*2, '1'*2, ' '*2, SSID_RESC + (WIDE_SSID << 1) + HEADER_END,
	CONTROL_UI, PID_NONE};

enum aprs_states {SM_INIT, SFLAG, AX25_HEADER, AX25_DATA, AX25_FCS1, AX25_FCS2, EFLAG};
volatile uint16_t aprs_state = SM_INIT;
volatile uint16_t fcs = 0;
volatile uint8_t bitcnt = 8;
volatile uint8_t onecnt = 0;
volatile uint8_t finished = 0;
volatile uint8_t stuffing = 0;

#ifndef TEST
inline void calculate_fcs(void) {
	int i;
	CRCINIRES = 0xffff;
	for (i = 0; i < APRS_HEADER_LEN; i++) {
		CRCDI_L = aprs_header[i];
	}

	for (i = 0; i < APRS_BUF_LEN; i++) {
		CRCDI_L = aprs_buf[i];
	}
	fcs = CRCRESR ^ 0xffff;
}
#else
void calculate_fcs(void) {}
#endif

inline void aprs_init(void) {
	uint8_t i;
	aprs_state = SM_INIT;
	finished = 0;
	bitcnt = 8;
	onecnt = 0;
	stuffing = 0;
	for (i = 0; i < APRS_LAT_LEN; i++)
		aprs_buf[APRS_LAT_START + i] = tlm_lat[i+1];
	for (i = 0; i < APRS_LON_LEN; i++)
		aprs_buf[APRS_LON_START + i] = tlm_lon[i+1];
	for (i = 0; i < APRS_ALT_LEN; i++)
		aprs_buf[APRS_ALT_START + i] = tlm_alt_ft[i];
	if (tlm_lat[0] == '+') {
		aprs_buf[APRS_LAT_START + APRS_LAT_LEN] = 'N';
	} else {
		aprs_buf[APRS_LAT_START + APRS_LAT_LEN] = 'S';
	}
	if (tlm_lon[0] == '+') {
		aprs_buf[APRS_LON_START + APRS_LON_LEN] = 'E';
	} else {
		aprs_buf[APRS_LON_START + APRS_LON_LEN] = 'W';
	}
	calculate_fcs();
}

/*
 * get_next_byte
 *
 * fetches the next byte to be transmitted, until there is none left. it advances the
 * internal AX.25 state machine. first, a configurable number of flags are returned,
 * which are followed by the header, the payload, and end flags. an infinite number of
 * end flags are returned, until the state machine is reset by init_aprs()
 *
 * returns:
 * 	byte to be transmitted
 */
uint8_t get_next_byte(void) {
	static int i = 0;
	uint8_t retval;
	switch (aprs_state) {
		case SM_INIT:
			stuffing = 0;
			aprs_state = SFLAG;
			i = 0;
		case SFLAG:
			retval = AX25_FLAG;
			if (++i >= AX25_SFLAGS) {
				aprs_state = AX25_HEADER;
				i = 0;
			}
			break;
		case AX25_HEADER:
			stuffing = 1;
			retval = aprs_header[i];
			if (++i >= APRS_HEADER_LEN) {
				aprs_state = AX25_DATA;
				i = 0;
			}
			break;
		case AX25_DATA:
			retval = aprs_buf[i];
			if (++i >= APRS_BUF_LEN) {
				aprs_state = AX25_FCS1;
				i = 0;
			}
			break;
		case AX25_FCS1:
			retval = (uint8_t)fcs;
			aprs_state = AX25_FCS2;
			break;
		case AX25_FCS2:
			retval = (uint8_t)(fcs >> 8);
			aprs_state = EFLAG;
			break;
		case EFLAG:
			stuffing = 0;
			retval = AX25_FLAG;
			if (++i > AX25_EFLAGS) {
				finished = 1;
				i = 0;
			}
			break;
		default:
			retval = AX25_FLAG;
	}
	return retval;
}

/*
 * get_next_bit
 *
 * fetches the next bit for the data stream.
 *
 * returns: the next bit to be transmitted, already NRZI coded and bit stuffed
 */
uint8_t get_next_bit(void) {
	static uint8_t byte = 0;
	static uint8_t bit = 0;
	static uint8_t bit_d = 0;
	if (bitcnt >= 8) {
		byte = get_next_byte();
		bitcnt = 0;
	}
	/* all bytes LSB first */
	bit = byte & 0x01;
	if (bit) {
		/* bit stuffing */
		onecnt++;
		if ((stuffing == 1) && (onecnt >= 5)) {
			/* next time the same bit will be a zero -> stuffed bit */
			byte &= ~0x01;
			onecnt = 0;
		} else {
			byte >>= 1;
			bitcnt = bitcnt + 1;
		}

	} else {
		/* grab next bit only if not stuffed */
		onecnt = 0;
		byte >>= 1;
		bitcnt = bitcnt + 1;
	}

	/* NRZI encoding */
	if (bit == 0) {
		bit_d ^= 0x01;
	}
	return bit_d;
}

#ifndef TEST
void tx_aprs(void) {
	uint16_t fcw = SPACE_FCW;
	uint16_t pac = 0;
	uint16_t offset = 0;
	uint8_t samp_cnt = 0;

	aprs_init();
	serial_disable();

	/* use 2FSK mode so we can adjust the OFFSET register */
	si4060_setup(MOD_TYPE_2FSK);
	si4060_start_tx(0);

	aprs_tick = 0;
	do {
		if (aprs_tick) {
			/* running with APRS sample clock */
			aprs_tick = 0;
			if (++samp_cnt >= 7) {
				/* running with bit clock (1200 / sec) */
				WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
				samp_cnt = 0;
				if (get_next_bit()) {
					fcw = SPACE_FCW;
				} else {
					fcw = MARK_FCW;
				}
			}
#define APRS_FM
#ifdef APRS_FM
			/* NCO core */
			pac = (pac + fcw) & 1023;
			switch (pac & 0x0300) {
				case 0x0000:
					/* 0 <= pac < 256 */
					offset = sin_table[pac];
					break;
				case 0x0100:
					/* 256 <= pac < 512 */
					offset = sin_table[255 - (pac & 0x00ff)];
					break;
				case 0x0200:
					/* 512 <= pac < 768 */
					offset = SIN_MAX - sin_table[pac & 0x00ff];
					break;
				case 0x0300:
					/* 768 <= pac < 1024 */
					offset = SIN_MAX - sin_table[255 - (pac & 0x00ff)];
					break;
				default:
					break;
			}
			si4060_set_offset(offset);
#else
			if (fcw == MARK_FCW) {
				si4060_set_offset(0);
			} else {
				si4060_set_offset(70);
			}
#endif

			/* tell us when we fail to meet the timing */
			if (aprs_tick) {
				while(1) {
					WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
				}
			}
		}
	} while(!finished);
	si4060_get_cts(0);
	si4060_set_offset(SIN_OFF);
	si4060_get_cts(0);
	si4060_stop_tx();
	serial_enable();
}

#endif
