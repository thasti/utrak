#include <inttypes.h>
#include <msp430.h>
#include "nmea.h"
#include "tlm.h"
#include "main.h"
#include "rtty.h"
#include "si4060.h"

extern volatile uint16_t tlm_tick;
extern uint16_t tx_buf_rdy;
extern uint16_t tx_buf_length;
extern char tx_buf[TX_BUF_MAX_LENGTH];

/*
 * tx_rtty
 *
 * transmits the TX buffer via RTTY at 50 baud (at 100Hz rtty_tick)
 * LSB first, in 7bit-ASCII format, 1 start bit, 2 stop bits
 *
 * the systick-flag is used for timing.
 */
void tx_rtty(void) {
	enum c_states {IDLE, START, CHARACTER, STOP1, STOP2};
	static uint16_t tx_state = 0;
	static uint16_t char_state = IDLE;
	static uint8_t data = 0;
	static uint16_t i = 0;
	static uint16_t tx_buf_index = 0;	/* the index for reading from the buffer */
	if (!tx_buf_rdy) {
		if (tx_state == 1) {
			si4060_stop_tx();
			tx_state = 0;
		}
		return;
	}
	/* tx_buffer is ready */
	if (tx_state == 0) {
		si4060_setup(MOD_TYPE_2FSK);
		si4060_start_tx(0);
		tx_state = 1;
		tx_buf_index = 0;
	}

	if (!tlm_tick)
		return;

	tlm_tick = 0;
	switch (char_state) {
		case IDLE:
			P1OUT |= SI_DATA;
			i++;
			if (i == NUM_IDLE_BITS) {
				char_state = START;
				i = 0;
			}
			break;
		case START:
			P1OUT &= ~SI_DATA;
			i = 0;
			data = tx_buf[tx_buf_index];
			char_state = CHARACTER;
			break;
		case CHARACTER:
			i++;
			if (data & 0x01) {
				P1OUT |= SI_DATA;
			} else {
				P1OUT &= ~SI_DATA;
			}
			data >>= 1;
			if (i == 7) {
				char_state = STOP1;
			}
			break;
		case STOP1:
			P1OUT |= SI_DATA;
			char_state = STOP2;
			break;
		case STOP2:
			i = 0;
			char_state = START;
			tx_buf_index++;
			if (tx_buf_index >= tx_buf_length) {
				char_state = IDLE;
				tx_buf_rdy = 0;
			}
			break;
		default:
			break;
	}
}
