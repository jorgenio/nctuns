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
#include <event.h>
#include <assert.h>
#include <object.h>
#include <netinet/in.h>
#include <satellite/dvb_rcs/common/pcr.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/si_config.h>
#include <satellite/dvb_rcs/common/sac.h>
#include <satellite/dvb_rcs/ncc/ncc_ctl.h>
#include <satellite/dvb_rcs/ret/queue/rcst_queue_manager.h>
#include <satellite/dvb_rcs/ret/rcs_mac.h>
#include <ip.h>
#include <tcp.h>
#include <udp.h>
#include "rcst_ctl.h"
#include "rcst_queue_config.h"
#include <regcom.h>
#include <packet.h>


#define SAC_SYMBOL_LEN		296
#define QPSK_BIT_PER_SYMBOL	2
MODULE_GENERATOR(Rcst_ctl)

extern RegTable                 RegTable_;

void
Rcst_ctl::Slot_pool::clear(uint16_t superframe_count, uint8_t frame_number, Slot_type type)
{
	list<slot_info>::iterator it, tmp;

	assert (type==DATA || type==REQUEST);

	list<slot_info>& to_process = (type==DATA) ? _data_slot_list : _req_slot_list;


	for (it = to_process.begin(); it != to_process.end();) 
	{
		tmp = it++;

		if (tmp->superframe_count==superframe_count &&
		    tmp->frame_number==frame_number)
		{
			to_process.erase(tmp);
		}
	}
}


int
Rcst_ctl::Slot_pool::insert(slot_info &info, uint16_t superframe_count, 
			    uint8_t frame_number, Slot_type type)
{
	assert (type==DATA || type==REQUEST);

	list<slot_info>& to_process = (type==DATA) ? _data_slot_list : _req_slot_list;

	to_process.push_back(info);

	return (0);
}


uint32_t
Rcst_ctl::Slot_pool::size(uint16_t superframe_count, uint8_t frame_number, Slot_type type)
{
	list<slot_info>::iterator it;

	assert (type==DATA || type==REQUEST);

	list<slot_info>& to_process = (type==DATA) ? _data_slot_list : _req_slot_list;

	uint32_t cnt = 0;


	for (it = to_process.begin(); it != to_process.end();it++) 
	{
		if (it->superframe_count==superframe_count &&
		    it->frame_number==frame_number)
		{
			cnt++;
		}
	}

	return (cnt);
}


/************************************************************
 * Find the first slot info of matching superframe_count, 
 * frame_number, and type.
 ***********************************************************/
slot_info*
Rcst_ctl::Slot_pool::pop_front(uint16_t superframe_count, uint8_t frame_number, Slot_type type)
{
	slot_info* info = NULL;

	list<slot_info>::iterator it, tmp;

	assert (type==DATA || type==REQUEST);

	list<slot_info>& to_process = (type==DATA) ? _data_slot_list : _req_slot_list;


	for (it = to_process.begin(); (it!=to_process.end()) && !info;) 
	{
		tmp = it++;

		if (tmp->superframe_count==superframe_count &&
		    tmp->frame_number==frame_number)
		{
			assert (info = new slot_info(*tmp));

			to_process.erase(tmp);
		}
	}

	return (info);
}



/*
 * constructor
 */
Rcst_ctl::Rcst_ctl(u_int32_t type , u_int32_t id , struct plist* pl , const char* name)
:NslObject(type , id , pl , name) 
{
	rcst_state = RCST_STATE_OFF;

	char buf[200];

	/*
	 * version number initialization
	 */
	pat_cur_version_num = pmt_cur_version_num = nit_cur_version_num = int_cur_version_num = sct_cur_version_num = fct_cur_version_num = tct_cur_version_num = tbtp_cur_version_num = cmt_cur_version_num = spt_cur_version_num = INIT_VER_NUM;

	pat_next_version_num = pmt_next_version_num = nit_next_version_num = int_next_version_num = sct_next_version_num = fct_next_version_num = tct_next_version_num = tbtp_next_version_num = cmt_next_version_num = spt_next_version_num = INIT_VER_NUM;

	/*
	 * state initialization of all control tables
	 */
	_patState = _pmtState = _int_pmtState = _trf_pmtState = 
		_nitState = _intState = _sctState = _tbtpState = 
		_fctState = _tctState = _ccdState = NOT_READY;

	_first_ready = true;
	_fwd_ctl_state = FWD_CTL_NOT_READY;
	_ret_ctl_state = RET_CTL_NOT_READY;


	_mpe_flag = false;


	_node_type = NODE_ID_RCST;
	REG_VAR("NODE_TYPE", &_node_type);
	REG_VAR("RCST_PTR", this);

	schedule_timer_for_mac = false;

	/*
	 * allocate rcst_queue_config object
	 */
	sprintf(buf, "%s.dvbrcs.rcst_queue.%d", GetScriptName(), get_nid());
	assert((_rcst_config = new Rcst_queue_config(buf)));
	REG_VAR("RCST_CONFIG_PTR", _rcst_config);

	/*
	 * calculate jitter
	 */
	jitter_fptr = NULL;

	granted_slot_list.clear();
}

int
Rcst_ctl::_parse_nodeid_cfg(char *filename, list<dvbrcs_node_id> &global_system_node_id)
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


/*
 * destructor
 */
Rcst_ctl::~Rcst_ctl() 
{

	/*
	 * calculate jitter
	 */
	if (jitter_fptr) {
		fclose(jitter_fptr);
		jitter_fptr = NULL;
	}

	delete (_rcst_config);
//	_free_tbtp_timeslot_info_entry_list(&tbtp_timeslot_info_entry_list);
}

/*
 * init function
 */
int
Rcst_ctl::init() 
{
	char 		buf[100];
	Rcst_id		rcst_id;
	char		filename[1000];

	/*
	 * get ncc_ctl module pointer
	 */
	/* get ncc_node_id */
	sprintf(filename, "%s.dvbrcs.nodeid", GetScriptName());
	if (_parse_nodeid_cfg(filename, _node_id_cfg))
	{
		uint32_t	ncc_node_id;
		char		ncc_ctl_module_name[100];

		for (list<dvbrcs_node_id>::iterator it = _node_id_cfg.begin();
		     it != _node_id_cfg.end(); it++)
		{
			list<rcst_node_id>	&p_list = it->rcst_nid_list;
			for(list<rcst_node_id>::iterator it_rcst_nid = p_list.begin(); 
			    it_rcst_nid != p_list.end(); it_rcst_nid++)
			{
				if(it_rcst_nid->rcst_nid == get_nid())
				{
					ncc_node_id = it->ncc_nid;
					break;
				}
			}
		}

		sprintf(ncc_ctl_module_name, "Node%u_NCC_CTL_LINK_1", ncc_node_id);
		_ncc_module = (Ncc_ctl*) RegTable_.lookup_Instance(ncc_node_id, ncc_ctl_module_name);
		if (!_ncc_module)
			assert(0);
		_ncc_config = &(_ncc_module->ncc_config);
	}
	else
	{
		assert(0);
	}

	Rcst_info_list* const rcst_info_list = &(_ncc_module->rcst_info_list);
	REG_VAR("NCC_CONFIG", _ncc_config);
	REG_VAR("TIMEOUT_IN_SUPERFRAME", &rbdc_timeout);

	assert(!rcst_info_list->get_rcst_id(rcst_id, get_nid()));
	assert(!rcst_info_list->get_vbdc_max(vbdc_max, rcst_id));
	assert(!rcst_info_list->get_cra_level(cra_level, rcst_id));
	assert(!rcst_info_list->get_rbdc_max(rbdc_max, rcst_id));
	assert(!rcst_info_list->get_rbdc_timeout(rbdc_timeout, rcst_id));

	_my_group_id = rcst_id.group_id;
	_my_logon_id = rcst_id.logon_id;
	assert(!rcst_info_list->get_superframe_id_for_transmission(_my_superframe_id, 
								   rcst_id));
	REG_VAR("MY_SUPERFRAME_ID", &_my_superframe_id);

	_RcstIPaddr 	= GET_REG_VAR1(get_portls(), "IP", u_long *);
	assert(_RcstIPaddr);
	/*
	 * get queue manager object pointer from rcs_mac
	 */
	_rcst_qm = GET_REG_VAR1(get_portls(), "QUEUE_QM_PTR", Rcst_queue_manager **);
	_rcs_rcst_mac = GET_REG_VAR1(get_portls(), "RCS_RCST_MAC_PTR", Rcs_mac *);

	assert(_rcst_qm);
	assert(_rcs_rcst_mac);

	last_time = ~0x00;
	last_interval = ~0x00;
	if (!jitter_fptr) 
	{
		sprintf(buf, "%s.jitter", GetScriptName());
		assert((jitter_fptr = fopen(buf, "w+")));
	}

	//Note: The timer is triggered one tick after start time of each frame.
	//We plus one tick to ensoure _compute_demand() is called before
	//_send_sac_to_next_module() is called.
	timer_to_grant_demand_and_compute_demand.setCallOutObj(this , 
							       (int (NslObject::*)(Event_ *))
							       &Rcst_ctl::_grant_demand_and_compute_demand);

	timer_to_grant_demand_and_compute_demand.start(_ncc_config->frame_duration, 
						       _ncc_config->frame_duration);

	return (NslObject::init());
}

int
Rcst_ctl::send_csc()
{
	return 0;
}

int
Rcst_ctl::check_tim()
{
	return 0;
}

/*NOTE: send function only support raw data now.*/
/*
 * send function
 */
int
Rcst_ctl::send(ePacket_ *Epkt) 
{
	Dvb_pkt		*dvb_pkt;
	Packet		*pkt;
	struct ip 	*iphdr = NULL;
	struct tcphdr 	*tcp = NULL;
	struct udphdr 	*udp = NULL;
	uint32_t 	queue_id;


	assert(pkt = (Packet*)Epkt->DataInfo_);

	/*
	 * find ip and tcphdr header
	 */
	iphdr = (struct ip *)pkt->pkt_sget();
	switch (iphdr->ip_p) {
		case IPPROTO_TCP:
			tcp = (struct tcphdr *)((char *)iphdr + (iphdr->ip_hl << 2));
			queue_id = _rcst_config->match_flow_rule(iphdr->ip_src,
				tcp->th_sport, iphdr->ip_dst,
				tcp->th_dport, iphdr->ip_p);
			break;
		case IPPROTO_UDP:
			udp = (struct udphdr *)((char *)iphdr + (iphdr->ip_hl << 2));
			queue_id = _rcst_config->match_flow_rule(iphdr->ip_src,
				udp->uh_sport, iphdr->ip_dst,
				udp->uh_dport, iphdr->ip_p);
			break;
		default:
			printf("\e[35m RCST send a packet which is neither TCP nor UDP.\e[m\n");
			queue_id = _rcst_config->match_flow_rule(iphdr->ip_src, 0, iphdr->ip_dst, 0, iphdr->ip_p);
			break;
	}

	/*
	 * queue_id is equal to zero, that mean this packet cannot match any
	 * rules
	 */
	if (queue_id == 0) {
//		printf("\e[33m Classification error!! This packet is dropped.\e[m\n");
		freePacket(Epkt);
		return (1);
	}

	/*
	 * just transfer original pkt to dvb pkt, then sent it.
	 */
	dvb_pkt = new Dvb_pkt();
	dvb_pkt->convert_from_nctuns_pkt(pkt);
	dvb_pkt->pkt_settype(PKT_RAWDATA);
	dvb_pkt->pkt_setflag(FLOW_SEND);
	delete (pkt);

	/*
	 * setting queue_id
	 */
	dvb_pkt->pkt_getretinfo()->queue_id = queue_id;

	Epkt->DataInfo_ = dvb_pkt;
	return (NslObject::send(Epkt));
}

/*
 * recv function
 * Judge input data which is either TCP/IP pkts or control messages, then handle them individually.
 */
int
Rcst_ctl::recv(ePacket_ *Epkt) 
{
	Dvb_pkt		*dvb_pkt;
	packet_type	pkt_type;

	assert(dvb_pkt = (Dvb_pkt*)Epkt->DataInfo_);

	pkt_type = dvb_pkt->pkt_gettype();

	switch (pkt_type) {

		case (PKT_RAWDATA): {

			return _recv_data(Epkt);
		} 
		case (PKT_TABLE): {

			return _recv_control_mes(Epkt);
		} 
		default:
			assert(0);
	}
}

/*
 * _locate_MPEstream_pid function
 * By this function, we can parse tables needed and look up which pid rcst has. 
 */
int
Rcst_ctl::_locate_MPEstream_pid()
{ 
	Linkage_descriptor			*link_des;
	Ip_mac_stream_location_descriptor	*str_loc_des;
	uint16_t				mpe_service_id_by_int , mpe_com_tag_by_int;
	uint16_t				pat_ts_by_int;
	uint16_t				int_service_id , int_ts_id , int_pmt_pid , int_pid , trf_pmt_pid;
	uint16_t				mpe_stream_pid;

	link_des = (Linkage_descriptor*)(_nit->get_network_loop_des(LINKAGE_DESCRIPTOR , 2 , 0x0B));
	
	/*
	 * linkage descriptor 0x0B with valid platform id present
	 */
	if (link_des){

		int_service_id = link_des->get_service_id();
		int_ts_id = link_des->get_transport_stream_id();
		delete link_des;

		/*
		 * need no tune to other transport stream
		 */
		if (int_ts_id == TS_ID) {

			/*	
			 * lookup PMT's pid of INT service by int_service_id
			 */
			int_pmt_pid = _pat->get_PMT_pid(int_service_id);
		
			/*
			 * becasue PMT's pid of INT service has been defined by user
			 * so if int_pmt_pid == "INT_PMT_PID", it indicate int_pmt_pid value is we need 
			 * otherwise, it is wrong.
			 */
			if (int_pmt_pid == INT_PMT_PID) {

				/*
				 * get pid of INT service 
		   	 	 */
				int_pid = _int_pmt->get_es_info_des_pid(2 , DATA_BROADCAST_ID_DESCRIPTOR  , 0x0B);
	
				/*
				 * becasue pid of INT service has been defined by user
				 * so if int_pid == "INT_PID", it indicate int_pid value is we need 
				 * otherwise, it is wrong.
				 */
				if (int_pid == INT_PID) {

					/*
					 * By looking up INT table, we will get transport stream id, service id of MPE data 
					 * and component tag according to local Rcst's IP address.
					 */
					str_loc_des = (Ip_mac_stream_location_descriptor *)(_int->get_operational_descriptor_by_IP_address(TARGET_IP_SLASH_DESCRIPTOR , *_RcstIPaddr));
					assert(str_loc_des);
					mpe_service_id_by_int = str_loc_des->get_service_id();
					mpe_com_tag_by_int = str_loc_des->get_component_tag();
					pat_ts_by_int = str_loc_des->get_transport_stream_id();
					delete str_loc_des;

					/*
					 * becasue transport stream id has been defined by user
					 * so if pat_ts_by_int == "TS_ID", it indicate pat_ts_by_int value is we need 
					 * otherwise, it is wrong.
					 */
					if (pat_ts_by_int == TS_ID) {

						/*
						 * By mpe_service_id_by_int, we can get PMT's pid of tcp/ip traffic.
						 */
						trf_pmt_pid = _pat->get_PMT_pid(mpe_service_id_by_int);

						/*
						 * becasue PMT's pid of tcp/ip traffic has been defined by user
						 * so if trf_pmt_pid == "TRF_PMT_PID", it indicate trf_pmt_pid value is we need 
						 * otherwise, it is wrong.
						 */
						if (trf_pmt_pid == TRF_PMT_PID) {

							/*
							 * Finally, we will get elementary stream PID of MPE data by
							 * component tag.
							 */
							_fwd_ctl_state = FWD_CTL_READY;
							
							mpe_stream_pid = _trf_pmt->get_es_info_des_pid(2 , STREAM_IDENTIFIER_DESCRIPTOR , mpe_com_tag_by_int);
							return mpe_stream_pid;
						}
						else {
							assert(0);
						}
					}
					/*
					 * need to tune to other transport stream
					 * non-implement yet.
					 */
					else {
						assert(0);
					}
				}
				else {
					assert(0);
				}
			}
			else {
				assert(0);
			}
		}
		/*
		 * have to tune to other transport stream.
		 * non-implement yet.
		 */
		else {


		}
	}
	/*
	 * have to lookup linkage descriptor of 0x0C, 0x04.
	 * non-implement yet.
	 */
	else {


	}
        return -1;
}

/*
 * _recv_data function
 * According to received data type, if this is TCP/IP pkt, we will judge pid of the pkt and 
 * decide whether send it to interface or not.
 */
int
Rcst_ctl::_recv_data(ePacket_ *Epkt)
{ 
	Dvb_pkt		*dvb_pkt;
	Packet		*pkt;
	uint16_t	pid;

	/*
	 * if rcst is not ready, drop this pkt.
	 */
	if (_fwd_ctl_state == FWD_CTL_NOT_READY) 
	{
		dvb_freePacket(Epkt);
		return (1);
	}
	else
	{
		assert(dvb_pkt = (Dvb_pkt*)Epkt->DataInfo_);

		pid = (dvb_pkt->pkt_getfwdinfo())->pid;

		/*
		 * the pkt is sent to me, therefore we have to convert its pkt struct and forward it.
	 	 */
		if (pid == _MPEstream_pid) 
		{
			uint64_t	interval;

			/*
			 * calculate jitter
			 */
			assert(jitter_fptr);

			if (last_time == (uint64_t)~0x00) 
			{
				last_time = GetCurrentTime();
			}
			else if (last_interval == (uint64_t)~0x00) 
			{
				last_interval = GetCurrentTime() - last_time;
				last_time = GetCurrentTime();
			}
			else 
			{
				interval = GetCurrentTime() - last_time;

				last_time = GetCurrentTime();

				fprintf(jitter_fptr, 
					"[JITTER] Current = %llu, jitter = %ld\n", 
					last_time, 
					labs(interval - last_interval));

				last_interval = interval;
			}
			pkt = dvb_pkt->convert_to_nctuns_pkt();	

			delete (dvb_pkt);

			Epkt->DataInfo_ = pkt;


			return (NslObject::recv(Epkt));
		}
		/*
		 * the pkt is not sent to me, so drop it.
		 */
		else 
		{
			dvb_freePacket(Epkt);
			return (1);
		}
	}

        return (0);
}

int
Rcst_ctl::_recv_control_mes(ePacket_ *Epkt)
{ 
	Dvb_pkt		*dvb_pkt;
	Table		*table;
	uint16_t	pid;

	/*
	 * get table_id of control tables.
	 */
	assert(dvb_pkt = (Dvb_pkt*)Epkt->DataInfo_);

	pid = (dvb_pkt->pkt_getfwdinfo())->pid;

	table = (Table*)(dvb_pkt->pkt_detach());



	/*
	 * judge if the table is need.
	 * if version number of tables received are <= "current version number", then drop them.
	 * otherwise, append them into tables' circleqs if "current_next indicatior" is set to "CUR".
	 */
	switch (table->get_table_id()) 
	{

		/*
		 * no other possible pids maybe present.
		 */
		case (PAT_TABLE_ID): {

			if (_patState == NOT_READY) {
				_pat = (Pat*) table;
				_patState = READY;
			}
			else
			{
				delete ((Pat*) table);
			}
			break;	
		} 
		/*	
	 	 * now we accept all PMT tables, but this mechanism may be changed later.
		 */
		case (PMT_TABLE_ID): {
		
			/* this table is PMT table of INT service */
			if (pid == INT_PMT_PID) {

				if (_int_pmtState == NOT_READY) {
					_int_pmt = (Pmt*) table;
					_int_pmtState = READY;
				}
				else
				{
					delete ((Pmt*) table);
				}

				if (_int_pmtState == READY && _trf_pmtState == READY)
				{
						_pmtState = READY;
				}
				break;
			}
			/* this table is PMT table of tcp/ip traffic service */
			if (pid == TRF_PMT_PID) {

				if (_trf_pmtState == NOT_READY) {
					_trf_pmt = (Pmt*) table;
					_trf_pmtState = READY;
				}
				else
				{
					delete ((Pmt*) table);
				}

				if (_int_pmtState == READY && _trf_pmtState == READY)
				{
						_pmtState = READY;
				}
				break;
			}

			delete ((Pmt*) table);
			break;
		} 
		/*
		 * no other possible pids maybe present.
		 */
		case (NIT_ACTIVE_TABLE_ID): {
			if (_nitState == NOT_READY) 
			{
				_nit = (Nit*) table;
				_nitState = READY;
			}
			else
			{
				delete ((Nit*) table);
			}
			break;	
		}
		/*	
	 	 * now we accept only one INT table ==> "one IP_MAC_PLATFORM assumption", but this mechanism may be changed later.
		 */
		case (INT_TABLE_ID): {

			if (_intState == NOT_READY) {
				_int = (Int*) table;
				_intState = READY;
			}
			else
			{
				delete ((Int*) table);
			}
			break;	
		} 
		case (SCT_TABLE_ID):
		{
			char version_of_incoming_table 	= ((Sct*) table)->get_version_number();

			set_timer_to_delete_sct((Sct*) table);

			list<Sct*>::iterator	it;

			for(it = _sct_list.begin(); it!=_sct_list.end(); it++)
			{
				if((*it)->get_version_number()==version_of_incoming_table)
				{
					delete (*it);
					_sct_list.erase(it);
					break;
				}
			}

			_sct_list.push_back((Sct*) table);

			/* Trigger logon procedure, if RCST is currently in OFF state and
			 * control tables is ready.
			 */
			_sctState = READY;
			if(_ready_for_logon() && (rcst_state==RCST_STATE_OFF))
			{
				rcst_state = RCST_STATE_RECEIVE_SYNC;
			}

			break;	
		} 
		case (FCT_TABLE_ID): {

			char version_of_incoming_table 	= ((Fct*) table) ->get_version_number();
			list<Fct*>::iterator	it;

			// Remove the table of same version.
			for(it = _fct_list.begin(); it!=_fct_list.end(); it++)
			{
				Fct*	fct;
				char version	= (*it) ->get_version_number();

				if(version==version_of_incoming_table)
				{
					fct = *it;
					_fct_list.erase(it);
					delete fct;
					break;
				}
			}

			_fct_list.push_back((Fct*) table);
			set_timer_to_delete_fct((Fct*) table);
			break;
		} 
		case (TCT_TABLE_ID): {

			char version_of_incoming_table 	= ((Tct*) table) ->get_version_number();
			list<Tct*>::iterator	it;

			// Remove the table of same version.
			for(it = _tct_list.begin(); it!=_tct_list.end(); it++)
			{
				Tct	*tct;
				char version	= (*it) ->get_version_number();

				if(version==version_of_incoming_table)
				{
					tct = *it;
					_tct_list.erase(it);
					delete tct;
					break;
				}
			}

			_tct_list.push_back((Tct*) table);
			set_timer_to_delete_tct((Tct*) table);
			break;

		} 
		case (TBTP_TABLE_ID): {
			uint8_t group_id = ((Tbtp *)table)->get_group_id();

			if (group_id == _my_group_id) // Only parse TBTPs devoted to my group.
			{
				_parse_tbtp((Tbtp *)table);
			}

			delete ((Tbtp*) table);
			break;	
		} 
		/*	
		 * non-implement yet.
		 */
		case (CMT_TABLE_ID): {
			break;	
		} 
		/*	
		 * non-implement yet.
		 */
		case (SPT_TABLE_ID): {
			break;	
		} 
		case (TIM_TABLE_ID):
		{
			Tim			*tim;
			u_char		mac_addr_in_tim[6];
			char		status = 0;

			tim = (Tim*) table;
			tim -> get_mac_address(mac_addr_in_tim);

			if( memcmp(mac_addr_in_tim, my_mac_address, sizeof(mac_addr_in_tim))==0 )
			/* The MAC address in TIM is the same as myself one.
			 * Which indicate the status should be interpreted as "RCST status".
			 */
			{
				bool	logon_fail		= status & 64;
				bool	logon_denied	= status & 32;

				tim_received_since_csc_sent_out = true;

				if( !(logon_fail | logon_denied)) //Logon success.
				{
					Logon_initialize_descriptor *logon_des	= ((Logon_initialize_descriptor*) 
						tim ->get_descriptor(1, LOGON_INITIALIZE_DESCRIPTOR));
					assert(logon_des);
					_my_logon_id = logon_des ->get_logon_id();
					_my_group_id = logon_des ->get_group_id();
					cra_level = logon_des ->get_CRA_level();
					vbdc_max = logon_des ->get_VBDC_max();
					rbdc_max = logon_des ->get_RBDC_max();
					rbdc_timeout = logon_des ->get_RBDC_timeout();
				}
				else
				{
				}
			}
			else
			/* Otherwise, the status should be interpreted as "Network status". */
			{
				/* Draw superframe_id, csc_response_timeout, csc_max_losses, and
				 * max_time_before_retry subfields in Contention_control_descriptor.
				 */
				Contention_control_descriptor *c_c_des	= ((Contention_control_descriptor*)
					tim ->get_descriptor(1, CONTENTION_CONTROL_DESCRIPTOR));
				assert(c_c_des);
				if(c_c_des)
				{
					superframe_id_for_logon = c_c_des ->get_superframe_id();
					csc_response_timeout = c_c_des ->get_csc_response_timeout();
					csc_max_losses = c_c_des ->get_csc_max_losses();
					max_time_before_retry = c_c_des ->get_max_time_before_retry();
					_ccdState = READY;
				}
			}

			delete ((Tim*) table);
			break;
		}
		/*
		 * this rcst need no this type table.
		 */
		default: {
			assert(0);
			dvb_freePacket(Epkt);
			return 1;
		}
	}

	/*
	 * if all control tables are ready, then update pid of MPE data
	 */
	if (_patState == READY && _pmtState == READY && _nitState == READY && _intState == READY && _first_ready) {
		_first_ready = false;

		int	mpe_pid;

		mpe_pid = _locate_MPEstream_pid();

		if (mpe_pid != -1)
			_MPEstream_pid = (uint16_t)mpe_pid;

		else
			assert(0);

		_mpe_flag = true;
	}

	dvb_freePacket(Epkt);
        return 1;
}

/* compute queue's request, then put these requests to sac */
Sac*
Rcst_ctl::_pack_request_to_sac(Slot_flags& flags)
{
	uint8_t			req_num;
	uint64_t		queue_len;	//in ATM cell.
	uint64_t		rate;	//in 2k bits/sec.

	Sac			*sac;


	sac = new Sac();

	sac->set_route_id_flag(flags.route_id_flag);
	

	sac->set_request_flag(flags.request_flag);
	
	req_num = flags.capacity_request_number;

	sac->set_m_and_c_flag(flags.m_and_c_flag);
	

	sac->set_group_id_flag(flags.group_id_flag);
	
	if(flags.group_id_flag==1)
	{
		sac->set_group_id(_my_group_id);
	}
	
	sac->set_logon_id_flag(flags.logon_id_flag);
	
	if(flags.group_id_flag==1)
	{
		sac->set_logon_id(_my_logon_id);
	}

	sac->set_acm_flag(flags.acm_flag);
	
	
	rate = demand_to_send_in_next_frame.rate;
	queue_len = demand_to_send_in_next_frame.queue_len;
	demand_to_send_in_next_frame.rate = 0;
	demand_to_send_in_next_frame.queue_len = 0;

	// Channel_id is not used --> set as 0.
	sac->fill_req(Sac::CAPACITY_REQUEST_TYPE_VALUE_RBDC,
		      rate, 0, req_num);

	if ((*_rcst_qm)->next_request_is_avbdc())
	{
		sac->fill_req(Sac::CAPACITY_REQUEST_TYPE_VALUE_AVBDC,
			      queue_len, 0, req_num);
	}
	else //VBDC.
	{
		sac->fill_req(Sac::CAPACITY_REQUEST_TYPE_VALUE_VBDC,
			      queue_len, 0, req_num);
	}

	sac->pad_dummy_req(req_num);

	return (sac);

}


int
Rcst_ctl::_send_sac_to_next_module(Event *ep)
{
	Dvb_pkt		*dvb_pkt;
	Sac		*sac;
	void		*sac_message;
	uint8_t		len;
	Slot_flags	*flags;
	slot_info	*ctrl_slot_info;


	ctrl_slot_info = ((slot_info*) ep->DataInfo_);
	flags = &(ctrl_slot_info->flags);

	dvb_pkt = new Dvb_pkt;

	SET_EVENT_CALLOUTOBJ(ep, NULL, NULL, (void *)dvb_pkt);

	sac = _pack_request_to_sac((*flags));
	len = sac->sac_length();

	sac_message = sac->gen_sac_message(len);

	dvb_pkt->pkt_attach(sac_message, len);

	dvb_pkt->pkt_setflag(FLOW_SEND);

	dvb_pkt->pkt_settype(PKT_RCSSYNC);

	dvb_pkt->pkt_getretinfo()->timeslot = *ctrl_slot_info;
	dvb_pkt->pkt_getretinfo()->queue_id = 0 ;

	delete (ctrl_slot_info);

	delete (sac);

	ep->DataInfo_ = dvb_pkt;

	return (NslObject::send(ep));
}


/* parse tbtp and inform rcs_mac that how many timeslots be granted*/
/*
 * data slot --> attached into free_slot_list.
 * req slot --> set up timer(s) to send SAC.
 */
int
Rcst_ctl::_parse_tbtp(Tbtp *tbtp)
{
	list<Tbtp_timeslot_info_entry>	tbtp_timeslot_info_list;
	uint32_t			num_of_req_timeslots = 0;
	uint32_t			num_of_data_timeslots = 0;
	uint8_t				channel_id;
	const uint16_t superframe_count = superframe_count_in_the_last_tbtp = tbtp->get_superframe_count();

	num_sac_entry_to_send = 0;

	const uint8_t frame_number_in_tbtp = frame_number_in_the_last_tbtp = tbtp->get_frame_number();

	/* get timeslot infos from TBTP. */
	tbtp->get_timeslot_info_list(frame_number_in_tbtp, _my_logon_id, 
				     channel_id, tbtp_timeslot_info_list);


	_slot_pool.clear(superframe_count, 
			 frame_number_in_tbtp, 
			 Slot_pool::DATA);

	_slot_pool._req_slot_list.clear();

	/*Check if TBTP is out of date.*/
	if (_ncc_config->frame_start(GetCurrentTime(), superframe_count, 
				     frame_number_in_tbtp) < GetCurrentTime())
	{
		printf("\e[35mTBTP is out of date!!!\e[m\n");
		return (0);
	}

	// For each slot sequence...
	for (list<Tbtp_timeslot_info_entry>::iterator it = tbtp_timeslot_info_list.begin();
	     it != tbtp_timeslot_info_list.end(); it++)
	{
		if (it->get_logon_id() != _my_logon_id) // Not for me.
			continue;

		const uint16_t start_slot = it->get_start_slot();
		const uint16_t end_slot = start_slot + it->get_assignment_count();


		/* Keep track of the vbdc_empty_flag. */
		const bool vbdc_empty_flag = it->get_vbdc_queue_empty_flag();
		bool record_exist = false;
		for (list<Vbdc_empty_record>::iterator it= _vbdc_empty_table.begin();
		     it != _vbdc_empty_table.end(); it++)
		{
			if (it->frame.superframe_count==superframe_count &&
			    it->frame.frame_number==frame_number_in_tbtp)
			{
				record_exist = true;
				it->vbdc_empty = vbdc_empty_flag;
				break;
			}
		}

		if (!record_exist)
		{
			Vbdc_empty_record record;
			record.frame.frame_number = frame_number_in_tbtp;
			record.frame.superframe_count = superframe_count;
			record.vbdc_empty = vbdc_empty_flag;
			_vbdc_empty_table.push_back(record);
		}



		// Handle each slot for me.
		for (uint16_t timeslot_number = start_slot;
		     timeslot_number <= end_slot; timeslot_number++)
		{
			slot_info	s_info;
			if (_fetch_slot_info(superframe_count, frame_number_in_tbtp,
					     timeslot_number, &s_info))
			{
				const Slot_type slot_type =  _slot_type(superframe_count, 
									frame_number_in_tbtp, 
									timeslot_number);

				if (slot_type==SLOT_TYPE_REQUEST)
				{
					/* handle req timeslot */
					/*
					 * for each req timeslot 'i', set up a timer(start at the begining of timeslot 'i') to do follows:
					 * 1. ask rcs_mac to return how many req data(eg, return n) want to send
					 * 2. pack these n req to a SAC ctrl pkt
					 * 3. send SAC ctrl pkt to next module
					 * these 3 task occur at the same time(simulation time)
					 */


					/* setup a timer to call _send_sac_to_next_module */
					BASE_OBJTYPE(type);
					Event* ep = createEvent();
					assert(ep);
					type = POINTER_TO_MEMBER(Rcst_ctl, _send_sac_to_next_module);


					const uint64_t send_time = s_info.start_time + s_info.burst_start_offset;
					setObjEvent(ep, send_time, 0, this, 
						    type, new slot_info(s_info));

					num_of_req_timeslots++;

					_slot_pool.insert(s_info, superframe_count,
							  frame_number_in_tbtp,
							  Slot_pool::REQUEST);
					// Store the number of total SAC entries for compute_demand() to 
					// compute the maximum request total which could be send in the
					// next frame.
					num_sac_entry_to_send += (s_info.flags.capacity_request_number + 1);
				}
				else if(slot_type==SLOT_TYPE_DATA)
				{
					_slot_pool.insert(s_info, superframe_count,
							  frame_number_in_tbtp,
							  Slot_pool::DATA);

					num_of_data_timeslots++;
				}
				else // SLOT_TYPE_CAN_NOT_DETERMINE
				{
					// Do nothing.
				}
			}
		}
	}

	assert (num_of_req_timeslots <= 1);

	return (0);
}


int
Rcst_ctl::queue_grant_demand(uint16_t superframe_count, uint8_t frame_number, uint32_t queue_id, uint8_t amount)
{
	for (uint8_t cnt = 0; cnt < amount; cnt++)
	{
		slot_info* info = _slot_pool.pop_front(superframe_count, 
						       frame_number, 
						       Slot_pool::DATA);

		if (info)
		{
			info->queue_id = queue_id;
			granted_slot_list.push_back(*info);
			delete (info);
		}
		else
		{
			printf("Rcst_ctl::queue_grant_demand [Warning] -- no enough free slot\n");
			assert(0);
			return (-1);
		}
	}

	return (0);
}


void print_timeslot(slot_info *slot)
{
	printf("***************** slot_info **********************\n");
	printf("queue_id;        %u\n"  ,slot->queue_id        );
	printf("superframe_count;%hu\n" ,slot->superframe_count);
	printf("frame_number;    %hhu\n",slot->frame_number    );
	printf("start_slot;      %hhu\n",slot->start_slot      );
	printf("start_time;      %llu\n",slot->start_time      );
	printf("timeslot_amount; %hhu\n",slot->timeslot_amount );
	printf("***************** slot_info **********************\n");

}

struct  slot_info*      
Rcst_ctl::require_timeslot(struct slot_info *one_slot_info, uint32_t queue_id)
{
	assert(one_slot_info);
	/* update granted list, delete overtime slot_info entry*/
	_remove_out_of_date_grant();

	/* search _granted_list */
	for (list<slot_info>::iterator it = granted_slot_list.begin();
	     it != granted_slot_list.end(); it++)
	{
		if (it->queue_id == queue_id) // The nearest granted slot is found.
		{
			if (it->start_time < GetCurrentTime())
			{
				printf("Error!! _remove_out_of_date_grant() doesn't work correctly??\n");
				assert(0);
			}

			*one_slot_info = (*it);

			return (one_slot_info);
		}
	}

	return (NULL);
}


void
Rcst_ctl::_remove_out_of_date_grant()
{
	granted_slot_list.remove_if(_is_out_of_date());
}


bool
Rcst_ctl::_ready_for_logon()
{
	if((_sctState==READY) && (_fctState==READY) && (_tctState==READY) && (_ccdState==READY))
		return true;
	else
		return false;
}


int		
Rcst_ctl::set_timer_to_delete_sct(Sct* recv_sct)
{
	// We keep this table in ten superframe duration.
	uint64_t del_time = (GetCurrentTime()+ 10 * _ncc_config->superframe_duration);

	Sct** sct = (Sct**) malloc(sizeof(Sct*));

	*sct = recv_sct;

	BASE_OBJTYPE(type);

	Event_ *ep = createEvent();
	assert (ep);

	type = POINTER_TO_MEMBER(Rcst_ctl, _del_sct);

	setObjEvent(ep, del_time , 0, this, type, sct);

	return (0);
}


int		
Rcst_ctl::set_timer_to_delete_fct(Fct* recv_fct)
{
	// We keep this table in ten superframe duration.
	uint64_t del_time = (GetCurrentTime()+ 10 * _ncc_config->superframe_duration);

	Fct** fct = (Fct**) malloc(sizeof(Fct*));

	*fct = recv_fct;

	BASE_OBJTYPE(type);

	Event_ *ep = createEvent();
	assert (ep);

	type = POINTER_TO_MEMBER(Rcst_ctl, _del_fct);

	setObjEvent(ep, del_time , 0, this, type, fct);

	return (0);
}


int		
Rcst_ctl::set_timer_to_delete_tct(Tct* recv_tct)
{
	// We keep this table in ten superframe duration.
	uint64_t del_time = (GetCurrentTime()+ 10 * _ncc_config->superframe_duration);

	Tct** tct = (Tct**) malloc(sizeof(Tct*));

	*tct = recv_tct;

	BASE_OBJTYPE(type);

	Event_ *ep = createEvent();
	assert (ep);

	type = POINTER_TO_MEMBER(Rcst_ctl, _del_tct);

	setObjEvent(ep, del_time , 0, this, type, tct);

	return (0);
}

int
Rcst_ctl::_del_sct(Event* ep)
{
	list<Sct*>::iterator	it;

	Sct* sct = (Sct*) ep->DataInfo_;

	for(it = _sct_list.begin(); it!=_sct_list.end(); it++)
	{
		if(sct == *it)
		{
			_sct_list.erase(it);
			delete sct;
			break;
		}
	}
	freeEvent(ep);

	return 0;
}


int
Rcst_ctl::_del_tct(Event* ep)
{
	list<Tct*>::iterator	it;

	Tct* tct = (Tct*) ep->DataInfo_;

	for(it = _tct_list.begin(); it!=_tct_list.end(); it++)
	{
		if(tct == *it)
		{
			_tct_list.erase(it);
			delete tct;
			break;
		}
	}
	freeEvent(ep);

	return 0;
}

int
Rcst_ctl::_del_fct(Event* ep)
{
	list<Fct*>::iterator	it;

	Fct* fct = (Fct*) ep->DataInfo_;

	for(it = _fct_list.begin(); it!=_fct_list.end(); it++)
	{
		if(fct == *it)
		{
			_fct_list.erase(it);
			delete fct;
			break;
		}
	}
	freeEvent(ep);

	return 0;
}



/************************************************************
 * _fetch_slot_info() pass SCT, FCT, and TCT to find out
 * timeslot parameters.
 ***********************************************************/
slot_info*	
Rcst_ctl::_fetch_slot_info(uint16_t superframe_count, uint8_t frame_number, 
		 uint16_t timeslot_number, slot_info* info_buf)
{
	assert(info_buf);

	uint8_t		version, frame_id, timeslot_id;

	bool frame_info_found = false;

	// Search appliable SCT to find frame_id and version.
	for (list<Sct*>::iterator sct_it = _sct_list.begin(); 
	     sct_it!=_sct_list.end(); sct_it++)
	{
		Sct_frame_info		sct_frame_info_buf;

		if ((frame_info_found = ! ((*sct_it)->get_frame_info(_my_superframe_id, superframe_count, 
							       frame_number, &sct_frame_info_buf))))
		{
			frame_id = sct_frame_info_buf.get_frame_id();
			version	= (*sct_it) ->get_version_number();
			break;
		}
	}

	if (!frame_info_found)
	{
		return (NULL);
	}

	bool fct_found = false;
	// Search appliable FCT to find timeslot_id.
	for(list<Fct*>::iterator fct_it = _fct_list.begin(); 
	    fct_it!=_fct_list.end(); fct_it++)
	{
		Fct_timeslot_info	fct_timeslot_info_buf;

		if (version == (*fct_it) ->get_version_number() ) // FCT found.
		{
			if (!(*fct_it) ->get_timeslot_info(frame_id, timeslot_number, 
							   &fct_timeslot_info_buf)) 
			{
				timeslot_id = fct_timeslot_info_buf.get_timeslot_id();
			}
			else
			{
//				printf("\e[36mMatching FCT found, but Fct_timeslot_info doesn't exist.\e[m\n");
				return (NULL);
			}
			fct_found = true;
			break;
		}
	}
	
	if (!fct_found)
	{
//		printf("\e[36mCan't find any matching FCT.\e[m\n");
		return (NULL);
	}

	bool tct_found = false;
	// Search appliable TCT to find slot_info.
	for (list<Tct*>::iterator tct_it = _tct_list.begin(); 
	     tct_it!=_tct_list.end(); tct_it++)
	{
		if (version == (*tct_it) ->get_version_number() )
		{
			Tct_timeslot_info	tct_timeslot_info;
			uint8_t			p0, preamble_length;
			uint16_t		p1, p2, p3;

			if (!(*tct_it)->get_timeslot_info(&tct_timeslot_info, timeslot_id, 
							  p0, p1, p2, p3, preamble_length))
			{
				// Fill Timeslot parameters and then return.

				info_buf->flags.acm_flag = tct_timeslot_info.acm_flag;
				info_buf->flags.capacity_request_number = tct_timeslot_info.capacity_requests_number;
				info_buf->flags.group_id_flag = tct_timeslot_info.group_id_flag;
				info_buf->flags.logon_id_flag = tct_timeslot_info.logon_id_flag;
				info_buf->flags.m_and_c_flag = tct_timeslot_info.m_and_c_flag;
				info_buf->flags.request_flag = tct_timeslot_info.request_flag;
				info_buf->flags.route_id_flag = tct_timeslot_info.route_id_flag;
				info_buf->frame_number = frame_number;
				info_buf->group_id = _my_group_id; 
				info_buf->inner_code_type = tct_timeslot_info.inner_code_type;
				info_buf->inner_code_puncturing = (inner_coding_puncture)tct_timeslot_info.inner_code_puncturing;
				info_buf->inner_code_ordering = tct_timeslot_info.inner_code_ordering;
				info_buf->logon_id = _my_logon_id;

				const uint32_t centre_freq = _ncc_config->channel_centre_frequency(_my_superframe_id);
				const uint32_t bandwidth = _ncc_config->bandwidth_per_superframe();
				info_buf->max_frequency = (uint32_t) (centre_freq + 0.5*bandwidth); 
				info_buf->min_frequency = (uint32_t) (centre_freq - 0.5*bandwidth);
				info_buf->new_permutation = tct_timeslot_info.new_permutation;

				info_buf->outer_coding = tct_timeslot_info.outer_coding;
				info_buf->preamble_length = preamble_length;
				info_buf->queue_id = 0; 
				info_buf->start_slot = timeslot_number;

				info_buf->burst_start_offset = _ncc_config->burst_start_offset();
				info_buf->start_time = _ncc_config->timeslot_start(GetCurrentTime(),
										   superframe_count,
										   frame_number,
										   timeslot_number);

				info_buf->superframe_count = superframe_count;
				info_buf->symbol_rate = tct_timeslot_info.symbol_rate;
				info_buf->timeslot_amount = 1;
				info_buf->timeslot_payload_type = tct_timeslot_info.timeslot_payload_type;
					
				return (info_buf);
			}
			else
			{
				return (NULL);
			}
			tct_found = true;
			break;
		}
	}

	// Reach here only when TCT is not found.
	assert(!tct_found);
	return (NULL);
}


Rcst_ctl::Slot_type	
Rcst_ctl::_slot_type(uint16_t superframe_count, uint8_t frame_number,
	   uint16_t timeslot_number)
{
	slot_info	info;

	if (_fetch_slot_info(superframe_count, frame_number,
			     timeslot_number, &info))
	{
		return (info.flags.request_flag) ? SLOT_TYPE_REQUEST : SLOT_TYPE_DATA;
	}
	else
	{
		return (SLOT_TYPE_CAN_NOT_DETERMINE);
	}
}


/************************************************************
 * Dispatch slots for the incoming frame.
 ***********************************************************/
int		
Rcst_ctl::_grant_demand()
{
	// _grant_demand() is called in end of the frame closely before the target frame.
	const Frame_ident incoming_frame = _ncc_config->next_frame(_ncc_config->current_frame());

	const Frame_ident tbtp_frame(superframe_count_in_the_last_tbtp, 
				     frame_number_in_the_last_tbtp);

	const uint32_t data_slot_num = _slot_pool.size(incoming_frame.superframe_count,
						       incoming_frame.frame_number,
						       Slot_pool::DATA);

	const uint32_t req_slot_num = _slot_pool.size(incoming_frame.superframe_count, 
						      incoming_frame.frame_number, 
						      Slot_pool::REQUEST);


	const bool tbtp_lost = !(incoming_frame==tbtp_frame);

	bool vbdc_empty = false;

	for (list<Vbdc_empty_record>::iterator it= _vbdc_empty_table.begin();
	     it != _vbdc_empty_table.end(); it++)
	{
		if (it->frame==incoming_frame)
		{
			vbdc_empty = it->vbdc_empty;
			_vbdc_empty_table.erase(it);
			break;
		}
	}

	/* informs rcs_mac having how many data timeslots of frame "check_frame_number"*/
	(*_rcst_qm)->grant_demand(incoming_frame.superframe_count, incoming_frame.frame_number, 
				  _ncc_config->num_of_atm_per_slot, vbdc_max,
				  data_slot_num, req_slot_num, false, tbtp_lost, vbdc_empty);

	// Clear un-used free slot.
	_slot_pool.clear(incoming_frame.superframe_count, 
			 incoming_frame.frame_number, 
			 Slot_pool::DATA);
	return (0);
}

int		
Rcst_ctl::_grant_demand_and_compute_demand(Event* ep)
{
	/* For VR-JT and JT, demand computation should be done after demand granting.
	 * However, for RT and VR-RT, demand granting should be done first.
	 */

	// Actually, request is sent in the next frame.
	const Frame_ident current_frame = _ncc_config->next_frame(_ncc_config->current_frame());

	
	/* comptute requests of RT/VR-RT queues */
	const uint32_t spent_rate = (*_rcst_qm)->compute_demand_for_rt_vrrt(num_sac_entry_to_send,
									    _ncc_config->frame_duration,
									    current_frame, _msl());

	_grant_demand();

	/* comptute the request in all queues */
	demand_to_send_in_next_frame = (*_rcst_qm)->compute_demand_for_vrjt_jt(num_sac_entry_to_send,
									       spent_rate,
									       _ncc_config->frame_duration,
									       current_frame, _msl());

	// Remind RCS_MAC to get timeslot info.
	if (schedule_timer_for_mac)
	{
		schedule_timer_for_mac = false;
		_rcs_rcst_mac->schedule_wait_timer();
	}

	freeEvent(ep);
	num_sac_entry_to_send = 0;

	return (0);
}

/************************************************************
 * Return :
 * 	zero		->	fail(no request slot gotten).
 * 	non-zero	->	success.
 ***********************************************************/
uint32_t
Rcst_ctl::_msl()
{
	uint64_t	tbtp_sent;
	uint8_t		frame_number;
	uint16_t	superframe_counter, timeslot_number;

	// We check the only one request slot to fetch out the time to send SAC.
	list<slot_info>::iterator req_slot_info = _slot_pool._req_slot_list.begin();

	if (req_slot_info==_slot_pool._req_slot_list.end()) // No request slot.
	{
		return (0);
	}

	// Compute the time and the frame number in which SAC is sent.
	const uint64_t sac_send_start = (req_slot_info->start_time +
					 req_slot_info->burst_start_offset);

	superframe_counter = req_slot_info->superframe_count;

	frame_number = req_slot_info->frame_number;

	const Frame_ident frame_in_which_sac_sent(superframe_counter, frame_number);

	const uint64_t tx_time = _rcs_rcst_mac->calculate_tx_time(SAC_SYMBOL_LEN,
								  req_slot_info->symbol_rate / 1000.0);
	// Compute the frame number in which TBTP sent.
	const uint64_t sac_received = (sac_send_start + tx_time + 
				       _ncc_module->ncc_to_rcst_delay(get_nid()));

	_ncc_config->current_slot(sac_received, superframe_counter, 
				  frame_number, timeslot_number);

	Frame_ident frame_in_which_sac_received(superframe_counter, frame_number);

	// In NCC, we assume the SAC receiving is later than scheduling.
	if ((sac_received % _ncc_config->frame_duration) == 0)
	{
		printf("\e[36mRcst_ctl::Warning:SAC receiving and scheduling happen at the same time.\e[m\n");
		frame_in_which_sac_received = _ncc_config->next_frame(frame_in_which_sac_received);
	}


	const Frame_ident frame_in_which_tbtp_sent = _ncc_config->next_frame(frame_in_which_sac_received);

	// Compute the time TBTP sent.
	tbtp_sent = _ncc_config->frame_start(GetCurrentTime(), 
					     frame_in_which_tbtp_sent.superframe_count, 
					     frame_in_which_tbtp_sent.frame_number);

	// Compute the time TBTP received.
	const uint64_t tbtp_received = tbtp_sent + _ncc_module->scheduling_delay();

	// Compute the frame number in which TBTP received.
	_ncc_config->current_slot(tbtp_received, superframe_counter, 
				  frame_number, timeslot_number);

	const Frame_ident frame_in_which_tbtp_received(superframe_counter, frame_number);

	const Frame_ident target_frame = _ncc_config->next_frame(frame_in_which_tbtp_received);

	return _ncc_config->frame_minus(target_frame,
					frame_in_which_sac_sent);
}
