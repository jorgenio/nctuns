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

#ifndef __NCTUNS_rcst_queue_jt_h__
#define __NCTUNS_rcst_queue_jt_h__

#include <stdint.h>

class Rcst_queue_basic;

/*
 * Rcst Queue for RT
 */
class Rcst_queue_jt : public Rcst_queue_basic {

private:
	/*
	 * private member
	 */
	double		_max_vbdc_rate;
	uint32_t	_requested_before_msl; // Amount of data which has sended request before MSL.

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
	Rcst_queue_jt(uint32_t queue_id, uint16_t priority, uint32_t qlen, double max_vbdc, Ncc_config* config);

	~Rcst_queue_jt();

	void		grant_demand_init();

	uint32_t	compute_demand(rcst_queue_demand_type type, uint32_t amount, 
				       Frame_ident frame, double frame_period,
				       uint32_t msl, bool request_lost);

	virtual int	grant_demand(const uint32_t amount,const uint16_t superframe_count, 
				     const uint8_t frame_id);

	uint32_t	sum_old_request();
	
	void		update_demand_list(uint16_t superframe_count, uint8_t frame_number);
}; 

#endif	/* __NCTUNS_rcst_queue_jt_h__ */
