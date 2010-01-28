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

 
#ifndef __NCTUNS_GprsGmm_h__
#define __NCTUNS_GprsGmm_h__

#include <object.h>
#include <packet.h>
#include <timer.h>
#include <sys/types.h>
#include <ip.h>
#include <tcp.h>
#include <gprs/include/GprsObject.h>
#include <gprs/include/mytype.h>
#include <gprs/GtpGmm/mydefine.h>
#include <gprs/include/bss_message.h>
#include <gprs/SndcpGmm/GPRS_mm_message.h>
#include <gprs/GtpGmm/HLR/HLR.h>
//class sndcp;
class gmm_;
class snsm_;
class sndcp_;
class	GprsGmm;

/*this table store packet data from oter network*/
struct Packet_table{
	ulong		imsi;
	Packet	 	*pkt;
	Packet_table	*next;
};
struct MS_status{
	ulong	imsi;
	uchar	status;//0:idle 1:standby 2:ready
	MS_status *next;
};
class gmm_ {
 private:
	GprsGmm        *uplayer; 
	uchar		status;//0:idle 1:standby 2:ready
	//ulong		RAI;
		
 public:
	gmm_(GprsGmm *upper);
	~gmm_();
	int	attach(ePacket_ *pkt);
	int	detach(ePacket_ *pkt);
	int	Indi_rep(ePacket_ *pkt);
	int	update_location_rep(ePacket_ *pkt);
	int	SGSN_context_req(ePacket_ *pkt);
	int	send_PDP_req(uchar qos,ushort nsapi,ulong imsi,ulong tlli, const char *control);
	//int	bind_to_socket(ushort *port,ulong *ip);
	//int	changeVLR(int s);
	//int	VLR_GPRS_Detach(ePacket_ *pkt);
	//int	Location_Update_req(ePacket_ *pkt);
	int	initial_HLR(HLR_record *H_rec);
	
};

class snsm_{
 public:
 	snsm_(GprsGmm *upper){};
 	//~snsm(){};
};

class sndcp_{
 private:
 	GprsGmm			*uplayer;
 	char			*control_m ;
 	char			*data_m ;
 	                	                        
 public:
	sndcp_(GprsGmm *upper);
	//int			snsmactivateindi(ushort *nsapi, uchar *qos); 
	//int			snsmdeactivateindi(ushort *nsapi, uchar *qos); 
	//int			snsmmodifyindi(ushort *nsapi, uchar *qos); 
	//int			LLestablishreq(ushort *nsapi,uchar *qos); 	
};
class GprsGmm : public GprsObject {

 private:
 	//char			*tmp_RAI;
 	ulong			imsi;
 	ulong			RAI;
 	u_char			msisdn1;
	uchar			flag;//0:normal data, 1: control data
	uchar 			qos;//1-4,default value:1
	uchar			control;//control signal
	ushort			nsapi;
	struct link_status	*head; //point to the head of link_status list
	struct link_status	*L_status;//0:not linked 1:linked
	uchar			confirm;//exist confirm from LLC,2: confirm fail 1:confirm success 0:not exist
	uchar			indication; //exist indication from LLC,0:not exit 1:success 2:fail 
	timerObj		T3350;//Timer 3350 which is used for ptmsi relocation
	timerObj		T3322;//Timer 3322 which is used for network detach request
	int			c_state;
	/*
	current state:
	attach state: 0:GMM-DEREGISTERED 1:GMM-DEREGISTERED-INITIATED 2:GMM-REGISTERED 		      
	*/			
	int			attempt_count;
	//MS_status		*ms_state;
	struct MS_status	*ms_state_head;
	//struct Packet_table	*pkt_table;
	struct Packet_table	*pkt_table_h;
  public:
 				
	GprsGmm(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~GprsGmm();
	//int			NK_detach(int type1,ulong tlli);
	//int			mid_NK_detach();
	//int			mid_NK_detach_attach();
	int			Attach_acpt();
	int			init();
	int			recv(ePacket_ *);
	int			send(ePacket_ *);
	//int			get(ePacket_ *pkt, MBinder *frm);
	//int			put(ePacket_ *pkt, MBinder *mo);
	//ePacket_ * 		put1(ePacket_ *pkt, MBinder *mo) ;
	int			route_area_update_ap(struct route_msg *rt_msg);
	int			paging_rp(struct Paging_resp *P_resp);
	int			set_in_standby(struct in_standby *standby);
	int			intra_area_update_req(struct intra_route_msg *intra_rt_msg);
	int			attach_complete(struct attach_accept *msg1);
	//int			NK_detach_acpt(struct detach_accept *dtch_accept);
	int			freeBss_message(bss_message *p);
	ulong			getRAI();
	friend class		sndcp_;
	friend class		gmm;
	friend class		snsm;
	sndcp_			*sndcp1_;
	gmm_			*gmm1_;
	snsm_			*snsm1_;
};
#endif

