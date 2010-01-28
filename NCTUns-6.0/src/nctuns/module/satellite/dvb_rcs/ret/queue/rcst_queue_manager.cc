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

#include <stdlib.h>
#include <nctuns_api.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/atm_cell.h>
#include <satellite/dvb_rcs/rcst/rcst_ctl.h>
#include <satellite/dvb_rcs/rcst/rcst_queue_config.h>
#include "rcst_queue_manager.h"
#include "rcst_queue_basic.h"
#include "rcst_queue_ctrl.h"
#include "rcst_queue_rt.h"
#include "rcst_queue_jt.h"
#include "rcst_queue_vrrt.h"
#include "rcst_queue_vrjt.h"
#include "../rcs_mac.h"

/*
 * Constructor, It need fetch configure file to dynamic create queue list
 */
Rcst_queue_manager::Rcst_queue_manager(Rcst_queue_config *rcst_config, Rcst_ctl *rcst_ctl, 
				       Ncc_config *ncc_config, uint32_t timeout_in_frame)
{
	_rcst_config = rcst_config;
	_rcst_obj = rcst_ctl;
	_next_request_is_avbdc = false;
	_request_lost = false;
	_rbdc_timeout_in_frame = timeout_in_frame;
	_ncc_config = ncc_config;

	_next_shift.state = READY_FIND_LATEST_TIMESLOT;
	_next_shift.queue = NULL;
	_next_shift.timeslot = NULL;

	_initialize_queue_list();
}


/*
 * Destructor
 */
Rcst_queue_manager::~Rcst_queue_manager()
{
	_free_queue();
}


/*
 * Creating queue
 */
void Rcst_queue_manager::_initialize_queue_list()
{
	struct Rcst_queue_config::queue_conf *queue;
	struct queue_parameter para;

	_queue_head = (struct queue_head){NULL, NULL, NULL, NULL, NULL};

	_queue_head.data[0] = &_queue_head.rt;
	_queue_head.data[1] = &_queue_head.vrrt;
	_queue_head.data[2] = &_queue_head.vrjt;
	_queue_head.data[3] = &_queue_head.jt;

	/*
	 * create control queue
	 */
	para.type = Rcst_queue_basic::CTRL;

	para.len = 20000;
	assert(_create_queue(para) == 0);

	queue = _rcst_config->get_queue_pointer();

	while (queue) {
		para.queue_id = queue->queue_id;
		para.priority = queue->priority;
		para.type = queue->type;
		para.len = queue->queue_len;
		para.cra_rate = queue->cra_rate;
		para.max_rbdc_rate = queue->max_rbdc_rate;
		para.max_vbdc_rate = queue->max_vbdc_rate;

		assert(_create_queue(para) == 0);
		queue = queue->next;
	}
}


/*
 * create control and data queue
 */
int Rcst_queue_manager::_create_queue(struct queue_parameter para)
{
	Rcst_queue_basic *queue;

	switch (para.type) {
	case Rcst_queue_basic::CTRL:
		queue = new Rcst_queue_ctrl(para.len, _ncc_config);

		/*
		 * because of A rcst just only have one control queue
		 */
		/*
		 * linked this queue list
		 */
		_queue_head.ctrl = queue;
		break;
	case Rcst_queue_basic::RT:
		queue = new Rcst_queue_rt(para.queue_id, para.priority, 
					  para.len, para.cra_rate, _ncc_config);

		/*
		 * linked this queue list
		 */
		_list_insert_entry(&_queue_head.rt, queue);
		break;
	case Rcst_queue_basic::VR_RT:
		queue = new Rcst_queue_vrrt(para.queue_id, para.priority, para.len, para.cra_rate, 
					    para.max_rbdc_rate, _rbdc_timeout_in_frame,
					    _ncc_config);

		/*
		 * linked this queue list
		 */
		_list_insert_entry(&_queue_head.vrrt, queue);
		break;
	case Rcst_queue_basic::VR_JT:
		queue = new Rcst_queue_vrjt(para.queue_id, para.priority, 
					    para.len, para.max_rbdc_rate, 
					    _rbdc_timeout_in_frame, _ncc_config);

		/*
		 * linked this queue list
		 */
		_list_insert_entry(&_queue_head.vrjt, queue);
		break;
	case Rcst_queue_basic::JT:
		queue = new Rcst_queue_jt(para.queue_id, para.priority, 
					  para.len, para.max_vbdc_rate,
					  _ncc_config);

		/*
		 * linked this queue list
		 */
		_list_insert_entry(&_queue_head.jt, queue);
		break;
	default:
		assert(0);
		return (1);
	};
	return (0);
}


/*
 * Insert a queue into linked list, it must sort all linked list order by queue
 * type, priority, and insert order
 */
void Rcst_queue_manager::_list_insert_entry(Rcst_queue_basic **head, Rcst_queue_basic *queue)
{
	assert(queue);
	if (_has_exist_queue_id(queue->get_queue_id())) {
		printf("[Rcst_queue_manager] duplication queue_id = %u\n", queue->get_queue_id());
		delete queue;
		return;
	}

	/*
	 * linked list is empty
	 */
	if (!(*head)) {
		*head = queue;
	}
	/*
	 * linked list has entries in it.
	 * It must sort order by queue priority, If the priority is equal
	 * between both queue, then keep the sequence of inserting.
	 */
	else {
		Rcst_queue_basic *ptr, *last;

		last = ptr = *head;;

		while (ptr && ptr->get_priority() >= queue->get_priority()) {
			last = ptr;
			ptr = ptr->get_next_queue();
		}

		/*
		 * all priority entries is lower then this queue
		 */
		if (ptr == *head) {
			*head = queue;
			queue->set_next_queue(ptr);
		}
		/*
		 * all priority entries is higher then this queue
		 * or the priority of this queue is between the entries.
		 */
		else {
			last->set_next_queue(queue);
			queue->set_next_queue(ptr);
		}

	}
}


/*
 * check all data queue if exist this queue_id
 * return:
 *     1	exist
 *     0	non-exist
 */
int Rcst_queue_manager::_has_exist_queue_id(uint32_t queue_id)
{
	Rcst_queue_basic *queue;

	for (int i = 0; i < 4; i++) {
		queue = *_queue_head.data[i];
		while (queue) {
			if (queue->get_queue_id() == queue_id)
				return (1);

			queue = queue->get_next_queue();
		}
	}

	return (0);
}

/*
 * Destroy all queue in linked list
 */
void Rcst_queue_manager::_free_queue()
{
	/*
	 * because of A rcst just only have one control queue
	 */
	delete _queue_head.ctrl;
	_queue_head.ctrl = NULL;

	for (int i = 0; i < 4; i++) {
		_free_queue(_queue_head.data[i]);
	}
}


/*
 * free all queue for specified queue head of linked list
 */
void Rcst_queue_manager::_free_queue(Rcst_queue_basic **head)
{
	Rcst_queue_basic *queue;

	while (*head) {
		queue = *head;
		*head = (*head)->get_next_queue();
		delete queue;
	}
	*head = NULL;
}


/*
 * push the Dvb_pkt packet into queue manager, then QM must identify the
 * Dvb_pkt packet type and into the specified queue
 * the queue_id equal zero always be ctrl queue
 */
int Rcst_queue_manager::push(Dvb_pkt *pkt)
{
	Rcst_queue_basic *queue;
	uint32_t queue_id = pkt->pkt_getretinfo()->queue_id;

	/*
	 * control packet
	 */
	if (queue_id == 0) {
		return _queue_head.ctrl->push(pkt);
	}
	/*
	 * data packet
	 */
	else {
		for (int i = 0; i < 4; i++) {
			queue = *_queue_head.data[i];
			while (queue) {
				if (queue->get_queue_id() == queue_id) {
					return queue->push(pkt);
				}
				queue = queue->get_next_queue();
			}
		}

		delete pkt;
		return (1);
	}
}



/*
 * find the latest timeslot for control or data queue, and change the state.
 * the output is stored in _next_shift.
 */
struct slot_info *Rcst_queue_manager::get_timeslot()
{
	Rcst_queue_basic *queue;

	/*
	 * if I have control burst, then must compare contrl and all data
	 * burst which will be sent first according to timeslot information
	 */

	/*
	 * get the timeslot information of queue to compare others data queue
	 */
	_next_shift.queue = _queue_head.ctrl;
	_next_shift.timeslot = _next_shift.queue->get_timeslot();

	/*
	 * require all timeslot for each queue_id
	 */
	for (int i = 0; i < 4; i++) 
	{
		for (queue = *_queue_head.data[i]; queue; queue = queue->get_next_queue())
		{
			struct slot_info* const timeslot = queue->get_timeslot();

			if (_rcst_obj->require_timeslot(timeslot, queue->get_queue_id()))
			{
				assert(timeslot->start_time >= GetCurrentTime());

				/*
				 * find the earliest start time
				 */
				if ((!_next_shift.timeslot) || 
				    (_next_shift.timeslot->start_time > timeslot->start_time))
				{
					_next_shift.timeslot = timeslot;
					_next_shift.queue = queue;
				}
				else if (_next_shift.timeslot->start_time==timeslot->start_time)
					assert(0);
			}
		}
	}

	if (_next_shift.timeslot == NULL)
		return NULL;
	else {
		_next_shift.state = READY_SHIFT_DVB_PKT;
		return _next_shift.timeslot;
	}
}


/*
 * shift the latest timeslot data or control packet
 */
Dvb_pkt *Rcst_queue_manager::shift()
{
	Dvb_pkt *pkt;

	/*
	 * wrong state, it must finish get_timeslot()
	 */
	if (_next_shift.state != READY_SHIFT_DVB_PKT)
	{
		printf("Wrong state!! forget to find slot before shift?\n");
		assert(0);
	}

	assert(_next_shift.queue && _next_shift.timeslot);

	/*
	 * control packet, directly shift the first packet of the control queue
	 */
	if (_next_shift.queue->get_type() == Rcst_queue_basic::CTRL) {
		pkt = (Dvb_pkt *)_next_shift.queue->shift();
		pkt->pkt_getretinfo()->burst_type = pkt->pkt_gettype();
	}

	/*
	 * data queue, must merge cell into burst, then copy timeslot information into packet
	 * and return it
	 */
	else {
		uint32_t num_of_cell, i, len;
		struct atm_cell *ptr, *head;

		num_of_cell = _next_shift.timeslot->timeslot_payload_type;
		len = _next_shift.queue->get_current_queue_len();

		assert (num_of_cell <= len);


		/*
		 * may be attach TCT information to struct pkt_timer_pbuf or struct
		 * return_link_info if attach pkt_timer_pbuf, then the receiver need
		 * ask ncc control again to decode. On the other way, the receiver do
		 * not need to ask again.
		 */
		pkt = new Dvb_pkt();

		/*
		 * fill the timeslot information into dvb_pkt and clean the
		 * timeslot of queue
		 */
		memcpy(&pkt->pkt_getretinfo()->timeslot, _next_shift.timeslot, sizeof(struct slot_info));
		memset(_next_shift.queue->get_timeslot(), 0, sizeof(struct slot_info));
		pkt->pkt_getretinfo()->burst_type = PKT_RCSMAC;

		ptr = head = NULL;
		for (i = 0; i < num_of_cell; i++) {
			if (!head) {
				ptr = head = (struct atm_cell *)_next_shift.queue->shift();
			}
			else {
				ptr->next = (struct atm_cell *)_next_shift.queue->shift();
				ptr = ptr->next;
			}
		}
		pkt->pkt_getretinfo()->queue_id = _next_shift.queue->get_queue_id();
		pkt->pkt_attach(head, num_of_cell);
		pkt->pkt_settype(PKT_ATM);
	}

	_next_shift.state = READY_FIND_LATEST_TIMESLOT;
	return pkt;
}


uint32_t 
Rcst_queue_manager::_grant_demand_jt(const uint16_t superframe_count, const uint8_t frame_number,
				     const uint32_t available)
{
	Rcst_queue_jt*	queue;
	uint32_t spent, remain_slot = available;

	const Frame_ident frame(superframe_count, frame_number);

	const uint32_t queue_cnt = _num_of_queue(_queue_head.jt);

	if (queue_cnt==0)
		return (0);

	uint32_t* demand = new uint32_t[queue_cnt];

	assert (demand);

	/*
	 * Handle old request first.
	 */
	queue = (Rcst_queue_jt*) _queue_head.jt;
	for (uint32_t i = 0; i < queue_cnt; ++i) 
	{
		assert(queue);
		demand[i] = queue->sum_old_request();
		queue = (Rcst_queue_jt*) queue->get_next_queue();
	}

	spent = _grant_demand_for_queue_evenly(frame, remain_slot, _queue_head.jt, demand);

	assert (spent <= remain_slot);
	remain_slot -= spent;

	/*
	 * Secondly, handle data which doesn't request yet.
	 */
	queue = (Rcst_queue_jt*) _queue_head.jt;
	for (uint32_t i = 0; i < queue_cnt; ++i) 
	{
		assert(queue);
		demand[i] = queue->_next_demand_slot();
		queue = (Rcst_queue_jt*) queue->get_next_queue();
	}

	spent = _grant_demand_for_queue_evenly(frame, remain_slot, _queue_head.jt, demand);

	assert (spent <= remain_slot);
	remain_slot -= spent;


	/*
	 * Finally, handle young request.
	 */
	queue = (Rcst_queue_jt*) _queue_head.jt;
	for (uint32_t i = 0; i < queue_cnt; ++i) 
	{
		assert(queue);
		demand[i] = queue->demand_list_statistic();
		queue = (Rcst_queue_jt*) queue->get_next_queue();
	}

	spent = _grant_demand_for_queue_evenly(frame, remain_slot, _queue_head.jt, demand);

	assert (spent <= remain_slot);
	remain_slot -= spent;


	delete (demand);

	return (available - remain_slot);
}


/************************************************************
 * After parsing TBTP, RCST_CTL would call grant_demand() to
 * grant timeslots of amount 'data_slot_amount' for queues
 * depend on specified policy.
 * NOTE: If two_pass is turned on, there will be second pass
 * to assign remainder slots.
 ***********************************************************/
int Rcst_queue_manager::grant_demand(const uint16_t superframe_count, const uint8_t frame_number, 
				     const uint8_t num_of_atmcell_of_timeslot, const uint32_t vbdc_max,
				     const uint32_t data_slot_amount, const uint32_t req_slot_amount,
				     const bool two_pass, const bool tbtp_lost, const bool vbdc_empty_flag)
{
	uint32_t remain_slot = data_slot_amount;
	uint32_t cost;
	int ret = 0, i;

	/*
	 * Grant Policy:
	 * grant order by RT, VR-RT, VR-JT, JT. it will satisfy the first queue
	 * type, then second, thrid, ... etc.
	 * if queue manager has many the same queue type, than order by queue
	 * priority.
	 * if queue priority has the same level, then take round robin method
	 * to satisfy it.
	 */

	_grant_demand_init();

	/*
	 * for RT
	 */
	cost = _grant_demand_for_queue_one_by_one(superframe_count, frame_number, 
						  remain_slot, _queue_head.rt);

	assert(cost <= remain_slot);
	remain_slot -= cost;
	/*
	 * for VR-RT
	 */
	cost = _grant_cra_demand_for_vrrt_queue(superframe_count, frame_number, 
						remain_slot);
	assert(cost <= remain_slot);
	remain_slot -= cost;

	cost = _grant_rbdc_demand_for_vrrt_queue(superframe_count, frame_number, 
						 remain_slot);
	assert(cost <= remain_slot);
	remain_slot -= cost;


	/*
	 * for VR-JT
	 */
	cost = _grant_demand_for_queue_one_by_one(superframe_count, frame_number, 
						  remain_slot, _queue_head.vrjt);
	assert(cost <= remain_slot);
	remain_slot -= cost;

	/* 
	 * Determine if AVBDC request should be submitted for JT queues.
	 */

	/* Count amount of total demands which are older than MSL. */
	uint32_t total = 0; 
	for (Rcst_queue_jt *queue = (Rcst_queue_jt*)_queue_head.jt; queue; 
	     queue = (Rcst_queue_jt*) queue->get_next_queue())
	{
		queue->update_demand_list(superframe_count, frame_number);
		total += queue->sum_old_request();
	}

	_request_lost = vbdc_empty_flag && (total > remain_slot);
	

	/*
	 * for JT
	 */
	cost = _grant_demand_jt(superframe_count, frame_number, remain_slot);
	
	assert(cost <= remain_slot);

	remain_slot -= cost;

	if (remain_slot == 0) ret = 1;

	/*
	 * if ret == 1, that mean all timeslot are be used. else ret == 0 that
	 * mean we have remain timeslot
	 */
	if (ret == 0 && two_pass) {
		//Not supported yet.
		assert(0);
	}

	if (remain_slot == 0) ret = 1;

	/*
	 * notice rcst_ctl to create timeslot linked list.
	 */
	for (i = 0; i < 4; i++) {
		Rcst_queue_basic *queue;

		queue = *_queue_head.data[i];

		while (queue) {
			const uint32_t notice_slot = queue->get_next_notice_ctrl_grant_len();

			if (notice_slot > 0) 
			{
				_rcst_obj->queue_grant_demand(superframe_count, frame_number,
							      queue->get_queue_id(), notice_slot);
			}
			queue = queue->get_next_queue();
		}
	}
	return (ret);
}


void
Rcst_queue_manager::_grant_demand_init()
{
	// Init for JT queues.
	for (Rcst_queue_basic *queue = _queue_head.jt; queue; queue = queue->get_next_queue())
	{
		((Rcst_queue_jt*) queue)->grant_demand_init();
	}
}
/*
 * grant timeslot for specified queues in guaranteed way.
 */
uint32_t 
Rcst_queue_manager::_grant_demand_for_queue_one_by_one(const uint16_t superframe_count,
						       const uint8_t frame_number, 
						       const uint32_t remain_slot, 
						       Rcst_queue_basic *head)
{
	uint32_t remainder = remain_slot;
	/*
	 * process all queue in this linked list
	 */
	for (Rcst_queue_basic *queue = head; queue;
	     queue = queue->get_next_queue())
	{
		const uint32_t demand = queue->get_first_require_timeslot();

		const uint32_t given = remainder > demand ? demand : remainder;

		const uint32_t spent = queue->grant_demand(given, superframe_count, frame_number);

		remainder -= spent;
	}
	//return amount of slots spent.
	return (remain_slot - remainder);
}

uint32_t 
Rcst_queue_manager::_grant_demand_for_queue_evenly(Frame_ident frame, 
						   const uint32_t available, 
						   Rcst_queue_basic *head,
						   uint32_t* demand)
{
	assert(head && demand);
	/*
	 * process all queue in this linked list
	 */
	uint32_t *cnt = _dispatch_evenly(head, demand, available);

	Rcst_queue_basic *queue = head;

	uint32_t remainder = available;

	for (uint32_t i = 0; i < _num_of_queue(head); ++i) 
	{
		assert(remainder >= cnt[i]);
		queue->grant_demand(cnt[i], frame.superframe_count, frame.frame_number);
		remainder -= cnt[i];
		queue = queue->get_next_queue();
	}
	delete (cnt);

	return (available - remainder);
}


uint32_t		
Rcst_queue_manager::_grant_cra_demand_for_vrrt_queue(uint16_t superframe_count, 
						     uint8_t frame_number, 
						     uint32_t remain_slot)
{
	uint32_t remainder = remain_slot;
	/*
	 * process all queue in VR-RT linked list
	 */
	for (Rcst_queue_vrrt *queue = (Rcst_queue_vrrt*)_queue_head.vrrt; queue;
	     queue = (Rcst_queue_vrrt*) queue->get_next_queue())
	{
		const uint32_t demand = queue->get_first_require_timeslot(CRA);

		if (remainder < demand)
		{
			printf("[Warning] CRA for VR-RT is not satisfied.\n");
		}

		const uint32_t given = remainder > demand ? demand : remainder;

		const uint32_t spent = queue->grant_demand(CRA, given, superframe_count, frame_number);

		remainder -= spent;
	}
	//return amount of ATM cells spent.
	return (remain_slot - remainder);
}


uint32_t		
Rcst_queue_manager::_grant_rbdc_demand_for_vrrt_queue(uint16_t superframe_count, 
						      uint8_t frame_number, 
						      uint32_t remain_slot)
{
	//The policy here in this version is the same as '_grant_demand_for_queue_one_by_one'.
	uint32_t remainder = remain_slot;
	/*
	 * process all queue in VR-RT linked list
	 */
	for (Rcst_queue_vrrt *queue = (Rcst_queue_vrrt*)_queue_head.vrrt; queue;
	     queue = (Rcst_queue_vrrt*) queue->get_next_queue())
	{
		const uint32_t demand = queue->get_first_require_timeslot(RBDC);

		const uint32_t given = remainder > demand ? demand : remainder;

		const uint32_t spent = queue->grant_demand(RBDC, given, superframe_count, frame_number);

		remainder -= spent;
	}
	//return amount of ATM cells spent.
	return (remain_slot - remainder);
}


/*
 * compute the demand array of the same priority queue
 */
uint32_t*
Rcst_queue_manager::_dispatch_evenly(Rcst_queue_basic *head, uint32_t* demand,
				     uint32_t available)
{
	const uint32_t queue_cnt = _num_of_queue(head);
	Rcst_queue_basic *queue;
	uint32_t *cnt = new uint32_t[queue_cnt];
	uint32_t total = 0;
	uint32_t i, demand_cnt = 0;

	assert(head && demand);

	memset(cnt, 0, sizeof(uint32_t) * queue_cnt);

	if (available == 0)
		return cnt;

	/*
	 * Compute the amount of queues requiring timeslot and total demand.
	 */
	for (i = 0, queue = head; i < queue_cnt && queue;
	     ++i, queue = queue->get_next_queue()) 
	{
		if (demand[i]) 
		{
			++demand_cnt;
			total += demand[i];
		}
	}

	/*
	 * if available is more than total demand, then return the demands.
	 */
	if (total <= available) 
	{
		memcpy(cnt, demand, sizeof(uint32_t) * queue_cnt);
		return (cnt);
	}

	/*
	 * calculate the average grant timeslot for all queue
	 */
	while (available > 0 && demand_cnt > 0) 
	{
		/*
		 * if number of queue which need timeslot is more than remain
		 * timeslot, that mean we cannot fairly allocate remain
		 * timeslot. So we allocate it by sequence of queue list.
		 */
		uint32_t min_grant_amount = available / demand_cnt;
	
		if (min_grant_amount == 0)
		{
			// We can't dispatch faster. Dispatch one-by-one below.
			min_grant_amount = 1;
		}

		for (i = 0; (i < queue_cnt) && (available > 0); ++i)
		{
			const uint32_t grant = MIN(demand[i], min_grant_amount);

			cnt[i] += grant;
			available -= grant;
			demand[i] -= grant;
			if (grant > 0 && demand[i] == 0) 
				--demand_cnt;
		}
	}

	return (cnt);
}


uint32_t
Rcst_queue_manager::compute_demand_for_rt_vrrt(uint8_t capacity_request_number, double frame_period, 
					       Frame_ident current_frame, uint32_t msl)
{
	/*
	 * calculate the max require demand for rate
	 */
	uint32_t available_rate = 255 * 16 * 2000 * capacity_request_number; // in bits/s
	uint32_t	required_rate = 0;
	Rcst_queue_basic *queue;


	/*
	 * queue type "RT" needn't require for timeslot, but we need record the
	 * timeslot of CRA
	 */
	for (queue = _queue_head.rt; queue; queue = queue->get_next_queue()) {
		queue->compute_demand(Rcst_queue_basic::QD_QUEUE_LEN, available_rate, 
				      current_frame, frame_period, msl);
	}

	/*
	 * queue type "VR-RT" need require for rate
	 */
	for (queue = _queue_head.vrrt; queue; queue = queue->get_next_queue()) 
	{
		const uint32_t rate = queue->compute_demand(Rcst_queue_basic::QD_RATE, 
							    available_rate, current_frame, 
							    frame_period, msl);
		assert (available_rate >= rate);
		available_rate -= rate;
		required_rate += rate;
	}


	return (required_rate);
}
struct Rcst_queue_manager::queue_demand 
Rcst_queue_manager::compute_demand_for_vrjt_jt(uint8_t capacity_request_number, 
					       uint32_t spent_rate, double frame_period, 
					       Frame_ident current_frame, uint32_t msl)
{
	struct queue_demand require_demand = {0, 0};
	Rcst_queue_basic *queue;

	/*
	 * calculate the max require demand for rate
	 */
	assert ((255 * 16 * 2000 * (uint32_t)capacity_request_number) >= spent_rate);
	uint32_t available_rate = ((255 * 16 * 2000 * capacity_request_number) - 
				   spent_rate); // in bits/s

	uint32_t required_rate = spent_rate;


	/*
	 * queue type "VR-JT" need require for rate
	 */
	for (queue = _queue_head.vrjt; queue; queue = queue->get_next_queue()) 
	{
		const uint32_t rate = queue->compute_demand(Rcst_queue_basic::QD_RATE, 
							    available_rate, current_frame, 
							    frame_period, msl);
		assert (available_rate >= rate);
		available_rate -= rate;
		required_rate += rate;
	}

	require_demand.rate = (uint32_t) ceil(required_rate / 2000.0); // bit/s to 2kbit/s
	/*
	 * re-calculate the require timeslot for queue length
	 */
	/*
	 * Compute how many SAC entries RBDC takes. Then compute
	 * the maximum VBDC request which can be sent.
	 */
	uint32_t sac_taken = 0;
	uint32_t value = (uint32_t) ceil(required_rate / 2000.0);
	while (value > 0)
	{
		if (value > 255) // Scaling is needed.
		{
			const uint32_t req = value / 16;

			const uint8_t req_value = (req > 255) ? 255 : req;

			const uint32_t scaled_req_value = req_value * 16;

			value = (value > scaled_req_value) ? value - scaled_req_value : 0;
		}
		else
		{
			value = 0;
		}

		sac_taken++;
	}

	const uint32_t sac_available = (capacity_request_number > sac_taken ? 
					capacity_request_number - sac_taken : 0);

	const uint32_t available_volumn = sac_available * 255 * 16;

	uint32_t available_slot = available_volumn / _ncc_config->num_of_atm_per_slot;
	/*
	 * queue type "JT" need require for atm cell length
	 */
	for (Rcst_queue_jt* queue = (Rcst_queue_jt*)_queue_head.jt; 
	     queue; queue = (Rcst_queue_jt*)queue->get_next_queue()) 
	{
		const uint32_t require_slot = queue->compute_demand(Rcst_queue_basic::QD_QUEUE_LEN, 
								    available_slot, current_frame,
								    frame_period, msl, 
								    _request_lost);

		assert (available_slot >= require_slot);
		available_slot -= require_slot;
		require_demand.queue_len += require_slot * _ncc_config->num_of_atm_per_slot;
	}

	return (require_demand);
}

uint16_t		
Rcst_queue_manager::_num_of_queue(Rcst_queue_basic *head)
{
	uint16_t cnt = 0;
	for (Rcst_queue_basic* ptr = head; ptr; ptr = ptr->get_next_queue(), cnt++) ;

	return (cnt);
}
int
Rcst_queue_manager::get_total_queue_id(int *queue_id_array)
{
	int	cnt=0;
	for(int i=0; i<4; i++)
	{
		Rcst_queue_basic	*ptr;
		ptr = *(_queue_head.data[i]);
		while(ptr)
		{
			cnt++;
			*queue_id_array++ = ptr->get_queue_id();
			ptr = ptr->get_next_queue();
		}
	}
	return cnt;
}
int
Rcst_queue_manager::get_next_shift_queue_id()
{
	return _next_shift.queue->get_queue_id();
}
