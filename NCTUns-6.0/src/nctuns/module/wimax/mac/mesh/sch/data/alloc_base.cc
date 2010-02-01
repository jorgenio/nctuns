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

#include <cstdio>

#include "alloc_base.h"
#include "../../msh_dsch.h"

#define VERBOSE_LEVEL           MSG_INFO
#include "../../../verbose.h"


Alloc_base::Alloc_base(const class Alloc_base& alloc)
: _frame_start(alloc._frame_start)
, _frame_range(alloc._frame_range)
, _slot_start(alloc._slot_start)
, _slot_range(alloc._slot_range)
{
}

Alloc_base::Alloc_base(uint16_t frame_start, uint8_t frame_range,
		uint8_t slot_start, uint8_t slot_range)
: _frame_start(frame_start)
, _frame_range(frame_range)
, _slot_start(slot_start)
, _slot_range(slot_range)
{
}

Alloc_base::~Alloc_base()
{
}

uint8_t
Alloc_base::frame_remain(uint16_t frame_base) const
{
	if (_frame_range == FRAME_RANGE_PERSISTENCE)
		return FRAME_RANGE_PERSISTENCE;

	uint16_t expired_frame = (_frame_start + _frame_range) & FRAME_BIT_MASK;
	frame_base &= FRAME_BIT_MASK;

	if (!is_active(frame_base))
		assert(0);

	if (((expired_frame - frame_base) & FRAME_BIT_MASK) < FRAME_DIFF_MAX)
		return (expired_frame - frame_base) & FRAME_BIT_MASK;
	else
		return 0;
}

bool
Alloc_base::is_active(uint16_t frame_base) const
{
	/*
	 * FIXME: How to determine if a persistent frame is active?
	 */
	if (_frame_range == FRAME_RANGE_PERSISTENCE)
		return true;

	if (((frame_base - _frame_start) & FRAME_BIT_MASK) < FRAME_DIFF_MAX)
		return true;
	return false;
}

bool
Alloc_base::is_in_between(const Alloc_base& container) const
{
	/*
	 * Slot interval checking.
	 */
	if (_slot_start < container._slot_start ||
			_slot_start + _slot_range
			> container._slot_start + container._slot_range)
		return false;
	/*
	 * If the frame range is persistent, frame start is meaningless.
	 */
	if (_frame_range == FRAME_RANGE_PERSISTENCE &&
			container.frame_range() == FRAME_RANGE_PERSISTENCE)
		return true;
	/*
	 * Frame interval checking. Wrapping checking is needed.
	 */
	if (((_frame_start - container._frame_start) & FRAME_BIT_MASK)
			>= FRAME_DIFF_MAX ||
			(((container._frame_start + container._frame_range)
			  - (_frame_start + _frame_range)) & FRAME_BIT_MASK)
			>= FRAME_DIFF_MAX)
		return false;

	return true;
}

void
Alloc_base::sync_frame_with_avail(const Alloc_base& avail)
{
	_frame_start = (avail._frame_start & ~MSH_DSCH::FRAME_BIT_MASK) |
		(_frame_start & MSH_DSCH::FRAME_BIT_MASK);

	if (((_frame_start - avail._frame_start) & FRAME_BIT_MASK) <
			FRAME_DIFF_MAX)
		return;

	_frame_start += FRAME_START_MAX;
}

bool
Alloc_base::sync_frame_with_grant(const Alloc_base& grant)
{
	if (!is_match_with_grant(grant))
		return false;

	_frame_start = grant._frame_start;
	return true;
}

bool
Alloc_base::operator==(const class Alloc_base& alloc) const
{
	if (_frame_start == alloc._frame_start &&
			_frame_range == alloc._frame_range &&
			_slot_start == alloc._slot_start &&
			_slot_range == alloc._slot_range)
		return true;

	return false;
}

bool
Alloc_base::operator<(const class Alloc_base& alloc) const
{
	return _slot_start < alloc._slot_start;
}

void
Alloc_base::dump(const char* tag) const
{
	printf("Alloc_base::%s: %s\n", __FUNCTION__, tag);
	printf("\tframe: [%#03x, %#03x](%03u), slot: [%03u, %03u](%03u)\n",
			_frame_start,
			_frame_start + _frame_range - (_frame_range?1:0),
			_frame_range,
			_slot_start,
			_slot_start + _slot_range - (_slot_range?1:0),
			_slot_range);
}

bool
Alloc_base::is_match_with_grant(const class Alloc_base& grant) const
{
        if ((_frame_start & MSH_DSCH::FRAME_BIT_MASK) !=
			(grant._frame_start & MSH_DSCH::FRAME_BIT_MASK))
		return false;

	return true;
}

