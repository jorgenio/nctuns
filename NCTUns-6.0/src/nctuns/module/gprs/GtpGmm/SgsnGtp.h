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

#ifndef __NCTUNS_SgsnGtp_h__
#define __NCTUNS_SgsnGtp_h__

#include <object.h>
#include <time.h>
#include <iostream>
#include <packet.h>
#include <timer.h>
#include <ip.h>
#include <tcp.h>
#include <misc/log/logHeap.h>
#include <misc/log/logchktbl.h>
#include <ethernet.h>
#include <gprs/include/mytype.h>
#include "mydefine.h"
#include <gprs/GtpGmm/HLR/HLR.h>
#include <gprs/include/GprsObject.h>

struct SGSN_table{
	ulong	ptmsi;
	ulong	imsi;
	ulong	TLLI;
	ushort	NSAPI;
	uchar	TID;
	ulong	GGSNIP;
	uchar	qos;
	SGSN_table *next;		
};


class SgsnGtp : public NslObject{
	private:
		SGSN_table	*sgsn_head_t;
		ulong		RAI;
		char		*tmp_RAI;
		char		*tmp_band;
		char		*tmp_time;
		char		*tmp_v_time;
		char		*tmp_f_time;
		char		*cmd_table;
		char		*cmd_FILEPATH;
		int		delay_time;
		int		v_delay_time;
		int		first_delay_time;
		ulong		band_width;
		ManageQue	*manageque;
		u_char          ptr_log;
		timerObj        ptrlogTimer;
		
		        
	public:
		logChkTbl               *logTbl;
		SgsnGtp(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
		~SgsnGtp();
		
		//int           recv(ePacket_ *pkt);
		int             send(ePacket_ *pkt);
		int 		sendlog(ePacket_ *pkt,u_int64_t s_delay_time,ulong band_width);
		int 		recvlog(ePacket_ *pkt,u_int64_t delay_time,ulong band_width);
		int             rslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2);
		int             relog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t node_id1,u_int32_t node_id2);
		int 		sslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2);
		int 		selog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t node_id1,u_int32_t node_id2);
		int             init();
		int		logFlush();	 		        
		//Path Management Messages
		//int	echo_req();
		//int	echo_resp();
		//Tunnel Management Messages
		void		pushManage();
		int		sendManage(ePacket_ *pkt);
		//ip_structure	*NK_create_PDP_req(ePacket_ *pkt);
		int		update_location(control_msg *msg);
		int		update_location_resp_b(control_msg *msg);
		ip_structure 	*create_PDP_req(ePacket_ *pkt);
		//int		create_PDP_resp();
		int		update_PDP_req(ePacket_ *pkt);
		//int		update_PDP_resp();
		int   		Deactivate_PDP_req(ePacket_ *pkt);
		int		Delete_PDP_req(ePacket_ *pkt);
		//int		Delete_PDP_resp();
		//int		Error_indi();
		int		PDU_notify_resp(ip_structure *respip);
		int		PDU_notify_req_b(ulong srcip,ip_array *ip_list);
		//int		PDU_notify_rj_req();//PDU notification reject request
		//int		PDU_notify_rj_resp();
		//Location Management Messages
		//ulong	S_rout_info_GPRS_req(ulong *imsi);//send routing information for GPRS Request
		//int		note_MS_GPRS_pt_resp();//note MS GPRS Present response
		int		note_MS_GPRS_pt_req_b(ip_array *ip_list);
		
		//Mobility Management messages
		int		sendpacket(ePacket_ *pkt);
		ulong		Indi_req(control_msg *msg);//Indentification Request
		ulong		Indi_resp_b(control_msg *msg);
		int		SGSN_context_req(ePacket_ *pkt);
		SGSN_table	*SGSN_context_resp_b(route_msg *msg);
		int		initial_maptable();
};

#endif
