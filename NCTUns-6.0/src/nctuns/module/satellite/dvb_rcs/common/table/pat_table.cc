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

#include "pat_table.h"

void Pat_ass_info_circleq::free() {
	Pat_ass_info_entry*		_entry;
	while(this-> cqh_first != ((Pat_ass_info_entry*) this))
	{	
		_entry = this-> cqh_first;	
		CIRCLEQ_REMOVE(this, _entry, entries);
		delete _entry;
	}														
}

void Pat_ass_info_circleq::copy(Pat_ass_info_circleq* dst,
		Pat_ass_info_circleq* src) 
{
	Pat_ass_info_entry		*_dst_ass_info_entry, *_src_ass_info_entry;
	CIRCLEQ_INIT(dst);						
	CIRCLEQ_FOREACH(_src_ass_info_entry, src, entries)
	{
		_dst_ass_info_entry = new Pat_ass_info_entry(*_src_ass_info_entry);
		CIRCLEQ_INSERT_HEAD(dst, _dst_ass_info_entry, entries);		
	}
}

Pat::Pat() {
	_table_id = PAT_TABLE_ID;
	CIRCLEQ_INIT(&_pat_ass_info_circleq);
}


Pat::Pat(u_int16_t ts_id, u_char version_number, 
							u_char current_next_indicator) {
	_table_id = PAT_TABLE_ID;
	CIRCLEQ_INIT(&_pat_ass_info_circleq);
	_transport_stream_id = ts_id;
	_version_number = version_number;
	_current_next_indicator = current_next_indicator;
}

Pat::~Pat() {
	// Free the dynamically allocated queue.
	_pat_ass_info_circleq.free();
}

Pat* Pat::copy() {
	Pat*		clone;

	// Copy all fields but the dynamically allocated queue.
	clone = new Pat(*this);

	Pat_ass_info_circleq::copy(&(clone-> _pat_ass_info_circleq), 
			&(this-> _pat_ass_info_circleq));
	
	return clone;
}

int
Pat::get_pat_ass_info(u_int16_t program_number, Pat_ass_info *ass_info_buf) {

	Pat_ass_info_entry*		_ass_info_entry;

	CIRCLEQ_FOREACH(_ass_info_entry, &_pat_ass_info_circleq, entries) {	
		
		/* found */
		if(_ass_info_entry->get_program_number() == program_number) {
			*ass_info_buf = _ass_info_entry->ass_info;
			return 1;
		}
	}
	/* not found */
	return -1;
}

int Pat::add_pat_ass_info(u_int16_t program_number,u_int16_t pid) {
	Pat_ass_info_entry			*_ass_info_entry, *_new_entry;

	// Check if there already exists the same information entry.
	CIRCLEQ_FOREACH(_ass_info_entry, &_pat_ass_info_circleq, entries)
	{
		if(program_number==(_ass_info_entry-> get_program_number()) &&
			pid==(_ass_info_entry-> get_program_map_pid()))
		{
			return 1;
		}
	}
	
	_new_entry = new Pat_ass_info_entry(program_number, pid);
	CIRCLEQ_INSERT_HEAD(&_pat_ass_info_circleq,
		_new_entry, entries); 

	return 0;
}

int	Pat::remove_pat_ass_info(u_int16_t program_number) {
	Pat_ass_info_entry			*_ass_info_entry;

	CIRCLEQ_FOREACH(_ass_info_entry, &_pat_ass_info_circleq, entries)
	{
		if(program_number==(_ass_info_entry-> get_program_number()))
		{
			CIRCLEQ_REMOVE(&_pat_ass_info_circleq, _ass_info_entry, entries);
			delete (_ass_info_entry);
		}
	}

	return 0;
}


/*
 * get_PMT_pid function
 * add by bryan in 06/6/6
 */
int16_t
Pat::get_PMT_pid(u_int16_t pro_num) {

	Pat_ass_info	tmp_pat_ass_info;

	/* not found */
	if (get_pat_ass_info(pro_num , &tmp_pat_ass_info) == -1)
		return -1;

	else
		return tmp_pat_ass_info.net_pro_pid;
}
