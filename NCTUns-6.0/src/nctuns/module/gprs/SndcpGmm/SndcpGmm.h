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

#ifndef __NCTUNS_SndcpGmm_h__
#define __NCTUNS_SndcpGmm_h__

#include <object.h>
#include <packet.h>
#include <timer.h>
#include <sys/types.h>
#include <ip.h>
#include <tcp.h>
#include <udp.h>
#include <nctuns_api.h>
#include <gprs/include/GprsObject.h>
#include <gprs/include/mytype.h>
#include <gprs/GtpGmm/mydefine.h>
#include <gprs/include/bss_message.h>
#include "GPRS_mm_message.h"


class gmm;
class snsm;
class sndcp;
class SndcpGmm;

class gmm {
 private:
	SndcpGmm        *uplayer; 
	int		attempt_count;//Max attempt count is four	
	ms_storage	*ms_store;
	ulong		new_RAI;
	uchar		CI;	
 public:
	gmm(SndcpGmm *upper);
	~gmm();
	int			init();
	inline int		get_current_attempt();
	inline int		reset_attempt_count();
	int			attach();
	int			detach(const char *detach_type1,uchar type);
	int			activate_PDP_req(const char *msg);
	int			update_PDP_req(ushort nsapi,uchar qos);
	int			deactivate_PDP_req();
	int			route_area_update_req(ePacket_ *pkt);
	int			iden_reqst();
	int			periodic();
	inline int		attempt_add();
	int			Modify_PDP_req(ePacket_ *pkt);
	//int			change_ptmsi(ulong ptmsi);
	inline int		change_ptmsi_sig(ulong ptmsi,ulong ptmsi_sig);
	int			NK_activate_PDP_req(ePacket_ *pkt);
	inline ms_storage*	get_ms_store();
	inline int		setRAI(ulong RAI);
	
};

class snsm{
 public:
 	snsm(SndcpGmm *upper){};
 	~snsm(){};
};

class sndcp{
 private:
 	SndcpGmm		*uplayer;
 	char			*control_m ;
 	char			*data_m ;
 	                	                        
 public:
	sndcp(SndcpGmm *upper);
	~sndcp();
 	//int			sndatareqst(ePacket_ *p,ushort nsapi,uchar qos,link_status *head); 
	int			snunitdatareqst(ePacket_ *pkt,ushort nsapi,uchar qos,ulong tlli,link_status *head); 
	//int			snsmactivateindi(ushort nsapi, uchar qos); 
	//int			snsmdeactivateindi(ushort nsapi, uchar qos); 
	//int			snsmmodifyindi(ushort nsapi, uchar qos); 
	//int			LLestablishreq(ushort nsapi,uchar qos); 
	//int			LLreleasereq(uchar *fieldv,ushort nsapi,uchar qos); 
	//int			LLdatareq(ePacket_ *pkt,ushort nsapi,uchar qos); 
	int			LLunitdatareq(ePacket_ *pkt,ushort nsapi,uchar qos,ulong tlli); 
	//int			LLestablishreqpure(ushort nsapi,uchar qos);
	
	
};
class SndcpGmm : public GprsObject {

 private:
 	char			*tmp_msisdn;
 	char			*tmp_imsi;
	char			*action_table;
 	ulong			msisdn1;
 	ulong			imsi1;
	uchar			flag;//0:normal data, 1: control data
	uchar 			qos;//1-4,default value:1
	uchar			control;//control signal
	ushort			nsapi;
	ushort			update_nsapi;
	uchar			update_qos;
	struct link_status	*head; //point to the head of link_status list
	struct link_status	*L_status;//0:not linked 1:linked
	uchar			confirm;//exist confirm from LLC,2: confirm fail 1:confirm success 0:not exist
	uchar			indication; //exist indication from LLC,0:not exit 1:success 2:fail 
	uchar			c_state;
	/*
	current state:
	attach state: 0:GMM-DEREGISTERED 1:GMM-REGISTERED-INITIATED 2:GMM-REGISTERED 
		      3:GMM-DEREGISTERED-INITIATED
	*/
	uchar			status;//0:idle 1:standby 2:ready
	timerObj		T3310;//Timer 3310 which is used for attach request
	timerObj		T3321;//Timer 3321 which is used for detach request
	timerObj		T3314;//Timer 3314 which is used in ready state
	timerObj		T3312;//Timer 3312 which is used for periodic routing update
	timerObj		T3330;//Timer 3330 which is used for normal route area update
	timerObj		doingTimer;
	struct doingAction	*TimerAction_head;
  public:
 				
	SndcpGmm(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~SndcpGmm();
	int                     freeBss_message(bss_message *p);
	int			Detach_acpt();
	inline ulong		getMSISDN();
	inline ulong		getIMSI();
	int			set_cstate(int cstate);
	int			periodic();
	//int			readyexpire();
	inline int		mid_detach();
	inline int		mid_attach();
	int			readyexpire();
	//int			standbystate();
	int			updateexpire();
	int			init();
	int			recv(ePacket_ *);
	int			send(ePacket_ *);
	//int			get(ePacket_ *pkt, MBinder *frm);
	//int			put(ePacket_ *pkt, MBinder *mo);
	//ePacket_ * 		put1(ePacket_ *pkt, MBinder *mo);
	int			attach_acpt(struct attach_accept *atch_accept);
	//int			NK_detach_acpt(struct NK_detach_type *nk_detach);
	int			route_area_update_acpt(struct route_accept *rt_accept);
	int			paging_reqst(struct Paging_ps *p_ps);
	int			set3321();
	int			gprs_imsi_attach_action();
	int			MS_gprs_detach_action();
	int			activate_PDP_r_action();
	inline int		update_PDP_r_action();
	inline int		deactivate_PDP_r_action();
	friend class		sndcp;
	friend class		gmm;
	friend class		snsm;
	sndcp			*sndcp1;
	gmm			*gmm1;
	snsm			*snsm1;
};
#endif

