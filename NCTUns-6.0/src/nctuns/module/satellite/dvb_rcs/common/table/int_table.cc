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

#include "int_table.h"

Int::Int() {
	
	_table_id		= INT_TABLE_ID;
	_version_number 	= 0x00;
	_current_next_indicator	= 0x00;
	_platform_id		= 0;
	_processing_order	= 0x00;
	platform_des_loop	= NULL;
	for_loop_cnt		= 0;
	for_loop_head		= NULL;
	for_loop_tail		= NULL;

}

Int::Int(char vnum, char cur_nxt_ind, u_int32_t pfid, char porder) {

	_table_id		= INT_TABLE_ID;
	_version_number 	= vnum;
	_current_next_indicator	= cur_nxt_ind;
	_platform_id		= pfid;
	_processing_order	= porder;
	platform_des_loop	= NULL;
	for_loop_cnt		= 0;
	for_loop_head		= NULL;
	for_loop_tail		= NULL;

}

Int::~Int() {

	struct for_loop_entry *	del_entry;
	struct for_loop_entry * next_del_entry;
	u_int32_t		i;

	if(for_loop_cnt > 0) {
		del_entry = for_loop_head;
		next_del_entry = del_entry->next;
	
		for(i=0; i<for_loop_cnt && del_entry != NULL; i++) {
			next_del_entry = del_entry->next;
			delete del_entry->target_des_loop;
			delete del_entry->operational_des_loop;
			delete del_entry;
			del_entry = next_del_entry;
		}
	}

	if(platform_des_loop != NULL && platform_des_loop->descriptor_count > 0)
	{
		delete (platform_des_loop);
	}

}

Int * Int::copy() {

	class Int *		cpy_table;
	struct for_loop_entry *	tmp_entry;
	struct for_loop_entry * cpy_entry;
	u_int32_t		i;	

	cpy_table = new class Int(_version_number, _current_next_indicator, _platform_id, _processing_order);

	if(platform_des_loop != NULL) {
		cpy_table->platform_des_loop = platform_des_loop->copy();
	}

	if(for_loop_head != NULL) {
		tmp_entry = for_loop_head;
		
		for(i=0; i<for_loop_cnt && tmp_entry != NULL; i++) {
			cpy_entry = new struct for_loop_entry;
			cpy_entry->target_des_loop = tmp_entry->target_des_loop->copy();
			cpy_entry->operational_des_loop = tmp_entry->operational_des_loop->copy();
			cpy_entry->prev = NULL;
			cpy_entry->next = NULL;

			if(cpy_table->for_loop_head == NULL) {
				cpy_table->for_loop_head = cpy_entry;
				cpy_table->for_loop_tail = cpy_entry;
			}
			else {
				cpy_table->for_loop_tail->next = cpy_entry;
				cpy_entry->prev = cpy_table->for_loop_tail;
				cpy_table->for_loop_tail = cpy_entry;
			}

			cpy_table->for_loop_cnt++;			

			tmp_entry = tmp_entry->next;
		}
	}
	
	return cpy_table;
}

void Int::add_platform_descriptor(Descriptor * des) {

	if(platform_des_loop == NULL) { 
		platform_des_loop = new struct Descriptor_circleq;
		assert(platform_des_loop);
	}

	if(platform_des_loop->add_descriptor(des) < 0)
		assert(0);
}

void Int::add_target_and_operational_descriptor(Descriptor * tar, Descriptor * ope) {

	struct for_loop_entry *		tmp_entry;
	
	tmp_entry = new struct for_loop_entry;
	tmp_entry->target_des_loop = new struct Descriptor_circleq;
	tmp_entry->operational_des_loop = new struct Descriptor_circleq;
	if(tmp_entry->target_des_loop->add_descriptor(tar) < 0)
		assert(0);
	if(tmp_entry->operational_des_loop->add_descriptor(ope) < 0)
		assert(0);

	tmp_entry->prev = NULL;
	tmp_entry->next = NULL;

	if(for_loop_head == NULL) {
		for_loop_head = tmp_entry;
		for_loop_tail = tmp_entry;
	}
	else {
		for_loop_tail->next = tmp_entry;
		tmp_entry->prev = for_loop_tail;
		for_loop_tail = tmp_entry;
	}
	
	for_loop_cnt++;
		
}

void Int::remove_platform_descriptor(u_int8_t tag) {
	
	if(platform_des_loop->remove_descriptor(1, tag) < 0) 
		assert(0);
}


Descriptor * Int::get_platform_descriptor(u_int8_t tag) {

	return platform_des_loop->get_descriptor(1, tag);
}

Descriptor * Int::get_operational_descriptor_by_IP_address(u_int8_t tag, u_long ip) {
	
	int			location_num;
	struct for_loop_entry	*tmp_entry;
	u_int32_t		i;

	for(i=0, tmp_entry = for_loop_head; 
	    i<for_loop_cnt && tmp_entry != NULL; 
	    i++, tmp_entry = tmp_entry->next) {
		if(tmp_entry->target_des_loop->get_descriptor(2, tag, ip) != NULL) {
			location_num = tmp_entry->target_des_loop->get_location_num(2, tag, ip);
			return tmp_entry->operational_des_loop->get_descriptor_by_location_num(location_num);
		}
	}
	return NULL;
}

