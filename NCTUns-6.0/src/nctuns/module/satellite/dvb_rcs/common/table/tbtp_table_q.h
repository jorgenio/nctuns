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

#ifndef __NCTUNS_tbtp_table_q_h__
#define __NCTUNS_tbtp_table_q_h__

#include <mylist.h>
#include <stdint.h>
#include "table.h"
#include "tbtp_table.h"

/* 
 * Define the entry of tbtp queue. 
 */
struct Tbtp_entry {

	CIRCLEQ_ENTRY(Tbtp_entry)		entries;
	Tbtp					*tbtp;
	
	/* 
	 * Constructors
	 */
	Tbtp_entry();
	Tbtp_entry(Tbtp* set_tbtp) { tbtp = set_tbtp; }

	/* Destructor */
	~Tbtp_entry(){ free(); }

	inline void				free();
};
 
void Tbtp_entry::free() {

        delete tbtp;
}

struct Tbtp_circleq {

	struct Tbtp_entry		*cqh_first;		// first element
	struct Tbtp_entry		*cqh_last;		// last element
	
	/*
	 * function
	 */
	Tbtp_circleq();
	~Tbtp_circleq();
	void				free();
	int				add_tbtp(Tbtp* added_tbtp);
	Tbtp*				get_tbtp(u_char ver_num , uint16_t net_id , uint8_t group_id , uint16_t superframe_count);		
	Tbtp*				get_sequencing_tbtp(int sequence);		
	int				remove_tbtp(u_char ver_num , uint16_t net_id , uint8_t group_id , uint16_t superframe_count);

 private:
	Tbtp_entry*			get_tbtp_entry(u_char ver_num , uint16_t net_id , uint8_t group_id , uint16_t superframe_count);		
};

#endif /* __NCTUNS_tbtp_table_q_h__ */
