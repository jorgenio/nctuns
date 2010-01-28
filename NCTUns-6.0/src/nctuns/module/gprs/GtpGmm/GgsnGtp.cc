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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <nctuns_api.h>
#include <mbinder.h>
#include <gprs/include/debug.h>
#include "GgsnGtp.h"
#include <gprs/GtpGmm/HLR/HLR.h>

#ifdef  __DMALLOC_DEBUG__
#include <dmalloc.h>
#endif


extern HLR                 HLR_;

MODULE_GENERATOR(GgsnGtp);

GgsnGtp::GgsnGtp(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
        : NslObject(type, id, pl, name), ggsn_head_t(NULL), pkt_table_h(NULL), tid(0), count(0)
{

	vBind("Base_Delay_Time",&tmp_time);
        vBind("BandWidth",&tmp_band);
	vBind("Variance_Time",&tmp_v_time);
        //initial GGSN table
        //make sgsn_head_t point to the front of sgsn_t

}
int GgsnGtp::init()
{
	delay_time = atoi(tmp_time);
	v_delay_time = atoi(tmp_v_time);
        band_width = atol(tmp_band);

	if ( WireLogFlag && !strcasecmp(WireLogFlag, "on") ) {

		if ( !ptrlogFileOpenFlag ) {
			ptrlogFileOpenFlag = true;

 			char	*ptrFile;
			if( ptrlogFileName ) {
				ptrFile = (char *)malloc(strlen
					(GetConfigFileDir())+strlen
					(ptrlogFileName)+1);
				sprintf(ptrFile,"%s%s",GetConfigFileDir(),
							ptrlogFileName);
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}
			else {
				ptrFile = (char *)malloc(strlen
					(GetScriptName())+5);
				sprintf(ptrFile, "%s.ptr", GetScriptName());
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}

			if( fptr == NULL ) {
				if(DEBUG_LEVEL >= MSG_ERROR)
					printf("Error :GgsnGtp:: Can't create file %s\n",ptrFile);
				exit(-1);
			}

			/* triggler the heapHandle event */
			Event_ *heapHandle = createEvent();
			u_int64_t chkInt;
			MILLI_TO_TICK(chkInt, 100);
			chkInt += GetCurrentTime();
			setEventTimeStamp(heapHandle, chkInt, 0);

			/* heapHandle is handled by the function
			 * in logHeap.cc : DequeueLogFromHeap
			 */
			int (*__fun)(Event_ *) = (int (*)(Event_ *))&DequeueLogFromHeap;
			setEventCallOutFunc(heapHandle, __fun, heapHandle);
			scheduleInsertEvent(heapHandle);
		}

		ptr_log = 1;

		/* every a interval, flush the logchktbl */
		u_int64_t life;
		MILLI_TO_TICK(life,10);
		int execint = life / 6;

		logTbl = new logChkTbl(life);

		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(GgsnGtp, logFlush);
		ptrlogTimer.setCallOutObj(this, type);
		ptrlogTimer.start(execint, execint);
	}

	return(1);
}
int GgsnGtp::logFlush()
{
        logTbl->flush();
	return(1);
}


GgsnGtp::~GgsnGtp()
{
	GGSN_table      *tmp_ggsn_p,*tmp_ggsn;
        Packet_table    *tmp_pkt_p,*tmp_pkt;
	tmp_ggsn_p = ggsn_head_t;
	tmp_pkt_p = pkt_table_h;
	/*free GGSN table*/
	while(tmp_ggsn_p != NULL)
	{
		tmp_ggsn = tmp_ggsn_p;
		tmp_ggsn_p = tmp_ggsn_p->next;
		free(tmp_ggsn);
	}
	/*free Packet table*/
	while(tmp_pkt_p != NULL)
	{
		tmp_pkt = tmp_pkt_p;
		tmp_pkt_p = tmp_pkt_p->next;
		free(tmp_pkt);
	}
}

int GgsnGtp::sslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2)
{
        Packet *pkt_;
        GET_PKT(pkt_, pkt);
        /* prepare logEvent for Sgsn */
        logEvent *sslogEvent = new logEvent[1];
        sslogEvent->PROTO = PROTO_802_3;
        sslogEvent->Time = start_time;
        sslogEvent->Time += START;
        /* prepare mac802_3_log */
        struct mac802_3_log *ss8023log = new struct mac802_3_log[1];
        ss8023log->PROTO = PROTO_802_3;
        ss8023log->Time = ss8023log->sTime = start_time;
        ss8023log->NodeType = get_type();
        ss8023log->NodeID = node_id1;
        ss8023log->Event = StartTX;
	ss8023log->IP_Src = 0;
        ss8023log->IP_Dst = 0;
        ss8023log->PHY_Src = node_id1;
	ss8023log->PHY_Dst = node_id2;
        ss8023log->RetryCount = 0;
        ss8023log->FrameID = 0;
        ss8023log->FrameType = FRAMETYPE_DATA;
        ss8023log->FrameLen = pkt_->pkt_getlen();
        ss8023log->DropReason = DROP_NONE;
        sslogEvent->log = (char *)ss8023log;
        logQ.push(sslogEvent);
	return(1);

}
int GgsnGtp::selog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t
node_id1,u_int32_t node_id2)
{
        Packet *pkt_;
        GET_PKT(pkt_, pkt);
        /* prepare logEvent */
        logEvent *selogEvent = new logEvent[1];
        selogEvent->PROTO = PROTO_802_3;
        selogEvent->Time = end_time;
        selogEvent->Time += ENDING;
        /* prepare mac802_3_log */
        struct mac802_3_log *se8023log = new struct mac802_3_log[1];
        se8023log->PROTO = PROTO_802_3;
        se8023log->sTime = start_time;
        se8023log->Time = end_time;
        se8023log->NodeType = get_type();
        se8023log->NodeID = node_id1;
        se8023log->Event = SuccessTX;
        se8023log->IP_Src = 0;
        se8023log->IP_Dst = 0;
        se8023log->PHY_Src = node_id1;
	se8023log->PHY_Dst = node_id2;
        se8023log->RetryCount = 0;
        se8023log->FrameID = 0;
        se8023log->FrameType = FRAMETYPE_DATA;
        se8023log->FrameLen = pkt_->pkt_getlen();
        se8023log->DropReason = DROP_NONE;

        selogEvent->log = (char *)se8023log;
        logQ.push(selogEvent);
	return(1);

}
int GgsnGtp::relog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t node_id1,u_int32_t node_id2)
{
	Packet *pkt_;
        GET_PKT(pkt_, pkt);
        /* prepare logEvent */
        logEvent *selogEvent = new logEvent[1];
        selogEvent->PROTO = PROTO_802_3;
        selogEvent->Time = end_time;
        selogEvent->Time += ENDING;
        /* prepare mac802_3_log */
        struct mac802_3_log *se8023log = new struct mac802_3_log[1];
        se8023log->PROTO = PROTO_802_3;
        se8023log->sTime = start_time;
        se8023log->Time = end_time;
        se8023log->NodeType = get_type();
        se8023log->NodeID = node_id1;
        se8023log->Event = SuccessRX;
        se8023log->IP_Src = 0;
        se8023log->IP_Dst = 0;
        se8023log->PHY_Src = node_id1;
        se8023log->PHY_Dst = node_id2;
        se8023log->RetryCount = 0;
        se8023log->FrameID = 0;
        se8023log->FrameType = FRAMETYPE_DATA;
        se8023log->FrameLen = pkt_->pkt_getlen();
        se8023log->DropReason = DROP_NONE;

        selogEvent->log = (char *)se8023log;
        logQ.push(selogEvent);
	return(1);

}

int GgsnGtp::rslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2)
{
	Packet *pkt_;
        GET_PKT(pkt_, pkt);
        /* prepare logEvent for Sgsn */
        logEvent *sslogEvent = new logEvent[1];
        sslogEvent->PROTO = PROTO_802_3;
        sslogEvent->Time = start_time;
        sslogEvent->Time += START;
        /* prepare mac802_3_log */
        struct mac802_3_log *ss8023log = new struct mac802_3_log[1];
        ss8023log->PROTO = PROTO_802_3;
        ss8023log->Time = ss8023log->sTime = start_time;
        ss8023log->NodeType = get_type();
        ss8023log->NodeID = node_id1;
        ss8023log->Event = StartRX;;
        ss8023log->IP_Src = 0;
        ss8023log->IP_Dst = 0;
        ss8023log->PHY_Src = node_id1;
        ss8023log->PHY_Dst = node_id2;
        ss8023log->RetryCount = 0;
        ss8023log->FrameID = 0;
        ss8023log->FrameType = FRAMETYPE_DATA;
        ss8023log->FrameLen = pkt_->pkt_getlen();
        ss8023log->DropReason = DROP_NONE;
        sslogEvent->log = (char *)ss8023log;
        logQ.push(sslogEvent);
	return(1);
}



int GgsnGtp::recvlog(ePacket_ *pkt,u_int64_t s_delay_time,ulong band_width)
{
	u_int64_t start_time;
        u_int64_t end_time;
        u_int32_t node_id1,node_id2;
        MBinder *mo1;
        Packet *pkt_;
        GET_PKT(pkt_, pkt);
        /*calculate transmission time*/
        float trans_time = (float)(pkt_->pkt_getlen())/band_width;

        /* prepare logEvent for Ggsn */
        start_time = GetCurrentTime();
	mo1 = sendtarget_;
	for(int i =0;i<6;i++)
        {
                mo1 = mo1->bindModule()->sendtarget_;
                if(mo1->bindModule()->get_nid() != get_nid())
                {
                        node_id1=mo1->bindModule()->get_nid();
                        break;
                }
        }
	node_id2 = get_nid();
        rslog(pkt,start_time,node_id1,node_id2);

        /* prepare logEvent for start transmission in GGSN */
        end_time = GetCurrentTime()+(u_int64_t)(trans_time*(1000000000.0/TICK));
        relog(pkt,start_time,end_time,node_id1,node_id2);
	return(1);
        /* prepare logEvent for virtual Switch */
        start_time = start_time-(u_int64_t)(s_delay_time*(1.0/2));
        mo1 = recvtarget_;
	for(int i =0;i<2;i++)
        {
                mo1 = mo1->bindModule()->recvtarget_;
                if(mo1->bindModule()->get_nid() != get_nid())
                {
                        node_id1=mo1->bindModule()->get_nid();
                        break;
                }
        }
        rslog(pkt,start_time,node_id1,node_id2);

        /* prepare logEvent for start transmission in virtual Switch */
        end_time = start_time+(u_int64_t)(trans_time*(1000000000.0/TICK));
        relog(pkt,start_time,end_time,node_id1,node_id2);


}

int GgsnGtp::sendlog(ePacket_ *pkt,u_int64_t s_delay_time,ulong band_width)
{
	u_int64_t start_time;
        u_int64_t end_time;
        u_int32_t node_id1,node_id2;
        MBinder *mo1;
        Packet *pkt_;
        GET_PKT(pkt_, pkt);
        /*calculate transmission time*/
        float trans_time = (float)(pkt_->pkt_getlen())/band_width;

        /* prepare logEvent for Sgsn */
        start_time = GetCurrentTime();
        node_id1 = get_nid();
	mo1 = sendtarget_;
        for(int i =0;i<6;i++)
        {
                mo1 = mo1->bindModule()->sendtarget_;
                if(mo1->bindModule()->get_nid() != get_nid())
                {
                        node_id2=mo1->bindModule()->get_nid();
                        break;
                }
        }
        sslog(pkt,start_time,node_id1,node_id2);

        /* prepare logEvent for start transmission in SGSN */
        end_time = GetCurrentTime()+(u_int64_t)(trans_time*(1000000000.0/TICK));
        selog(pkt,start_time,end_time,node_id1,node_id2);
	return(1);
        /* prepare logEvent for virtual Switch */
        start_time = end_time+(u_int64_t)(s_delay_time*(1.0/2));
        mo1 = sendtarget_;
        for(int i =0;i<2;i++)
        {
                mo1 = mo1->bindModule()->sendtarget_;
                if(mo1->bindModule()->get_nid() != get_nid())
                {
                        node_id1=mo1->bindModule()->get_nid();
                        break;
                }
        }
        node_id1 = mo1->bindModule()->get_nid();
        sslog(pkt,start_time,node_id1,node_id2);

        /* prepare logEvent for start transmission in virtual Switch */
        end_time = start_time+(u_int64_t)(trans_time*(1000000000.0/TICK));
        selog(pkt,start_time,end_time,node_id1,node_id2);


}
int GgsnGtp::GGSN_context_resp_b(unsigned long RAI,SGSN_table *table)
{
	int found =0;
	struct GGSN_table *g_table;
	g_table = ggsn_head_t;
	/*
	Now we are to change GGSN table
	*/
	while(g_table!=NULL)
        {
                if(g_table->imsi==table->imsi)
                {
                	g_table->SGSNIP = RAI;
			g_table->NSAPI = table->NSAPI;
                	found = 1;
        	}
		g_table = g_table->next;
	}
	if(found)
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GgsnGtp::we change GGSN record successfully!!\n");
		return(1);

	}
	else
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GgsnGtp::we didn't change GGSN record successfully!!\n");
		return(-1);
	}

}

ip_array *GgsnGtp::S_rout_info_GPRS_req(ulong *ms_ip)
{
	/*
	The GGSN sends a Send Routeing Information for GPRS Request message to the HLR
	to obtain the IP address of the SGSN where the MS is located
	*/
	//IMSI is to be used as a key to get the IP address of the SGSN
	ip_array *r_value=NULL;
	r_value = HLR_.search_record(ms_ip);
	return(r_value);

}
int GgsnGtp::note_MS_GPRS_pt_req(ulong *imsi)
{
	ip_array *ip_list,*ip_list1;
	int echomsg;
	NslObject *peer;
	//struct sockaddr_in sin;
	/*
	use this specific ip to look up HLR database to get sgsnip and
	see if the peer side's pdp context is existing or not
	*/
	ip_list=HLR_.search_record(imsi);
	ip_list1=ip_list;
	while(ip_list1 != NULL)
	{
		/*
		check these sgsnip's sgsn table to see if their pdp
		context have already been activated
		*/
		peer = InstanceLookup(ip_list1->ip,"SGSNGtp");
                echomsg = ((SgsnGtp*)peer)->note_MS_GPRS_pt_req_b(ip_list1);
		if(echomsg < 0)
		{
			if(DEBUG_LEVEL >= MSG_ERROR)
			printf("GgsnGtp::Note MS GPRS Present request fail!!\n");
		}
		ip_list1 = ip_list1->next;

	}
	return(1);

}
int GgsnGtp::createGGSNTABLE(ip_array *respip,ulong sgsnip)
{
	int found = 0;
	GGSN_table *g_table,*tmp_table;
	g_table = ggsn_head_t;
	/*
	Now we are to change GGSN table
	*/
	while(g_table!=NULL)
        {
                if(g_table->NSAPI==respip->nsapi && (g_table->imsi==respip->imsi || g_table->ptmsi==respip->ptmsi))
                {
			g_table->ptmsi = respip->ptmsi;
			g_table->imsi = respip->imsi;
			g_table->TLLI = respip->tlli;
			g_table->NSAPI = respip->nsapi;
			g_table->TID = respip->tid;
			g_table->SGSNIP = sgsnip;
			g_table->qos = respip->qos;
			g_table->msisdn = respip->msisdn;
			found = 1;
        	}
		g_table = g_table->next;
	}
	if(!found)
	{
		tmp_table = (GGSN_table *)malloc(sizeof(GGSN_table));
		bzero(tmp_table,sizeof(GGSN_table));
		tmp_table->ptmsi = respip->ptmsi;
		tmp_table->imsi = respip->imsi;
		tmp_table->TLLI = respip->tlli;
		tmp_table->NSAPI = respip->nsapi;
		tmp_table->TID = respip->tid;
		tmp_table->SGSNIP = sgsnip;
		tmp_table->qos = respip->qos;
		tmp_table->msisdn = respip->msisdn;
		tmp_table->next = ggsn_head_t;
		ggsn_head_t = tmp_table;

	}
	return(1);


}
int GgsnGtp::PDU_notify_resp_b(ulong srcip, ip_structure *respip) {

	 if(!strcmp(respip->cause,"request_accept"))
	 {

		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GgsnGtp::PDU notify success!!\n");

	        const char* tmp1 = "flag";
		char* tmp2 = (char *)respip;


                Event*  pkt    = createEvent();
	        Packet* p      = new Packet;
	        pkt->DataInfo_ = reinterpret_cast<Packet*> (p);

		p->pkt_addinfo(tmp1,tmp2,sizeof(ip_structure));
		free(respip);

		return(sendpacket(pkt,srcip));

	 }
	 else
	 {
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GgsnGtp::PDU notify fail !!\n");

		free(respip);
                return(-1);

	 }

	 return(1);

}

int GgsnGtp::PDU_notify_req(ip_array *ip)
{
	int echomsg;
	NslObject *peer;
	ulong srcip;
	srcip = nodeid_to_ipv4addr(get_nid(),get_port());
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SgsnGtp at PDU_notify_req:%ld\n",ip->ip);
	peer = InstanceLookup(ip->ip,"SGSNGtp");
        echomsg =((SgsnGtp*)peer)->PDU_notify_req_b(srcip,ip);
	if(echomsg < 0)
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GgsnGtp::PDU notify request fail!!\n");
		return(echomsg);
	}
	else
		return(1);
}

int GgsnGtp::send(ePacket_ *pkt)
{
	ip     *ip_header=NULL;
	Packet *p;
	Packet_table *tmp_table=NULL;
	ip_array *r_value1=NULL;
	int echomsg=0;
	assert(pkt&&pkt->DataInfo_);
	p = (Packet *)pkt->DataInfo_;
	p->pkt_setflow(PF_RECV);
	ip_header = (ip *)p->pkt_sget();

	//use ms'ip as the key to search for its sgsnip in HLR
	r_value1 = S_rout_info_GPRS_req(&ip_header->ip_dst);
	if(r_value1->ip == 0)
	{
		if(DEBUG_LEVEL >= MSG_ERROR)
		printf("GgsnGtp::we can't find SGSN for this outer packet because this user doesn't activate GPRS service!!\n");
		free(r_value1);
		freePacket(pkt);
		return(1);
	}
	//store this packet in packet table until the link is established
	tmp_table = (Packet_table *)malloc(sizeof(struct Packet_table));
	bzero(tmp_table,sizeof(struct Packet_table));
	tmp_table->pkt = (Packet *)pkt->DataInfo_;
	tmp_table->imsi = r_value1->imsi;
	tmp_table->next = pkt_table_h;
	pkt_table_h = tmp_table;
	//GGSN call the PDU Notification request to SGSN
	echomsg = PDU_notify_req(r_value1);
	if(echomsg < 0)
	{
		if(DEBUG_LEVEL >= MSG_ERROR)
			printf("GgsnGtp::PDU Notification request to SGSN fail!!\n");
		pkt_table_h = tmp_table->next;
		freePacket(pkt);
		free(tmp_table);
		return(1);
	}
	else
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GgsnGtp::PDU Notification request to SGSN successfully!!\n");
		return(1);
	}
	return(1);
}

int GgsnGtp::recvbottom(ePacket_ *pkt)
{
	Packet* p = (Packet*)pkt->DataInfo_;
	struct control_msg *s_delay=NULL;
	s_delay = (struct control_msg *)p->pkt_getinfo("flag");
	p->pkt_setflow(PF_RECV);
	if(DEBUG_LEVEL >= MSG_DEBUG)
	{
		printf("GgsnGtp debug: <%llu> At Node %d's interface module,p->pkt_getlen()= %d, p->pkt_getpid() = %llu\n",
		GetCurrentTime(),get_nid(),p->pkt_getlen(),p->pkt_getpid());
		printf("GgsnGtp::we receive pure data packet at GGSN side!!\n");
	}
	recvlog(pkt,s_delay->delay_time,band_width);
	return(NslObject::recv(pkt));


}

int GgsnGtp::sendpacket(ePacket_ *pkt,ulong sgsnip)
{
	Packet* p         = NULL;
	ip_structure* msg = NULL;
	NslObject* peer   = NULL;

	u_int64_t t_simulate_delay_time;
        ulong     simulate_delay_time;

	int retno=0;

	Packet_table *tmp_pkt_tab=NULL,*cur_pkt_tab=NULL;

	tmp_pkt_tab = pkt_table_h;
	p = (Packet *)pkt->DataInfo_;

	msg = (ip_structure *)p->pkt_getinfo("flag");

	while(tmp_pkt_tab!=NULL) {

		if(tmp_pkt_tab->imsi == msg->imsi) {

			pkt->DataInfo_ = (Packet *)tmp_pkt_tab->pkt;
			p = (Packet *)tmp_pkt_tab->pkt;
			p->pkt_setflow(PF_RECV);

			/*add imsi into the packet*/
			struct control_msg *con_msg=NULL;
			con_msg = (struct control_msg *)malloc(sizeof(struct control_msg));
        		bzero(con_msg,sizeof(struct control_msg));
        		strcpy(con_msg->control,"data_packet");

			simulate_delay_time = delay_time + rand()% v_delay_time;

                        MILLI_TO_TICK(t_simulate_delay_time, simulate_delay_time);

			sendlog(pkt,t_simulate_delay_time,band_width);

			con_msg->imsi = tmp_pkt_tab->imsi;
                        con_msg->delay_time = t_simulate_delay_time;

                        const char *tmp1 = "flag";
			char *tmp2 = reinterpret_cast<char*>(con_msg);

                        p->pkt_addinfo(tmp1, tmp2, sizeof(struct control_msg));
                        free(con_msg);

			peer = InstanceLookup(sgsnip,"SGSNGtp");

			BASE_OBJTYPE(type);
                        type = POINTER_TO_MEMBER(SgsnGtp,sendpacket);
                        setObjEvent(pkt,GetCurrentTime()+t_simulate_delay_time,0,
                            peer, type,pkt->DataInfo_);

			cur_pkt_tab = tmp_pkt_tab;
			/*
			 * ystseng: 2007/01/10 fixed
			 * don't forget to check the free packet is the head of
			 * this linked list
			 */
			if (pkt_table_h == cur_pkt_tab)
				pkt_table_h = pkt_table_h->next;

			tmp_pkt_tab = tmp_pkt_tab->next;

			free(cur_pkt_tab);
       		 	return(retno);

		}

		tmp_pkt_tab = tmp_pkt_tab->next;
	}
	if(DEBUG_LEVEL >= MSG_ERROR)
		printf("GgsnGtp::sendpacket()::we can't find packet in tmp table so we can't send data to SGSN!!\n");

	return(-1);

}
int GgsnGtp::recvconbottom(struct ip_structure *myip)
{
	//uchar alen;
	GGSN_table *current,*path,*prev,*next,*tmp_table;
	//struct sockaddr_in fsin;
	HLR_record *hlr_rd=NULL;
        int found=0,find_record=0;

	/*if(!strcmp(myip->control , "NK_create_PDP_r"))
	{
		tmp_table = (GGSN_table *)malloc(sizeof(struct GGSN_table));
		bzero(tmp_table,sizeof(struct GGSN_table));
		tmp_table->TLLI = myip->tlli;
		tmp_table->imsi = myip->imsi;
		tmp_table->msisdn = myip->msisdn;
		tmp_table->NSAPI = myip->nsapi;
		tmp_table->qos = myip->qos;
		tmp_table->TID = tid++;
		tmp_table->SGSNIP = myip->ip;
		tmp_table->next = ggsn_head_t;
		ggsn_head_t = tmp_table;
		myip->tid = tid;
		return(1);
	}*/
	if(!strcmp(myip->control,"create_PDP_r"))
	{
		//fill in GGSN table and create PDP context
		hlr_rd = (HLR_record *)malloc(sizeof(HLR_record));
        	bzero(hlr_rd,sizeof(HLR_record));

		tmp_table = (GGSN_table *)malloc(sizeof(struct GGSN_table));
		bzero(tmp_table,sizeof(struct GGSN_table));
		tmp_table->TLLI = myip->tlli;
		tmp_table->imsi = myip->imsi;
		tmp_table->msisdn = myip->msisdn;
		tmp_table->NSAPI = myip->nsapi;
		tmp_table->qos = myip->qos;
		tmp_table->TID = tid++;
		tmp_table->SGSNIP = myip->ip;
		tmp_table->next = ggsn_head_t;
		ggsn_head_t = tmp_table;
		myip->tid = tid;

		strcpy(myip->control,"create_PDP_rp");

		//add PDP context into HLR database
		//if this record is already in the HLR database
		//we add PDP context address into it
		hlr_rd->tlli=myip->tlli;
		hlr_rd->nsapi=myip->nsapi;
		hlr_rd->ms_ip = myip->ms_ip;
		hlr_rd->tid=myip->tid;
		hlr_rd->ggsnip=nodeid_to_ipv4addr(get_nid(),get_port());
		hlr_rd->sgsnip=myip->ip;
		hlr_rd->qos=myip->qos;
		hlr_rd->imsi=myip->imsi;
		hlr_rd->ptmsi = myip->ptmsi;
		hlr_rd->msisdn=myip->msisdn;
		hlr_rd->ca=0;
		hlr_rd->ra=0;
		HLR_.modify_record(hlr_rd);
		free(hlr_rd);
		//}
		return(1);


	}
	if(!strcmp(myip->control,"update_PDP_r"))
        {
		hlr_rd = (HLR_record *)malloc(sizeof(HLR_record));
        	bzero(hlr_rd,sizeof(HLR_record));

		path = ggsn_head_t;
		 while(path!=NULL)
                {
                        if(path->imsi == myip->imsi && path->NSAPI == myip->nsapi)
                        {
				path->qos = myip->qos;
				found = 1;
				break;

			}
			path = path->next;

		}
		if(found == 0)
			strcpy(myip->cause,"PDP_non_exist");
		else
			strcpy(myip->cause,"request_accept");
		strcpy(myip->control,"update_PDP_rp");
                //(void) sendto(s,(char *)&myip,sizeof(myip),0,(struct sockaddr *)&fsin,sizeof(fsin));
		//hlr_rd->nsapi = myip->nsapi;
		hlr_rd->qos=myip->qos;
		hlr_rd->imsi=myip->imsi;
		HLR_.modify_record(hlr_rd);
		free(hlr_rd);
		return(found);

	}
	if(!strcmp(myip->control,"delete_PDP_r"))
	{
		path = ggsn_head_t;
		//tmp_table = (struct GGSN_table*)malloc(sizeof(GGSN_table));
        	//bzero(tmp_table,sizeof(GGSN_table));
		//tmp_table->next = ggsn_head_t;
        	//ggsn_head_t = tmp_table;
		prev = ggsn_head_t;
        	//head = ggsn_head_t;

		while(path!=NULL)
        	{

			next = path->next;
			if(path->imsi==myip->imsi || path->ptmsi==myip->ptmsi)
                	{
				found = 1;
				current = path;
                                if(ggsn_head_t == current)
					ggsn_head_t = ggsn_head_t->next;
				prev->next = next;
                                path = next;
                                free(current);
                                find_record=1;

			}
			if(!find_record)
			{
				prev = path;
	                	path = next;
			}
			find_record=0;


		}
		if(found == 0)
			strcpy(myip->cause,"PDP_non_exist");
		else
			strcpy(myip->cause,"request_accept");
		//strcpy(myip->control,"delete_PDP_r");
		return(found);

	}
	if(!strcmp(myip->control,"deactivate_PDP_r"))
	{
		//delete the specific item in GGSN table
		path = ggsn_head_t;
		prev = ggsn_head_t;


		while(path!=NULL)
        	{

			next = path->next;
			if(path->NSAPI==myip->nsapi &&(path->imsi==myip->imsi || path->ptmsi==myip->ptmsi))
                	{
				found = 1;
				current = path;
                                if(ggsn_head_t == current)
					ggsn_head_t = ggsn_head_t->next;
				prev->next = next;
                                path = next;
                                free(current);
                                find_record=1;

			}
			if(!find_record)
			{
				prev = path;
	                	path = next;
			}
			find_record = 0;

		}
		if(found == 0)
			strcpy(myip->cause,"PDP_non_exist");
		else
			strcpy(myip->cause,"request_accept");
		//strcpy(myip->control,"delete_PDP_r");
		return(found);

	}
	return(1);


}


