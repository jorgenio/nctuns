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

#ifndef __NCTUNS_int_table_h__
#define __NCTUNS_int_table_h__

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <mylist.h>
#include <assert.h>
#include "table.h"
#include "../descriptor/descriptor_q.h"
#include "../si_config.h"

#define FIRST_ACTION	0x00

struct for_loop_entry {
	struct Descriptor_circleq *	target_des_loop;
	struct Descriptor_circleq *	operational_des_loop;

	struct for_loop_entry *		prev;
	struct for_loop_entry *		next;
};


class Int : public Table {
	friend class 			Int_table_to_section_handler;
	friend class 			Int_section_to_table_handler;

  private:
	u_char				_version_number;
	u_char				_current_next_indicator;
	u_int32_t			_platform_id;
	u_char				_processing_order;
	
	struct Descriptor_circleq * 	platform_des_loop;
	u_int32_t			for_loop_cnt;
	struct for_loop_entry *		for_loop_head;
	struct for_loop_entry *		for_loop_tail;

  public:
	Int();
	Int(char vnum, char cur_nxt_ind, u_int32_t pfid, char porder);
	~Int();
	
	Int *			copy();
	u_char			get_version_number() {return _version_number;}
	u_char			get_current_next_indicator() {return _current_next_indicator;}
	u_int32_t		get_platform_id() {return _platform_id;}
	u_char			get_processing_order() {return _processing_order;}

	void			set_version_number(u_char num) {_version_number = num;}
	void			set_current_next_indicator(u_char ind) {_current_next_indicator = ind;}
	void			set_platform_id(u_int32_t id) {_platform_id = id;}
	void			set_processing_order(u_char order) {_processing_order = order;}

	void			add_platform_descriptor(Descriptor * des);
	void			add_target_and_operational_descriptor(Descriptor * tar, Descriptor * ope);

	void			remove_platform_descriptor(u_int8_t tag);

	Descriptor *		get_platform_descriptor(u_int8_t tag);
	Descriptor *		get_operational_descriptor_by_IP_address(u_int8_t tag, u_long ip);
};

#endif /*__NCTUNS_int_table_h__*/
