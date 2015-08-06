#ifndef BACKLOG_H_
#define BACKLOG_H_

#include "fix.h"

/* number of backlog fixes to save*/
#define BACKLOG_MAX_ENTRIES	96
/* number of backlog fixes to skip between transmissions.
 * this distributes transmitted backlog fixes more evenly, so that after long
 * out-of-range-times the history is filled in an "interleaved" fashion
 *
 * be sure to set this value so that different fixes get transmitted every time!
 * if the number of fixes is even, this number must be uneven!
 */
#define BACKLOG_TX_INCREMENT	5

void backlog_add_fix(struct gps_fix *fix);
struct gps_fix* backlog_get_next_fix(void);
void backlog_clear_fixes(void);
void backlog_invalidate_fixes(void);

#endif
