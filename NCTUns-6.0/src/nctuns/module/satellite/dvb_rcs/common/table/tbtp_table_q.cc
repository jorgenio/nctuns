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

#include "tbtp_table_q.h"

/*
 * constructor
 */
Tbtp_circleq::Tbtp_circleq() {

	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Tbtp_circleq::~Tbtp_circleq() {

	free();
}

/*
 * free function
 */
void
Tbtp_circleq::free() {

	Tbtp_entry *tbtp_entry = NULL;	

	while (cqh_first != (Tbtp_entry*)this) {

		tbtp_entry = cqh_first;
		CIRCLEQ_REMOVE(this , tbtp_entry , entries);
		delete tbtp_entry;
	}
}

/*
 * add_tbtp function
 */
int
Tbtp_circleq::add_tbtp(Tbtp* added_tbtp) {

	Tbtp_entry *new_tbtp_entry = NULL;

	/* clone original table and insert it to tbtp circleq */
	new_tbtp_entry = new Tbtp_entry(added_tbtp->copy());

	if (!new_tbtp_entry)
			return -1;

	/* insert this tbtp_entry into head of tbtp circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_tbtp_entry , entries);
		return 0;
	}
}

/*
 * get_tbtp function
 */
Tbtp*
Tbtp_circleq::get_tbtp(u_char ver_num , uint16_t net_id , uint8_t group_id , uint16_t superframe_count) {

	Tbtp_entry *tbtp_entry = NULL;

	tbtp_entry = get_tbtp_entry(ver_num , net_id , group_id , superframe_count);

	if (!tbtp_entry)
		return NULL;

	/* return clone tbtp*/
	else
		return ((tbtp_entry->tbtp)->copy());
}

/*
 * remove_tbtp function
 */
int
Tbtp_circleq::remove_tbtp(u_char ver_num , uint16_t net_id , uint8_t group_id , uint16_t superframe_count) {

	Tbtp_entry	*rm_tbtp_entry = NULL;

	rm_tbtp_entry = get_tbtp_entry(ver_num , net_id , group_id , superframe_count);

	if (rm_tbtp_entry){

		CIRCLEQ_REMOVE(this , rm_tbtp_entry , entries);
		delete rm_tbtp_entry;
		return 0;
	}
	else
		return -1;
}

/*
 * get_tbtp_entry function
 */
Tbtp_entry*
Tbtp_circleq::get_tbtp_entry(u_char ver_num , uint16_t net_id , uint8_t group_id , uint16_t superframe_count) {

	Tbtp_entry *tmp = NULL;
	
	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* tbtp version matched */
		if ((tmp->tbtp)->get_version_number() == ver_num) {

			/* net_id and group_id and superframe count matched */
			if (((tmp->tbtp)->get_network_id() == net_id) && ((tmp->tbtp)->get_group_id() == group_id) && ((tmp->tbtp)->get_superframe_count() == superframe_count))
				return tmp;
		}
	}
	return NULL;
}

/*
 * get_sequencing_tbtp function
 */
Tbtp*
Tbtp_circleq::get_sequencing_tbtp(int sequence) {

	Tbtp_entry *tmp = NULL;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* tbtp sequence matched */
		if (seq_count == sequence) {

			return (tmp->tbtp)->copy();
		}
		seq_count++;
	}
	return NULL;
}
