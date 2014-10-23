#include <msp430.h>
#include <inttypes.h>
#include "aprs.h"
#include "main.h"
#include "hw.h"
#include "si4060.h"
#include "sin_table.h"

extern volatile uint16_t aprs_tick;
extern uint16_t aprs_buf_len;
extern char aprs_buf[APRS_BUF_LEN];

const char aprs_header[APRS_HEADER_LEN] = {
	'A'*2, 'P'*2, 'R'*2, 'S'*2, ' '*2, ' '*2, SSID_RESC + DST_SSID,
	'D'*2, 'K'*2, '3'*2, 'S'*2, 'B'*2, ' '*2, SSID_RESC + SRC_SSID,
	'W'*2, 'I'*2, 'D'*2, 'E'*2, '1'*2, ' '*2, SSID_RESC + WIDE_SSID,
	CONTROL_UI, PID_NONE};

enum aprs_states {SM_INIT, SFLAG, AX25_HEADER, AX25_DATA, AX25_FCS1, AX25_FCS2, EFLAG};
volatile uint16_t aprs_state = SM_INIT;
volatile uint16_t fcs = 0;
volatile uint8_t bitcnt = 8;
volatile uint8_t onecnt = 0;
volatile uint8_t finished = 0;


inline void calculate_fcs(void) {
	int i;
	CRCINIRES = 0xffff;
	for (i = 0; i < APRS_HEADER_LEN; i++) {
		CRCDIRB_L = aprs_header[i];
	}

	for (i = 0; i < APRS_BUF_LEN; i++) {
		CRCDIRB_L = aprs_buf[i];
	}
	fcs = CRCINIRES;
}

inline void aprs_init(void) {
	aprs_state = SM_INIT;
	finished = 0;
	bitcnt = 8;
	onecnt = 0;
	/* copy LAT + dir */
	/* copy LON + dir */
	/* copy ALT_FT */
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
			retval = (uint8_t)(fcs >> 8);
			aprs_state = AX25_FCS2;
			break;
		case AX25_FCS2:
			aprs_state = EFLAG;
			retval = (uint8_t)fcs;
			break;
		case EFLAG:
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
	static uint8_t byte;
	static uint8_t bit;
	static uint8_t bit_d;
	if (bitcnt >= 8) {
		byte = get_next_byte();
		bitcnt = 0;
	}
	/* all bytes LSB first */
	bit = byte & 0x01;
	if (bit) {
		/* bit stuffing */
		onecnt++;
		if (aprs_state != SFLAG && aprs_state != EFLAG && (onecnt >= 5)) {
			/* next time the same bit will be a zero -> stuffed bit */
			byte &= ~BIT0;
			onecnt = 0;
		} else {
			byte >>= 1;
			bitcnt = (bitcnt + 1) & 0x07;
		}

	} else {
		/* grab next bit only if not stuffed */
		onecnt = 0;
		byte >>= 1;
		bitcnt = bitcnt + 1;
	}
	if (bit_d == bit) {
		/* no transition */
		/* save current bit */
		bit_d = bit;
		return 1;
	} else {
		/* transition */
		/* save current bit */
		bit_d = bit;
		return 0;
	}
}

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
	while(!finished) {
		if (aprs_tick) {
			/* running with APRS sample clock */
			aprs_tick = 0;
			samp_cnt++;
			if (samp_cnt >= SAMP_PER_BIT) {
				/* running with bit clock (1200 / sec) */
				WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
				samp_cnt = 0;
				if (get_next_bit()) {
					fcw = SPACE_FCW;
				} else {
					fcw = MARK_FCW;
				}
			}

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

			/* tell us when we fail to meet the timing */
			if (aprs_tick) {
				while(1) {
					WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
				}
			}
		}
	}
	si4060_set_offset(0);
	si4060_stop_tx();
	serial_enable();


}
