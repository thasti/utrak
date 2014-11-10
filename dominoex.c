#include <msp430.h>
#include <inttypes.h>
#include "dominoex.h"
#include "nmea.h"
#include "sin_table.h"
#include "dominoex-var.h"
#include "si4060.h"
#include "main.h"

extern int8_t varicode[128][3];
extern uint8_t secondary_space[3];
extern volatile uint16_t tlm_tick;
extern uint16_t tx_buf_rdy;
extern uint16_t tx_buf_index;
extern uint16_t tx_buf_length;
extern char tx_buf[TX_BUF_MAX_LENGTH];

void tx_dominoex(void) {
	enum c_states {SYNC, CHAR};
	static int16_t tone;
	static uint16_t tx_state = 0;
	static uint16_t char_state = SYNC;
	static uint16_t next_tone = 0;

	static uint16_t table_index = 0;
	static uint16_t tone_cnt = 0;

	if (!tx_buf_rdy) {
		if (tx_state == 1) {
			si4060_stop_tx();
			tx_state = 0;
		}
		return;
	}
	/* tx_buffer is ready */
	if (tx_state == 0) {
		si4060_start_tx(0);
		tx_state = 1;
		tx_buf_index = 0;
	}

	if (!tlm_tick)
		return;
	tlm_tick = 0;

	/* calculate the next tone */
	switch (char_state) {
		case SYNC:
			tone = secondary_space[table_index++];
			if (table_index > 2) {
				table_index = 0;
				tone_cnt++;
			}
			if (tone_cnt >= SYNC_CHARS) {
				char_state = CHAR;
				tone_cnt = 0;
			}
			break;
		case CHAR:
			tone = varicode[(int8_t)tx_buf[tx_buf_index]][tone_cnt++];
			if (tone == -1 || tone_cnt > 3 ) {
				/* we finished this char, advance to the next */
				tone_cnt = 0;
				tx_buf_index++;
				if (tx_buf_index >= tx_buf_length) {
					tx_buf_rdy = 0;
					tx_buf_index = 0;
					char_state = SYNC;
					return;
				}
				tone = varicode[(int8_t)tx_buf[tx_buf_index]][tone_cnt++];
			}
			break;
		default:
			break;
	}
	next_tone = next_tone + tone + 2;
	if (next_tone >= 18) {
		next_tone -= 18;
	}
	/* add the offset to ensure we're on the center frequency */
	/* 16.3676 MHz TCXO results in 7,8xxHz resolution, shift for DominoEX is about double that */
	si4060_set_offset(SIN_OFF_70CM + 2*next_tone);

}

