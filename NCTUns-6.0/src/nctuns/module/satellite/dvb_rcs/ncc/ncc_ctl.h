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

#ifndef	__NCTUNS_ncc_ctl_h__
#define	__NCTUNS_ncc_ctl_h__

#include <timer.h>
#include <event.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/table/table.h>
#include <satellite/dvb_rcs/common/table/sct_table.h>
#include <satellite/dvb_rcs/common/table/fct_table.h>
#include <satellite/dvb_rcs/common/table/tct_table.h>
#include <satellite/dvb_rcs/common/table/tbtp_table.h>
#include <satellite/dvb_rcs/common/table/tim_table.h>
#include <satellite/dvb_rcs/common/table/sct_table_q.h>
#include <satellite/dvb_rcs/common/table/fct_table_q.h>
#include <satellite/dvb_rcs/common/table/tct_table_q.h>
#include <satellite/dvb_rcs/common/table/tbtp_table_q.h>
#include <satellite/dvb_rcs/common/table/tbtp_table_list.h>
#include <satellite/dvb_rcs/common/si_config.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <nctuns_api.h>
#include "timeslot_scheduler.h"
#include <list>
#include <map>
#include <satellite/dvb_rcs/common/rcst_info.h>
#include <satellite/dvb_rcs/common/sac.h>
#include <satellite/dvb_rcs/common/ncc_config.h>
#include <satellite/dvb_rcs/common/parse_ncc_config.h>
#include <satellite/dvb_rcs/common/delay.h>
#include <nctuns_api.h>
#include <regcom.h>

class		Ncc_ctl;
struct 		Slot_assignment;
class		Timeslot_scheduler;
class		Slot_assignment_record;
class		Dvb_s2;

	

enum Timeslot_id
{
	TIMESLOT_ID_DATA	= 0x00,
	TIMESLOT_ID_REQ		= 0x01
};

class Rcst_capacity_request
{
      public:
	Rcst_id 				rcst_id;
	Sac::Capacity_request_type_value	capacity_request_type;
	uint8_t        				channel_id;
	uint32_t         			capacity_request_value;

	Rcst_capacity_request (Rcst_id r_id, Sac::Capacity_request c_r);
};

/* It's a container to store Rcst capacity requests */
class Heap_of_capacity_request
{
      public:
	list < Rcst_capacity_request > rbdc_requests;
	list < Rcst_capacity_request > vbdc_requests;
	list < Rcst_capacity_request > avbdc_requests;

      private:
      public:
	void            clear ();

	void            insert (Rcst_capacity_request
				rcst_capacity_request);
};



class           Ncc_ctl:public NslObject
{
	friend class    Timeslot_scheduler;
	friend class	Slot_assignment_record;

	/************************** Private member data ************************/
  private:

	Heap_of_capacity_request heap_of_capacity_request;

	timerObj        _SctTimer;
	timerObj        _FctTimer;
	timerObj        _TctTimer;
	timerObj        _TbtpTimer;
	timerObj        _BroadcastTimTimer;
	timerObj        _scheduleTimer;
	timerObj	_gen_sct_fct_and_tct_timer;


	/*
	 * all tables' circleq
	 */
	list <Sct*>	_SCTs;
	list <Fct*>	_FCTs;
	list <Tct*>	_TCTs;
	Tbtp_table_list	_TBTPs;
	Tim             unicast_tim, broadcast_tim;

	Timeslot_scheduler *_scheduler;
	uint8_t         superframe_id_for_logon;
	dvb_node_type   _node_type;

	uint32_t	csc_response_timeout;
	uint8_t		csc_max_losses;
	uint32_t	max_time_before_retry;
	
	Dvb_s2*		_feeder_ptr;

	u_char		next_sct_version;
	u_char		next_fct_version;
	u_char		next_tct_version;
	u_char		next_tbtp_version;

	/************************** Private member functions *******************/
	int		_parse_nodeid_cfg(char *path, list<dvbrcs_node_id> &node_id);
	uint64_t	queueing_delay();
	int		gen_sct_fct_and_tct(Event_ *pEvn);
	int             GenSCT (Event_ * pEvn);
	int             GenFCT (Event_ * pEvn);
	int             GenTCT (Event_ * pEvn);
	int             GenTBTP (Event_ * pEvn);
	int             GenBroadcastTIM (Event_ * pEvn);
	int             frame_scheduling (Event_ * pEvn);

      public:
	/************************** Public member data ************************/
	Ncc_config			ncc_config;
	Rcst_info_list			rcst_info_list;
	Slot_assignment_record*		slot_assignment_record;

	/************************** Public member functions *******************/
		Ncc_ctl (uint32_t type, uint32_t id, struct plist *pl, const char *name);
	               ~Ncc_ctl ();

	uint64_t	ncc_to_sat_deley();
	uint64_t	ncc_to_rcst_delay(uint32_t rcst_node_id);
	uint64_t	max_sat_to_rcst_delay();
	uint64_t	scheduling_delay();
	int             init ();
	int             send (ePacket_ * pkt);
	int             recv (ePacket_ * pkt);
	int             recv_sac (Dvb_pkt * pkt);

	void            set_sct ();
	void            set_fct ();
	void            set_tct ();
	void            set_tbtp ();
	void            set_broadcast_tim ();
	uint32_t	bps_to_spf (uint64_t bps);	//Bits/second to slots/frame.
};

#endif /* __NCTUNS_ncc_ctl_h__ */
