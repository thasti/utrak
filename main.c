/*
 * main tracker software
 *
 * Stefan Biereigel
 *
 */

#include <msp430.h>
#include <inttypes.h>
#include "gps.h"
#include "main.h"
#include "nmea.h"
#include "si4060.h"
#include "spi.h"
#include "string.h"
#include "aprs.h"
#include "hw.h"
#include "tlm.h"
#include "rtty.h"
#include "geofence.h"

/*
 * GLOBAL VARIABLES
 */

/*
 * housekeeping variables
 */
volatile uint16_t seconds = 0;		/* timekeeping via timer */
volatile uint16_t sec_overflows = 0;	/* overflow counter for second generation */
volatile uint16_t tlm_tick = 0;		/* flag for slow telemetry handling (ISR -> main) */
volatile uint16_t aprs_tick = 0;	/* flag for APRS handling (ISR -> main) */
volatile uint16_t aprs_baud_tick = 0;	/* flag for APRS baud rate (ISR -> main) */

/*
 * the NMEA data buffer
 * it was confirmed that the Linx RXM-GPS-RM sticks to the standard
 */
volatile uint16_t nmea_buf_index = 0;	/* the index for writing to the buffer */
volatile uint16_t nmea_buf_rdy = 0;	/* the ready-flag (ISR -> main) */
volatile char nmea_buf[NMEA_BUF_SIZE] = { 0 };	/* the actual buffer */

/*
 * the TX data buffer
 * contains ASCII data, which is either transmitted as CW oder RTTY
 */
uint16_t tx_buf_index = 0;			/* the index for reading from the buffer */
uint16_t tx_buf_rdy = 0;			/* the read-flag (main -> main) */
uint16_t tx_buf_length = 0;			/* how many chars to send */
char tx_buf[TX_BUF_MAX_LENGTH] = {SYNC_PREFIX "$$" PAYLOAD_NAME ","};	/* the actual buffer */

/*
 * the APRS data buffer
 * contains ASCII data
 */
uint16_t aprs_buf_len = APRS_BUF_LEN;
char aprs_buf[APRS_BUF_LEN] = "!xxxxxxxx/xxxxxxxxxO/A=xxxxxx " APRS_COMMENT;

/*
 * GPS fix data and data for tlm string
 * extracted from NMEA sentences by GPS data processing
 */
char tlm_sent_id[SENT_ID_LENGTH_MAX] = { 0 };
char tlm_time[TIME_LENGTH] = { 0 };
char tlm_lat[LAT_LENGTH+1] = { 0 };
char tlm_lon[LON_LENGTH+1] = { 0 };
uint8_t tlm_alt_length;
char tlm_alt[ALT_LENGTH_MAX] = { 0 };
char tlm_alt_ft[APRS_ALT_LEN] = { 0 };
char tlm_sat[SAT_LENGTH] = { 0 };
char tlm_volt[VOLT_LENGTH] = { 0 };
char tlm_temp[TEMP_LENGTH+1] = { 0 };

/*
 * uart_process
 *
 * checks the UART buffer status and processes full NMEA sentences
 *
 * returns:	0 if no fix was received or the last frame was not GPGGA at all
 * 		n - the number of satellites in the last fix
 */
uint8_t uart_process(void) {
	uint8_t i = 0;
	if (nmea_buf_rdy) {
		nmea_buf_rdy = 0;
		if (NMEA_sentence_is_GGA(nmea_buf)) {
			if (GPGGA_has_fix(nmea_buf)) {
				i = GPGGA_get_data(nmea_buf, tlm_lat, tlm_lon, tlm_alt, &tlm_alt_length, tlm_alt_ft, tlm_sat, tlm_time);
				if (!i) {
					return 0;
				}
				atoi8(tlm_sat, SAT_LENGTH, &i);
				return i;
			}
		}
	}
	return i;
}

int main(void) {
	uint16_t fix_sats = 0;
	uint16_t i;
	enum {TX_RTTY, TX_APRS} tlm_state = TX_RTTY;
	/* set watchdog timer interval to 11 seconds */
	/* reset occurs if Si4060 does not respond or software locks up */
	WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
	/* init all hardware components */
	hw_init();
	P1OUT |= LED_A;
	/* initialize the transmission buffer (for development only) */
	init_tx_buffer();

	/* reset the radio chip from shutdown */
	si4060_reset();
	/* check radio communication */
	i = si4060_part_info();
	if (i != 0x4060) {
		while(1);
	}
	si4060_power_up();
	si4060_setup(MOD_TYPE_OOK);

	WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
	/* wait for the GPS to boot */
	gps_startup_delay();
	/* tell it to use GPS only and output GGA messages on every fix */
	gps_set_nmea();
	gps_set_gps_only();
	gps_set_gga_only();
	gps_set_airborne_model();
	gps_set_power_save();
	gps_power_save(0);
	gps_save_settings();

	/* power up the Si4060 and set it to OOK, for transmission of blips */
	/* the Si4060 occasionally locks up here, the watchdog gets it back */
	si4060_setup(MOD_TYPE_OOK);
	si4060_freq_2m_rtty();
	si4060_start_tx(0);

	/* entering wait state */
	/* the tracker outputs RF blips while waiting for a GPS fix */
	while (fix_sats < 5) {
		WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
		fix_sats = uart_process();
		tx_blips(fix_sats);
	}
	si4060_stop_tx();
	/* modulation from now on will be RTTY */
	si4060_setup(MOD_TYPE_2FSK);
	/* activate power save mode as fix is stable */
	gps_power_save(1);
	seconds = TLM_RTTY_INTERVAL + 1;
	P1OUT &= ~LED_A;
	/* entering operational state */
	/* in fixed intervals, a new TX buffer is prepared and transmitted */
	/* watchdog timer is active for resets, if somethings locks up */
	while(1) {
		WDTCTL = WDTPW + WDTCNTCL + WDTIS1;
		uart_process();
		
		switch (tlm_state) {
			case TX_RTTY:
				if ((!tx_buf_rdy) && (seconds > TLM_RTTY_INTERVAL)) {
					seconds = 0;
					prepare_tx_buffer();
					geofence_aprs_frequency(tlm_lat, tlm_lon);
					tx_aprs();
					si4060_freq_2m_rtty();
					/* possible switchover to APRS only */
					if (!(geofence_slow_tlm_altitude(tlm_alt, tlm_alt_length))) {
						tlm_state = TX_APRS;
						/* set the tx buffer to not ready to inhibit tx_rtty() from sending */
						tx_buf_rdy = 0;
					}
				}
				tx_rtty();
				break;
			case TX_APRS:
				/* change to TX_RTTY when below 4k */
				if (seconds > TLM_APRS_INTERVAL) {
					seconds = 0;
					prepare_tx_buffer();
					geofence_aprs_frequency(tlm_lat, tlm_lon);
					tx_aprs();
					/* possible switchover to RTTY + APRS transmission */
					if (geofence_slow_tlm_altitude(tlm_alt, tlm_alt_length)) {
						tlm_state = TX_RTTY;
						tx_buf_rdy = 0;
					}
				}
				break;
		} /* switch (tlm_state) */
	} /* while(1) */
} /* main() */

/*
 * USCI A0 ISR
 *
 * USCI A is UART. RX appends incoming bytes to the NMEA buffer
 */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
	switch(UCA0IV) {
		case 0:						/* Vector 0 - no interrupt */
			break;
		case 2:						/* Vector 2 - RXIFG */
			if (nmea_buf_index < (NMEA_BUF_SIZE - 1))
				nmea_buf_index++;
			if (UCA0RXBUF == '$')
				nmea_buf_index = 0;
			if (UCA0RXBUF == '\n')
				nmea_buf_rdy = 1;
			nmea_buf[nmea_buf_index] = UCA0RXBUF;
			break;
		case 4:						/* Vector 4 - TXIFG */
			break;
		default:
			break;
	}
}

/*
 * Timer0 A0 ISR
 * CCR0 compare interrupt (highest priority)
 *
 * generates the various baud rates and clocks needed in the design
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
{
	TA0CCR0 += N_APRS_NCO - 1;
	aprs_tick = 1;
}

/*
 * Timer0 A0 ISR
 * Realizes APRS baud rate, telemetry baud rate and second counter
 * as msp430-gcc does not support the __even_in_range-intrinsic,
 * we use this inline assembly routine for the vectoring.
 *
 * source: http://sourceforge.net/p/mspgcc/mailman/message/25525509/
 */
#pragma vector = TIMER0_A1_VECTOR
__interrupt void timera0x_handler(void)
{
	__asm__ __volatile__("add   %[src]       ,r0       ":: [src] "m" (TA0IV));
	__asm__ __volatile__("reti                         "::); // NO INT
	__asm__ __volatile__("jmp   timera0_cc1_handler    "::); // CC1
	__asm__ __volatile__("jmp   timera0_cc2_handler    "::); // CC2
	__asm__ __volatile__("reti                         "::); // RESERVED
	__asm__ __volatile__("reti                         "::); // RESERVED
	__asm__ __volatile__("reti                         "::); // RESERVED
	__asm__ __volatile__("reti                         "::); // RESERVED
	__asm__ __volatile__("jmp   timera0_ifg_handler    "::); // IFG
}

/*
 * CCR1, generates APRS baud rate
 */
__interrupt void timera0_cc1_handler(void)
{
		TA0CCR1 += N_APRS_BAUD - 1;
		aprs_baud_tick = 1;
}

/*
 * CCR2, generates RTTY baud rate
 */
__interrupt void timera0_cc2_handler(void)
{
#ifndef	TLM_DOMINOEX
		TA0CCR2 += N_TLM;
		tlm_tick = 1;
		sec_overflows++;
		if (sec_overflows >= TLM_HZ) {
			seconds++;
			sec_overflows = 0;
		}
#endif
}

/*
 * Overflow, generates DominoEX baud rate
 */
__interrupt void timera0_ifg_handler(void)
{
#ifdef TLM_DOMINOEX
		tlm_tick = 1;
		sec_overflows++;
		if (sec_overflows >= TLM_HZ) {
			seconds++;
			sec_overflows = 0;
		}
#endif
}

