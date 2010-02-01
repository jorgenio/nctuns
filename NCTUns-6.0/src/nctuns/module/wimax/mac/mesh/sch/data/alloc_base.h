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

#ifndef __NCTUNS_WIMAX_ALLOC_BASE_H__
#define __NCTUNS_WIMAX_ALLOC_BASE_H__

#include <inttypes.h>


class Alloc_base {

public:
	enum {
		PERSISTENCE_MAX	= 128,
		FRAME_START_MAX	= 0x0100,
//		FRAME_DIFF_MAX	= PERSISTENCE_MAX + FRAME_START_MAX,
		FRAME_BIT_MASK	= 0x0fff,
		FRAME_DIFF_MAX	= FRAME_BIT_MASK >> 1,
		FRAME_RANGE_PERSISTENCE = 0x00ff,
		SLOT_RANGE_MAX	= 0x007f,
	};

private:
	uint16_t	_frame_start	:12;
	uint8_t		_frame_range;
	uint8_t		_slot_start;
	uint8_t		_slot_range;

public:
	Alloc_base(const class Alloc_base& alloc);
	Alloc_base(uint16_t frame_start = 0, uint8_t slot_start = 0,
			uint8_t frame_range = 0, uint8_t slot_range = 0);
	virtual ~Alloc_base();

	uint8_t frame_remain(uint16_t) const;
	bool is_active(uint16_t) const;
	bool is_in_between(const Alloc_base&) const;
	void sync_frame_with_avail(const Alloc_base&);
	bool sync_frame_with_grant(const Alloc_base&);

	virtual void dump(const char* tag = "") const;

	bool operator==(const class Alloc_base&) const;
	bool operator<(const class Alloc_base&) const;

	inline uint16_t frame_start() const { return _frame_start; }
	inline uint8_t frame_range() const { return _frame_range; }
	inline uint8_t slot_start() const { return _slot_start; }
	inline uint8_t slot_range() const { return _slot_range; }

	inline void frame_start(uint16_t start) { _frame_start = start; }
	inline void frame_range(uint8_t range) { _frame_range = range; }
	inline void slot_start(uint8_t start) { _slot_start = start; }
	inline void slot_range(uint8_t range) { _slot_range = range; }

	inline size_t area() const { return _frame_range * _slot_range; }

	inline bool operator==(size_t a) const { return (area() == a); }
	inline bool operator!=(size_t a) const { return (area() != a); }
	inline bool operator<=(size_t a) const { return (area() <= a); }
	inline bool operator>=(size_t a) const { return (area() >= a); }
	inline bool operator<(size_t a) const { return (area() < a); }
	inline bool operator>(size_t a) const { return (area() > a); }

	bool is_match_with_grant(const class Alloc_base&) const;
};


#endif	/* __NCTUNS_WIMAX_ALLOC_BASE_H__ */
