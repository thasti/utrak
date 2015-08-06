#include <msp430.h>
#include "fix.h"
#include "backlog.h"

/* backlog fixes */
struct gps_fix backlog[BACKLOG_MAX_ENTRIES] __attribute__((noinit)) __attribute__ ((section(".text"))); 

/* 
 * backlog_add_fix
 *
 * replaces an old fix with a current one, if it is new enough. for now this
 * one adds fixes at a one hour interval.
 */
void backlog_add_fix(struct gps_fix* fix) {
	static int newest_fix_id = 0;

	/* if the fix is unsuitable (2D only or no fix at all), do nothing */
	if (fix->type < 3) 
		return;
	
	if (fix->hour != backlog[newest_fix_id].hour) {
		newest_fix_id = (newest_fix_id + 1) % BACKLOG_MAX_ENTRIES;
		backlog[newest_fix_id] = *fix;	
	}

}

/* 
 * backlog_get_next_fix
 *
 * returns the next backlog fix to be transmitted. if the next fix is unsuitable
 * for transmission (no fix at all, only 2D-fix, ...) then NULL is returned.
 */
struct gps_fix* backlog_get_next_fix(void) {
	static int last_fix_id = 0;
	int fix_id;

	fix_id = last_fix_id;
	last_fix_id = (last_fix_id + BACKLOG_TX_INCREMENT) % BACKLOG_MAX_ENTRIES;
	
	if (backlog[fix_id].type > 2)
		return &backlog[fix_id];
	else
		return 0;
}

void backlog_invalidate_fixes(void) {
	int i;

	for (i=0; i < BACKLOG_MAX_ENTRIES; i++) {
		backlog[i].type = 0;
		backlog[i].num_svs = 0;
		backlog[i].year = 0;
		backlog[i].month = 0;
		backlog[i].day = 0;
		backlog[i].hour = 0;
		backlog[i].min = 0;
		backlog[i].sec = 0;
		backlog[i].lat = 0;
		backlog[i].lon = 0;
		backlog[i].alt = 0;
	}
}

