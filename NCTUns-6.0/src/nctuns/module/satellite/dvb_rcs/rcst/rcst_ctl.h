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

#ifndef	__NCTUNS_rcst_ctl_h__
#define	__NCTUNS_rcst_ctl_h__

#include <list>
#include <stdint.h>
#include <satellite/dvb_rcs/common/table/table.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/si_config.h>
#include <satellite/dvb_rcs/common/sch_info.h>
#include <nctuns_api.h>
#include <timer.h>
#include <object.h>

#include <satellite/dvb_rcs/common/table/pat_table.h>
#include <satellite/dvb_rcs/common/table/pmt_table.h>
#include <satellite/dvb_rcs/common/table/nit_table.h>
#include <satellite/dvb_rcs/common/table/int_table.h>
#include <satellite/dvb_rcs/common/table/sct_table.h>
#include <satellite/dvb_rcs/common/table/fct_table.h>
#include <satellite/dvb_rcs/common/table/tct_table.h>
#include <satellite/dvb_rcs/common/table/tbtp_table.h>
#include <satellite/dvb_rcs/common/table/tim_table.h>
#include <satellite/dvb_rcs/common/table/pat_table_q.h>
#include <satellite/dvb_rcs/common/table/pmt_table_q.h>
#include <satellite/dvb_rcs/common/table/nit_table_q.h>
#include <satellite/dvb_rcs/common/table/int_table_q.h>
#include <satellite/dvb_rcs/common/table/sct_table_q.h>
#include <satellite/dvb_rcs/common/table/fct_table_q.h>
#include <satellite/dvb_rcs/common/table/tct_table_q.h>
#include <satellite/dvb_rcs/common/table/tbtp_table_q.h>

#include <satellite/dvb_rcs/common/descriptor/common.h>
#include <satellite/dvb_rcs/common/descriptor/linkage_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/ip_mac_stream_location_descriptor.h>
#include <satellite/dvb_rcs/ret/queue/rcst_queue_manager.h>
#include <satellite/dvb_rcs/common/ncc_config.h>

#define	RCST_GET_SLOT_LOG
#define	RCST_REQ_SLOT_LOG
#define	SEND_TCP_LOG
#define	RECV_TCP_LOG

class Rcs_mac;

enum Ctl_state {
	FWD_CTL_NOT_READY	= 0x0000,
	FWD_CTL_READY		= 0x0010,
	RET_CTL_NOT_READY	= 0x0100,
	RET_CTL_READY		= 0x1000
};

enum Rcst_state
{
	RCST_STATE_OFF,
	RCST_STATE_RECEIVE_SYNC,
	RCST_STATE_READY_FOR_COARSE_SYNC,
	RCST_STATE_READY_FOR_FINE_SYNC,	
	RCST_STATE_FINE_SYNC
};

enum Table_state
{
	NOT_READY		= false,
	READY			= true
};

class _is_out_of_date
{
  public:
	bool operator() (const slot_info & info) const
	{
		return (info.start_time < GetCurrentTime());
	}
};

struct rcst_node_id
{
	uint32_t		rcst_nid;
};

struct dvbrcs_node_id
{
	uint32_t		sat_nid;
	uint32_t		feeder_nid;
	uint32_t		gw_nid;
	uint32_t		sp_nid;
	uint32_t		ncc_nid;
	list<rcst_node_id>	rcst_nid_list;
};

class Rcst_queue_config;
class Ncc_ctl;
class Rcst_ctl : public NslObject
{
/******************** Private Data **************************/
 private:
	/*
	 * all tables' circleq
	 */

	Pat*			_pat;
	Pmt*			_int_pmt;
	Pmt*			_trf_pmt;
	Nit*			_nit;
	Int*			_int;
	list<Sct*>		_sct_list;
	list<Tbtp*>		_tbtp_list;
	list<Fct*>		_fct_list;
	list<Tct*>		_tct_list;

	
	Table_state		_patState , _pmtState , _int_pmtState , _trf_pmtState ,
		_nitState , _intState, _sctState , _fctState , _tctState, _tbtpState, _ccdState;
	bool		_first_ready;

	Ctl_state	_fwd_ctl_state , _ret_ctl_state;
	int		pat_cur_version_num , pmt_cur_version_num , nit_cur_version_num , int_cur_version_num , sct_cur_version_num , fct_cur_version_num , tct_cur_version_num , tbtp_cur_version_num , cmt_cur_version_num , spt_cur_version_num;
	int		pat_next_version_num , pmt_next_version_num , nit_next_version_num , int_next_version_num , sct_next_version_num , fct_next_version_num , tct_next_version_num , tbtp_next_version_num , cmt_next_version_num , spt_next_version_num;
	uint16_t	_now_used_suprframe_count;

	uint16_t	_MPEstream_pid;
	uint16_t	_INT_pid;
	uint8_t		_my_superframe_id;
	uint8_t		_my_group_id;
	uint16_t	_my_logon_id;
	u_char		my_mac_address[6];

	Rcst_state	rcst_state;
	/*
	 * IP addr can get it from interface module though get_reg_var function
	 */
	u_long		*_RcstIPaddr;

	dvb_node_type	_node_type;
	bool		_mpe_flag;

	struct Vbdc_empty_record{
		Frame_ident frame;
		bool vbdc_empty;
	};

	list <Vbdc_empty_record> _vbdc_empty_table;
	/*
	 * calculate jiffier
	 */
	FILE		*jitter_fptr;
	u_int64_t	last_time;
	u_int64_t	last_interval;
	u_int64_t	time_intend_to_logon;
	bool		tim_received_since_csc_sent_out;
	timerObj	timer_to_send_csc;
	timerObj	timer_to_grant_demand_and_compute_demand;
	uint64_t	csc_try;
	uint8_t		superframe_id_for_logon;
	uint32_t	csc_response_timeout;
	uint8_t		csc_max_losses;
	uint32_t	max_time_before_retry;
	uint32_t	cra_level; // In bps.
	uint16_t	vbdc_max; // In timeslots.
	uint32_t	rbdc_max; // In bps.
	uint16_t	rbdc_timeout; // In suerframe.

	/*
	 * for parse configuration object
	 */
	Rcst_queue_config	*_rcst_config;

	/*
	 * for queue manager
	 */
	Rcst_queue_manager	**_rcst_qm;

	/* granted_list is sorted by start_time.
	 * slot info of smallest start_time is put at begin.
	 */
	list<slot_info>		granted_slot_list;
	class Slot_pool{
	  friend class Rcst_ctl;
		list<slot_info>		_data_slot_list; // Store slot info for un-dispatched data slots.
		list<slot_info>		_req_slot_list; // Store slot info for request slots.
	  public:
		enum Slot_type
		{
			DATA,
			REQUEST,
			UNKNOWN
		};

		int		insert(slot_info &info,
				       uint16_t superframe_count, 
				       uint8_t frame_number, 
				       Slot_type type);

		void		clear(uint16_t superframe_count, 
				      uint8_t frame_number, 
				      Slot_type type);

		uint32_t	size(uint16_t superframe_count, 
				     uint8_t frame_number, 
				     Slot_type type);

		slot_info*	pop_front(uint16_t superframe_count, 
					  uint8_t frame_number, 
					  Slot_type type);
	} _slot_pool;

	uint16_t		num_sac_entry_to_send; // Number of SAC entries to be send within the incoming frame.
	uint16_t		superframe_count_in_the_last_tbtp;
	uint8_t			frame_number_in_the_last_tbtp;
	// The computed queue demand to be sent within the next frame.
	Rcst_queue_manager::queue_demand	demand_to_send_in_next_frame;
	u_int16_t		start_slot;
	u_int32_t               sync_count;
	/*
	 * get ncc_ctl module pointer
	 */
	Ncc_ctl       		*_ncc_module;
	Ncc_config		*_ncc_config;

	Rcs_mac			*_rcs_rcst_mac;
	/* 
	 * for store xxx.dvbrcs.nodeid file
	 */
	list<dvbrcs_node_id>		_node_id_cfg;


/******************** Private Function **********************/
 private:
	enum Slot_type{
		SLOT_TYPE_CAN_NOT_DETERMINE,
		SLOT_TYPE_REQUEST,
		SLOT_TYPE_DATA
	};
	int		_locate_MPEstream_pid();
	int		_recv_data(ePacket_ *Epkt);
	int		_recv_control_mes(ePacket_ *Epkt);
	bool		_ready_for_logon();
	int		send_csc();
	int		check_tim();
	void            _remove_out_of_date_grant();
	int             _parse_tbtp(Tbtp *tbtp);
	struct Sac*     _pack_request_to_sac(Slot_flags& flags);
	int             _send_sac_to_next_module(Event *ep);
	int		set_timer_to_delete_sct(Sct* recv_sct);
	int		set_timer_to_delete_fct(Fct* recv_fct);
	int		set_timer_to_delete_tct(Tct* recv_tct);
	int		_del_sct(Event* ep);
	int		_del_fct(Event* ep);
	int		_del_tct(Event* ep);
	uint32_t	_msl();

	slot_info*	_fetch_slot_info(uint16_t superframe_count, uint8_t frame_number, 
					 uint16_t timeslot_number, slot_info* info_buf);

	Slot_type	_slot_type(uint16_t superframe_count, uint8_t frame_number,
				   uint16_t timeslot_number);

	int		_grant_demand_and_compute_demand(Event* ep);
	int		_grant_demand();
	int		_parse_nodeid_cfg(char *path, list<dvbrcs_node_id> &node_id);
/******************** Public Function ***********************/
 public:
	Rcst_ctl(uint32_t type, 
		 uint32_t id, 
		 struct plist* pl, 
		 const char *name);

	~Rcst_ctl();

	int	init();

	int	send(ePacket_ *Epkt);

	int	recv(ePacket_ *Epkt);

	int 	logon();

	int     queue_grant_demand(uint16_t superframe_count, 
				   uint8_t frame_number, 
				   uint32_t queue_id, 
				   uint8_t amount);

	struct	slot_info* 	ctrl_require_timeslot(struct slot_info *slot_info); 

	struct	slot_info* 	require_timeslot(struct slot_info *slot_info, 
						 uint32_t queue_id);
	bool	schedule_timer_for_mac;
};


#endif /* __NCTUNS_rcst_ctl_h__ */
