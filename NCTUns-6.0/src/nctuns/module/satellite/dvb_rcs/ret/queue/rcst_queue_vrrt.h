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

#ifndef __NCTUNS_rcst_queue_vrrt_h__
#define __NCTUNS_rcst_queue_vrrt_h__

#include <stdint.h>

#define	CRA	true
#define	RBDC	false
class Rcst_queue_basic;

/*
 * Rcst Queue for RT
 */
class Rcst_queue_vrrt : public Rcst_queue_basic {

private:
	/*
	 * private member
	 */
	uint32_t 	_cra_rate;
	uint32_t 	_max_rbdc_rate;
	uint32_t	_rbdc_timer; // The timer count down for RBDC time out.
	uint32_t	_last_rbdc; // The value of last RBDC slot submitted.
	Frame_ident	_last_rbdc_frame; // The last frame within which RBDC is applied.
	uint32_t	_rbdc_timeout; // In frame number.

	struct pkt_demand	*_cra_demand_head; // Head of CRA demand list.
	struct pkt_demand	*_cra_demand_tail; // Tail of CRA demand list.

private:
	/*
	 * private function
	 */
	void	_append_pkt_demand(bool cra, struct pkt_demand *demand);

	struct pkt_demand* _detach_demand_head(bool cra);
public:
	/*
	 * public member
	 */

public:
	/*
	 * public function
	 */
	Rcst_queue_vrrt(uint32_t queue_id, uint16_t priority, uint32_t qlen, uint32_t cra, 
			uint32_t max_rbdc, uint32_t timeout, Ncc_config* ncc_config);

	~Rcst_queue_vrrt();

	inline uint32_t		get_max_rbdc(){ return _max_rbdc_rate;}

	inline uint32_t		get_cra_rate(){ return _cra_rate;}

	virtual uint32_t	compute_demand(rcst_queue_demand_type type, uint32_t amount, 
					       Frame_ident frame, double frame_period, uint32_t msl);

	virtual int	grant_demand(bool cra, const uint32_t amount,
				     const uint16_t superframe_count, 
				     const uint8_t frame_id);

	uint32_t get_first_require_timeslot(bool cra);
	uint32_t get_first_require_timeslot(bool cra, Frame_ident& frame);

	pkt_demand*	_search_demand(bool cra, Frame_ident target_frame);
}; 

#endif	/* __NCTUNS_rcst_queue_vrrt_h__ */
