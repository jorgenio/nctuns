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

#ifndef __NCTUNS_WIMAX_BITMAP_H__
#define __NCTUNS_WIMAX_BITMAP_H__

#include <stdint.h>

#include "alloc_base.h"


class Bitmap {

private:
	enum {
		FRAME_MAX = Alloc_base::FRAME_START_MAX +
			Alloc_base::PERSISTENCE_MAX,
	};

public:
	typedef enum search_policy {
		MINISLOT,
		FRAME,
	} search_policy_t;

private:
	bool*		_bitmap;
	const uint16_t	_nf_frame	:12;
	const uint8_t	_nf_slot;
	uint16_t	_frame_start_max;
	uint16_t	_frame_base	:12;

	uint32_t	_frame_start;
	uint32_t	_frame_end;
	uint8_t		_slot_start;
	uint8_t		_slot_end;

	bool		_debug_mode;
	bool		_frame_persistent_flag;

	bool		_range_check(uint16_t frame_start, uint8_t slot_start, uint16_t& frame_range, uint8_t& slot_range);

	/*
	 * 'needed_area = 0' means don't care this condition.
	 */
	bool		_prefer_minislot(Alloc_base* ret_free_alloc,
					uint16_t frame_len, uint8_t slot_len, size_t needed_area);
	bool		_prefer_frame(Alloc_base* ret_free_alloc,
					uint16_t frame_len, uint8_t slot_len, size_t needed_area);
 	bool 		_block_check_all_unused(uint32_t frame_start, uint8_t slot_start, uint32_t frame_range, uint8_t slot_range);
	void		_dump_bounary_info();
 	void		_dump_incoming_info(uint32_t frame_start, uint8_t slot_start, uint32_t frame_range, uint8_t slot_range);
	void		_adjust_free_alloc_by_needed_area(class Alloc_base& ret_free_alloc,
								size_t needed_area);
	int		_get_nf_free_slot(uint32_t frame_start, uint8_t slot_start,
					uint32_t frame_end, uint8_t slot_end);

public:
	Bitmap(uint8_t, uint16_t);
	virtual ~Bitmap();

	bool bit_get(uint16_t frame, uint8_t slot);
	void bit_set(uint16_t frame_start, uint8_t slot_start, uint16_t frame_range, uint8_t slot_range);
	void bit_clr(uint16_t frame_start, uint8_t slot_start, uint16_t frame_range, uint8_t slot_range);

	void init(uint16_t frame_base);
	int cumulate(const Alloc_base& add_alloc);

	/*
	 * 'needed_area = 0' means don't care this condition.
	 */
	bool get_free_alloc(class Alloc_base&, search_policy_t, uint16_t,
				uint8_t, size_t needed_area = 0);
	bool set_boundary(uint16_t frame_start = 0, uint8_t slot_start = 0,
		uint16_t frame_range = 0, uint8_t slot_range = 0);
	void dump();
	void set_debug_mode();
};

#endif	/* __NCTUNS_WIMAX_BITMAP_H__ */
