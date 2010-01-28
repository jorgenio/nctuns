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

#include "tct_table_q.h"

/*
 * constructor
 */
Tct_circleq::Tct_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Tct_circleq::~Tct_circleq() {

	free();
}

/*
 * free function
 */
void
Tct_circleq::free() {

	Tct_entry *tct_entry = NULL;	

	while (cqh_first != (Tct_entry*)this) {

		tct_entry = cqh_first;
		CIRCLEQ_REMOVE(this , tct_entry , entries);
		delete tct_entry;
	}
}

/*
 * add_tct function
 */
int
Tct_circleq::add_tct(Tct* added_tct) {

	Tct_entry *new_tct_entry = NULL;

	/* clone original table and insert it to tct circleq */
	new_tct_entry = new Tct_entry(added_tct->copy());

	if (!new_tct_entry)
			return -1;

	/* insert this tct_entry into head of tct circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_tct_entry , entries);
		return 0;
	}
}

/*
 * get_tct function
 */
Tct*
Tct_circleq::get_tct(u_char ver_num , uint16_t net_id) {

	Tct_entry *tct_entry = NULL;

	tct_entry = get_tct_entry(ver_num , net_id);

	if (!tct_entry)
		return NULL;

	/* return clone tct*/
	else
		return ((tct_entry->tct)->copy());
}

/*
 * remove_tct function
 */
int
Tct_circleq::remove_tct(u_char ver_num , uint16_t net_id) {

	Tct_entry	*rm_tct_entry = NULL;

	rm_tct_entry = get_tct_entry(ver_num , net_id);

	if (rm_tct_entry){

		CIRCLEQ_REMOVE(this , rm_tct_entry , entries);
		delete rm_tct_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_tct_entry function
 */
Tct_entry*
Tct_circleq::get_tct_entry(u_char ver_num , uint16_t net_id) {

	Tct_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* tct version matched */
		if ((tmp->tct)->get_version_number() == ver_num) {

			/* tct matched */
			if ((tmp->tct)->get_network_id() == net_id)
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_tct function
 */
Tct*
Tct_circleq::get_sequencing_tct(int sequence) {

	Tct_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* tct sequence matched */
		if (seq_count == sequence) {

			return (tmp->tct)->copy();
		}
		seq_count++;
	}
	return NULL;
}
