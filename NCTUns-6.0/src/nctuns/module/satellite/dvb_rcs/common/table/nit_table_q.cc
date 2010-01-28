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

#include <stdlib.h>
#include "nit_table_q.h"

/*
 * constructor
 */
Nit_circleq::Nit_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Nit_circleq::~Nit_circleq() {

	free();
}

/*
 * free function
 */
void
Nit_circleq::free() {

	Nit_entry *nit_entry = NULL;	

	while (cqh_first != (Nit_entry*)this) {

		nit_entry = cqh_first;
		CIRCLEQ_REMOVE(this , nit_entry , entries);
		delete nit_entry;
	}
}

/*
 * add_nit function
 */
int
Nit_circleq::add_nit(Nit* added_nit) {

	Nit_entry *new_nit_entry = NULL;

	/* clone original table and insert it to nit circleq */
	new_nit_entry = new Nit_entry(added_nit->copy());

	if (!new_nit_entry)
			return -1;

	/* insert this nit_entry into head of nit circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_nit_entry , entries);
		return 0;
	}
}

/*
 * get_nit function
 */
Nit*
Nit_circleq::get_nit(u_char ver_num , uint16_t intact_net_id) {

	Nit_entry *nit_entry = NULL;

	nit_entry = get_nit_entry(ver_num , intact_net_id);

	if (!nit_entry)
		return NULL;

	/* return clone nit*/
	else
		return ((nit_entry->nit)->copy());
}

/*
 * remove_nit function
 */
int
Nit_circleq::remove_nit(u_char ver_num , uint16_t intact_net_id) {

	Nit_entry	*rm_nit_entry = NULL;

	rm_nit_entry = get_nit_entry(ver_num , intact_net_id);

	if (rm_nit_entry){

		CIRCLEQ_REMOVE(this , rm_nit_entry , entries);
		delete rm_nit_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_nit_entry function
 */
Nit_entry*
Nit_circleq::get_nit_entry(u_char ver_num , uint16_t intact_net_id) {

	Nit_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* nit version matched */
		if ((tmp->nit)->get_version_number() == ver_num) {

			/* nit matched */
			if ((tmp->nit)->get_network_id() == intact_net_id)
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_nit function
 */
Nit*
Nit_circleq::get_sequencing_nit(int sequence) {

	Nit_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* nit sequence matched */
		if (seq_count == sequence) {

			return (tmp->nit)->copy();
		}
		seq_count++;
	}
	return NULL;
}
