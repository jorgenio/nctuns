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

#ifndef __NCTUNS_rcst_queue_ctrl_h__
#define __NCTUNS_rcst_queue_ctrl_h__

#include <stdint.h>

class Rcst_queue_basic;

/*
 * Rcst Queue for RT
 */
class Rcst_queue_ctrl : public Rcst_queue_basic {

private:
	/*
	 * private member
	 */

private:
	/*
	 * private function
	 */
public:
	/*
	 * public member
	 */

public:
	/*
	 * public function
	 */
	Rcst_queue_ctrl(uint32_t qlen, Ncc_config* ncc_config);
	~Rcst_queue_ctrl();

	int	push(Dvb_pkt *pkt);
	int	grant_demand(const uint32_t amount, int extra);
	struct	slot_info *get_timeslot();
}; 

#endif	/* __NCTUNS_rcst_queue_ctrl_h__ */
