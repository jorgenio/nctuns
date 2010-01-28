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

#ifndef __NCTUNS_fct_table_q_h__
#define __NCTUNS_fct_table_q_h__

#include <mylist.h>
#include <stdint.h>
#include "fct_table.h"

/* 
 * Define the entry of fct queue. 
 */
struct Fct_entry {

	CIRCLEQ_ENTRY(Fct_entry)		entries;
	Fct					*fct;
	
	/* 
	 * Constructors
	 */
	Fct_entry();
	Fct_entry(Fct* set_fct) { fct = set_fct; }

	/* Destructor */
	~Fct_entry(){ free(); }

	inline void				free();
};
 
void Fct_entry::free() {

        delete fct;
}

struct Fct_circleq {

	struct Fct_entry		*cqh_first;		// first element
	struct Fct_entry		*cqh_last;		// last element
	
	/*
	 * function
	 */
	Fct_circleq();
	~Fct_circleq();
	void				free();
	int				add_fct(Fct* added_fct);
	Fct*				get_fct(u_char ver_num , uint16_t net_id);		
	Fct*				get_sequencing_fct(int sequence);		
	int				remove_fct(u_char ver_num , uint16_t net_id);

 private:
	Fct_entry*			get_fct_entry(u_char ver_num , uint16_t net_id);		
};

#endif /* __NCTUNS_fct_table_q_h__ */
