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

#ifndef __NCTUNS_rcst_queue_manager_h__
#define __NCTUNS_rcst_queue_manager_h__

#include <stdint.h>
#include "rcst_queue_basic.h"
#include <satellite/dvb_rcs/common/ncc_config.h>

#define CONFSTR_QUEUESTRATEGY	"QueueStrategy"
#define CONFSTR_RT	"RT"
#define CONFSTR_VRRT	"VR-RT"
#define CONFSTR_VRJT	"VR-JT"
#define CONFSTR_JT	"JT"

#define BOUNDARY_ATMCELL_TO_TIMESLOT(_cost, _atm_num)	\
{							\
	int _remain;					\
	_remain = _cost % _atm_num;			\
	if (_remain)					\
		_cost += _atm_num - _remain;		\
}

class Dvb_pkt;
class Rcst_ctl;
class Rcst_queue_config;
class Rcst_queue_basic;

/*
 * Rcst Queue Manager, this instance will be invoked in rcs_mac module
 */
class Rcst_queue_manager {

private:
	struct queue_parameter {
		uint32_t	queue_id;
		uint16_t	priority;
		Rcst_queue_basic::rcst_queue_type type;
		uint32_t	len; // Unit --> ATM cell.
		uint32_t	cra_rate; // Unit --> bits/s.
		uint32_t	max_rbdc_rate; // Unit --> bits/s.
		uint32_t	max_vbdc_rate; // Unit --> bits/s.
	};
	int			_number_of_queue;

	/*
	 * head of queue list
	 */
	struct queue_head {
		Rcst_queue_basic	*ctrl;
		Rcst_queue_basic	*rt;
		Rcst_queue_basic	*vrrt;
		Rcst_queue_basic	*vrjt;
		Rcst_queue_basic	*jt;
		Rcst_queue_basic	**data[4];
	} _queue_head;

	enum timeslot_state {
		READY_FIND_LATEST_TIMESLOT,
		READY_SHIFT_DVB_PKT
	};

	struct {
		timeslot_state state;
		Rcst_queue_basic *queue;
		struct slot_info *timeslot;
	} _next_shift;

	/*
	 * this object pointer will pointer to rcst queue config and rcst ctrl
	 */
	Rcst_queue_config	*_rcst_config;
	Rcst_ctl		*_rcst_obj;
	Ncc_config		*_ncc_config;
	uint32_t		_rbdc_timeout_in_frame;

	bool			_next_request_is_avbdc;
	bool			_request_lost;
public:
	struct queue_demand 
	{
		uint64_t	rate;		// in 2k bits/sec
		uint64_t	queue_len;	// in ATM cell
	};

private:
	uint16_t		_num_of_queue(Rcst_queue_basic *head);
	void			_initialize_queue_list();
	int			_create_queue(struct queue_parameter para);
	void			_list_insert_entry(Rcst_queue_basic **head,
					Rcst_queue_basic *queue);
	int			_has_exist_queue_id(uint32_t queue_id);

	void			_free_queue();
	void			_free_queue(Rcst_queue_basic **head);

	void			_grant_demand_init();

	uint32_t		_grant_cra_demand_for_vrrt_queue(uint16_t superframe_count, 
								 uint8_t frame_number, 
								 uint32_t remain_atm_cell);

	uint32_t		_grant_rbdc_demand_for_vrrt_queue(uint16_t superframe_count, 
								  uint8_t frame_number, 
								  uint32_t remain_atm_cell);


	uint32_t		_grant_demand_for_queue_one_by_one(const uint16_t superframe_count,
								   const uint8_t frame_number,
								   const uint32_t amount,
								   Rcst_queue_basic *head);

	uint32_t		_grant_demand_for_queue_evenly(Frame_ident frame, 
							       const uint32_t available, 
							       Rcst_queue_basic *head,
							       uint32_t *demand);

	uint32_t*		_dispatch_evenly(Rcst_queue_basic *head, uint32_t* demand,
						 uint32_t available);

	void			_require_timeslot();

public:
	Rcst_queue_manager(Rcst_queue_config *rcst_config, Rcst_ctl *rcst_ctl, Ncc_config *ncc_config, uint32_t timeout_in_frame);
	~Rcst_queue_manager();

	int			push(Dvb_pkt *pkt);
	Dvb_pkt			*shift();
	struct slot_info	*get_timeslot();
	int			grant_demand(const uint16_t superframe_count, const uint8_t frame_number, 
					     const uint8_t num_of_atmcell_of_timeslot, const uint32_t vbdc_max,
					     const uint32_t data_slot_amount, const uint32_t req_slot_amount, 
					     const bool two_pass, const bool tbtp_lost, const bool vbdc_empty_flag);

	uint32_t		_grant_demand_jt(const uint16_t superframe_count, 
						 const uint8_t frame_number, 
						 const uint32_t available);

	uint32_t		compute_demand_for_rt_vrrt(uint8_t capacity_request_number, double frame_period,
							   Frame_ident frame, uint32_t msl);

	struct queue_demand	compute_demand_for_vrjt_jt(uint8_t capacity_request_number, uint32_t spent_rate,
							   double frame_period, Frame_ident frame, 
							   uint32_t msl);

	bool			next_request_is_avbdc() {return _next_request_is_avbdc;}
	int			get_total_queue_id(int *queue_id_array);
	int			get_next_shift_queue_id();
}; 

#endif	/* __NCTUNS_rcst_queue_manager_h__ */
