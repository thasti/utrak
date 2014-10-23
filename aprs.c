#include <msp430.h>
#include <inttypes.h>
#include "aprs.h"
#include "main.h"
#include "hw.h"
#include "si4060.h"
#include "sin_table.h"

extern volatile uint16_t aprs_tick;

void tx_aprs(void) {
	serial_disable();
	/* use 2FSK mode so we can adjust the OFFSET register */
	si4060_setup(MOD_TYPE_2FSK);
	si4060_start_tx(0);
	uint16_t fcw = SPACE_FCW;
	uint16_t pac = 0;
	uint16_t offset = 0;
	uint8_t samp_cnt = 0;

	aprs_tick = 0;
	while(1) {
		WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
		if (aprs_tick) {
			/* running with APRS sample clock */
			aprs_tick = 0;
			samp_cnt++;
			if (samp_cnt >= SAMP_PER_BIT) {
				/* running with bit clock (1200 / sec) */
				samp_cnt = 0;
				if (fcw == MARK_FCW) {
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
	si4060_stop_tx();
	si4060_set_offset(0);
	serial_enable();


}
