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

#include "pat_table_q.h"

/*
 * constructor
 */
Pat_circleq::Pat_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Pat_circleq::~Pat_circleq() {

	free();
}

/*
 * free function
 */
void
Pat_circleq::free() {

	Pat_entry *pat_entry = NULL;	

	while (cqh_first != (Pat_entry*)this) {

		pat_entry = cqh_first;
		CIRCLEQ_REMOVE(this , pat_entry , entries);
		delete pat_entry;
	}
}

/*
 * add_pat function
 */
int
Pat_circleq::add_pat(Pat* added_pat) {

	Pat_entry *new_pat_entry = NULL;

	/* clone original table and insert it to pat circleq */
	new_pat_entry = new Pat_entry(added_pat->copy());

	if (!new_pat_entry)
			return -1;

	/* insert this pat_entry into head of pat circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_pat_entry , entries);
		return 0;
	}
}

/*
 * get_pat function
 */
Pat*
Pat_circleq::get_pat(u_char ver_num , uint16_t ts_id) {

	Pat_entry *pat_entry = NULL;

	pat_entry = get_pat_entry(ver_num , ts_id);

	if (!pat_entry)
		return NULL;

	/* return clone pat*/
	else
		return ((pat_entry->pat)->copy());
}

/*
 * remove_pat function
 */
int
Pat_circleq::remove_pat(u_char ver_num , uint16_t ts_id) {

	Pat_entry	*rm_pat_entry = NULL;

	rm_pat_entry = get_pat_entry(ver_num , ts_id);

	if (rm_pat_entry){

		CIRCLEQ_REMOVE(this , rm_pat_entry , entries);
		delete rm_pat_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_pat_entry function
 */
Pat_entry*
Pat_circleq::get_pat_entry(u_char ver_num , uint16_t ts_id) {

	Pat_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* pat version matched */
		if ((tmp->pat)->get_version_number() == ver_num) {

			/* pat matched */
			if ((tmp->pat)->get_transport_stream_id() == ts_id)
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_pat function
 */
Pat*
Pat_circleq::get_sequencing_pat(int sequence) {

	Pat_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* pat sequence matched */
		if (seq_count == sequence) {

			return (tmp->pat)->copy();
		}
		seq_count++;
	}
	return NULL;
}
