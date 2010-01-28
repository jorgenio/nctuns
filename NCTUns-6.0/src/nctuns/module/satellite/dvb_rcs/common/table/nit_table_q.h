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

#ifndef __NCTUNS_nit_table_q_h__
#define __NCTUNS_nit_table_q_h__

#include <mylist.h>
#include <stdint.h>
#include "table.h"
#include "nit_table.h"

/* 
 * Define the entry of nit queue. 
 */
struct Nit_entry {

	CIRCLEQ_ENTRY(Nit_entry)		entries;
	Nit					*nit;
	
	/* 
	 * Constructors
	 */
	Nit_entry();
	Nit_entry(Nit* set_nit) { nit = set_nit; }

	/* Destructor */
	~Nit_entry(){ free(); }

	inline void				free();
};
 
void Nit_entry::free() {

        delete nit;
}

struct Nit_circleq {

	struct Nit_entry		*cqh_first;		// first element
	struct Nit_entry		*cqh_last;		// last element
	
	/*
	 * function
	 */
	Nit_circleq();
	~Nit_circleq();
	void				free();
	int				add_nit(Nit* added_nit);
	Nit*				get_nit(u_char ver_num , uint16_t net_id);		
	Nit*				get_sequencing_nit(int sequence);		
	int				remove_nit(u_char ver_num , uint16_t net_id);

 private:
	Nit_entry*			get_nit_entry(u_char ver_num , uint16_t net_id);		
};

#endif /* __NCTUNS_nit_table_q_h__ */
