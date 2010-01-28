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

#ifndef __NCTUNS_pat_table_q_h__
#define __NCTUNS_pat_table_q_h__

#include <mylist.h>
#include <stdint.h>
#include "table.h"
#include "pat_table.h"

/* 
 * Define the entry of pat queue. 
 */
struct Pat_entry {

	CIRCLEQ_ENTRY(Pat_entry)		entries;
	Pat					*pat;
	
	/* 
	 * Constructors
	 */
	Pat_entry();
	Pat_entry(Pat* set_pat) { pat = set_pat; }

	/* Destructor */
	~Pat_entry(){ free(); }

	inline void				free();
};
 
void Pat_entry::free() {

        delete pat;
}

struct Pat_circleq {

	struct Pat_entry		*cqh_first;		// first element
	struct Pat_entry		*cqh_last;		// last element
	
	/*
	 * function
	 */
	Pat_circleq();
	~Pat_circleq();
	void				free();
	int				add_pat(Pat* added_pat);
	Pat*				get_pat(u_char ver_num , uint16_t ts_id);		
	Pat*				get_sequencing_pat(int sequence);		
	int				remove_pat(u_char ver_num , uint16_t ts_id);

 private:
	Pat_entry*			get_pat_entry(u_char ver_num , uint16_t ts_id);		
};

#endif /* __NCTUNS_pat_table_q_h__ */
