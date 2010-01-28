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
#include <math.h>
#include <satellite/dvb_rcs/common/sch_info.h>
#include <satellite/dvb_rcs/common/atm_cell.h>
#include <satellite/dvb_rcs/common/ncc_config.h>
#include <nctuns_api.h>
#include "rcst_queue_basic.h"
#include "rcst_queue_vrrt.h"


/*
 * Constructor
 */
Rcst_queue_vrrt::Rcst_queue_vrrt(uint32_t queue_id, uint16_t priority, 
				 uint32_t qlen, uint32_t cra, 
				 uint32_t max_rbdc, uint32_t timeout, Ncc_config *config)
: Rcst_queue_basic(queue_id, Rcst_queue_basic::VR_RT, priority, qlen, config), 
	_cra_rate(cra), _max_rbdc_rate(max_rbdc), _rbdc_timeout(timeout)
{
	_cra_demand_head = _cra_demand_tail = _demand_head = _demand_tail = NULL;
}

/*
 * Destructor
 */
Rcst_queue_vrrt::~Rcst_queue_vrrt()
{
	/*
	 * nothing to do
	 */
}


/*
 * Computing demand of require for rcst to send request packet to NCC
 */
uint32_t Rcst_queue_vrrt::compute_demand(rcst_queue_demand_type type, uint32_t amount, 
					 Frame_ident current_frame, double frame_period,
					 uint32_t msl)
{
	struct pkt_demand	*demand;
	Frame_ident		target_frame;
	uint32_t		require;

	/*
	 * VR-RT queue must return RBDC rate.
	 */
	if (type == Rcst_queue_basic::QD_QUEUE_LEN)
		return (0);
	else if (type == Rcst_queue_basic::QD_RATE) {
		/*
		 * Handle CRA part.
		 */
		const uint32_t cra_limit = _ncc_config->bps_to_spf(_cra_rate);

		target_frame = current_frame;

		require = _next_demand_slot() > cra_limit ? cra_limit : _next_demand_slot();

		_next_demand_cell -= require * _atm_cell_per_slot;

		//In the list, there should be no demands of the same target frame.
		assert (!_search_demand(CRA, target_frame));

		demand = new struct pkt_demand;

		demand->require_slot = require;

		demand->require_ticks = GetCurrentTime();

		demand->frame = target_frame;

		demand->next = NULL;

		//append one demand record into CRA demand list
		_append_pkt_demand(CRA, demand);

		/*
		 * Handle RBDC part.
		 */
		/* NOTE: Via RBDC_MAX, NCC limit rate capacity which RCSTs can request. */
		const uint32_t rbdc_limit = _ncc_config->bps_to_spf(_max_rbdc_rate);


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


		_last_rbdc_frame = target_frame = ((msl > 0) ? 
						   _ncc_config->frame_add(current_frame, msl) : 
						   _ncc_config->frame_add(_last_rbdc_frame, 1));

		// Count down the timer to RBDC timeout.
		_rbdc_timer = (_rbdc_timer > 0) ? _rbdc_timer - 1 : 0;

		_next_demand_cell -= require * _atm_cell_per_slot;

		//In the list, there should be no demands of the same target frame.
		assert(!_search_demand(RBDC, target_frame));

		if (require > 0)
		{
			demand = new struct pkt_demand;

			demand->require_slot = require;

			demand->require_ticks = GetCurrentTime();

			demand->frame = target_frame;

			demand->next = NULL;

			_append_pkt_demand(RBDC, demand);
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

int Rcst_queue_vrrt::grant_demand(bool cra, const uint32_t amount, 
				  const uint16_t superframe_count, 
				  const uint8_t frame_number)
{
	// Head of demand list to be processed.
	const struct pkt_demand *const head = cra ? _cra_demand_head : _demand_head; 


	// Check if the demand for the frame exist.
	// If it doesn't, do nothing.
	if(!(head && 
	     head->frame.superframe_count==superframe_count &&
	     head->frame.frame_number==frame_number))
	{
		return (0);
	}


	/*
	 * grant number of timeslot is more than demand require, then accept demand require to send.
	 */
	if (head->require_slot > amount) 
	{
		const uint32_t insufficience = head->require_slot - amount;

		//grant number of timeslot is less than demand require, so drop un-satisfied part.
		_drop_head(insufficience * _atm_cell_per_slot);
	}

	const uint32_t spent = (head->require_slot > amount) ? amount : head->require_slot;

	_next_notice_ctrl_len += spent;

	// This demand instance is not useless now. So we delete them.
	delete (_detach_demand_head(cra));

	return (spent);
}

void	
Rcst_queue_vrrt::_append_pkt_demand(bool cra, struct pkt_demand *demand)
{
	// Head of demand list to be processed.
	struct pkt_demand* &head = cra ? _cra_demand_head : _demand_head; 
	// Tail of demand list to be processed.
	struct pkt_demand* &tail = cra ? _cra_demand_tail : _demand_tail; 

	assert(demand);

	demand->next = NULL;

	if (!head) 
	{
		head = tail = demand;
	}
	else 
	{
		assert(tail);
		tail->next = demand;
		tail = demand;
	}
}


Rcst_queue_basic::pkt_demand*	
Rcst_queue_vrrt::_detach_demand_head(bool cra)
{
	// Head of demand list to be processed.
	struct pkt_demand* &head = cra ? _cra_demand_head : _demand_head; 
	// Tail of demand list to be processed.
	struct pkt_demand* &tail = cra ? _cra_demand_tail : _demand_tail; 
	

	if(head)
	{
		pkt_demand	*p_demand = head;

		if(tail==head)	//only one instance.
		{
			tail = head = NULL;
		}
		else
		{
			head = head->next;
		}

		return (p_demand);
	}
	else // No instance.
	{
		return (NULL);
	}

}

Rcst_queue_basic::pkt_demand*	
Rcst_queue_vrrt::_search_demand(bool cra, Frame_ident target_frame)
{
	pkt_demand *p_demand = cra ? _cra_demand_head : _demand_head;

	while (p_demand) 
	{
		if (p_demand->frame==target_frame)
		{
			return p_demand;
		}
		p_demand = p_demand->next;
	}

	// Not found.
	return (NULL);
}

uint32_t 
Rcst_queue_vrrt::get_first_require_timeslot(bool cra)
{
	// Head of demand list to be processed.
	struct pkt_demand *head = cra ? _cra_demand_head : _demand_head; 

	return (head ? head->require_slot : 0);
}

uint32_t 
Rcst_queue_vrrt::get_first_require_timeslot(bool cra, Frame_ident& target_frame)
{
	// Head of demand list to be processed.
	struct pkt_demand *head = cra ? _cra_demand_head : _demand_head; 

	if (head)
	{
		target_frame = head->frame;
	}

	return (head ? head->require_slot : 0);
}

