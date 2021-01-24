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
#include "si4060.h"
#include "spi.h"
#include "string.h"
#include "aprs.h"
#include "hw.h"
#include "tlm.h"
#include "rtty.h"
#include "geofence.h"
#include "backlog.h"

/*
 * GLOBAL VARIABLES
 */

/*
 * housekeeping and interrupt communication variables
 *
 * tick flags are set by ISRs and reset by the main software
 */
volatile uint16_t seconds = 0;		/* timekeeping via timer */
volatile uint16_t tlm_tick = 0;		/* flag for slow telemetry handling (ISR -> main) */
volatile uint16_t aprs_tick = 0;	/* flag for APRS handling (ISR -> main) */
volatile uint16_t aprs_baud_tick = 0;	/* flag for APRS baud rate (ISR -> main) */
volatile uint16_t aprs_bit = APRS_SPACE;/* the currently transmitted tone frequency (main -> ISR) */

/*
 * the TX data buffer
 * contains ASCII data, which is either transmitted as CW oder RTTY
 */
uint16_t tx_buf_rdy = 0;			/* the read-flag (main -> main) */
uint16_t tx_buf_length = 0;			/* how many chars to send */
char tx_buf[TX_BUF_MAX_LENGTH] = {SYNC_PREFIX "$$" PAYLOAD_NAME ","};	/* the actual buffer */

/* current (latest) GPS fix and measurements */
struct gps_fix current_fix;

void get_fix_and_measurements(void) {
	gps_get_fix(&current_fix);
	current_fix.temperature_int = get_die_temperature();
	current_fix.voltage_bat = get_battery_voltage();
	current_fix.voltage_sol = get_solar_voltage();
}

int main(void) {
	uint16_t i;
	uint16_t backlog_transmitted = 0;
	struct gps_fix *backlog_fix = 0;
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
	gps_startup_delay();
	while(!(gps_disable_nmea_output()));
	while(!(gps_set_gps_only()));
	while(!(gps_set_airborne_model()));
    while(!(gps_set_power_save()));
	while(!(gps_power_save(0)));
	while(!(gps_save_settings()));
	
	/* APRS test code (for measurement of tone frequencies and spectra) */
	/*
	while(1) {
		si4060_freq_aprs_eu();
		tx_aprs();
		if (aprs_bit == APRS_MARK) {
			aprs_bit = APRS_SPACE;
		} else {
			aprs_bit = APRS_MARK;
		}
	}
	*/
	
	/* power up the Si4060 and set it to OOK, for transmission of blips */
	si4060_setup(MOD_TYPE_OOK);
	si4060_freq_2m_rtty();
	si4060_start_tx(0);

	/* TODO remove before flight */
	/*backlog_invalidate_fixes(); */

	/* the tracker outputs RF blips while waiting for a GPS fix */
	while (current_fix.num_svs < 5 && current_fix.type < 3) {
		WDTCTL = WDTPW + WDTCNTCL + WDTIS0;
		if (seconds > BLIP_FIX_INTERVAL) {
			seconds = 0;
			gps_get_fix(&current_fix);
			tx_blips(1);
		} else {
			tx_blips(0);
		}
	}

	si4060_stop_tx();
	/* modulation from now on will be RTTY */
	si4060_setup(MOD_TYPE_2FSK);
	/* activate power save mode as fix is stable */
	gps_power_save(1);
	seconds = TLM_APRS_INTERVAL + 1;
	P1OUT &= ~LED_A;
	/* entering operational state */
	/* in fixed intervals, a new TX buffer is prepared and transmitted */
	/* watchdog timer is active for resets, if somethings locks up */
	while(1) {
		WDTCTL = WDTPW + WDTCNTCL + WDTIS0;

#ifdef TLM_RTTY /* APRS and RTTY transmission */
		switch (tlm_state) {
			case TX_RTTY:
				/* backlog transmission */
				if (seconds > TLM_BACKLOG_OFFSET && backlog_transmitted == 0) {
					backlog_fix = backlog_get_next_fix();
					if (backlog_fix != 0) {
						aprs_prepare_buffer(backlog_fix, 1);
						geofence_aprs_frequency(&current_fix);
						tx_aprs();
					}
					backlog_transmitted = 1;
				}
				/* regular APRS transmission, start of RTTY transmission */
				if ((!tx_buf_rdy) && (seconds > TLM_RTTY_INTERVAL)) {
					get_fix_and_measurements();
					backlog_add_fix(&current_fix);
					seconds = 0;
					if (current_fix.type > 2) {
						prepare_tx_buffer();
						aprs_prepare_buffer(&current_fix, 0);
					}
					geofence_aprs_frequency(&current_fix);
					tx_aprs();
					si4060_freq_2m_rtty();
					/* possible switchover to APRS only */
#ifdef TLM_RTTY_ONLY
					if (!(geofence_slow_tlm_altitude(&current_fix))) {
						tlm_state = TX_APRS;
						/* set the tx buffer to not ready to inhibit tx_rtty() from sending */
						tx_buf_rdy = 0;
					} else {
						tlm_init();	/* starts the RTTY transmission */
					}
#else
					tlm_init();
#endif
					backlog_transmitted = 0;
				}
				tx_rtty();
				break;
			case TX_APRS:
				/* backlog transmission */
				if (seconds > TLM_BACKLOG_OFFSET && backlog_transmitted == 0) {
					backlog_fix = backlog_get_next_fix();
					if (backlog_fix != 0) {
						aprs_prepare_buffer(backlog_fix, 1);
						geofence_aprs_frequency(&current_fix);
						tx_aprs();
					}
					backlog_transmitted = 1;
				}
				/* regular APRS transmission */
				if (seconds > TLM_APRS_INTERVAL) {
					get_fix_and_measurements();
					backlog_add_fix(&current_fix);
					seconds = 0;
					if (current_fix.type > 2) {
						/* if no current fix is available, the old fix is transmitted again */
						aprs_prepare_buffer(&current_fix, 0);
					}
					geofence_aprs_frequency(&current_fix);
					tx_aprs();
					/* possible switchover to RTTY + APRS transmission */
					if (geofence_slow_tlm_altitude(&current_fix)) {
						tlm_state = TX_RTTY;
					}
					backlog_transmitted = 0;
				}
				break;
			default:
				tlm_state = TX_RTTY;
				break;
		} /* switch (tlm_state) */

#else	/* APRS only */
		/* backlog transmission */
		if (seconds > TLM_BACKLOG_OFFSET && backlog_transmitted == 0) {
			backlog_fix = backlog_get_next_fix();
			if (backlog_fix != 0) {
				aprs_prepare_buffer(backlog_fix, 1);
				geofence_aprs_frequency(&current_fix);
				tx_aprs();
			}
			backlog_transmitted = 1;
		}
		/* regular APRS transmission */
		if (seconds > TLM_APRS_INTERVAL) {
			get_fix_and_measurements();
			backlog_add_fix(&current_fix);
			seconds = 0;
			if (current_fix.type > 2) {
				/* if no current fix is available, the old fix is transmitted again */
				aprs_prepare_buffer(&current_fix, 0);
			}
			geofence_aprs_frequency(&current_fix);
			tx_aprs();
			backlog_transmitted = 0;
		}
#endif
	} /* while(1) */
} /* main() */

/*
 * Timer0 A0 ISR
 * CCR0 compare interrupt (highest priority)
 *
 * generates the various baud rates and clocks needed in the design
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
{
	static uint16_t aprs_nco_count = 0;
	static uint16_t aprs_bit_count = 0;
	aprs_nco_count++;
	aprs_bit_count++;
	if (aprs_bit == APRS_SPACE && aprs_nco_count >= APRS_SPACE_TICKS) {
		aprs_tick = 1;
		aprs_nco_count = 0;
	}
	if (aprs_bit == APRS_MARK && aprs_nco_count >= APRS_MARK_TICKS) {
		aprs_tick = 1;
		aprs_nco_count = 0;
	}
	if (aprs_bit_count == APRS_BAUD_TICKS) {	
		aprs_baud_tick = 1;
		aprs_bit_count = 0;
	}
	TA0CCR0 += N_APRS_NCO - 1;
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
 * CCR1
 */
__interrupt void timera0_cc1_handler(void)
{
	/* unused */
}

/*
 * CCR2, generates RTTY baud rate
 */
__interrupt void timera0_cc2_handler(void)
{
	static uint16_t sec_overflows = 0;	/* overflow counter for second generation */
	static uint16_t cc2_overflow = 0;	/* CC2 has to overflow twice for one tlm period */
	
	TA0CCR2 += N_TLM;
	cc2_overflow = ~cc2_overflow;
	if (cc2_overflow) {
		tlm_tick = 1;
		sec_overflows++;
		if (sec_overflows >= TLM_HZ) {
			seconds++;
			sec_overflows = 0;
		}
	}
}

/*
 * Overflow
 */
__interrupt void timera0_ifg_handler(void)
{
	/* unused */
}

