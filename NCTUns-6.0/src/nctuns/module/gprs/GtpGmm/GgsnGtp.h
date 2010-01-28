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

#ifndef __NCTUNS_GgsnGtp_h__
#define __NCTUNS_GgsnGtp_h__
#include <packet.h>
#include <timer.h>
#include <ip.h>
#include <tcp.h>
#include <gprs/include/mytype.h>
#include "mydefine.h"
#include <gprs/GtpGmm/HLR/HLR.h>
#include "SgsnGtp.h"

/*this table store packet data from oter network*/
struct Packet_table{
	ulong		imsi;
	Packet		*pkt;
	Packet_table	*next;
};

struct GGSN_table{
	ulong	ptmsi;
	ulong	imsi;
	ulong   TLLI;
	ushort   NSAPI;
	uchar   TID;
	ulong   SGSNIP;
	uchar   qos;
	uchar	msisdn;
	GGSN_table *next;
};



class GgsnGtp : public NslObject{
	private:
		GGSN_table	*ggsn_head_t;
		Packet_table	*pkt_table_h;
		uchar		tid;
		uchar		count;
		char            *tmp_band;
		char            *tmp_time;
		char 		*tmp_v_time;
		int		v_delay_time;
		int             delay_time;
		ulong           band_width;
		u_char          ptr_log;
		timerObj        ptrlogTimer;


	public:
		logChkTbl               *logTbl;
		GgsnGtp(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
		~GgsnGtp();
		int                     logFlush();
		int                     rslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2);
		int                     relog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t node_id1,u_int32_t node_id2);
		int			recvlog(ePacket_ *pkt,u_int64_t s_delay_time,ulong band_width);
		int			sendlog(ePacket_ *pkt,u_int64_t s_delay_time,ulong band_width);
		int			sslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2);
		int			selog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t node_id1,u_int32_t node_id2);
		int			createGGSNTABLE(ip_array *respip,ulong sgsnip);
		int			sendpacket(ePacket_ *pkt,ulong sgsnip);
		int			GGSN_context_resp_b(unsigned long RAI,SGSN_table *table);
		int                     send(ePacket_ *pkt);
		//int                     recv(ePacket_ *pkt);
		int                     init();
		int	                recvconbottom(struct ip_structure *myip);
		int			recvbottom(ePacket_ *pkt);
		int	PDU_notify_req(ip_array *ip);
		int 	PDU_notify_resp_b(ulong srcip,ip_structure *respip);
		//Location Management Messages
		ip_array *S_rout_info_GPRS_req(ulong *ms_ip);//send routing information for GPRS Request
		int	note_MS_GPRS_pt_req(ulong *imsi);//note MS GPRS Present request
};
#endif
