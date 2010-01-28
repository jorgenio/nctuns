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

#ifndef	__NCTUNS_timeslot_scheduler_h_
#define __NCTUNS_timeslot_scheduler_h_

#include <assert.h>
#include <sys/types.h>
#include <map>
#include <list>
#include <satellite/dvb_rcs/common/pcr.h>
#include <satellite/dvb_rcs/common/rcst_info.h>
#include <satellite/dvb_rcs/common/sac.h>
#include "ncc_ctl.h"

using std::map;
using std::list;

#define	FCA_LOG

enum Slot_type
{
	SLOT_TYPE_DATA,
	SLOT_TYPE_REQUEST
};

struct Slot_need
{
	uint8_t		superframe_id;
	Rcst_id		rcst_id;
	uint16_t	num_slots_needed;
};

struct Slot_need_and_given
{
	uint8_t		superframe_id;
	Rcst_id		rcst_id;
	uint16_t	num_slots_needed;
	uint16_t	num_slots_given;
}; 


/* Slot_assignment specifies a sequence of slots assigned to certain Rcst. */
struct Slot_assignment
{
	Rcst_id		rcst_id;
	uint16_t	start_slot; // Slot number of the first given slot.
	uint8_t		assignment_count; // One less than the amount of given slots.
};
	
class Heap_of_capacity_request;
class Ncc_ctl;

struct ltstr
{
	bool operator()(const Rcst_id id1, const Rcst_id id2) const
	{
		if(id1.group_id < id2.group_id)
		{
			return true;
		}
		else if((id1.group_id == id2.group_id) && (id1.logon_id < id2.logon_id))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

class Slot_assignment_record
{
  private:
	/**************** Private Data ******************************/
	Rcst_info_list	&rcst_info_list;
	Ncc_ctl		&ncc_ctl;
	uint16_t	num_total_data_slot;
	uint16_t	num_total_req_slot;
	uint16_t	num_total_slot;
	uint16_t	num_residual_data_slot[MAX_SUPERFRAME_ID+1];// Indexed by superframe_id.
	uint16_t	num_residual_req_slot[MAX_SUPERFRAME_ID+1];// Indexed by superframe_id.
	uint16_t	first_unused_slot[MAX_SUPERFRAME_ID+1]; // Indexed by superframe_id.

	map <Rcst_id, uint16_t, ltstr> num_data_slot_of_rcst;
	map <Rcst_id, uint16_t, ltstr> num_req_slot_of_rcst;


	/**************** Private Function **************************/


	int		num_slot_to_chunk_list(Slot_type type);

	/************************************************************
	 * assign_slot() updates request_slot_chunk_list/
	 * data_slot_chunk_list.
	 ***********************************************************/
	int		assign_slot(Rcst_id rcst_id, Slot_type type,
				    uint16_t start_slot, uint16_t count);
  public:

  	/**************** Public Data *******************************/
	list<Slot_assignment> 		request_slot_chunk_list;
	list<Slot_assignment> 		data_slot_chunk_list;

	/**************** Public Function ***************************/
  	Slot_assignment_record( uint16_t num_total_req_slot,
				uint16_t num_total_data_slot,
				Ncc_ctl &ncc_ctl);

  	void		init();

	/************************************************************
	 * get_num_residual_slot() return the amount of residual slots
	 * in channel 'superframe_id'.
	 ***********************************************************/
	uint16_t	get_num_residual_slot(uint8_t superframe_id,
					      Slot_type type);

	/************************************************************
	 * get_num_slot() return the amount of slots given to
	 * RCST 'rcst_id'. If such RCST doesn't exist, zero
	 * is returned.
	 ***********************************************************/
	uint16_t	get_num_slot(Rcst_id rcst_id, 
				     Slot_type type);

	/************************************************************
	 * add_num_slot() increases the amount of slots
	 * given to RCST 'rcst_id' and the residual amount is reduced.
	 ***********************************************************/
	int		add_num_slot(uint16_t superframe_id, 
					  Rcst_id rcst_id,
					  Slot_type type,
					  uint16_t num_slots);

	/************************************************************
	 * set_up_slot_assignment() set up 
	 * 'data_slot_chunk_list' and 'request_slot_chunk_list' 
	 * according to 'num_data_slot_of_rcst' and
	 * 'num_req_slot_of_rcst'.
	************************************************************/
	int		set_up_slot_assignment();
};

class Timeslot_scheduler
{
  friend class Ncc_ctl;
  private:

	/**************** Private Data ******************************/
	Rcst_info_list			&rcst_infos;

	list <Rcst_info>::iterator	it_rcst_info_list[MAX_SUPERFRAME_ID+1];

	map <Rcst_id, double, ltstr>	_vbdc_history;
	map <Rcst_id, uint32_t, ltstr>	_vbdc_demand;

	list <Rcst_info>::iterator	it_pr[MAX_SUPERFRAME_ID+1];

	list <Rcst_info>::iterator	it_rr[MAX_SUPERFRAME_ID+1];

	/* heap_of_capacity_request stores capacity requests which
	 * are collected in the last frame period.
	 */
	Heap_of_capacity_request	&heap_of_capacity_request;

	Ncc_ctl				&ncc_ctl;

	Slot_assignment_record		&slot_assignment_record;

	double				wait_for_compensate[65536]; // Timeslot amount to be compensate. Index by logon_id/node_id.

	class Rcst_credit
	{
	  public:
		enum Credit_type
		{
			RB_CREDIT,
			VB_CREDIT
		};

		struct Credit_entry
		{
			Rcst_id		rcst_id;
			Credit_type	credit_type;
			uint64_t	value; // Unit--> bps for RB while ATM slots for VB.

			/* TTL is meaningless for VB. TTL indicates how many frames from now
			 * the RB credit will be hold.
			 */
			int64_t			TTL; 
			#define	TTL_INFINITE	10000000
		};


		/**************** Public Data *******************************/
	  	list<Credit_entry>	credit_list;

		/**************** Public Function ***************************/
	  	int	increase_credit_value(Rcst_id rcst_id, 
						Credit_type type, 
						uint64_t value);
						
	  	int	decrease_credit_value(Rcst_id rcst_id, 
						Credit_type type, 
						uint64_t value);

		bool	credit_entry_exist(Rcst_id rcst_id, 
					   Credit_type type);

		int	create_credit_entry(Rcst_id rcst_id,
					    Credit_type type);

		int	remove_credit_entry(Rcst_id rcst_id,
					    Credit_type type);

		int	get_credit_value(Rcst_id rcst_id,
					Credit_type type, 
					uint64_t &value);

		int	set_credit_value(Rcst_id rcst_id,
					Credit_type type, 
					uint64_t value);

		/* Note: The arguments 'type' for get_ttl_value() and set_ttl_value() should be 'RB_CREDIT'.*/
		int	get_ttl_value(Rcst_id rcst_id,
				      Credit_type type, 
				      uint64_t &TTL);

		int	set_ttl_value(Rcst_id rcst_id, 
					Credit_type type, 
					uint64_t TTL);

		int	decrease_ttl_value_by_one(Rcst_id rcst_id, 
						Credit_type type);
	};
	Rcst_credit	rcst_credits;
	  	
	/**************** Private Function **************************/
	
	/************************************************************
	 * scheduling_for_req() set up structure 'num_req_slot_of_rcst'
	 * of 'slot_assignment_record'.
	 ***********************************************************/
	int		scheduling_for_req();
	int		scheduling_for_rt();
	int		scheduling_for_rbdc();
	int		scheduling_for_vbdc_and_avbdc();
	int		scheduling_for_fca_rr();
	int		scheduling_for_fca_pr();
	int		compensate();


  public:
	/**************** Public Function ***************************/

	Timeslot_scheduler(Ncc_ctl& pa_ncc_ctl);

	~Timeslot_scheduler();

	int		scheduling();
};

#endif /* __NCTUNS_timeslot_scheduler_h_ */
