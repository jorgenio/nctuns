/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

#include "pcr.h"
#include <math.h>
#include <nctuns_api.h>

uint32_t tick_to_pcr(uint64_t tick)
{
	uint64_t pcr_base;
	uint16_t pcr_ext;

	tick_to_pcr_base_and_ext(tick, pcr_base, pcr_ext);

	return (pcr_base * 300 + pcr_ext);
}


void tick_to_pcr_base_and_ext(uint64_t tick, uint64_t& base, uint16_t& ext)
{
	base = (uint64_t) ((tick * TICK * 27000000) / (300 * 1000000000.0));
	base <<= (64-33);
	base >>= (64-33);

	ext = (uint16_t) fmod(((tick * TICK * 27000000) / 1000000000.0), 300);
}

uint64_t pcr_to_tick(uint32_t pcr)
{
	return (uint64_t)(pcr / 27000000.0);
}
