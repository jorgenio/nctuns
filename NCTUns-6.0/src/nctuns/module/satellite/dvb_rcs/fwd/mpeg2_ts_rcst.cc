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
#include <regcom.h>
#include "mpeg2_ts_rcst.h"
#include <nctuns_api.h>
#include "../common/dvbrcs_api.h"
#include <math.h>
#include <satellite/dvb_rcs/common/table/section_draft.h>

extern RegTable RegTable_;

MODULE_GENERATOR(Mpeg2_ts_rcst);

//
//  Constructor
//
Mpeg2_ts_rcst::Mpeg2_ts_rcst(uint32_t type, uint32_t id, struct plist *pl, const char *name)
: NslObject(type, id, pl, name), _node_type(NODE_ID_NONE)
{

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
Mpeg2_ts_rcst::~Mpeg2_ts_rcst()
{

	free_sec_info(si_start);
	delete si_start;

	free_conti_count(cc_start);
	delete cc_start;
}

int
Mpeg2_ts_rcst::init() {
	int *type_id;


	/*
	 * get _node_type from control module
	 */
	type_id = GET_REG_VAR1(get_portls(), "NODE_TYPE", int *); 
	if (type_id)
		_node_type = (dvb_node_type) *type_id;

	return NslObject::init();
}


/*
 * Receive
 */
int
Mpeg2_ts_rcst::recv(ePacket_ *pkt)
{
	Dvb_pkt			*dvb_pkt;
	sec_info		*curr_tspkt_si;
	int			len_of_section;
	class Ts_header		*ts_pkt;
	void			*sec_data;	// 188 bytes section data

	/*
	 * if this node is not rcst, by pass all packet
	 */
	if (_node_type != NODE_ID_RCST)
		return NslObject::recv(pkt);

	// Get Dvb_pkt pointer
	dvb_pkt = (Dvb_pkt *)pkt->DataInfo_;

	if(dvb_pkt-> pkt_gettype() == PKT_MPEG2_TS)
	{
		/* Detach Dvb_pkt data */
		sec_data = dvb_pkt->pkt_detach();

		ts_pkt = new Ts_header;
		memcpy(ts_pkt, sec_data, 4);

		curr_tspkt_si = search_sec_info_entry(ts_pkt->_pid);

	/*
		 * For continue state, the discontinue Ts_header wrll drop
		 * If the pkt is continue then handle it or drop
		 */
		if(ts_pkt->_payload_unit_start_indicator == 1
		|| (curr_tspkt_si != 0
			&& ts_pkt->_continuity_counter == curr_tspkt_si->next_conti_cnt) )
		{
			// if Ts_header is a start, then create new sec_info_entry
			if( ts_pkt->_payload_unit_start_indicator==1 )
			{
				sec_info 	*new_si;
				Section_draft	*sec_dft;

				// to get section_len
				sec_dft = ((class Section_draft*)
					(&((uint8_t*)
					sec_data)[ts_pkt->header_len()]));

				len_of_section = sec_dft->get_section_length() + 3;

				new_si = new sec_info;
				new_si->sect_len = len_of_section;

				if( curr_tspkt_si!=0 )
				{
					free(curr_tspkt_si->data_ptr);
					del_sec_info_entry(ts_pkt->_pid);
				}

				new_si->pid = ts_pkt->_pid;
				new_si->ts_pkt_cnt = (int)ceil(new_si->sect_len/184.0);
				new_si->next_conti_cnt = ts_pkt->_continuity_counter+1;
				new_si->data_ptr = malloc(new_si->sect_len);
				new_si->next = si_start->next;
				si_start->next = new_si;
				curr_tspkt_si = new_si;
			}

			len_of_section = curr_tspkt_si->sect_len;

			// put section_payload to correct memory location
			if(curr_tspkt_si->ts_pkt_cnt == 1 && (len_of_section % 184) != 0) {
				memcpy((char*)(curr_tspkt_si->data_ptr) +
						184 * ((int)(ceil((double)len_of_section / 184))
							- (curr_tspkt_si->ts_pkt_cnt)),
						(char *)sec_data + 4, len_of_section % 184);
			} else {
				memcpy((char*)(curr_tspkt_si->data_ptr) +
						184 * ((int)(ceil((double)len_of_section/184))
							- (curr_tspkt_si->ts_pkt_cnt)),
						(char *)sec_data + 4, 184);
			}
			/*
			 * Release the payload of Dvb_pkt received from the upper layer.
			 */

			curr_tspkt_si->next_conti_cnt = ts_pkt->_continuity_counter + 1;

			// recv a complete section
			if((--curr_tspkt_si->ts_pkt_cnt)==0)
			{
				class Dvb_pkt*	dvb_pkt1;

				dvb_pkt1 = new Dvb_pkt;
				dvb_pkt1->pkt_attach(curr_tspkt_si->data_ptr, 
						curr_tspkt_si->sect_len);
				curr_tspkt_si->data_ptr = 0;
				dvb_pkt1->pkt_setflag(FLOW_RECV);
				dvb_pkt1->pkt_settype(PKT_SECTION);
				dvb_pkt1->pkt_getfwdinfo()->pid = ts_pkt->_pid;
				pkt->DataInfo_ = dvb_pkt1;

				free(curr_tspkt_si->data_ptr);
				del_sec_info_entry(ts_pkt->_pid);

				delete(dvb_pkt);

				assert(NslObject::recv(pkt));
			}
			else
				dvb_freePacket(pkt);
		}
		// if the Ts_header is not continue, then drop it,
		// and clear 
		else
		{	
			if( curr_tspkt_si )
				free(curr_tspkt_si->data_ptr);
			del_sec_info_entry(ts_pkt->_pid);
			dvb_freePacket(pkt);
		}
		free(sec_data);
		delete(ts_pkt);
		return 1;
	}
	else
	{
		assert(0);
	}
}


/*
 * Send.
 */
int
Mpeg2_ts_rcst::send(ePacket_ *pkt)
{
	/* Mpeg2_ts_rcst has no send function */
	assert(0);

}

/*
 * Search conti_count entry of the pid
 * in the cc_start link list
 */
conti_count*
Mpeg2_ts_rcst::search_conti_count_entry(int pid) {
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
Mpeg2_ts_rcst::search_sec_info_entry(int pid) {
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
Mpeg2_ts_rcst::del_sec_info_entry(int pid)
{
	sec_info	*curr_si, *prev_si;
	if( (curr_si=search_sec_info_entry(pid))!=0 ) {
		prev_si = search_prev_sect_info_entry(curr_si);
		prev_si->next = curr_si->next;
		//free(curr_si->data_ptr);
		delete(curr_si);
		return 0;
	}
	return -1;
}


/*
 * search the previous sect_info_entry of
 * the current sect_info_entry
 */
sec_info*
Mpeg2_ts_rcst::search_prev_sect_info_entry(sec_info *curr_si){
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
Mpeg2_ts_rcst::free_conti_count(conti_count *ptr){
	conti_count *tmp;

	while(ptr->next != 0) {
		tmp = ptr->next;
		ptr->next=(ptr->next)->next;
		delete tmp;
	}
}

void
Mpeg2_ts_rcst::free_sec_info(sec_info *ptr){
	sec_info *tmp;

	while(ptr->next != 0) {
		tmp = ptr->next;
		ptr->next=(ptr->next)->next;
		free(tmp->data_ptr);
		delete tmp;
	}
}

