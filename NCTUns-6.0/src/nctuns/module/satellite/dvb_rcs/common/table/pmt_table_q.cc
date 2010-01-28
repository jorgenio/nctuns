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

#include "pmt_table_q.h"

/*
 * constructor
 */
Pmt_circleq::Pmt_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Pmt_circleq::~Pmt_circleq() {

	free();
}

/*
 * free function
 */
void
Pmt_circleq::free() {

	Pmt_entry *pmt_entry = NULL;	

	while (cqh_first != (Pmt_entry*)this) {

		pmt_entry = cqh_first;
		CIRCLEQ_REMOVE(this , pmt_entry , entries);
		delete pmt_entry;
	}
}

/*
 * add_pmt function
 */
int
Pmt_circleq::add_pmt(Pmt* added_pmt) {

	Pmt_entry *new_pmt_entry = NULL;

	/* clone original table and insert it to pmt circleq */
	new_pmt_entry = new Pmt_entry(added_pmt->copy());

	if (!new_pmt_entry)
			return -1;

	/* insert this pmt_entry into head of pmt circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_pmt_entry , entries);
		return 0;
	}
}

/*
 * get_pmt function
 */
Pmt*
Pmt_circleq::get_pmt(u_char ver_num , uint16_t pro_num) {

	Pmt_entry *pmt_entry = NULL;

	pmt_entry = get_pmt_entry(ver_num , pro_num);

	if (!pmt_entry)
		return NULL;

	/* return clone pmt*/
	else
		return ((pmt_entry->pmt)->copy());
}

/*
 * remove_pmt function
 */
int
Pmt_circleq::remove_pmt(u_char ver_num , uint16_t pro_num) {

	Pmt_entry	*rm_pmt_entry = NULL;

	rm_pmt_entry = get_pmt_entry(ver_num , pro_num);

	if (rm_pmt_entry){

		CIRCLEQ_REMOVE(this , rm_pmt_entry , entries);
		delete rm_pmt_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_pmt_entry function
 */
Pmt_entry*
Pmt_circleq::get_pmt_entry(u_char ver_num , uint16_t pro_num) {

	Pmt_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* pmt version matched */
		if ((tmp->pmt)->get_version_number() == ver_num) {

			/* pmt matched */
			if ((tmp->pmt)->get_program_number() == pro_num)
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_pmt function
 */
Pmt*
Pmt_circleq::get_sequencing_pmt(int sequence) {

	Pmt_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* pmt sequence matched */
		if (seq_count == sequence) {

			return (tmp->pmt)->copy();
		}
		seq_count++;
	}
	return NULL;
}
