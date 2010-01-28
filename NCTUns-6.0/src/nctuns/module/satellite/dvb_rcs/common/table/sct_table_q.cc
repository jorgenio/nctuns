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

#include "sct_table_q.h"

/*
 * constructor
 */
Sct_circleq::Sct_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Sct_circleq::~Sct_circleq() {

	free();
}

/*
 * free function
 */
void
Sct_circleq::free() {

	Sct_entry *sct_entry = NULL;	

	while (cqh_first != (Sct_entry*)this) {

		sct_entry = cqh_first;
		CIRCLEQ_REMOVE(this , sct_entry , entries);
		delete sct_entry;
	}
}

/*
 * add_sct function
 */
int
Sct_circleq::add_sct(Sct* added_sct) {

	Sct_entry *new_sct_entry = NULL;

	/* clone original table and insert it to sct circleq */
	new_sct_entry = new Sct_entry(added_sct->copy());

	if (!new_sct_entry)
			return -1;

	/* insert this sct_entry into head of sct circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_sct_entry , entries);
		return 0;
	}
}

/*
 * get_sct function
 */
Sct*
Sct_circleq::get_sct(u_char ver_num , uint16_t intact_net_id) {

	Sct_entry *sct_entry = NULL;

	sct_entry = get_sct_entry(ver_num , intact_net_id);

	if (!sct_entry)
		return NULL;

	/* return clone sct*/
	else
		return ((sct_entry->sct)->copy());
}

/*
 * remove_sct function
 */
int
Sct_circleq::remove_sct(u_char ver_num , uint16_t intact_net_id) {

	Sct_entry	*rm_sct_entry = NULL;

	rm_sct_entry = get_sct_entry(ver_num , intact_net_id);

	if (rm_sct_entry){

		CIRCLEQ_REMOVE(this , rm_sct_entry , entries);
		delete rm_sct_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_sct_entry function
 */
Sct_entry*
Sct_circleq::get_sct_entry(u_char ver_num , uint16_t intact_net_id) {

	Sct_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* sct version matched */
		if ((tmp->sct)->get_version_number() == ver_num) {

			/* sct matched */
			if ((tmp->sct)->get_network_id() == intact_net_id)
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_sct function
 */
Sct*
Sct_circleq::get_sequencing_sct(int sequence) {

	Sct_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* sct sequence matched */
		if (seq_count == sequence) {

			return (tmp->sct)->copy();
		}
		seq_count++;
	}
	return NULL;
}
