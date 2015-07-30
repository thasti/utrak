#ifndef BACKLOG_H_
#define BACKLOG_H_

#include "fix.h"

/* start of the backlog memory in FRAM 
 * be sure to check the binary size against this!
 *
 * if the program exceeds 13kB, 0xF500 can not be used anymore! 
 */
#define BACKLOG_MAX_ENTRIES	24

void backlog_add_fix(struct gps_fix *fix);
struct gps_fix* backlog_get_next_fix(void);
void backlog_clear_fixes(void);
void backlog_invalidate_fixes(void);

#endif
