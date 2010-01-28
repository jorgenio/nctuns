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

#ifndef __NCTUNS_ncc_config__
#define __NCTUNS_ncc_config__

#include <stdint.h>
#include <sys/types.h>
#include <iostream>
#include <list>
#include <math.h>
#include <assert.h>
#include <nctuns_api.h>

#include <satellite/dvb_rcs/common/descriptor/common.h>
#include <satellite/dvb_rcs/common/sch_info.h>

//using namespace std;
using std::cout;
using std::endl;
using std::list;
using std::ostream;
#define	MAX_SUPERFRAME_ID	254
#define	MAX_SUPERFRAME_COUNT	65535
#define NUM_REQ_SLOT_FOR_EACH_RCST	5

struct Frame_ident {
	uint16_t	superframe_count;
	uint8_t		frame_number;

	Frame_ident(uint16_t s_count, uint8_t f_number) : 
		superframe_count(s_count), frame_number(f_number){}

	Frame_ident(){}
};

struct Ncc_config
{
	/******************** Public Data ***************************/
	/* Information obtained from configuration file */
	uint32_t		sat_node_id;
	uint16_t		max_superframe_id;
	uint32_t		min_frequency;
	uint64_t		symbol_rate;
	double			roll_off_factor;
	uint16_t		num_of_atm_per_slot;
	uint16_t		num_of_data_slot_per_frame;
	uint16_t		num_of_req_slot_per_frame;
	uint16_t		num_of_slot_per_frame;
	uint16_t		num_of_frame_per_superframe;
	uint8_t			preamble_length;
	enum fec_inner_coding_ratio	fec_inner_coding_ratio;
	enum modulation_type_id	modulation_type_id;
	uint16_t		modulation_rate; // in bits/symbol.
	bool			fca_turned_on;
	uint16_t		burst_start_symbol_len; // in symbols.

	uint16_t		max_group_id;

	/* Anothers */
	uint64_t		timeslot_duration, frame_duration, superframe_duration;


	/********************* Public Functions ***********************/
	uint32_t		bandwidth_per_superframe();
	uint64_t		burst_start_offset();
	uint32_t		bps_to_spf(uint32_t bps);
	uint32_t		atm_to_bps(uint32_t amount_atm_cell);
	uint32_t		spf_to_bps(uint32_t spf);
	uint32_t		channel_capacity();
	uint32_t		channel_centre_frequency(uint16_t superframe_id);

	Frame_ident		next_frame(Frame_ident frame);

	Frame_ident		frame_add(const Frame_ident& frame, uint32_t num);

	uint32_t		frame_minus(const Frame_ident& f1, const Frame_ident& f2);

	uint64_t		superframe_start(uint64_t ref_time, uint16_t superframe_count);

	uint64_t		frame_start(uint64_t ref_time, uint16_t superframe_count, 
					    uint8_t frame_number);

	uint64_t		timeslot_start(uint64_t ref_time, uint16_t superframe_count, 
					       uint8_t frame_number, uint16_t timeslot_number);

	void			current_slot(uint64_t current_time, uint16_t &superframe_count, 
					     uint8_t &frame_number, uint16_t &timeslot_number);

	Frame_ident		current_frame();
};


/********************* Global Function **********************/
bool operator== (const Frame_ident& f1, const Frame_ident& f2);

#endif /* __NCTUNS_ncc_config__ */
