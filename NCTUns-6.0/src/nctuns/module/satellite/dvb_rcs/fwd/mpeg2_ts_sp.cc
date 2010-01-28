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

#include <regcom.h>
#include "mpeg2_ts_sp.h"
#include <nctuns_api.h>
#include "../common/dvbrcs_api.h"
#include <math.h>
#include <satellite/dvb_rcs/common/table/section_draft.h>
#include <satellite/dvb_rcs/fwd/dvb_s2_feeder.h>

extern RegTable RegTable_;

MODULE_GENERATOR(Mpeg2_ts_sp);

//
//  Constructor
//
Mpeg2_ts_sp::Mpeg2_ts_sp(uint32_t type, uint32_t id, struct plist *pl, const char *name)
: NslObject(type, id, pl, name), _node_type(NODE_ID_NONE)
{
	/*
	 * Initialize _feeder_name and _feeder_nid
	 */
	_feeder_nid = 0;
	_feeder_name = (char *)malloc(sizeof(char) * 100);

	// Initialize sec_info link list
	si_start = new sec_info;
	si_start->pid = 0xffff;
	si_start->ts_pkt_cnt = 7;
	si_start->data_ptr = NULL;
	si_start->next = NULL;


	// Initialize conti_count link list
	cc_start = new conti_count;
	cc_start->pid = 0xffff;
	cc_start->conti_cnt = 7;
	cc_start->next = NULL;
}


//
// Destructor
//
Mpeg2_ts_sp::~Mpeg2_ts_sp()
{
	free(_feeder_name);

	free_sec_info(si_start);
	delete si_start;

	free_conti_count(cc_start);
	delete cc_start;
}

int
Mpeg2_ts_sp::init() {
	int *type_id;
	char *file_path;
	FILE *fd;
	char buf[101];
	int match_cnt;

	/*
	 * get _node_type from control module
	 */
	type_id = GET_REG_VAR1(get_portls(), "NODE_TYPE", int *); 
	if (type_id)
		_node_type = (dvb_node_type) *type_id;

	/*
	 * get lower layer object name
	 */

	file_path = (char *)malloc(strlen(GetScriptName())+30);
	sprintf(file_path, "%s%s%d", GetScriptName(), ".dvbrcs.to.feeder.", get_nid());

	if ((fd=fopen(file_path, "r")) == NULL) {
		printf("Error: can't open file %s\n", file_path);
		exit(-1);
	}
	free(file_path);

	while( !feof(fd) ) {
		buf[0] = '\0'; fgets(buf, 100, fd);
		if ((buf[0]=='\0')||(buf[0]=='#'))
			continue;

		match_cnt = sscanf(buf, "%d.%s", &_feeder_nid, _feeder_name);
		if (match_cnt != 2) {
			printf("Warning: format error -> %s\n", buf);
			exit(-1);
		}
	}
	fclose(fd);

	Dvb_s2_feeder		*feeder_obj = NULL;
	assert(feeder_obj = (Dvb_s2_feeder*) RegTable_.lookup_Instance(_feeder_nid, _feeder_name));
	feeder_obj->set_mpeg2_ts_sp(this);
	return NslObject::init();
}


/*
 * Receive
 */
int
Mpeg2_ts_sp::recv(ePacket_ *pkt)
{
	/* mpeg2_ts_sp has no recv function */
	assert(0);
}


/*
 * Send.
 */
int
Mpeg2_ts_sp::send(ePacket_ *pkt)
{
	int			i, len_of_section, section_pid;
	char			*data_ptr;
	conti_count		*curr_pkt_cc;
	forward_link_info	*info;
	Dvb_pkt			*dvbpkt;
	Dvb_s2_feeder		*feeder_obj = NULL;

	/*
	 * if this node is rcst, by pass all packet
	 */
	if (_node_type == NODE_ID_RCST)
		return NslObject::send(pkt);

	// Get Dvb_pkt info
	dvbpkt = (Dvb_pkt*)pkt->DataInfo_;

	if(dvbpkt-> pkt_gettype() == PKT_SECTION)
	{
		// Get Dvb_pkt info: pid, section length

		// Get pid. section length
		info = dvbpkt->pkt_getfwdinfo();
		section_pid = info->pid;
		len_of_section = dvbpkt->pkt_getlen();

		// Detach data
		data_ptr = (char *)((Dvb_pkt *)pkt->DataInfo_)->pkt_getdata();
		if( (curr_pkt_cc=search_conti_count_entry(section_pid))==0 )
		{	
			conti_count *ccptr = new conti_count;
			ccptr->pid = section_pid;
			ccptr->conti_cnt = 0;
			ccptr->next = cc_start->next;
			cc_start->next = ccptr;
			curr_pkt_cc = ccptr;
		}

		for(i=0; i<(int)ceil(len_of_section/184.0); i++) {
			class Dvb_pkt	*Dpkt;
			class Ts_header	*TSpkt;
			void		*ts188;
			ePacket_ 	*pkt1;

			TSpkt = new Ts_header;
			TSpkt->_sync_byte			= 0x47;
			TSpkt->_transport_error_indicator	= 0x00;
			TSpkt->_payload_unit_start_indicator	= !i;
			TSpkt->_transport_priority		= 0;
			TSpkt->_pid				= section_pid;
			TSpkt->_transport_scrambling_ctl	= 0x00;
			TSpkt->_adaptation_field_ctl		= 0x01;
			TSpkt->_continuity_counter		= curr_pkt_cc->conti_cnt;
			TSpkt->_adaptation_field		= NULL;
			TSpkt->_data_byte			= (char*)data_ptr + 184 * i;


			ts188 = malloc(188);
			// serialize, copy data to ts188
			memcpy(ts188, TSpkt, 4);
			if((int)ceil((double)len_of_section/184)-i == 1)
				memcpy((char *)ts188+4, TSpkt->_data_byte, len_of_section%184);
			else
				memcpy((char *)ts188+4, TSpkt->_data_byte, 184);

			Dpkt = new Dvb_pkt;
			Dpkt->pkt_attach(ts188, 188);
			Dpkt->pkt_setflag(FLOW_SEND);
			Dpkt->pkt_settype(PKT_MPEG2_TS);
			memcpy(Dpkt->pkt_getfwdinfo(), info, sizeof(struct forward_link_info));

			Dpkt->pkt_getfwdinfo()->interface = 0;

			pkt1 = new ePacket_;
			pkt1->DataInfo_ = Dpkt;
			curr_pkt_cc->conti_cnt++;
			delete(TSpkt);

			/*
			 * find lower layer object pointer and jump send to
			 * upper layer, upper layer may be at other node
			 */
			assert(feeder_obj = (Dvb_s2_feeder*) RegTable_.lookup_Instance(_feeder_nid, _feeder_name));



			if (is_forward_control(Dpkt) == 1)
			{
				if (feeder_obj->send(pkt1) < 0) {
					/*
					 * The packet sending fails due to the buffer
					 * below is full, so we should release the
					 * un-processed packet.
					 */
					assert(0);
					dvb_freePacket(pkt1);
				}
			}
			else
			{
				//put the traffic TS packet into waiting queue.
				_ts_buf.push_back(pkt1);
			}
		}
		// success send all ts pkts
		/*
		 * Release the event received from upper layer.
		 */
		dvb_freePacket(pkt);
		return 1;
	}
	/* 
	 * pkt_type is not a section_pkt, then bypass it
	 */
	else
	{
		return NslObject::send(pkt);
	}
	assert(0);
}

int
Mpeg2_ts_sp::send_traffic_ts()
{
	Dvb_s2_feeder		*feeder_obj = NULL;

	assert(feeder_obj = (Dvb_s2_feeder*) RegTable_.lookup_Instance(_feeder_nid, _feeder_name));

	while (!_ts_buf.empty() && (feeder_obj->remain_len() > 0))
	{
		ePacket_* pkt = _ts_buf.front();

		_ts_buf.pop_front();

		if (feeder_obj->send(pkt) < 0) 
		{
			dvb_freePacket(pkt);
		}
	}

	return (0);
}


/*
 * Search conti_count entry of the pid
 * in the cc_start link list
 */
conti_count*
Mpeg2_ts_sp::search_conti_count_entry(int pid) {
	conti_count	*p;
	p = cc_start;
	while(p != NULL) {
		if(p->pid == pid)
			return p;
		p = p->next;
	}
	return NULL;
}

/*
 * Search section_info entry of the pid
 * in the sec_info link list
 */
sec_info*
Mpeg2_ts_sp::search_sec_info_entry(int pid) {
	sec_info	*p;
	p = si_start;
	while(p != NULL) {
		if(p->pid == pid)
			return p;
		p = p->next;
	}
	return NULL;
}


int
Mpeg2_ts_sp::del_sec_info_entry(int pid)
{
	sec_info	*curr_si, *prev_si;
	if( (curr_si=search_sec_info_entry(pid))!=0 ) {
		prev_si = search_prev_sect_info_entry(curr_si);
		prev_si->next = curr_si->next;
		free(curr_si);
		return 0;
	}
	return -1;
}


/*
 * search the previous sect_info_entry of
 * the current sect_info_entry
 */
sec_info*
Mpeg2_ts_sp::search_prev_sect_info_entry(sec_info *curr_si){
	sec_info	*prev_si;
	prev_si = si_start;
	while( prev_si->next != NULL ){
		if( prev_si->next->pid == curr_si->pid )
			return prev_si;
		prev_si = prev_si->next;
	}
	return NULL;
}

void
Mpeg2_ts_sp::free_conti_count(conti_count *ptr){
	conti_count *tmp;

	while(ptr->next != 0) {
		tmp = ptr->next;
		ptr->next=(ptr->next)->next;
		delete tmp;
	}
}

void
Mpeg2_ts_sp::free_sec_info(sec_info *ptr){
	sec_info *tmp;

	while(ptr->next != 0) {
		tmp = ptr->next;
		ptr->next=(ptr->next)->next;
		delete tmp;
	}
}

