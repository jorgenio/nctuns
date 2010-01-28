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

#include "fct_table_q.h"

/*
 * constructor
 */
Fct_circleq::Fct_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Fct_circleq::~Fct_circleq() {

	free();
}

/*
 * free function
 */
void
Fct_circleq::free() {

	Fct_entry *fct_entry = NULL;	

	while (cqh_first != (Fct_entry*)this) {

		fct_entry = cqh_first;
		CIRCLEQ_REMOVE(this , fct_entry , entries);
		delete fct_entry;
	}
}

/*
 * add_fct function
 */
int
Fct_circleq::add_fct(Fct* added_fct) {

	Fct_entry *new_fct_entry = NULL;

	/* clone original table and insert it to fct circleq */
	new_fct_entry = new Fct_entry(added_fct->copy());

	if (!new_fct_entry)
			return -1;

	/* insert this fct_entry into head of fct circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_fct_entry , entries);
		return 0;
	}
}

/*
 * get_fct function
 */
Fct*
Fct_circleq::get_fct(u_char ver_num , uint16_t net_id) {

	Fct_entry *fct_entry = NULL;

	fct_entry = get_fct_entry(ver_num , net_id);

	if (!fct_entry)
		return NULL;

	/* return clone fct*/
	else
		return ((fct_entry->fct)->copy());
}

/*
 * remove_fct function
 */
int
Fct_circleq::remove_fct(u_char ver_num , uint16_t net_id) {

	Fct_entry	*rm_fct_entry = NULL;

	rm_fct_entry = get_fct_entry(ver_num , net_id);

	if (rm_fct_entry){

		CIRCLEQ_REMOVE(this , rm_fct_entry , entries);
		delete rm_fct_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_fct_entry function
 */
Fct_entry*
Fct_circleq::get_fct_entry(u_char ver_num , uint16_t net_id) {

	Fct_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* fct version matched */
		if ((tmp->fct)->get_version_number() == ver_num) {

			/* fct matched */
			if ((tmp->fct)->get_network_id() == net_id)
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_fct function
 */
Fct*
Fct_circleq::get_sequencing_fct(int sequence) {

	Fct_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* fct sequence matched */
		if (seq_count == sequence) {

			return (tmp->fct)->copy();
		}
		seq_count++;
	}
	return NULL;
}
