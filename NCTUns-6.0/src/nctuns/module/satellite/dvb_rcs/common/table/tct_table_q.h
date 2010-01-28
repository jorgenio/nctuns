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

#ifndef __NCTUNS_tct_table_q_h__
#define __NCTUNS_tct_table_q_h__

#include <mylist.h>
#include <stdint.h>
#include "table.h"
#include "tct_table.h"

/* 
 * Define the entry of tct queue. 
 */
struct Tct_entry {

	CIRCLEQ_ENTRY(Tct_entry)		entries;
	Tct					*tct;
	
	/* 
	 * Constructors
	 */
	Tct_entry();
	Tct_entry(Tct* set_tct) { tct = set_tct; }

	/* Destructor */
	~Tct_entry(){ free(); }

	inline void				free();
};
 
void Tct_entry::free() {

        delete tct;
}

struct Tct_circleq {

	struct Tct_entry		*cqh_first;		// first element
	struct Tct_entry		*cqh_last;		// last element
	
	/*
	 * function
	 */
	Tct_circleq();
	~Tct_circleq();
	void				free();
	int				add_tct(Tct* added_tct);
	Tct*				get_tct(u_char ver_num , uint16_t net_id);		
	Tct*				get_sequencing_tct(int sequence);		
	int				remove_tct(u_char ver_num , uint16_t net_id);

 private:
	Tct_entry*			get_tct_entry(u_char ver_num , uint16_t net_id);		
};

#endif /* __NCTUNS_tct_table_q_h__ */
