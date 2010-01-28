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

#include <assert.h>
#include <stdlib.h>
#include <satellite/dvb_rcs/common/sch_info.h>
#include <satellite/dvb_rcs/common/ncc_config.h>
#include <nctuns_api.h>
#include "rcst_queue_basic.h"
#include "rcst_queue_jt.h"
/*
 * Constructor
 */
Rcst_queue_jt::Rcst_queue_jt(uint32_t queue_id, uint16_t priority, uint32_t qlen, double max_vbdc, Ncc_config* config)
: Rcst_queue_basic(queue_id, Rcst_queue_basic::JT, priority, qlen, config), _max_vbdc_rate(max_vbdc)
{
	_requested_before_msl = 0;
}

/*
 * Destructor
 */
Rcst_queue_jt::~Rcst_queue_jt()
{
	/*
	 * nothing to do
	 */
}


	
uint32_t	
Rcst_queue_jt::sum_old_request()
{
	return (_requested_before_msl);
}



/*
 * Computing demand of require for rcst to send request packet to NCC
 */
uint32_t Rcst_queue_jt::compute_demand(rcst_queue_demand_type type, uint32_t available_slot, 
				       Frame_ident current_frame, double frame_period,
				       uint32_t msl, bool request_lost)
{

	assert(!( (msl==0) && (available_slot > 0) ));
	/*
	 * JT queue must return require atm cell (queue_length)
	 */
	if (type == Rcst_queue_basic::QD_QUEUE_LEN) 
	{
		uint32_t	request; // in timeslot.
		Frame_ident	target_frame;
		

		if (request_lost)
		{
			_next_demand_cell += _requested_before_msl * _atm_cell_per_slot; 
			_requested_before_msl = 0;
		}

		// 'request' stands for amount of slot requested in SAC.
		request = _next_demand_slot() > available_slot ? available_slot : _next_demand_slot();

		if (request > 0)
		{
			/*
			 * Attach a new demand instance.
			 */
			target_frame = _ncc_config->frame_add(current_frame, msl);

			assert(!_search_demand(target_frame));

			struct pkt_demand *demand = new struct pkt_demand;
			demand->require_slot = request;
			demand->require_ticks = GetCurrentTime();
			demand->frame = target_frame;
			demand->next = NULL;
			_append_pkt_demand(demand);
		}


		_next_demand_cell -= request * _atm_cell_per_slot;
		return (request);
	}
	else if (type == Rcst_queue_basic::QD_RATE)
		return (0);
	else
		assert(0);

	return (0);
}
void		
Rcst_queue_jt::grant_demand_init()
{
	_next_notice_ctrl_len = 0;
}

int Rcst_queue_jt::grant_demand(const uint32_t amount, const uint16_t superframe_count, 
				const uint8_t frame_number)
{
	uint32_t remain = amount;
	uint32_t given;
	assert (_queue_len >= amount);

	/*
	 * Handle old request first.
	 */
	given = (_requested_before_msl > remain) ?  remain : _requested_before_msl;

	_requested_before_msl -= given;

	remain -= given;


	/*
	 * Secondly, handle data which doesn't request yet.
	 */
	given = (remain > _next_demand_slot() ?  _next_demand_slot() : remain);

	remain -= given;

	_next_demand_cell -= given * _atm_cell_per_slot;


	/*
	 * Finally, handle young request.
	 */
	while (_demand_head && remain > 0)
	{
		if (remain >= _demand_head->require_slot)
		{
			remain -= _demand_head->require_slot;
			delete (_detach_demand_head());
		}
		else
		{
			_demand_head->require_slot -= remain;
			remain = 0;
		}
	}


	_next_notice_ctrl_len += (amount - remain);

	return (amount - remain);
}

void
Rcst_queue_jt::update_demand_list(uint16_t superframe_count, uint8_t frame_number)
{
	/*
	 * Detach the request older than MSL. We have to keep track the amount of
	 * old request.
	 */
	if (_demand_head && 
	    _demand_head->frame.superframe_count==superframe_count &&
	    _demand_head->frame.frame_number==frame_number)
	{
		_requested_before_msl += _demand_head->require_slot;
		delete (_detach_demand_head());
	}
}
