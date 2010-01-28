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
#include <satellite/dvb_rcs/common/atm_cell.h>
#include <satellite/dvb_rcs/common/ncc_config.h>
#include <nctuns_api.h>
#include "rcst_queue_basic.h"
#include "rcst_queue_vrjt.h"
/*
 * MSL: we define it be the max round trip time of GEO satellite network system
 * the distant between GEO satellite and earth is around 36000 KM.
 */
#define MSL_ms	240

/*
 * Constructor
 */
Rcst_queue_vrjt::Rcst_queue_vrjt(uint32_t queue_id, uint16_t priority, uint32_t qlen, uint32_t max_rbdc, uint32_t timeout, Ncc_config *config)
: Rcst_queue_basic(queue_id, Rcst_queue_basic::VR_JT, priority, qlen, config), 
	_max_rbdc_rate(max_rbdc), _rbdc_timeout(timeout)
{
	/*
	 * nothing to do
	 */
}

/*
 * Destructor
 */
Rcst_queue_vrjt::~Rcst_queue_vrjt()
{
	/*
	 * nothing to do
	 */
}


/*
 * Computing demand of require for rcst to send request packet to NCC
 */
uint32_t Rcst_queue_vrjt::compute_demand(rcst_queue_demand_type type, uint32_t amount, 
					 Frame_ident current_frame, double frame_period,
					 uint32_t msl)
{
	/*
	 * VR-JT queue must return require rate
	 */
	if (type == Rcst_queue_basic::QD_QUEUE_LEN) 
	{
		return (0);
	}
	else if (type == Rcst_queue_basic::QD_RATE) 
	{
		Frame_ident	target_frame;
		uint32_t	require;


		/*
		 * append one demand record into linked list
		 */
		/* NOTE: Via queue's RBDC_MAX, limit rate capacity which particular queue can request. */
		const uint32_t rbdc_limit = _ncc_config->bps_to_spf(_max_rbdc_rate);

		_last_rbdc_frame = target_frame = ((msl > 0) ? 
						   _ncc_config->frame_add(current_frame, msl) : 
						   _ncc_config->frame_add(_last_rbdc_frame, 1));



		if (amount > 0)
		{
			// It has opportunity to send RBDC request. Calculate request.
			_last_rbdc = require = (_next_demand_slot() > rbdc_limit ? 
						rbdc_limit : _next_demand_slot());
			assert (amount >= require);

			_rbdc_timer = _rbdc_timeout;
		}
		else // It has no opportunity to send RBDC request. 
		{
			uint32_t limit = (_rbdc_timer > 0) ? _last_rbdc : 0;
			require = (_next_demand_slot() > limit) ? limit : _next_demand_slot();
		}

		// Count down the timer to RBDC timeout.
		_rbdc_timer = (_rbdc_timer > 0) ? _rbdc_timer - 1 : 0;

		_next_demand_cell -= require * _atm_cell_per_slot;

		/*
		 * Create new demand and then attach it onto demand list.
		 */
		//In the list, there should be no demands of the same target frame.
		assert(!_search_demand(target_frame));

		if (require > 0)
		{
			struct pkt_demand *demand = new struct pkt_demand;
			demand->require_slot = require;
			demand->require_ticks = GetCurrentTime();
			demand->frame = target_frame;
			demand->next = NULL;
			_append_pkt_demand(demand);
		}

		/*
		 * compute rate demand for require with unit as bit/s.
		 */
		const uint32_t bps = amount > 0 ? _ncc_config->spf_to_bps(require) : 0;

		return (bps);
	}
	else
		assert(0);

	return (0);
}


int Rcst_queue_vrjt::grant_demand(const uint32_t amount, const uint16_t superframe_count, 
				  const uint8_t frame_number)
{
	if (!(_demand_head && 
	      _demand_head->frame.superframe_count==superframe_count &&
	      _demand_head->frame.frame_number==frame_number)) // demand for the frame doesn't exist.
	{
		return (0);
	}

	const uint32_t spent = ((_demand_head->require_slot > amount) ?
				amount : _demand_head->require_slot);

	if (_demand_head->require_slot > amount) 
	{
		// Quantity of granted timeslot is less than demand require, 
		// so we increase the amount of next requirement.
		const uint32_t insufficience = _demand_head->require_slot - amount;
		_next_demand_cell += insufficience * _atm_cell_per_slot;
	}

	_next_notice_ctrl_len += spent;

	// This demand instance is useless now. So we delete it.
	delete (_detach_demand_head());

	return (spent);
}
