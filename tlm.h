#ifndef __tlm_h__
#define __tlm_h__

#include <inttypes.h>
#include "main.h"

/* because tick is slower when using DominoEX,
 * use less ticks for blipping
 */
#ifdef TLM_DOMINOEX
#define BLIP_FACTOR	1
#else
#define	BLIP_FACTOR	5
#endif

void tx_blips(uint8_t sats);
void init_tx_buffer(void);
void prepare_tx_buffer(void);

#endif
