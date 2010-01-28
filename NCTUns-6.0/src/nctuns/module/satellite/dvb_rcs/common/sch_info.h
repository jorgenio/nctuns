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

#ifndef __NCTUNS_sch_info_h__
#define __NCTUNS_sch_info_h__

#include <stdint.h>

/*
 * inner_code_puncturing table
 */
/*
 * Code Rate		Value		K = 7		Turbo
 * --------------------------------------------------------------
 * 1/2			0000		x		x
 * 2/3			0001		x		x
 * 3/4			0010		x		x
 * 5/6			0011		x		Not used
 * 7/8			0100		x		Not used
 * 1/3			0101		Not used	x
 * 2/5			0110		Not used	x
 * 4/5			0111		Not used	x
 * 6/7			1000		Not used	x
 * Reserved		1001 to 1110
 * ommitted		1111		x		x
 * --------------------------------------------------------------
 * x means supported
 */

/*
 * inner_coding_puncture enumeration
 */
enum inner_coding_puncture {
	PUNCTURING_1_2		= 0x0000,
	PUNCTURING_2_3		= 0x0001,
	PUNCTURING_3_4		= 0x0002,
	PUNCTURING_5_6		= 0x0003,
	PUNCTURING_7_8		= 0x0004,
	PUNCTURING_1_3		= 0x0005,
	PUNCTURING_2_5		= 0x0006,
	PUNCTURING_4_5		= 0x0007,
	PUNCTURING_6_7		= 0x0008,
	PUNCTURING_OMITTED	= 0x000F
};

struct Slot_flags
{
	bool	route_id_flag;
	bool	request_flag;
	uint8_t capacity_request_number;
	bool	m_and_c_flag;
	bool	group_id_flag;
	bool	logon_id_flag;
	bool	acm_flag;
};


/*
 * slot information structure, this structure will be used in Dvb_pkt return information link
 */
struct slot_info {
	uint32_t	queue_id;
	uint16_t	superframe_count;
	uint8_t		frame_number;
	uint16_t        start_slot;
	uint64_t 	start_time; // Slot start time in ticks.
	uint64_t	burst_start_offset; // in ticks.

	uint8_t 	timeslot_amount;
	uint32_t	symbol_rate;
	uint8_t		timeslot_payload_type;
	uint8_t		preamble_length;
	uint32_t	min_frequency;
	uint32_t	max_frequency;
	uint8_t		inner_code_type;
	uint8_t		inner_code_ordering;
	uint8_t		outer_coding;
	inner_coding_puncture	inner_code_puncturing;
	uint8_t		new_permutation;
	uint8_t		group_id;
	uint8_t		logon_id;
	Slot_flags	flags;
};

#endif /* __NCTUNS_sch_info_h__ */
