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
#include <satellite/dvb_rcs/common/ncc_config.h>
#include <nctuns_api.h>
#include "rcst_queue_basic.h"
#include "rcst_queue_rt.h"

/*
 * Constructor
 */
Rcst_queue_rt::Rcst_queue_rt(uint32_t queue_id, uint16_t priority, uint32_t qlen, uint32_t cra, Ncc_config* config)
: Rcst_queue_basic(queue_id, Rcst_queue_basic::RT, priority, qlen, config), _cra_rate(cra)
{
	/*
	 * setting cra rate;
	 */
}

/*
 * Destructor
 */
Rcst_queue_rt::~Rcst_queue_rt()
{
	/*
	 * nothing to do
	 */
}


/*
 * Computing demand of require for rcst to send request packet to NCC
 */
uint32_t Rcst_queue_rt::compute_demand(rcst_queue_demand_type type, uint32_t amount, 
				       Frame_ident current_frame, double frame_period,
				       uint32_t msl)
{
	if (type == Rcst_queue_basic::QD_QUEUE_LEN) {
		struct pkt_demand *demand = new struct pkt_demand;

		/*
		 * compute demand for require, this rate unit is kbit/s(1000bit/s)
		 */
		const uint32_t limit = _ncc_config->bps_to_spf(_cra_rate);

		const uint32_t require = _next_demand_slot() > limit ? limit : _next_demand_slot();

		const Frame_ident target_frame = current_frame;

		//In the list, there should be no demands of the same target frame.
		assert(!_search_demand(target_frame));

		demand->require_slot = require;

		demand->require_ticks = GetCurrentTime();

		demand->frame = target_frame;

		demand->next = NULL;

		/*
		 * append one demand record into linked list
		 */
		_append_pkt_demand(demand);

		_next_demand_cell -= require * _atm_cell_per_slot;
	
	}
	else if (type == Rcst_queue_basic::QD_RATE)
		return (0);
	else
		assert(0);

	return (0);
}

int Rcst_queue_rt::grant_demand(const uint32_t amount, const uint16_t superframe_count, 
				const uint8_t frame_number)
{
	uint32_t	require;
	if(!(_demand_head && 
	     _demand_head->frame.superframe_count==superframe_count &&
	     _demand_head->frame.frame_number==frame_number)) // demand for the frame doesn't exist.
	{
		return (0);
	}

	if ((require = _demand_head->require_slot) > amount) 
	{
		// It must be due to TBTP lost, thus the amount must be zero.
		assert (amount==0);
		_drop_head(require * _atm_cell_per_slot);
	}


	uint32_t spent = _next_notice_ctrl_len = (require > amount) ? amount : require;

	// This demand instance is not useless now. So we delete them.
	delete (_detach_demand_head());

	return (spent);
}
