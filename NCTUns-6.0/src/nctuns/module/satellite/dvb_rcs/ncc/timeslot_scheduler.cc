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

#include "timeslot_scheduler.h"
#include <err.h>


uint16_t	
Slot_assignment_record::get_num_residual_slot(uint8_t superframe_id,
					      Slot_type type)
{
	if(type==SLOT_TYPE_DATA)
	{
		return (num_residual_data_slot[superframe_id]);
	}
	else if(type==SLOT_TYPE_REQUEST)
	{
		return (num_residual_req_slot[superframe_id]);
	}
	else
	{
		printf("[Error]%s --- no such slot type\n",__FUNCTION__);
		return (0);
	}
}


uint16_t
Slot_assignment_record::get_num_slot(Rcst_id rcst_id,
				     Slot_type type)
{
	map <Rcst_id, uint16_t>	::iterator	it;

	if(type==SLOT_TYPE_DATA)
	{
		it = num_data_slot_of_rcst.find(rcst_id);

		if(it!=num_data_slot_of_rcst.end())	
		{
			return (*it).second;
		}
	}
	else if(type==SLOT_TYPE_REQUEST)
	{
		it = num_req_slot_of_rcst.find(rcst_id);

		if(it!=num_req_slot_of_rcst.end())	
		{
			return (*it).second;
		}
	}
	else
	{
		printf("[Error]%s --- no such slot type\n",__FUNCTION__);
	}

	return (0);
}


int
Slot_assignment_record::add_num_slot(uint16_t superframe_id,
				     Rcst_id rcst_id, 
				     Slot_type type,
				     uint16_t num_slots)
{
	map <Rcst_id, uint16_t>	::iterator	it;


	if(type==SLOT_TYPE_DATA)
	{
		uint16_t	&residual_slot = 
			num_residual_data_slot[superframe_id];

		it = num_data_slot_of_rcst.find(rcst_id);

		if(it==num_data_slot_of_rcst.end())
		{
			// create one entry.
			num_data_slot_of_rcst[rcst_id] = 0;
		}

		if(num_slots <= residual_slot)
		{
			num_data_slot_of_rcst[rcst_id] += num_slots;
			residual_slot -= num_slots;
			return (0);
		}
	}
	else if(type==SLOT_TYPE_REQUEST)
	{
		uint16_t	&residual_slot = 
			num_residual_req_slot[superframe_id];

		it = num_req_slot_of_rcst.find(rcst_id);

		if(it==num_req_slot_of_rcst.end())
		{
			// create one entry.
			num_req_slot_of_rcst[rcst_id] = 0;
		}

		if(num_slots <= residual_slot)
		{
			num_req_slot_of_rcst[rcst_id] += num_slots;
			assert(num_req_slot_of_rcst.find(rcst_id)!=num_req_slot_of_rcst.end());
			residual_slot -= num_slots;
			return (0);
		}
	}
	else
	{
		printf("[Error]%s --- no such slot type\n",__FUNCTION__);
	}

	return (-1);
}


Slot_assignment_record::Slot_assignment_record( uint16_t num_req_slot,
						uint16_t num_data_slot,
						Ncc_ctl &para_ncc_ctl) :
						rcst_info_list(para_ncc_ctl.rcst_info_list),
						ncc_ctl(para_ncc_ctl),
						num_total_data_slot(num_data_slot),
						num_total_req_slot(num_req_slot),
						num_total_slot(num_data_slot+num_req_slot)
{
}


void
Slot_assignment_record::init()
{
	request_slot_chunk_list.clear();
	data_slot_chunk_list.clear();
	num_data_slot_of_rcst.clear();
	num_req_slot_of_rcst.clear();
	/* Initialize amount of utilized slots as zero,
	 * for each active superframe id.
	 */
	for(uint8_t superframe_id=0; superframe_id<=MAX_SUPERFRAME_ID; superframe_id++)
	{
		num_residual_data_slot[superframe_id] = num_total_data_slot;
		num_residual_req_slot[superframe_id] = num_total_req_slot;
		first_unused_slot[superframe_id] = 0;
	}
}

int
Slot_assignment_record::assign_slot(Rcst_id rcst_id, Slot_type type,
				    uint16_t start_slot, uint16_t count)
{
	Slot_assignment		slot_assignment;
	if(type!=SLOT_TYPE_DATA && type!=SLOT_TYPE_REQUEST)
	{
		printf("[Error]%s --- no such slot type\n",__FUNCTION__);
		return (-1);
	}

	while (count > 0)
	{
		uint16_t give = count > 256 ? 256 : count;

		count -= give;

		if(start_slot + give <= num_total_slot) //Enough
		{
			slot_assignment.rcst_id = rcst_id;
			slot_assignment.start_slot = start_slot;
			start_slot += give;
			//assignment_count is one less than number of slots assigned.
			slot_assignment.assignment_count = (give - 1);

			if(type==SLOT_TYPE_DATA)
			{
				data_slot_chunk_list.push_back(slot_assignment);
			}
			if(type==SLOT_TYPE_REQUEST)
			{
				request_slot_chunk_list.push_back(slot_assignment);
			}
		}
		else
		{
			return (-1);
		}
	}

	return (0);
}



int
Slot_assignment_record::set_up_slot_assignment()
{
	int	result;
	
	result = num_slot_to_chunk_list(SLOT_TYPE_REQUEST);

	if(result!=0)
	{
		return (result);
	}

	result = num_slot_to_chunk_list(SLOT_TYPE_DATA);

	return (result);

}


	
int
Slot_assignment_record::num_slot_to_chunk_list(Slot_type type)
{
	Rcst_id				rcst_id;
	list <Rcst_info>::iterator	it_rcst_info_list;
	list <Rcst_info>		&info_list = rcst_info_list.info_list;
	uint8_t				superframe_id;
	uint16_t			num_slot;


	if( (type != SLOT_TYPE_DATA) &&
	    (type != SLOT_TYPE_REQUEST) )
	{
		printf("[Error]%s() --- no such slot type\n",__FUNCTION__);
		return (-1);
	}

	//DATA slots are arranged after 'num_total_req_slot' th slot.
	if (type==SLOT_TYPE_DATA)
	{
		for(uint8_t superframe_id=0; superframe_id<=MAX_SUPERFRAME_ID; superframe_id++)
		{
			assert(first_unused_slot[superframe_id] <= num_total_req_slot);
			first_unused_slot[superframe_id] = num_total_req_slot;
		}
	}


	// Assign slot for each RCST.
	for(it_rcst_info_list=info_list.begin();
	    it_rcst_info_list!=info_list.end();
	    it_rcst_info_list++)
	{
		superframe_id = it_rcst_info_list->superframe_id_for_transmission;

		rcst_id = it_rcst_info_list->rcst_id;

		if((num_slot = get_num_slot(rcst_id, type)) > 0)
		{
			int result = assign_slot(rcst_id,
						 type, 
						 first_unused_slot[superframe_id],
						 num_slot);

			if( result == 0 ) // Enough
			{
				first_unused_slot[superframe_id] += num_slot;
			}
			else
			{
				printf("[Error] %s() --- no enough slot.\n", __FUNCTION__);
				return (-1);
			}
		}
	}

	return (0);
}


bool			
Timeslot_scheduler::Rcst_credit::credit_entry_exist(Rcst_id rcst_id, Credit_type type)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			return true;
		}
	}
	
	//There exists no such entry.
	return false;
}


int			
Timeslot_scheduler::Rcst_credit::increase_credit_value(Rcst_id rcst_id, Rcst_credit::Credit_type type, uint64_t value)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			it-> value += value;
			return 0;
		}
	}
	
	//There exists no such entry.
	return -1;
}

int			
Timeslot_scheduler::Rcst_credit::decrease_credit_value(Rcst_id rcst_id, Rcst_credit::Credit_type type, uint64_t value)
{
	list<Credit_entry>::iterator	it;

	for (it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if ((it->rcst_id==rcst_id) && (it->credit_type==type))	
		{
			if (it->value < value)
				return (-1);

			it->value -= value;

			// If reach zero, erase the record.
			if (it->value==0)
			{
				credit_list.erase(it);
			}

			return (0);
		}
	}
	
	//There exists no such entry.
	return -1;
}

int	
Timeslot_scheduler::Rcst_credit::get_credit_value(Rcst_id rcst_id, Rcst_credit::Credit_type type, uint64_t &value)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			value = it->value;
			return 0;
		}
	}
	
	//There exists no such entry.
	value = 0;
	return -1;
}


int	
Timeslot_scheduler::Rcst_credit::set_credit_value(Rcst_id rcst_id, Rcst_credit::Credit_type type, uint64_t value)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			it-> value = value;
			return 0;
		}
	}
	
	//There exists no such entry.
	return -1;
}


int			
Timeslot_scheduler::Rcst_credit::create_credit_entry(Rcst_id rcst_id, Rcst_credit::Credit_type type)
{
	list<Credit_entry>::iterator	it;
	Credit_entry			new_entry;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			return -1;
		}
	}
	
	//There exists no such entry yet.
	new_entry.rcst_id = rcst_id;
	new_entry.credit_type = type;
	new_entry.value = 0;
	new_entry.TTL = 0;
	credit_list.push_back(new_entry);
	return 0;
}

int			
Timeslot_scheduler::Rcst_credit::remove_credit_entry(Rcst_id rcst_id, Rcst_credit::Credit_type type)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			credit_list.erase(it);
			return 0;
		}
	}
	
	//There exists no such entry.
	return -1;
}


int	
Timeslot_scheduler::Rcst_credit::get_ttl_value(Rcst_id rcst_id, Rcst_credit::Credit_type type, uint64_t &TTL)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			TTL = it-> TTL;
			return 0;
		}
	}
	
	//There exists no such entry.
	return -1;
}

int	
Timeslot_scheduler::Rcst_credit::set_ttl_value(Rcst_id rcst_id, Rcst_credit::Credit_type type, uint64_t TTL)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			it-> TTL = TTL;
			return 0;
		}
	}
	
	//There exists no such entry.
	return -1;
}

int	
Timeslot_scheduler::Rcst_credit::decrease_ttl_value_by_one(Rcst_id rcst_id, Rcst_credit::Credit_type type)
{
	list<Credit_entry>::iterator	it;

	for(it = credit_list.begin(); it!=credit_list.end(); it++)
	{
		if((it-> rcst_id==rcst_id) && (it-> credit_type==type))	
		{
			if(it-> TTL > 0)
			{
				it-> TTL --;
				return 0;
			}
			else
				return -1;
		}
	}
	
	//There exists no such entry.
	return -1;
}



Timeslot_scheduler::Timeslot_scheduler(Ncc_ctl& pa_ncc_ctl) :
	rcst_infos(pa_ncc_ctl.rcst_info_list),
	heap_of_capacity_request(pa_ncc_ctl.heap_of_capacity_request),
	ncc_ctl(pa_ncc_ctl),
	slot_assignment_record(*(pa_ncc_ctl.slot_assignment_record))
{
	for (uint16_t superframe_id=0; 
	    superframe_id<=MAX_SUPERFRAME_ID; 
	    superframe_id++)
	{
		it_rcst_info_list[superframe_id] = pa_ncc_ctl.rcst_info_list.info_list.begin();
		it_pr[superframe_id] = pa_ncc_ctl.rcst_info_list.info_list.begin();
		it_rr[superframe_id] = pa_ncc_ctl.rcst_info_list.info_list.begin();
	}

	for (uint32_t rcst = 0; rcst < 65536; rcst++)
		wait_for_compensate[rcst] = 0;
}


Timeslot_scheduler::~Timeslot_scheduler()
{
}


int
Timeslot_scheduler::scheduling_for_req()
{
	uint8_t			superframe_id;
	Slot_assignment		slot_assignment;
	Rcst_id			rcst_id;

	/* Initialize amount of request slots as zero,
	 * for each superframe id.
	 */
	for(superframe_id=0; superframe_id<=ncc_ctl.ncc_config.max_superframe_id; superframe_id++)
	{
		if(rcst_infos.no_rcst(superframe_id))
			continue;

		while(slot_assignment_record.get_num_residual_slot(
			superframe_id, SLOT_TYPE_REQUEST) > 0)
		{
			if(superframe_id==it_rcst_info_list[superframe_id]->
				superframe_id_for_transmission)
			{
				rcst_id = it_rcst_info_list[superframe_id]->rcst_id;

				/* if this rcst_id has been take one req timeslot, then break */
				if(slot_assignment_record.get_num_slot(rcst_id, SLOT_TYPE_REQUEST) >= 1)
				{
					it_rcst_info_list[superframe_id]++;

					// wrap around.
					if(it_rcst_info_list[superframe_id]==rcst_infos.info_list.end())
					{
						it_rcst_info_list[superframe_id] = rcst_infos.info_list.begin();
					}
					break;
				}

				assert(slot_assignment_record.add_num_slot(superframe_id,
									   rcst_id,
									   SLOT_TYPE_REQUEST,
									   1)==0);
			}

			it_rcst_info_list[superframe_id]++;

			// wrap around.
			if(it_rcst_info_list[superframe_id]==rcst_infos.info_list.end())
			{
				it_rcst_info_list[superframe_id] = rcst_infos.info_list.begin();
			}
		}
	}
	
	return (0);
}

int
Timeslot_scheduler::scheduling_for_rt()
{
	uint8_t		superframe_id;
	list<Rcst_info>::iterator	it;


	/* For each RCST with state 'RCST_STATE_FINE_SYNC':
		1. Compute number of slots needed per frame corresponding CRA Level.
		2. If there remains adequate slots in the superframe the RCST used,
			allocate slots.
	 */
	// For each RCST ...
	for(it = rcst_infos.info_list.begin(); 
	    it!=rcst_infos.info_list.end(); 
	    it++)
	{
		const Rcst_id rcst_id = it-> rcst_id;

		const uint32_t cra_level = it-> cra_level;

		superframe_id = it-> superframe_id_for_transmission;

		uint32_t num_slots_needed = ncc_ctl.bps_to_spf(cra_level);

		uint32_t num_slots_free = slot_assignment_record.get_num_residual_slot(superframe_id,
										       SLOT_TYPE_DATA);
		
		if(num_slots_free >= num_slots_needed)
		{
			slot_assignment_record.add_num_slot(superframe_id, 
							    rcst_id, 
							    SLOT_TYPE_DATA,
							    num_slots_needed);

		}
		else
		{
			printf("Timeslot-scheduling exception::No enough slots for CRA\n");
			assert(0);
		}
	}

	return 0;
}

/************************************************************
  'heap_of_capacity_request' --> 'rcst_credits'
  'rcst_credit' --> 'slot_assignment_record'
************************************************************/
int
Timeslot_scheduler::scheduling_for_rbdc()
{

	list<Rcst_capacity_request>::iterator	rbdc_request_it;
	Rcst_id			rcst_id;
	uint8_t			superframe_id;
	uint32_t		rbdc_max; // Unit--> bits/s.
	uint16_t		rbdc_timeout; // Unit--> superframes.
	uint16_t		num_of_frame_per_superframe;

	
	/* For each RBDC request, record it at rcst_credits. */
	for(rbdc_request_it = heap_of_capacity_request.rbdc_requests.begin();
	    rbdc_request_it != heap_of_capacity_request.rbdc_requests.end();
	    rbdc_request_it++)
	{
		rcst_id = rbdc_request_it-> rcst_id;

		if (rbdc_request_it->capacity_request_value==0)
		{
			rcst_credits.remove_credit_entry(rcst_id, Rcst_credit::RB_CREDIT);
			continue;
		}

		assert(rcst_infos.get_rbdc_max(rbdc_max, rcst_id)==0);
		/* NOTE: Via RBDC_MAX, NCC limit rates which RCSTs sense */
		const uint32_t rate_bps = ((rbdc_request_it->capacity_request_value > rbdc_max) ?
					   rbdc_max : rbdc_request_it->capacity_request_value);

		if(!rcst_credits.credit_entry_exist(rcst_id, Rcst_credit::RB_CREDIT))
		{
			rcst_credits.create_credit_entry(rcst_id, Rcst_credit::RB_CREDIT);
		}

		rcst_credits.set_credit_value(rcst_id, 
					      Rcst_credit::RB_CREDIT, 
					      rate_bps);

		assert(rcst_infos.get_rbdc_timeout(rbdc_timeout, rcst_id)==0);

		num_of_frame_per_superframe = ncc_ctl.ncc_config.num_of_frame_per_superframe;

		const uint64_t ttl = num_of_frame_per_superframe * rbdc_timeout;

		rcst_credits.set_ttl_value(rcst_id,
					   Rcst_credit::RB_CREDIT, 
					   ttl);
	}

	list<Rcst_credit::Credit_entry>::iterator	it_credit_list;

	// Read all RB credits to set up 'slot_assignment_record'.
	for (it_credit_list = rcst_credits.credit_list.begin();
	     it_credit_list != rcst_credits.credit_list.end();)
	{
		if (it_credit_list->credit_type==Rcst_credit::RB_CREDIT)
		{
			rcst_id = it_credit_list->rcst_id;

			assert(rcst_infos.get_superframe_id_for_transmission(superframe_id, rcst_id)==0);

			const uint32_t rate_bps = it_credit_list->value;

			const double num_slots_needed = ncc_ctl.bps_to_spf(rate_bps);

			const uint16_t num_slots_given = (uint16_t) floor(num_slots_needed);

			slot_assignment_record.add_num_slot(superframe_id, 
							    rcst_id, 
							    SLOT_TYPE_DATA,
							    num_slots_given);

			// Wait for compensation.
			wait_for_compensate[rcst_id.logon_id] += num_slots_needed -
								num_slots_given;
			// Check if time out occur.
			assert (it_credit_list->TTL > 0);
			it_credit_list->TTL--;
			list<Rcst_credit::Credit_entry>::iterator tmp = it_credit_list;
			it_credit_list++;
			// Time out => remove credit.
			if (tmp->TTL==0)
			{
				rcst_credits.credit_list.erase(tmp);
			}
		}
		else
		{
			//Skip.
			it_credit_list++;
		}
	}

	return (0);
}

int
Timeslot_scheduler::scheduling_for_vbdc_and_avbdc()
{

	list<Rcst_capacity_request>::iterator	request_it;
	uint8_t			superframe_id;
	const uint16_t		num_of_atm_per_slot = ncc_ctl.ncc_config.num_of_atm_per_slot;

	Slot_need_and_given	slot_need_and_given;
	list<Slot_need_and_given>		slot_need_and_given_list;


	_vbdc_demand.clear();
	
	/* For each VBDC request, record it at rcst_credits. */
	for(request_it = heap_of_capacity_request.vbdc_requests.begin();
	    request_it != heap_of_capacity_request.vbdc_requests.end();
	    request_it++)
	{
		const Rcst_id rcst_id = request_it-> rcst_id;

		const uint32_t num_atm_cell = request_it->capacity_request_value;

		if(!rcst_credits.credit_entry_exist(rcst_id, Rcst_credit::VB_CREDIT))
		{
			rcst_credits.create_credit_entry(rcst_id, Rcst_credit::VB_CREDIT);
		}
		rcst_credits.increase_credit_value(rcst_id, Rcst_credit::VB_CREDIT, num_atm_cell);
		// Record VBDC demand. For VBDC history computation.
		_vbdc_demand[rcst_id] += num_atm_cell;
	}

	/* For each AVBDC request, record it at rcst_credits. */
	for(request_it = heap_of_capacity_request.avbdc_requests.begin();
	    request_it != heap_of_capacity_request.avbdc_requests.end();
	    request_it++)
	{
		const Rcst_id rcst_id = request_it-> rcst_id;

		const uint32_t num_atm_cell = request_it->capacity_request_value;

		if(!rcst_credits.credit_entry_exist(rcst_id, Rcst_credit::VB_CREDIT))
		{
			rcst_credits.create_credit_entry(rcst_id, Rcst_credit::VB_CREDIT);
		}
		rcst_credits.set_credit_value(rcst_id, Rcst_credit::VB_CREDIT, num_atm_cell);
		// Record VBDC demand. For VBDC history computation.
		_vbdc_demand[rcst_id] = num_atm_cell;
	}


	/* Record VBDC history. */
	for(list<Rcst_info>::iterator it = rcst_infos.info_list.begin(); 
	    it!=rcst_infos.info_list.end(); it++)
	{
		uint64_t num_cell_needed;
		
		num_cell_needed = _vbdc_demand[it->rcst_id];

		_vbdc_history[it->rcst_id] = (_vbdc_history[it->rcst_id] * 0.5 + 
					      num_cell_needed / num_of_atm_per_slot * 0.5);
	}

	/* Reference the VB credits to construct struct 'slot_need_and_given_list'. */
	list<Rcst_credit::Credit_entry>::iterator		it_credit_list;

	slot_need_and_given_list.clear();
	for(it_credit_list = rcst_credits.credit_list.begin();
	    it_credit_list != rcst_credits.credit_list.end();
	    it_credit_list++)
	{
		if(it_credit_list->credit_type==Rcst_credit::VB_CREDIT)
		{
			const Rcst_id rcst_id = it_credit_list->rcst_id;

			uint16_t		vbdc_max; // Unit--> timeslot.

			if (rcst_infos.get_vbdc_max(vbdc_max, rcst_id)!=0)
			{
				errx(1, "Can't not find RCST (%hhu,%hu) in config file",
				     rcst_id.group_id, rcst_id.logon_id);
			}

			const uint16_t num_slots_needed = it_credit_list->value / num_of_atm_per_slot;


			/* Copy information of slot requirements into struct 'slot_need_and_given_list'. */
			assert(rcst_infos.get_superframe_id_for_transmission(superframe_id, rcst_id)==0);
			slot_need_and_given.superframe_id = superframe_id;
			slot_need_and_given.rcst_id = rcst_id;
			/* NOTE: NCC limit volume which RCSTs sense via VBDC_MAX */
			slot_need_and_given.num_slots_needed = (num_slots_needed > vbdc_max ?
								vbdc_max : num_slots_needed);

			slot_need_and_given.num_slots_given = 0;
			if (slot_need_and_given.num_slots_needed > 0)
			{
				slot_need_and_given_list.push_back(slot_need_and_given);
			}
		}
	}

	/* NCC dispatch timeslot in RR pattern.*/
	for (superframe_id=0; superframe_id<=ncc_ctl.ncc_config.max_superframe_id; superframe_id++)
	{
		list<Slot_need_and_given>::iterator	it, cur;

		if (rcst_infos.no_rcst(superframe_id))
		{
			//Skip this channel.
			continue;
		}

		uint64_t need_cnt = 0, total_need = 0, amount;

		amount = slot_assignment_record.get_num_residual_slot(superframe_id, SLOT_TYPE_DATA); 

		// Compute the number of RCST which require slot and the summation of
		// slot requirement.
		for (it=slot_need_and_given_list.begin(); it!=slot_need_and_given_list.end(); it++)
		{
			if (it->superframe_id == superframe_id)
			{
				need_cnt++;
				total_need += it->num_slots_needed;
			}
		}

		while (need_cnt > 0 && amount > 0)
		{
			uint32_t min_grant_amount = amount / need_cnt;

			if (min_grant_amount == 0)
			{
				// We can't dispatch faster. Dispatch one-by-one below.
				min_grant_amount = 1;
			}

			for(it=slot_need_and_given_list.begin(); 
			    it!=slot_need_and_given_list.end();) 
			{
				if (it->superframe_id == superframe_id)
				{
					cur = it++;
					if (amount == 0 || need_cnt == 0)
						break;
					if (cur->num_slots_needed == 0)
						continue;


					uint32_t grant_amount = (cur->num_slots_needed > min_grant_amount ?
								 min_grant_amount : cur->num_slots_needed);
	
					slot_assignment_record.add_num_slot(superframe_id, 
									    cur->rcst_id, 
									    SLOT_TYPE_DATA,
									    grant_amount);

					assert (amount >= grant_amount);
					amount -= grant_amount;
					/* Update the credit record. */
					rcst_credits.decrease_credit_value(cur->rcst_id, 
									   Rcst_credit::VB_CREDIT, 
									   grant_amount * num_of_atm_per_slot);

					/* Update the struct recording how many slots RCSTs need. */
					cur->num_slots_needed-= grant_amount;

					if(cur->num_slots_needed==0)
					{
						slot_need_and_given_list.erase(cur);
						need_cnt--;
					}
				}
				else
				{
					// Skip this RCST.
					it++;
				}
			}
		}
	}
	return 0;
}


int
Timeslot_scheduler::scheduling_for_fca_rr()
{
	uint8_t		superframe_id;
	uint16_t	remainder_slots;
	uint32_t	num_active_rcsts;
	list<Rcst_info>::iterator	it;


	/* For each active superframe_id 'sid', if there exists remainder slots in sid,
	 * assign remainder slots with RR way to RCSTs which use this superframe_id to transmit.
	 */
	for(superframe_id=0; superframe_id<=ncc_ctl.ncc_config.max_superframe_id; superframe_id++)
	{
		/* Compute how many active RCSTs exist. */
		num_active_rcsts = rcst_infos.active_rcst_amount(superframe_id);

		if(num_active_rcsts==0)
			continue;

		remainder_slots = 
		slot_assignment_record.get_num_residual_slot(superframe_id,
							     SLOT_TYPE_DATA);

		/* Give each active RCSTs slots with equal amount.*/
		if(remainder_slots > 0)
		{
			uint16_t	num_slots_for_each_active_rcst = remainder_slots / num_active_rcsts;
			for(it = rcst_infos.info_list.begin(); it!=rcst_infos.info_list.end(); it++)
			{
				if( (it->superframe_id_for_transmission==superframe_id) &&
				    (it->rcst_state == RCST_STATE_FINE_SYNC))
				{
					slot_assignment_record.add_num_slot(superframe_id, 
									    it->rcst_id, 
									    SLOT_TYPE_DATA,
									    num_slots_for_each_active_rcst);

					/* 
					 * Decrease credit if nessesary.
					 */
					uint64_t credit_value;

					const uint32_t atm_cell_per_rcst = (num_slots_for_each_active_rcst *
									    ncc_ctl.ncc_config.num_of_atm_per_slot);
					rcst_credits.get_credit_value(it->rcst_id, 
								      Rcst_credit::VB_CREDIT, 
								      credit_value);

					uint32_t amount_credit_decreased = ((credit_value > 
									     atm_cell_per_rcst) ? 
									    atm_cell_per_rcst :
									    credit_value);

					rcst_credits.decrease_credit_value(it->rcst_id, 
									   Rcst_credit::VB_CREDIT, 
									   amount_credit_decreased);

				}
			}
		}
		else
		{
			continue;
		}

		/* Dispatch remainder slots one-by-one. */
		while (slot_assignment_record.get_num_residual_slot(superframe_id, SLOT_TYPE_DATA))
		{
			if ((it_rr[superframe_id]->superframe_id_for_transmission==superframe_id) &&
			    (it_rr[superframe_id]->rcst_state == RCST_STATE_FINE_SYNC))
			{
			slot_assignment_record.add_num_slot(superframe_id, 
							    it_rr[superframe_id]->rcst_id, 
							    SLOT_TYPE_DATA,
							    1);
			/* 
			 * Decrease credit if nessesary.
			 */
			uint64_t credit_value;

			rcst_credits.get_credit_value(it_rr[superframe_id]->rcst_id, 
						      Rcst_credit::VB_CREDIT, 
						      credit_value);

			uint32_t amount_credit_decreased = (credit_value >= 2 )? 2 : 0;

			rcst_credits.decrease_credit_value(it_rr[superframe_id]->rcst_id, 
							   Rcst_credit::VB_CREDIT, 
							   amount_credit_decreased);
			}
			
			it_rr[superframe_id]++;
			// wrap around.
			if(it_rr[superframe_id]==rcst_infos.info_list.end())
			{
				it_rr[superframe_id] = rcst_infos.info_list.begin();
			}
		}
	}
	
	return 0;
}


int
Timeslot_scheduler::scheduling_for_fca_pr()
{
	uint8_t		superframe_id;
	uint16_t	remainder_slots;
	uint32_t	num_active_rcsts;
	list<Rcst_info>::iterator	it;

	/* For each active superframe_id 'sid', if there exists remainder slots in sid,
	 * assign remainder slots with RR way to RCSTs which use this superframe_id to transmit.
	 */
	for(superframe_id=0; superframe_id<=ncc_ctl.ncc_config.max_superframe_id; superframe_id++)
	{
		remainder_slots = 
		slot_assignment_record.get_num_residual_slot(superframe_id,
							     SLOT_TYPE_DATA);
		/* Compute how many active RCSTs exist. */
		num_active_rcsts = rcst_infos.active_rcst_amount(superframe_id);

		if(num_active_rcsts==0 || remainder_slots==0)
			continue;

		/* Compute total VBDC request. */
		double total_vbdc_history = 0;
		for(it = rcst_infos.info_list.begin(); it!=rcst_infos.info_list.end(); it++)
		{
			if( (it->superframe_id_for_transmission==superframe_id) &&
			    (it->rcst_state == RCST_STATE_FINE_SYNC))
			{
				total_vbdc_history += _vbdc_history[it->rcst_id];
			}
		}

		uint32_t remain = remainder_slots;
		/* Dispatch slot according to propotion of VBDC reqeust. */
		for(it = rcst_infos.info_list.begin(); 
		    it!=rcst_infos.info_list.end() && total_vbdc_history > 0.00000001; it++)
		{
			if( (it->superframe_id_for_transmission==superframe_id) &&
			    (it->rcst_state == RCST_STATE_FINE_SYNC))
			{
				uint32_t slot_given = (uint32_t) floor(remain * _vbdc_history[it->rcst_id] / 
								       total_vbdc_history);

				slot_assignment_record.add_num_slot(superframe_id, 
								    it->rcst_id, 
								    SLOT_TYPE_DATA,
								    slot_given);

				assert(remainder_slots >= slot_given);

				remainder_slots -= slot_given;

				/* 
				 * Decrease credit if nessesary.
				 */
				uint64_t credit_value;

				rcst_credits.get_credit_value(it->rcst_id, 
							      Rcst_credit::VB_CREDIT, 
							      credit_value);

				const uint32_t atm_cell_given = slot_given * ncc_ctl.ncc_config.num_of_atm_per_slot;

				uint32_t amount_credit_decreased = ((credit_value > atm_cell_given) ? 
								    atm_cell_given : credit_value);

				rcst_credits.decrease_credit_value(it->rcst_id, 
								   Rcst_credit::VB_CREDIT, 
								   amount_credit_decreased);

			}
		}

		/* Dispatch remainder slots one-by-one. */
		while (slot_assignment_record.get_num_residual_slot(superframe_id, SLOT_TYPE_DATA))
		{
			if ((it_pr[superframe_id]->superframe_id_for_transmission==superframe_id) &&
			    (it_pr[superframe_id]->rcst_state == RCST_STATE_FINE_SYNC))
			{
				slot_assignment_record.add_num_slot(superframe_id, 
								    it_pr[superframe_id]->rcst_id, 
								    SLOT_TYPE_DATA,
								    1);
			/* 
			 * Decrease credit if nessesary.
			 */
			uint64_t credit_value;

			rcst_credits.get_credit_value(it_pr[superframe_id]->rcst_id, 
						      Rcst_credit::VB_CREDIT, 
						      credit_value);

			uint32_t amount_credit_decreased = (credit_value >= 2 )? 2 : 0;

			rcst_credits.decrease_credit_value(it_pr[superframe_id]->rcst_id, 
							   Rcst_credit::VB_CREDIT, 
							   amount_credit_decreased);
			}
			
			it_pr[superframe_id]++;
			// wrap around.
			if(it_pr[superframe_id]==rcst_infos.info_list.end())
			{
				it_pr[superframe_id] = rcst_infos.info_list.begin();
			}
		}
	}
	
	return 0;
}

/************************************************************
 * Since the rate-based scheduling(RT/RBDC) leads to timeslot
 * of floating-point amount, we have to compensate for
 * the floating part.
************************************************************/
int
Timeslot_scheduler::compensate()
{
	return (0);
}

/************************************************************
  'heap_of_capacity_request' --> 'slot_assignment_record'
  'heap_of_capacity_request' is erased after scheduling.
************************************************************/
int
Timeslot_scheduler::scheduling()
{
	/* Initialize relative data structure. */
	slot_assignment_record.init();

	scheduling_for_req();

	scheduling_for_rt();

	scheduling_for_rbdc();

	scheduling_for_vbdc_and_avbdc();

	if(ncc_ctl.ncc_config.fca_turned_on)
	{
		scheduling_for_fca_rr();
	}
	

	if(slot_assignment_record.set_up_slot_assignment() != 0)
	{
		assert(0);
	}

	heap_of_capacity_request.clear();

	return (0);
}

