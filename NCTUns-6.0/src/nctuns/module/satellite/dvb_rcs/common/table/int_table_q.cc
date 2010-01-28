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

#include "int_table_q.h"

/*
 * constructor
 */
Int_circleq::Int_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Int_circleq::~Int_circleq() {

	free();
}

/*
 * free function
 */
void
Int_circleq::free() {

	Int_entry *int_entry = NULL;	

	while (cqh_first != (Int_entry*)this) {

		int_entry = cqh_first;
		CIRCLEQ_REMOVE(this , int_entry , entries);
		delete int_entry;
	}
}

/*
 * add_int function
 */
int
Int_circleq::add_int(Int* added_int) {

	Int_entry *new_int_entry = NULL;

	/* clone original table and insert it to int circleq */
	new_int_entry = new Int_entry(added_int->copy());

	if (!new_int_entry)
			return -1;

	/* insert this int_entry into head of int circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_int_entry , entries);
		return 0;
	}
}

/*
 * get_int function
 */
Int*
Int_circleq::get_int(u_char ver_num , uint8_t platform_id) {

	Int_entry *int_entry = NULL;

	int_entry = get_int_entry(ver_num , platform_id);

	if (!int_entry)
		return NULL;

	/* return clone int*/
	else
		return ((int_entry->int_)->copy());
}

/*
 * remove_int function
 */
int
Int_circleq::remove_int(u_char ver_num , uint8_t platform_id) {

	Int_entry	*rm_int_entry = NULL;

	rm_int_entry = get_int_entry(ver_num , platform_id);

	if (rm_int_entry){

		CIRCLEQ_REMOVE(this , rm_int_entry , entries);
		delete rm_int_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_int_entry function
 */
Int_entry*
Int_circleq::get_int_entry(u_char ver_num , uint8_t platform_id) {

	Int_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* int version matched */
		if ((tmp->int_)->get_version_number() == ver_num) {

			/* int matched */
			if ((tmp->int_)->get_platform_id() == platform_id)
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_int function
 */
Int*
Int_circleq::get_sequencing_int(int sequence) {

	Int_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* int sequence matched */
		if (seq_count == sequence) {

			return (tmp->int_)->copy();
		}
		seq_count++;
	}
	return NULL;
}
