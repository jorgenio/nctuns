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

#include "ncc_ctl.h"
#include <iostream>
#include <satellite/dvb_rcs/fwd/dvb_s2.h>

static uint32_t _rbdc_rate(const Sac::Capacity_request &request);
static uint32_t _num_atm_cell_of_req(const Sac::Capacity_request &request);
const uint8_t CAPACITY_REQUEST_NUMBER = 8;
extern RegTable                 RegTable_;

MODULE_GENERATOR(Ncc_ctl)


Rcst_capacity_request::Rcst_capacity_request(Rcst_id r_id, Sac::Capacity_request c_r)
{
	rcst_id = r_id;
	channel_id = c_r.channel_id;
	capacity_request_type = c_r.capacity_request_type;

	/*
	 * Compute the request value.
	 */
	switch (capacity_request_type)
	{
		case (Sac::CAPACITY_REQUEST_TYPE_VALUE_RBDC):
			capacity_request_value = _rbdc_rate(c_r);
			break;
		case (Sac::CAPACITY_REQUEST_TYPE_VALUE_VBDC):
		case (Sac::CAPACITY_REQUEST_TYPE_VALUE_AVBDC):
			capacity_request_value = _num_atm_cell_of_req(c_r);
			break;
		default:
			assert (0);
	}
}


void
Heap_of_capacity_request::clear()
{
	rbdc_requests.clear();
	vbdc_requests.clear();
	avbdc_requests.clear();
}


void
Heap_of_capacity_request::insert(Rcst_capacity_request rcst_capacity_request)
{
	switch (rcst_capacity_request.capacity_request_type)
	{
		case (Sac::CAPACITY_REQUEST_TYPE_VALUE_RBDC):
		{
			list<Rcst_capacity_request>::iterator	request_it;
			bool	rbdc_found = false;

			/*
			 * We merge all RBDC request entries within one SAC into a single entry.
			 */
			for (request_it = rbdc_requests.begin(); request_it != rbdc_requests.end(); request_it++)
			{
				const Rcst_id rcst_id = request_it-> rcst_id;
				if (rcst_capacity_request.rcst_id==rcst_id)
				{
					rbdc_found = true;
					request_it->capacity_request_value += rcst_capacity_request.capacity_request_value;
				}
			}

			if (!rbdc_found)
				rbdc_requests.push_back(rcst_capacity_request);
			break;
		}
		case (Sac::CAPACITY_REQUEST_TYPE_VALUE_VBDC):
		{
			vbdc_requests.push_back(rcst_capacity_request);
			break;
		}
		case (Sac::CAPACITY_REQUEST_TYPE_VALUE_AVBDC):
		{
			list<Rcst_capacity_request>::iterator	request_it;
			bool	avbdc_found = false;

			/*
			 * We merge all AVBDC request entries within one SAC into a single entry.
			 */
			for (request_it = avbdc_requests.begin(); request_it != avbdc_requests.end(); request_it++)
			{
				const Rcst_id rcst_id = request_it-> rcst_id;
				if (rcst_capacity_request.rcst_id==rcst_id)
				{
					avbdc_found = true;
					request_it->capacity_request_value += rcst_capacity_request.capacity_request_value;
				}
			}

			if (!avbdc_found)
				avbdc_requests.push_back(rcst_capacity_request);
			break;
		}
		default:
			assert(0);
	}
}



/*
 * constructor
 */
Ncc_ctl::Ncc_ctl(uint32_t type , uint32_t id , struct plist* pl , const char* name)
	:NslObject(type , id , pl , name) 
{
	/*
	 * Parse the NCC config file
	 */
	char		path[200];

	sprintf(path, "%s.dvbrcs.ncc.%d", GetScriptName(), get_nid());

	parse_ncc_config(path, ncc_config, rcst_info_list);

	_node_type = NODE_ID_NCC;
	REG_VAR("NODE_TYPE", &_node_type);
	REG_VAR("NCC_PTR", this);
	REG_VAR("NCC_CONFIG", &ncc_config);
}

/*
 * destructor
 */
Ncc_ctl::~Ncc_ctl()
{
	delete _scheduler;
	delete slot_assignment_record;
}

/*
 * init function
 */
int
Ncc_ctl::init() 
{
	uint64_t	send_start;


	next_sct_version = next_fct_version = next_tct_version = 
	next_tbtp_version = INIT_VER_NUM;


	slot_assignment_record = new Slot_assignment_record(
		ncc_config.num_of_req_slot_per_frame,
		ncc_config.num_of_data_slot_per_frame,
		(*this)
		);

	assert (ncc_config.frame_duration > 5);
	send_start = ncc_config.frame_duration - 5;// Let SCT,TCT,FCT sent before TBTP.

	/*
	 * initialize generation. 
	 */
	_gen_sct_fct_and_tct_timer.setCallOutObj(this , (int (NslObject::*)(Event_ *))&Ncc_ctl::gen_sct_fct_and_tct);
	_gen_sct_fct_and_tct_timer.start(send_start, ncc_config.superframe_duration);

	_scheduleTimer.setCallOutObj(this , (int (NslObject::*)(Event_ *))&Ncc_ctl::frame_scheduling);

	_scheduleTimer.start(ncc_config.frame_duration, 
			     ncc_config.frame_duration);

	/* Statically allocate timeslots to each user according to the file 'rcst.config'.*/
	assert(_scheduler = new Timeslot_scheduler((*this)));

	
	/*
	 * get feeder module pointer
	 */
	/* get feeder_node_id */
	char			filename[1000];
	list<dvbrcs_node_id>	node_id_cfg;
	uint32_t		feeder_nid;
	sprintf(filename, "%s.dvbrcs.nodeid", GetScriptName());
	if (_parse_nodeid_cfg(filename, node_id_cfg))
	{
		char		feeder_module_name[100];
		bool		nid_found = false;

		for (list<dvbrcs_node_id>::iterator it = node_id_cfg.begin();
		     it != node_id_cfg.end() && !nid_found; it++)
		{
			if (it->ncc_nid==get_nid())
			{
				feeder_nid = it->feeder_nid;
				nid_found = true;
			}
		}

		if (!nid_found)
		{
			printf("Node ID config file error??\n");
			assert(0);
		}

		sprintf(feeder_module_name, "Node%u_DVB_S2_FEEDER_LINK_1", feeder_nid);
		_feeder_ptr = (Dvb_s2*) RegTable_.lookup_Instance(feeder_nid, feeder_module_name);
		if (!_feeder_ptr)
			assert(0);
	}
	else
	{
		assert(0);
	}


	return (NslObject::init());
}

/*
 * send function
 */
/*OK*/
int
Ncc_ctl::send(ePacket_ *pkt) 
{

	return (NslObject::send(pkt));
}

/*
 * recv function
 */
/*OK*/
int
Ncc_ctl::recv(ePacket_ *Epkt)
{
	Dvb_pkt		*dvb_pkt;
	packet_type	pkt_type;


	dvb_pkt = (Dvb_pkt*)Epkt->DataInfo_;
	assert(dvb_pkt);
	pkt_type = dvb_pkt->pkt_gettype();

	switch (pkt_type) 
	{

		case (PKT_RCSSYNC):
		{
			// Handling the special case in which SAC receiving and scheduling happen at the same time.
			// In NCC, we assume the SAC receiving is later than scheduling.
			// So, we delay the receiving for one tick.
			if ((GetCurrentTime() % ncc_config.frame_duration) == 0)
			{
				printf("\e[36mNcc_ctl::Warning:SAC receiving and scheduling happen at the same time.\e[m\n");
				BASE_OBJTYPE(type);
				type = POINTER_TO_MEMBER(Ncc_ctl, recv);
				setObjEvent(Epkt,(GetCurrentTime()+1),0,this, type, Epkt->DataInfo_);
			}
			else
			{
				recv_sac(dvb_pkt);
				dvb_freePacket(Epkt);
			}
			break;
		}
		case (PKT_RAWDATA):
		{
			dvb_freePacket(Epkt);
			break;
		}

		case (PKT_RCSMAC):
		{
			dvb_freePacket(Epkt);
			break;
		}
		default:
			assert(0);
	}

	return (1);
}

uint64_t		
Ncc_ctl::ncc_to_sat_deley()
{
	return ( dvb_rcs_prop_delay(get_nid(), ncc_config.sat_node_id) );
}


uint64_t	
Ncc_ctl::ncc_to_rcst_delay(uint32_t rcst_node_id)
{
	const uint32_t sat_node_id = ncc_config.sat_node_id;

	return (dvb_rcs_prop_delay(rcst_node_id, sat_node_id) +
		ncc_to_sat_deley());
}

uint64_t		
Ncc_ctl::max_sat_to_rcst_delay()
{
	uint64_t			max_delay;
	list <Rcst_info>::iterator	it_rcst_info_list;
	

	max_delay = 0;

	uint32_t sat_node_id = ncc_config.sat_node_id;

	for(it_rcst_info_list=rcst_info_list.info_list.begin();
	    it_rcst_info_list!=rcst_info_list.info_list.end();
	    it_rcst_info_list++)
	{
		const uint32_t rcst_node_id = it_rcst_info_list->node_id;

		const uint64_t delay = dvb_rcs_prop_delay(sat_node_id, rcst_node_id);

		max_delay = (delay > max_delay) ?
			    delay : max_delay;
	}

	return (max_delay);
}

uint64_t		
Ncc_ctl::queueing_delay()
{
	return (ncc_config.frame_duration);
	uint64_t tick;
	double time = 150000 / (_feeder_ptr->get_symbol_rate() * 1000000);
	SEC_TO_TICK(tick, time);
	return (tick);
}

uint64_t		
Ncc_ctl::scheduling_delay()
{
	return (ncc_to_sat_deley() +
		max_sat_to_rcst_delay() +
		queueing_delay());
}


int
Ncc_ctl::frame_scheduling(Event_ *pEvn)
{
	_scheduler->scheduling();

	GenTBTP(NULL);

	dvb_freePacket(pEvn);
	return (0);
}

uint32_t
Ncc_ctl::bps_to_spf(uint64_t bps)
{
	return (ncc_config.bps_to_spf(bps));
}


uint32_t	
_rbdc_rate(const Sac::Capacity_request &request)
{
	Sac::Scaling_factor_value	scaling_factor;

	uint8_t		req_value = request.capacity_request_value;

	if(request.capacity_request_type != Sac::CAPACITY_REQUEST_TYPE_VALUE_RBDC)
	{
		printf("The type of capacity request isn't RBDC\n");
		assert(0);
	}

	scaling_factor = request.scaling_factor;

	return 2000 * req_value * scaling_factor;
}


uint32_t	
_num_atm_cell_of_req(const Sac::Capacity_request &request)
{
	if(request.capacity_request_type != Sac::CAPACITY_REQUEST_TYPE_VALUE_VBDC &&
		request.capacity_request_type != Sac::CAPACITY_REQUEST_TYPE_VALUE_AVBDC)
	{
		printf("The type of capacity request isn't VBDC/AVBDC\n");
		assert(0);
	}

	uint8_t		req_value = request.capacity_request_value;

	Sac::Scaling_factor_value	scaling_factor;

	scaling_factor = request.scaling_factor;

	return req_value * scaling_factor;
}
int 	
Ncc_ctl::recv_sac(Dvb_pkt *pkt)
{
	void*			sac_message;
	Rcst_id			rcst_id;
	uint8_t			num_req;
	Sac::Capacity_request	c_req;


	sac_message = pkt->pkt_detach();
	Sac*	sac = new Sac(sac_message, 1,1,CAPACITY_REQUEST_NUMBER-1 ,0,1,1,1);

	sac->get_group_id(rcst_id.group_id);
	sac->get_logon_id(rcst_id.logon_id);
	num_req = sac->get_capacity_request_number();
	/* For each capacity request in the received SAC message,
	 * insert it into the Capacity request heap.
	 */
	for(uint8_t i=0; i<=num_req; i++)
	{
		assert(!sac->pop_capacity_request(c_req));
		if (c_req.capacity_request_value > 0 ||
		    c_req.capacity_request_type==Sac::CAPACITY_REQUEST_TYPE_VALUE_RBDC)
		{
			heap_of_capacity_request.insert(Rcst_capacity_request(rcst_id, c_req));
		}
	}

	delete (sac);
	free (sac_message);
	return (1);
}


void
Ncc_ctl::set_sct()
{

	Sct*			sct;
	Sct_superframe_info	superframe_info;	
	Sct_frame_info		frame_info;	
	uint64_t		superframe_start_base;
	uint16_t		superframe_start_ext;
	uint32_t		centre_freq;
	uint16_t		max_superframe_id;
	uint8_t			frame_id;


	
	assert(_SCTs.empty());

	// Create one SCT table instance.
	sct = new Sct(DVBS2_NETWORK_ID, next_sct_version++, CUR_INDICATOR);

	// Add superframe information for each pair of (superframe_id, superframe_counter).
	max_superframe_id = ncc_config.max_superframe_id;
	for(uint8_t id=0; id<=max_superframe_id; id++)
	{
		uint16_t	superframe_counter;
		uint8_t		frame_number;
		uint16_t	timeslot_number;

		superframe_info.set_superframe_id(id);

		superframe_info.set_uplink_polarization(0);

		const uint32_t superframe_duration_in_pcr =
			tick_to_pcr(ncc_config.superframe_duration);

		superframe_info.set_superframe_duration(superframe_duration_in_pcr);


		ncc_config.current_slot(GetCurrentTime() + scheduling_delay(),
					superframe_counter,
					frame_number, timeslot_number);


		// Only send out next superframe info in SCT.
		superframe_counter++;
		uint64_t superframe_start = ncc_config.superframe_start(GetCurrentTime(), 
									superframe_counter);

		tick_to_pcr_base_and_ext(superframe_start,
					 superframe_start_base,
					 superframe_start_ext);

		centre_freq = (uint32_t) ceil(ncc_config.channel_centre_frequency(id) / 100.0);
		// Add superframe info.
		superframe_info.set_superframe_start_time_base(superframe_start_base);
		superframe_info.set_superframe_start_time_ext(superframe_start_ext);
		superframe_info.set_superframe_centre_frequency(centre_freq);
		superframe_info.set_superframe_counter(superframe_counter);

		frame_id = id;
		frame_info.set_frame_id(frame_id);
		frame_info.set_frame_start_time(0);
		frame_info.set_frame_centre_frequency_offset(0);

		assert((sct->add_superframe_info(superframe_info, frame_info)==0));

		// Add the other frame information. 
		for(uint8_t frame_num=1; frame_num<ncc_config.num_of_frame_per_superframe; frame_num++)
		{
			uint64_t frame_start_offset = ncc_config.frame_duration * frame_num;
			uint32_t frame_start_pcr = tick_to_pcr(frame_start_offset);

			frame_info.set_frame_start_time(frame_start_pcr);
			frame_info.set_frame_centre_frequency_offset(0);

			assert((sct->add_frame_info(frame_id,
						    superframe_counter, 
						    frame_info,
						    frame_num))==0);
		}
	}

	// Attach the created SCT table onto the SCT table queue.
	_SCTs.push_back(sct);
}

void
Ncc_ctl::set_fct()
{
	Fct*			fct;
	Fct_timeslot_info 	timeslot_info;
	Fct_frame_info		frame_info;
	uint16_t		superframe_id;
	uint64_t		timeslot_start_pcr;
	uint16_t		num_data_slot, num_req_slot;
	uint8_t			repeat_count;
	uint16_t		timeslot_number;
	const uint16_t		MAX_REP_COUNT = 255;
	u_char			frame_id;


	assert(_FCTs.empty());

	const uint32_t frame_duration_in_pcr =
		tick_to_pcr(ncc_config.frame_duration);

	// Create one FCT table instance.
	fct = new Fct(DVBS2_NETWORK_ID, next_fct_version++, CUR_INDICATOR);

	for(superframe_id=0; superframe_id<=ncc_config.max_superframe_id; superframe_id++)
	{
		frame_id = superframe_id;

		num_req_slot = ncc_config.num_of_req_slot_per_frame;

		num_data_slot = ncc_config.num_of_slot_per_frame -
				num_req_slot;

		frame_info.set_frame_id(frame_id);
		frame_info.set_frame_duration(frame_duration_in_pcr);
		/* The first timeslot_info. */
		timeslot_info.set_timeslot_frequency_offset(0);
		timeslot_info.set_timeslot_time_offset(0);

		if(num_req_slot > 0)
		{
			timeslot_info.set_timeslot_id(TIMESLOT_ID_REQ);

			repeat_count = (num_req_slot > MAX_REP_COUNT) ?
					MAX_REP_COUNT : (num_req_slot-1);

			timeslot_info.set_repeat_count(repeat_count);

			num_req_slot -= (repeat_count + 1);
		}
		else if(num_data_slot > 0)
		{
			timeslot_info.set_timeslot_id(TIMESLOT_ID_DATA);

			repeat_count = (num_data_slot > MAX_REP_COUNT) ?
					MAX_REP_COUNT : (num_data_slot-1);

			timeslot_info.set_repeat_count(repeat_count);

			num_data_slot -= (repeat_count + 1);
		}
		else
		{
			break;
		}

		fct-> add_frame_info(frame_info, timeslot_info);

		timeslot_number = (repeat_count + 1);
		
	
		while(num_req_slot > 0)
		{
			/* NOTE: repeat_count==i means (i+1) loops. */
			repeat_count = (num_req_slot > MAX_REP_COUNT) ?
					MAX_REP_COUNT : (num_req_slot-1);

			timeslot_info.set_repeat_count(repeat_count);

			num_req_slot -= (repeat_count + 1);
				
			timeslot_info.set_timeslot_id(TIMESLOT_ID_REQ);

			timeslot_start_pcr = tick_to_pcr(timeslot_number *
							 ncc_config.timeslot_duration);

			timeslot_info.set_timeslot_time_offset(timeslot_start_pcr);

			fct->add_timeslot_info(frame_id, timeslot_info, timeslot_number);

			timeslot_number += (repeat_count + 1);
		}

		while(num_data_slot > 0)
		{
			timeslot_info.set_timeslot_id(TIMESLOT_ID_DATA);

			/* NOTE: repeat_count==i means (i+1) loops. */
			repeat_count = (num_data_slot > MAX_REP_COUNT) ?
					MAX_REP_COUNT : (num_data_slot-1);

			timeslot_info.set_repeat_count(repeat_count);

			num_data_slot -= (repeat_count + 1);
				
			timeslot_start_pcr = tick_to_pcr(timeslot_number *
							 ncc_config.timeslot_duration);

			timeslot_info.set_timeslot_time_offset(timeslot_start_pcr);

			fct->add_timeslot_info(frame_id, timeslot_info, timeslot_number);

			timeslot_number += (repeat_count + 1);
		}
	}

	// Attach the created FCT table onto the FCT table queue.
	_FCTs.push_back(fct);
}

void
Ncc_ctl::set_tct()
{
	Tct*			tct;
	Tct_timeslot_info 	timeslot_info;
	uint8_t			sac_len;


	assert(_TCTs.empty());

	// Create one TCT table instance.
	tct = new Tct(DVBS2_NETWORK_ID, next_tct_version++, CUR_INDICATOR);


	// Set timeslot info.
	timeslot_info.set_timeslot_id(TIMESLOT_ID_DATA);

	timeslot_info.set_symbol_rate(ncc_config.symbol_rate);

	

	timeslot_info.set_timeslot_duration(tick_to_pcr(ncc_config.timeslot_duration)); 

	timeslot_info.set_burst_start_offset(tick_to_pcr(ncc_config.burst_start_offset())); 

	timeslot_info.set_timeslot_payload_type(ncc_config.num_of_atm_per_slot);

	timeslot_info.set_sac_length(0);
	timeslot_info.set_request_flag(0);
	timeslot_info.set_capacity_requests_number(0);
	timeslot_info.set_inner_code_type(0);
	timeslot_info.set_inner_code_ordering(0);
	timeslot_info.set_outer_coding(1);
	timeslot_info.set_inner_code_puncturing(0);
	timeslot_info.set_modulation(1);
	timeslot_info.set_baseband_shaping(0);
	timeslot_info.set_route_id_flag(1);
	timeslot_info.set_acm_flag(1);
	timeslot_info.set_m_and_c_flag(0);
	timeslot_info.set_group_id_flag(0);
	timeslot_info.set_logon_id_flag(0);
	timeslot_info.set_new_permutation(0);


	// Add the first timeslot information for data slot.
	tct-> add_timeslot_info(timeslot_info, 0, 0, 0, 0, ncc_config.preamble_length);
	
	// Add the second timeslot information for request slot.
	timeslot_info.set_timeslot_id(TIMESLOT_ID_REQ);
	timeslot_info.set_timeslot_payload_type(TIMESLOT_TYPE_SYNC);
	timeslot_info.set_group_id_flag(1);
	timeslot_info.set_logon_id_flag(1);
	sac_len = Sac::sac_length(1,1,NUM_REQ_SLOT_FOR_EACH_RCST,0,1,1,1);
	timeslot_info.set_sac_length(sac_len);
	timeslot_info.set_request_flag(1);
	timeslot_info.set_capacity_requests_number(CAPACITY_REQUEST_NUMBER-1);
	tct-> add_timeslot_info(timeslot_info, 0, 0, 0, 0, ncc_config.preamble_length);

	// Attach the created TCT table onto the TCT table queue.
	_TCTs.push_back(tct);
}



void
Ncc_ctl::set_broadcast_tim()
{
	Contention_control_descriptor	ccd;
	broadcast_tim.set_current_next_indicator(CUR_INDICATOR);

	ccd.set_csc_max_losses(csc_max_losses);
	ccd.set_csc_response_timeout(csc_response_timeout);
	ccd.set_max_time_before_retry(max_time_before_retry);
	ccd.set_superframe_id(superframe_id_for_logon);
	broadcast_tim.add_descriptor(&ccd);
}

int		
Ncc_ctl::gen_sct_fct_and_tct(Event_ *pEvn)
{
	GenSCT(NULL);
	GenFCT(NULL);
	GenTCT(NULL);

	dvb_freePacket(pEvn);
	return (1);
}



int
Ncc_ctl::GenSCT(Event_ *pEvn)
{ 
	set_sct();

	for (list<Sct*>::iterator sct, it = _SCTs.begin();
	     it != _SCTs.end();)
	{
		sct = it++;
		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		Dvb_pkt *pPkt = new Dvb_pkt();

		assert (pPkt);

		pPkt->pkt_attach((void*)(*sct) , UNKNOWN_LEN);

		pPkt->pkt_settype(PKT_TABLE);

		(pPkt->pkt_getfwdinfo())->pid = SCT_PID;

		assert (pEvn = createEvent());

		pEvn->DataInfo_ = pPkt;

		send(pEvn);
		_SCTs.erase(sct);
	}
	return 1;
}


int
Ncc_ctl::GenFCT(Event_ *pEvn)
{ 
	set_fct();

	for (list<Fct*>::iterator fct, it = _FCTs.begin();
	     it != _FCTs.end();)
	{
		fct = it++;
		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		Dvb_pkt *pPkt = new Dvb_pkt();

		assert(pPkt);

		pPkt->pkt_attach((void*)(*fct) , UNKNOWN_LEN);

		pPkt->pkt_settype(PKT_TABLE);

		(pPkt->pkt_getfwdinfo())->pid = FCT_PID;

		assert (pEvn = createEvent());

		pEvn->DataInfo_ = pPkt;

		send(pEvn);

		_FCTs.erase(fct);
	}
	return 1;
}


int
Ncc_ctl::GenTCT(Event_ *pEvn)
{ 
	set_tct();

	for (list<Tct*>::iterator tct, it = _TCTs.begin();
	     it != _TCTs.end();)
	{
		tct = it++;
		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		Dvb_pkt *pPkt = new Dvb_pkt();

		assert(pPkt);

		pPkt->pkt_attach((void*)(*tct) , UNKNOWN_LEN);

		pPkt->pkt_settype(PKT_TABLE);

		(pPkt->pkt_getfwdinfo())->pid = TCT_PID;

		assert (pEvn = createEvent());

		pEvn->DataInfo_ = pPkt;

		send(pEvn);

		_TCTs.erase(tct);
	}
	return 1;
}


int
Ncc_ctl::GenTBTP(Event_ *pEvn)
{ 
	Dvb_pkt			*pPkt;
	Tbtp 			*cpy = NULL;
	list<Tbtp*>::iterator	it;
	

	set_tbtp();

	//For each TBTP table...
	for(it = _TBTPs.tbtp_tables.begin();
	    it != _TBTPs.tbtp_tables.end();
	    it++)
	{
		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		pPkt = new Dvb_pkt();
		assert(pPkt);
		cpy = (*it)->copy();
		pPkt->pkt_attach((void*)cpy , UNKNOWN_LEN);
		pPkt->pkt_settype(PKT_TABLE);
		(pPkt->pkt_getfwdinfo())->pid = TBTP_PID;
		pEvn = createEvent();
		assert(pEvn);
		pEvn->DataInfo_ = pPkt;

		send(pEvn);
	}
	return (1);
}

int
Ncc_ctl::GenBroadcastTIM(Event_ *pEvn)
{
	Dvb_pkt		*pPkt;
	Tim			*cpy;

	pPkt = new Dvb_pkt();
	assert(pPkt);
	cpy = broadcast_tim.copy();
	pPkt->pkt_attach((void*)cpy , UNKNOWN_LEN);
	pPkt->pkt_settype(PKT_TABLE);
	(pPkt->pkt_getfwdinfo())->pid = TIM_PID;
	delete pEvn;
	pEvn = createEvent();
	assert(pEvn);
	pEvn->DataInfo_ = pPkt;
	return send(pEvn);
}

void
Ncc_ctl::set_tbtp()
{
        list<Slot_assignment>::iterator	it;
	list<Tbtp*>::iterator		tbtp_it;
        Tbtp*				tbtp;
        Tbtp_frame_info			frame_info;
        Tbtp_timeslot_info_entry	timeslot_info_entry;
	uint16_t			superframe_counter;
	uint8_t				frame_number;
	uint16_t			timeslot_number;


	_TBTPs.free();

	ncc_config.current_slot(GetCurrentTime() + scheduling_delay(), 
				superframe_counter, frame_number, 
				timeslot_number);
	const Frame_ident frame = ncc_config.next_frame(Frame_ident(superframe_counter, 
								    frame_number));
	superframe_counter = frame.superframe_count;
	frame_number = frame.frame_number;
        /* Scan request_slot_chunk_list and add timeslot_info to tbtp */
        for(it = slot_assignment_record->request_slot_chunk_list.begin(); 
	    it!=slot_assignment_record->request_slot_chunk_list.end(); 
	    it++)
        {
                // Set up a Tbtp_timeslot_info_entry
                timeslot_info_entry.set_logon_id(it->rcst_id.logon_id);
                timeslot_info_entry.set_start_slot(it->start_slot);
                timeslot_info_entry.set_assignment_count(it->assignment_count);
                timeslot_info_entry.set_multiple_channels_flag(0);
                timeslot_info_entry.set_assignment_type(0);
                timeslot_info_entry.set_channel_id(0);

		uint64_t credit_value;

		_scheduler->rcst_credits.get_credit_value(it->rcst_id, Timeslot_scheduler::Rcst_credit::VB_CREDIT, credit_value);

		uint8_t queue_empty = (credit_value==0);

                timeslot_info_entry.set_vbdc_queue_empty_flag(queue_empty);


                if((tbtp = _TBTPs.get_tbtp(next_tbtp_version, 
					   DVBS2_NETWORK_ID,
					   it->rcst_id.group_id,
					   superframe_counter) ))
                {
                        tbtp->add_timeslot_info(frame_number, 
						timeslot_info_entry);
                }
                else {
                        tbtp = new Tbtp(DVBS2_NETWORK_ID, 
					next_tbtp_version, 
					CUR_INDICATOR, 
					superframe_counter, 
					it->rcst_id.group_id);

                        frame_info.set_frame_number(frame_number);

                        tbtp->add_frame_info(frame_info, 
					     timeslot_info_entry);

			_TBTPs.add_tbtp(tbtp);
                }
        }
        /* Scan data_slot_chunk_list and add timeslot_info to tbtp */
        for(it = slot_assignment_record->data_slot_chunk_list.begin(); 
	    it!=slot_assignment_record->data_slot_chunk_list.end(); 
	    it++)
        {
                // Set up a Tbtp_timeslot_info_entry
                timeslot_info_entry.set_logon_id(it->rcst_id.logon_id);
                timeslot_info_entry.set_start_slot(it->start_slot);
                timeslot_info_entry.set_assignment_count(it->assignment_count);
                timeslot_info_entry.set_multiple_channels_flag(0);
                timeslot_info_entry.set_assignment_type(0);
                timeslot_info_entry.set_channel_id(0);


		uint64_t credit_value;

		_scheduler->rcst_credits.get_credit_value(it->rcst_id, Timeslot_scheduler::Rcst_credit::VB_CREDIT, credit_value);

		uint8_t queue_empty = (credit_value==0);

                timeslot_info_entry.set_vbdc_queue_empty_flag(queue_empty);

                if((tbtp = _TBTPs.get_tbtp(next_tbtp_version, 
					   DVBS2_NETWORK_ID, 
					   it->rcst_id.group_id,
					   superframe_counter) )!=NULL)
                {
                        tbtp->add_timeslot_info(frame_number, 
						timeslot_info_entry);
                }
                else {
                        tbtp = new Tbtp(next_tbtp_version, 
					DVBS2_NETWORK_ID, 
					CUR_INDICATOR, 
					superframe_counter, 
					it->rcst_id.group_id);

                        frame_info.set_frame_number(frame_number);

                        tbtp->add_frame_info(frame_info, 
					     timeslot_info_entry);

			_TBTPs.add_tbtp(tbtp);
                }
        }


	next_tbtp_version++;
}

int
Ncc_ctl::_parse_nodeid_cfg(char *filename, list<dvbrcs_node_id> &global_system_node_id)
{
	global_system_node_id.clear();
	FILE    *nodeid_cfg;
	if (!(nodeid_cfg = fopen(filename, "r"))) {
		printf("[RCST_CTL] Warning: Cannot open file %s", filename);
		assert(0);
	}
	else {

		char    line[200];
		char    buf[200];
		while (fgets(line, 200, nodeid_cfg)) {
			dvbrcs_node_id	one_dvbrcs_node_id;
			if (sscanf(line, " DVB-RCS:%s", buf)) {
				char*   tok;
				tok = strtok(buf, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.sat_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.feeder_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.gw_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.sp_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.ncc_nid)));
					
				while ((tok = strtok(NULL, "_"))) {
					struct rcst_node_id	one_rcst_node_id;
					assert(sscanf(tok, "%u", &one_rcst_node_id.rcst_nid));
					one_dvbrcs_node_id.rcst_nid_list.push_back(one_rcst_node_id);
				}
			}
			else if (sscanf(line, " #%s", buf))
				continue;

			global_system_node_id.push_back(one_dvbrcs_node_id);
		}
		fclose(nodeid_cfg);
		return 1;
	}
}
