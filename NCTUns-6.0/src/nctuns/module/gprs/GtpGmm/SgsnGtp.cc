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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <nctuns_api.h>
#include "SgsnGtp.h"
#include "GgsnGtp.h"
#include "gmmmanage.h"
#include <gprs/include/debug.h>
#include <timer.h>
#include <maptable.h>
#include <mbinder.h>

#ifdef  __DMALLOC_DEBUG__
#include <dmalloc.h>
#endif


extern  HLR                 HLR_;

MODULE_GENERATOR(SgsnGtp);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
SgsnGtp::SgsnGtp(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
	: NslObject(type, id, pl, name), sgsn_head_t(NULL), manageque(NULL)
{
	vBind("SgsnIP",&tmp_RAI);
	vBind("Base_Delay_Time",&tmp_time);
	vBind("Variance_Time",&tmp_v_time);
	vBind("First_Extra_Delay",&tmp_f_time);
	vBind("BandWidth",&tmp_band);
	vBind("cmdtable",&cmd_table);
	//make sgsn_head_t point to the front of sgsn_t
}

SgsnGtp::~SgsnGtp()
{
	SGSN_table      *tmp_sgsn_p,*tmp_sgsn;
	tmp_sgsn_p = sgsn_head_t;
	while(tmp_sgsn_p != NULL)
	{
		tmp_sgsn = tmp_sgsn_p;
		tmp_sgsn_p = tmp_sgsn_p->next;
		free(tmp_sgsn);
	}
	free(cmd_FILEPATH);
	//free(sgsn_t);
}
int SgsnGtp::init()
{
	cmd_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(cmd_table)+1);
        sprintf(cmd_FILEPATH,"%s%s",GetConfigFileDir(),cmd_table);
	RAI = atol(tmp_RAI);
	delay_time = atoi(tmp_time);
	v_delay_time = atoi(tmp_v_time);
	first_delay_time = atoi(tmp_f_time);
	band_width = atol(tmp_band);
	BASE_OBJTYPE(type);
        u_int64_t wait_interval;
	if(DEBUG_LEVEL >= MSG_DEBUG)
        	printf("SgsnGtp::init()::RAI at SgsnGtp=%ld\n",RAI);
        Event_ *start_ep =  createEvent();
        SEC_TO_TICK(wait_interval, 0);
        type =POINTER_TO_MEMBER(SgsnGtp,initial_maptable);
        setObjEvent(start_ep,wait_interval,0,this,type,(void *)NULL);

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
				printf("Error : Can't create file %s\n",ptrFile);
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
		type = POINTER_TO_MEMBER(SgsnGtp, logFlush);
		ptrlogTimer.setCallOutObj(this, type); 
		ptrlogTimer.start(execint, execint);
	}
	return(1);
}
int SgsnGtp::logFlush()
{
        logTbl->flush();
	return(1);
}

void SgsnGtp::pushManage()
{
        ManageQue* q = manageque;
        if (!q || !q->next)return;
        manageque = (ManageQue*)manageque->next;
        NslObject::put(q->pkt,sendtarget_);
	return ;
}

int SgsnGtp::sendManage(ePacket_ *pkt)
{
        ePacket_ *retpkt;

        SET_MLEVEL_3(pkt);
        sendtarget_->set_upcall(this,(int (NslObject::*)(MBinder *))&SgsnGtp::pushManage);
        if ((retpkt = put1(pkt,sendtarget_))){
                ManageQue* q = manageque;
                if (!q){
                        q = (ManageQue*)malloc(sizeof(ManageQue));
                        q->pkt = retpkt;
                        q->next = NULL;
                }else{
                        while(q->next)q = (ManageQue*)q->next;
                        q->next = (ManageQue*)malloc(sizeof(ManageQue));
                        q = (ManageQue*)q->next;
                        q->pkt = retpkt;
                        q->next = NULL;
                }
        }
        return(1);
}


int SgsnGtp::initial_maptable()
{
        umtbl_add(get_nid(),get_port(),0,&RAI,0,0);
	return(1);
}

int SgsnGtp::PDU_notify_resp(ip_structure *respip)
{
	NslObject *peer;
	int echomsg;
	int retcode = 1;
	peer = InstanceLookup(respip->ip,"GGSNGtp");
        echomsg = ((GgsnGtp*)peer)->PDU_notify_resp_b(RAI,respip);

        if(retcode < 0)
	{
              	 if(DEBUG_LEVEL >= MSG_ERROR)
		printf("SgsnGtp::PDU_notify_rp::write data fail!!\n");
	
	}
	return(echomsg);

	
}
int SgsnGtp::PDU_notify_req_b(ulong srcip,ip_array *ip_list)
{
	
	//Packet *p= new Packet;
	NslObject *peer;
	int peers;
        //char *tmp1="flag",*tmp2;
        //ePacket_ *pkt=createEvent();
        //struct create_PDP *c_r_msg;
        struct ip_structure *responseip = (ip_structure *)malloc(sizeof(ip_structure));
	bzero(responseip,sizeof(ip_structure));
        int found = 0;
        struct SGSN_table *s_table,*tmp_table;

	//pkt->DataInfo_ = p;

        s_table = sgsn_head_t;

	responseip->ip=srcip;
	responseip->imsi = ip_list->imsi;
	//now check if this PDP context exists in SGSN table
	while(s_table!=NULL)
        {
		 if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SgsnGtp::PDU_notify_req_b::s_table's NSAPI:%d qos%d\n",s_table->NSAPI,s_table->qos);
                if(s_table->NSAPI==ip_list->nsapi && s_table->imsi ==ip_list->imsi && s_table->qos==ip_list->qos)
                {
			//now check if this PDP context has already been activated
			if(s_table->TID != 0)
			{
				//this pdp context has alreday been activated
				found = 1;
				break;
				
			}
			else
			{
				s_table->ptmsi = ip_list->ptmsi;
				s_table->qos = ip_list->qos;
				s_table->TID = ip_list->tid;
				s_table->TLLI = ip_list->tlli;
				s_table->GGSNIP = srcip;
				peer = InstanceLookup(srcip,"GGSNGtp");
				peers = ((GgsnGtp*)peer)->createGGSNTABLE(ip_list,RAI);
				found = 1;
				break;
			}
			
                }
                s_table = s_table->next;
        }
	if(!found)
	{
		tmp_table = (SGSN_table *)malloc(sizeof(SGSN_table));
		bzero(tmp_table,sizeof(struct SGSN_table));
		tmp_table->ptmsi = ip_list->ptmsi;
		tmp_table->imsi = ip_list->imsi;
		tmp_table->NSAPI = ip_list->nsapi;
		tmp_table->qos = ip_list->qos;
		tmp_table->TID = ip_list->tid;
		tmp_table->TLLI = ip_list->tlli;
		tmp_table->GGSNIP = srcip;
		tmp_table->next = sgsn_head_t;
		sgsn_head_t = tmp_table;

	}
	strcpy(responseip->cause,"request_accept");
	free(ip_list);
	ip_list = NULL;
	return(PDU_notify_resp(responseip));
        

}
int SgsnGtp::update_PDP_req(ePacket_ *pkt)
{
	
	Packet *p;
	//int nsapi,qos;
	//struct sockaddr_in sin;
        ulong ggsnip;
	//int s,retcode;
	NslObject *peer;
	
	//int i=0;
        ip_structure *myip = (struct ip_structure *)malloc(sizeof(ip_structure));
	bzero(myip,sizeof(ip_structure));
	ip_structure *myip1 = NULL;
	int	found=0,echomsg;
	FILE *iptable;
	struct SGSN_table *s_table=NULL;
	struct activate_type *msg=NULL;
	struct SGSN_table *tmp_table=NULL;
        p = (Packet *)pkt->DataInfo_;
        msg = (struct activate_type *)p->pkt_getinfo("flag");
        s_table = sgsn_head_t;
	iptable = fopen(cmd_FILEPATH,"r");

	/*
        check if it is in the SGSN table and if it is indeed in the SGSN
        we delete it from SGSN table
        */
        while(s_table!=NULL)
        {
                if(s_table->imsi==msg->imsi && s_table->NSAPI == msg->nsapi && s_table->qos==msg->qos)
                {
			ggsnip = s_table->GGSNIP;
			//s_table->qos = msg->qos;
			found = 1;
			
                }
                s_table = s_table->next;
        }
	if(found == 0)
        {
		tmp_table = (SGSN_table *)malloc(sizeof(struct SGSN_table));
                bzero(tmp_table,sizeof(struct SGSN_table));
                tmp_table->imsi = msg->imsi;
                tmp_table->TLLI = msg->tlli;
                tmp_table->NSAPI = msg->nsapi;
                tmp_table->qos = msg->qos;
		myip1 = create_PDP_req(pkt);
                if(myip1->ip == 0)
                {
                 	if(DEBUG_LEVEL >= MSG_ERROR)
                	printf("SgsnGtp::find user specified error ggsnip during update!!\n");
                	return(-1);
                }
                tmp_table->TID = myip1->tid;
               	tmp_table->GGSNIP = myip1->ip;
                tmp_table->next = sgsn_head_t;
                sgsn_head_t = tmp_table;
		free(myip1);
                return(1);
		
	
        }
        else
        {
        	
                //invoke GGSN control socket to serve SGSN

                peer = InstanceLookup(ggsnip,"GGSNGtp");
		strcpy(myip->control,"update_PDP_r");
		myip->imsi = msg->imsi;
		myip->nsapi = msg->nsapi;
                myip->qos = msg->qos;
                echomsg = ((GgsnGtp*)peer)->recvconbottom(myip);
                if(echomsg>0)
		{
                	if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("SgsnGtp::connection created by GGSN side successfully!!\n");
                }
		

	}
	
	if(!strcmp(myip->cause ,"request_accept"))
	{
		 if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SgsnGtp::update_PDP_req is accepted by GGSN!!\n");
		free(myip);
		myip = NULL;
                return(1);
        }
	 if(DEBUG_LEVEL >= MSG_DEBUG)
        printf("SgsnGtp::request not accepted because of non_exist PDP context in GGSN\n ");
        free(myip);
        myip = NULL;
	fclose(iptable);
	return(-1);
	
}

int SgsnGtp::Deactivate_PDP_req(ePacket_ *pkt)
{
	
	
	ulong ggsnip;
	
 	//struct sockaddr_in sin;
	NslObject *peer;	
	ip_structure *myip = (ip_structure *)malloc(sizeof(ip_structure));
	bzero(myip,sizeof(ip_structure));
	int found = 0,echomsg;
        struct SGSN_table *s_table;
        Packet *p;
        //int s,retcode;
        struct deactivate_type *msg;
        p = (Packet *)pkt->DataInfo_;
        msg = (struct deactivate_type *)p->pkt_getinfo("flag");
      
        s_table = sgsn_head_t;
	
        /*
	check if it is in the SGSN table and if it is indeed in the SGSN
	we delete it from SGSN table
	*/
	while(s_table!=NULL)
        {
                if(s_table->NSAPI==msg->nsapi && s_table->imsi==msg->imsi && s_table->qos==msg->qos)
                {
                	ggsnip = s_table->GGSNIP;
			s_table->TID = 0;
			s_table->GGSNIP = 0;
			found = 1;
				
		}
		//prev = s_table;
		s_table = s_table->next;
	}
	if(found == 0)
	{
		 if(DEBUG_LEVEL >= MSG_ERROR)
			printf("SgsnGtp::Deactivate PDP request not accepted because of non_exist PDP context in SGSN\n ");
		return(1);
	}
	else
	{
		
		//invoke GGSN control socket to serve SGSN

                peer = InstanceLookup(ggsnip,"GGSNGtp");
			
		//build control tunnel and send control msg to GGSN side
		strcpy(myip->control,"deactivate_PDP_r");
		myip->imsi = msg->imsi;
                myip->nsapi = msg->nsapi;
                myip->qos = msg->qos;
                echomsg = ((GgsnGtp*)peer)->recvconbottom(myip);
		if(echomsg<0)
		{
			if(DEBUG_LEVEL >= MSG_ERROR)
			printf("SgsnGtp::deactivate PDP record in GGSN table unsuccessfully!!\n");
		}
	}

	if(!strcmp(myip->cause,"request_accept"))
	{
		 if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SgsnGtp::Deactivate_PDP_req is accepted by GGSN!!\n");
		free(myip);
		myip = NULL;
		return(1);
	}
	if(DEBUG_LEVEL >= MSG_ERROR)
		printf("SgsnGtp::Deactivate PDP req is not accepted because non record in GGSN !!\n");
	free(myip);
	myip = NULL;
	return(1);
}
int SgsnGtp::Delete_PDP_req(ePacket_ *pkt)
{
        ulong ggsnip=0;
	int	find_record = 0;
	NslObject *peer;	
	ip_structure *myip = (ip_structure *)malloc(sizeof(struct ip_structure));
	bzero(myip,sizeof(ip_structure));
	int found = 0,echomsg;
        struct SGSN_table *s_table=NULL,*prev=NULL,*next=NULL,*current=NULL;
        Packet *p;
        struct control_msg *msg=NULL;
        p = (Packet *)pkt->DataInfo_;
        msg = (struct control_msg *)p->pkt_getinfo("flag");
        s_table = sgsn_head_t;
	
	prev = sgsn_head_t;
        /*
	check if it is in the SGSN table and if it is indeed in the SGSN
	we delete it from SGSN table
	*/
	while(s_table!=NULL)
        {
		
		next = s_table->next;
                if(s_table->imsi==msg->imsi)
                {
			found = 1;
			if(s_table->GGSNIP!=0)
			{
				ggsnip = s_table->GGSNIP;
			}
				
			current = s_table;
			if(sgsn_head_t == current)
				sgsn_head_t = sgsn_head_t->next;	
			prev->next = next;
			s_table = next;
			free(current);
			current = NULL;
			find_record=1;
			
		}
		if(find_record==0)
		{
			prev = s_table;
			s_table = next;
		}
		find_record = 0;

	}
	if(!found)
	{
		 if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SgsnGtp::request not accepted because of no PDP context in SGSN\n ");
                return(1);
	}
	if(ggsnip!=0)
	{
		//we connect to GGSN to delete its PDP context
		peer = InstanceLookup(ggsnip,"GGSNGtp");
		strcpy(myip->control,"delete_PDP_r");
		myip->imsi = msg->imsi;
		myip->ptmsi = msg->ptmsi;
		myip->nsapi = msg->nsapi;
		myip->qos = msg->qos;
		echomsg = ((GgsnGtp*)peer)->recvconbottom(myip);
		if(echomsg<0)
		{
			if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SgsnGtp::delete PDP record in GGSN table unsuccessfully!!\n");
		}
					
	}
	if(!strcmp(myip->cause , "request_accept"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SgsnGtp::Delete_PDP_req request is accepted by GGSN!!\n");
		free(myip);
		myip = NULL;
		return(1);
	}
	 if(DEBUG_LEVEL >= MSG_ERROR)
		printf("SgsnGtp::Delete_PDP_context request not accepted because of non_exist PDP context in GGSN\n ");
	free(myip);
	myip = NULL;
	return(1);

		
}

int SgsnGtp::recvlog(ePacket_ *pkt,u_int64_t s_delay_time,ulong band_width)
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
        start_time = start_time-(u_int64_t)(delay_time*(1.0/2));
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


int SgsnGtp::sendlog(ePacket_ *pkt,u_int64_t s_delay_time,ulong band_width)
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
int SgsnGtp::sslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2)
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
int SgsnGtp::rslog(ePacket_ *pkt,u_int64_t start_time,u_int32_t node_id1,u_int32_t node_id2)
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
        ss8023log->Event = StartRX;
        ss8023log->IP_Src = 0;
        ss8023log->IP_Dst = 0;
        ss8023log->PHY_Src = node_id1;
        ss8023log->PHY_Dst = node_id2;
        ss8023log->RetryCount = 0;
        //ss8023log->FrameID = pkt_->pkt_getpid();
	ss8023log->FrameID = 0;
        ss8023log->FrameType = FRAMETYPE_DATA;
        ss8023log->FrameLen = pkt_->pkt_getlen();
        ss8023log->DropReason = DROP_NONE;
        sslogEvent->log = (char *)ss8023log;
        logQ.push(sslogEvent);
	return(1);

}

int SgsnGtp::selog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t node_id1,u_int32_t node_id2)
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
int SgsnGtp::relog(ePacket_ *pkt,u_int64_t start_time,u_int64_t end_time,u_int32_t node_id1,u_int32_t node_id2)
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

int SgsnGtp::send(ePacket_ *pkt)
{
	
	int	found=0;//1:found 0:not found
	
	Packet *p;
	NslObject *peer;
	struct GTP_header *gtp_h;
	const char *tmp1="flag";
	char *tmp2;
	struct control_msg *msg = NULL;
	struct control_msg *msg1;
	p = (Packet *)pkt->DataInfo_;
	msg = (struct control_msg *)p->pkt_getinfo("flag");
	
	

	if(!strcmp(msg->control,"update_PDP_r") || !strcmp(msg->control,"sgsn_update_PDP_r"))
	{
		  struct activate_type *update_t=NULL,*update_t1=NULL;
		 Packet *p1= new Packet;
		 ePacket_ *pkt1=createEvent();
		 update_t = (struct activate_type *)p->pkt_getinfo("flag");
		 pkt1->DataInfo_ = p1;
		 update_t1 = (activate_type *)malloc(sizeof(struct activate_type));
		 bzero(update_t1,sizeof(struct activate_type));
		 update_t1->nsapi = update_t->nsapi;
		 update_t1->qos = update_t->qos;
		 update_t1->imsi = update_t->imsi;
		 update_t1->tlli = update_t->tlli;
		 if(update_PDP_req(pkt)>0)
		 {
		 	//Update PDP context success
		 	if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("SgsnGtp::Update PDP context success\n");
		 	if(!strcmp(update_t->control ,"update_PDP_r"))
		 	{
				strcpy(update_t1->control,"update_PDP_r_s");
		 	}
		 	else
		 	{
				strcpy(update_t1->control,"sgsn_update_PDP_r_s");
		 		update_t1->qos = update_t->qos;
		 	}
			tmp2 = (char *)update_t1;
			//when we call attach , we change status to ready
			p1->pkt_setflow(PF_RECV);
			p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
			//we send attach request message to the SGSN
			free(update_t1);
			update_t1 = NULL;
			freePacket(pkt);
			return(NslObject::recv(pkt1));	
		 }
		 else
		 {
		 	//Update PDP context fail
			if(DEBUG_LEVEL >= MSG_ERROR)
		 		printf("SgsnGtp::Update PDP context fail!!\n");
		 	if(!strcmp(update_t->control ,"update_PDP_r"))
				strcpy(update_t1->control,"update_PDP_r_f");
		 	else
				strcpy(update_t1->control,"sgsn_update_PDP_r_f");
		 	
			tmp2 = (char *)update_t1;
			//when we call attach , we change status to ready
			p1->pkt_setflow(PF_RECV);
			p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
			//we send attach request message to the SGSN
			free(update_t1);
			update_t1 = NULL;
			freePacket(pkt);
			return(NslObject::recv(pkt1));	
		 }
	}
	else if(!strcmp(msg->control ,"sgsn_context_r"))
	{
		struct route_msg *rt_msg=NULL,*msg2=NULL;
		msg2 = (struct route_msg *)p->pkt_getinfo("flag");
		rt_msg = (route_msg *)malloc(sizeof(struct route_msg));
		bzero(rt_msg,sizeof(struct route_msg));
		if(SGSN_context_req(pkt)>0)
		{
			
			Packet *p1 = new Packet;
        		ePacket_ *pkt1 = createEvent();
        		pkt1->DataInfo_ = p1 ;
			strcpy(rt_msg->control,"route_area_update_accept");  
        		rt_msg->RAI = RAI;
        		rt_msg->imsi = msg2->imsi;
        		rt_msg->ptmsi = msg2->ptmsi;
        		rt_msg->ptmsi_sig = msg2->ptmsi_sig;
        		rt_msg->CI = msg2->CI;
        		rt_msg->TEID = msg2->RAI;
			rt_msg->tlli = msg2->tlli;
        		tmp2 = (char *)rt_msg;	
			p1->pkt_setflow(PF_RECV);
        		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct route_msg));	
        		free(rt_msg);
			rt_msg = NULL;
        		freePacket(pkt);
        		return( NslObject::recv(pkt1));
		}
		else
		{
			 if(DEBUG_LEVEL >= MSG_ERROR)
				printf("ERROR::SgsnGtp::SGSN Context Request is fail!!\n");
			Packet *p1 = new Packet;
        		ePacket_ *pkt1 = createEvent();
        		pkt1->DataInfo_ = p1 ;
			strcpy(rt_msg->control,"sgsn_context_r_f");
			rt_msg->imsi = msg2->imsi;
			rt_msg->tlli = msg2->tlli; 
			tmp2 = (char *)rt_msg;	
			p1->pkt_setflow(PF_RECV);
        		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct route_msg));	
        		free(rt_msg);
			rt_msg = NULL;
        		freePacket(pkt);
        		return( NslObject::recv(pkt1)); 
			
		}
	}
	else if(!strcmp(msg->control,"delete_PDP_r"))
	{
		//generate new packet and send it to SGSN's GMM
		Packet *p1 = new Packet;
        	ePacket_ *pkt1 = createEvent();
        	ulong tmp_imsi;
        	struct HLR_record *H_rec = (HLR_record *)malloc(sizeof(struct HLR_record));
        	pkt1->DataInfo_ = p1 ;
        	msg1 = (control_msg *)malloc(sizeof(struct control_msg));       	
        	bzero(msg1,sizeof(control_msg));
		msg1->nsapi = msg->nsapi;
        	msg1->qos = msg->qos;
        	msg1->imsi = msg->imsi;
        	msg1->ptmsi = msg->ptmsi;
        	msg1->ptmsi_sig = msg->ptmsi_sig; 
        	msg1->type = msg->type;
		msg1->tlli = msg->tlli;
        	if(msg1->type == 0)
        	{   	
        		//initial_HLR(H_rec);
			bzero(H_rec,sizeof(HLR_record));
			H_rec->imsi = msg->imsi;
			H_rec->ptmsi=msg->ptmsi;
			H_rec->sgsnip=RAI;
			tmp_imsi = HLR_.modify_record(H_rec);	
			if(tmp_imsi < 0)
			{
				 if(DEBUG_LEVEL >= MSG_ERROR)
					printf("SgsnGtp::delete_PDP_r::modify sgsnip unsuccessfully!!\n");      
			}
			free(H_rec);
			H_rec = NULL; 
		}
		 	
		if(Delete_PDP_req(pkt)>0)
		{
			
			if(DEBUG_LEVEL >= MSG_DEBUG)	
				printf("SgsnGtp::delete_PDP_req successfully\n");
			strcpy(msg1->control,"delete_PDP_rp_s");
			tmp2 = (char *)msg1;	
        		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));
			p1->pkt_setflow(PF_RECV);	
        		free(msg1);
			msg1 = NULL;
        		freePacket(pkt);
        		return( NslObject::recv(pkt1));
		}
		assert(0);
		return 0;
		
	}
	else if(!strcmp(msg->control,"deactivate_PDP_r"))
	{
		
		 Packet *p1= new Packet;
		 ePacket_ *pkt1=createEvent();
		 struct deactivate_type *deac_type1=NULL;
		 pkt1->DataInfo_ = p1;
		 deac_type1 = (deactivate_type *)malloc(sizeof(struct deactivate_type));
		 bzero(deac_type1,sizeof(struct deactivate_type));
		 if(Deactivate_PDP_req(pkt)>0)
		 {
		 	//Deactivate PDP context success
			 if(DEBUG_LEVEL >= MSG_DEBUG)
		 		printf("SgsnGtp::Deactivate PDP context success\n");
			strcpy(deac_type1->control,"deactivate_PDP_r_s");
			deac_type1->imsi = msg->imsi;
			deac_type1->tlli = msg->tlli;
			tmp2 = (char *)deac_type1;
			//when we call attach , we change status to ready
			p1->pkt_setflow(PF_RECV);
			p1->pkt_addinfo(tmp1,tmp2,sizeof(struct deactivate_type));
			//we send attach request message to the SGSN
			
			freePacket(pkt);
			free(deac_type1);
			deac_type1 = NULL;
			return(NslObject::recv(pkt1));	
		 }
		 assert(0);
		 return 1;
		 
	}
	else if(!strcmp(msg->control,"indi_reqst"))
	{
		ulong tmp_imsi = Indi_req(msg);

		if(tmp_imsi > 0)
		{
			if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("SgsnGtp::obtain indi_reqst successfully");
			Packet *p1 = new Packet;
        		ePacket_ *pkt1 = createEvent();
        		pkt1->DataInfo_ = p1 ;
        		msg1 = (control_msg *)malloc(sizeof(struct control_msg));
			bzero(msg1,sizeof(struct control_msg));       	
                        msg1->imsi  = tmp_imsi;
        		msg1->nsapi = msg->nsapi;
        		msg1->qos = msg->qos;
        		msg1->ptmsi = msg->ptmsi;
        		msg1->TEID = msg->TEID;
        		msg1->ptmsi_sig = msg->ptmsi_sig; 
        		msg1->type = msg->type;
			strcpy(msg1->control,"indi_resp");     		               
			msg1->CI = msg->CI;
			msg1->RAI = RAI;
			msg1->ms_ip = msg->ms_ip;  
			msg1->msisdn = msg->msisdn;
			msg1->tlli = msg->tlli;  
			msg1->TID = 0;
			
			tmp2 = (char *)msg1;
			p1->pkt_setflow(PF_RECV);	
        		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));	
        		free(msg1);
			msg1 = NULL;
        		freePacket(pkt);
        		return( NslObject::recv(pkt1));
        	}
        	else
        	{
			if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("SgsnGtp::obtain indi_req fail and are to send iden_req!!");
        		Packet *p= new Packet;
			const char *tmp1="flag";
			char *tmp2;
			ePacket_ *pkt=createEvent();
			pkt->DataInfo_ = p; 
			struct control_msg *r_msg = (control_msg *)malloc(sizeof(struct control_msg));
			bzero(r_msg,sizeof(struct control_msg)); 
			strcpy(r_msg->control,"iden_req");
			r_msg->imsi = msg->imsi;		// Beware of this line.
			r_msg->tlli = msg->tlli;
			tmp2 = (char *)r_msg;
			//add control msg in PT_INFO
			p->pkt_setflow(PF_RECV);
			p->pkt_addinfo(tmp1,tmp2,sizeof(struct return_msg));
			free(r_msg);
			r_msg = NULL;
			return(NslObject::recv(pkt));		    
        	}
	
	}
	else if(!strcmp(msg->control , "update_location_req"))
	{
		Packet *p1 = new Packet;
        	ePacket_ *pkt1 = createEvent();
        	pkt1->DataInfo_ = p1 ;
        	msg1 = (control_msg *)malloc(sizeof(struct control_msg));       	
		bzero(msg1,sizeof(struct control_msg));
		if(update_location(msg)>0)
		{
        		msg1->nsapi = msg->nsapi;
        		msg1->qos = msg->qos;
        		msg1->imsi = msg->imsi;
        		msg1->ptmsi = msg->ptmsi;
        		msg1->ptmsi_sig = msg->ptmsi_sig; 
        		msg1->type = msg->type;  
        		msg1->TEID = msg->TEID;   
        		msg1->tlli = msg->tlli;
			strcpy(msg1->control,"update_location_resp");	               
			msg1->CI = msg->CI;
			tmp2 = (char *)msg1;
			p1->pkt_setflow(PF_RECV);	
        		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));	
        		free(msg1);
        		msg1 = NULL;
			freePacket(pkt);
        		return( NslObject::recv(pkt1));
		}
		assert(0);
		return 1;
		
	}
	//else if(!strcmp(msg->control,"create_PDP_r") || !strcmp(msg->control,"NK_create_PDP_r"))
	else if(!strcmp(msg->control,"create_PDP_r"))
	{
		
		
		ip_structure *myip=NULL;
		struct activate_type *act_type = (struct activate_type *)p->pkt_getinfo("flag");
		struct SGSN_table *s_table=NULL,*tmp_table=NULL;
		struct activate_type *act_type1 = (struct activate_type *)malloc(sizeof(struct activate_type));
		//act_type1 = NULL;
		bzero(act_type1,sizeof(struct activate_type));
		s_table = sgsn_head_t;
		Packet *p1= new Packet;
                //free(act_type1);
                //ePacket_ *pkt2=createEvent();
		ePacket_ *pkt1=createEvent();

		while(s_table != NULL)
		{
			if(s_table->NSAPI==act_type->nsapi && s_table->imsi==act_type->imsi && s_table->qos==act_type->qos)
			{
				if(s_table->TID == 0)
				{
					/*if(!strcmp(act_type->control , "NK_create_PDP_r"))
						myip = NK_create_PDP_req(pkt);
					else*/
					myip =  create_PDP_req(pkt);
					s_table->qos = myip->qos;
					s_table->TID = myip->tid;
					s_table->GGSNIP = myip->ip;
					found = 2;
					break;
				
				}
				else
				{
					found=1;
					break;
				}
			}
			s_table = s_table->next;
		}
		if(!found)
		{
			//not yet in SGSN table
			//add info into SGSN table
			 if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("SgsnGtp::create_PDP_r::not found in SGSN table and add into SGSN table!!\n");
			tmp_table = (SGSN_table *)malloc(sizeof(struct SGSN_table));
			bzero(tmp_table,sizeof(struct SGSN_table));
			tmp_table->imsi = act_type->imsi;
			tmp_table->TLLI = act_type->tlli;
			tmp_table->NSAPI = act_type->nsapi;
			//sgsn_t->qos = msg->qos;
			/*if(!strcmp(act_type->control , "NK_create_PDP_r"))
				myip = NK_create_PDP_req(pkt);
			else*/
			myip = create_PDP_req(pkt);
			if(myip->ip == 0)
                	{
				if(DEBUG_LEVEL >= MSG_DEBUG)
                 			printf("SgsnGtp::user specified error ggsnip!!\n");
                 		Packet *p1= new Packet;
				ePacket_ *pkt1=createEvent();
				pkt1->DataInfo_ = p1;
				strcpy(act_type1->control,"activate_error_ip");
				act_type1->imsi = act_type->imsi;
				act_type1->tlli = act_type->tlli;
				tmp2 = (char *)act_type1;
				p1->pkt_setflow(PF_RECV);
				p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
				free(act_type1);
				act_type1 = NULL;
				freePacket(pkt);
				free(myip);
				myip = NULL;
				return(NslObject::recv(pkt1));	
                	}
                	
                	tmp_table->qos = myip->qos;
			tmp_table->TID = myip->tid;
                	tmp_table->GGSNIP = myip->ip;
			tmp_table->next = sgsn_head_t;
			sgsn_head_t = tmp_table;
		}
			
		if(found==0 || found==2)
		{
			if(act_type->qos != myip->qos)
				strcpy(act_type1->control,"activate_accept_qos_n");
			else
				strcpy(act_type1->control,"activate_PDP_accept");
			free(myip);
                        myip = NULL;

		}
		else
			strcpy(act_type1->control,"activate_PDP_accept");
		//Packet *p1= new Packet;
		//free(act_type1);
		//ePacket_ *pkt1=createEvent();
		pkt1->DataInfo_ = p1;
		act_type1->imsi = act_type->imsi;
		act_type1->tlli = act_type->tlli;
		tmp2 = (char *)act_type1;
		p1->pkt_setflow(PF_RECV);
		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
		free(act_type1);
		act_type1 = NULL;
		freePacket(pkt);
		return(NslObject::recv(pkt1));	
		
		
	}
	else
	{
		//in other conditions,we send normal packet in tunnel to the peer side
		//check if it is in the SGSN table 
		//call create PDP request if it is not in the SGSN table
		ip_structure *myip = NULL;
		struct SGSN_table *s_table,*tmp_table;
		const char *tmp1="flag";
		char *tmp2;
		struct control_msg *s_delay=NULL;
		s_table = sgsn_head_t;
		u_int64_t t_simulate_delay_time;
                ulong simulate_delay_time;
		s_delay = (control_msg *)malloc(sizeof(struct control_msg));
                bzero(s_delay,sizeof(struct control_msg));

		while(s_table!=NULL)
		{
			if(s_table->NSAPI==msg->nsapi && s_table->imsi==msg->imsi && s_table->qos==msg->qos)
        		{
				/*
				already in SGSN table but its PDP context doesn't created yet
				*/
				 if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("SgsnGtp::send normal packet GGSN::NSAPI=%d QOS=%d IMSI=%lu\n",msg->nsapi,msg->qos,msg->imsi);
				if(s_table->TID == 0)
				{
					myip =  create_PDP_req(pkt);
					//printf("obtain's GGSNIP:%ld",myip->ip);
					if(myip->ip == 0)
					{
						if(DEBUG_LEVEL >= MSG_ERROR)
							printf("SgsnGtp::send normal packet::find user specified error ggsnip!!\n");
						freePacket(pkt);
                				return(1);
					}
					s_table->TID = myip->tid;
					s_table->GGSNIP = myip->ip;
				
					found=1;
				}
				else
				{
					myip = (ip_structure *)malloc(sizeof(ip_structure));
        				bzero(myip,sizeof(ip_structure));
					myip->ip = s_table->GGSNIP;
				}
				
				//and tunnel has already been created
				//now send it directly
				 //add GTP header to packet and transmit it to GGSN
				gtp_h = (GTP_header *)p->pkt_malloc(sizeof(GTP_header));
				gtp_h->Version = 1;
				gtp_h->PT = 1;
				gtp_h->E = 0;
				gtp_h->S = 0;
				gtp_h->PN = 0;
				gtp_h->msg_type = 240;
				gtp_h->TEID = myip->ip;
				
				/*calcute delay time for packet which has been activated*/
				//simulate_delay_time = 400 + rand()%150;
                        	
				simulate_delay_time = delay_time + rand()%v_delay_time;
				
				MILLI_TO_TICK(t_simulate_delay_time, simulate_delay_time);
				sendlog(pkt,t_simulate_delay_time,band_width);
				
				s_delay->delay_time = t_simulate_delay_time;
                                tmp2 = (char *)s_delay;
                                p->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));
				free(s_delay);
	
				peer =InstanceLookup(s_table->GGSNIP,"GGSNGtp");	
				BASE_OBJTYPE(type);
                                type = POINTER_TO_MEMBER(GgsnGtp, recvbottom);
                                setObjEvent(pkt, GetCurrentTime()+t_simulate_delay_time, 0,
                                peer, type,pkt->DataInfo_);
					
				found = 1;			
				break;
				
			}
				s_table = s_table->next;		
        	}		
		if(found==0)
		{
			//not yet in SGSN table
			//add info into SGSN table
			tmp_table = (SGSN_table *)malloc(sizeof(struct SGSN_table));
			bzero(tmp_table,sizeof(struct SGSN_table));
			tmp_table->imsi = msg->imsi;
			tmp_table->TLLI = msg->tlli;
			tmp_table->NSAPI = msg->nsapi;
			tmp_table->qos = msg->qos;		
			myip = create_PDP_req(pkt);
			if(myip->ip == 0)
                	{
                 		 if(DEBUG_LEVEL >= MSG_ERROR)
				printf("SgsnGtp::find user specified error ggsnip!!\n");
                		return(-1);
                	}
			tmp_table->TID = myip->tid;
                	tmp_table->GGSNIP = myip->ip;
			tmp_table->next = sgsn_head_t;
			sgsn_head_t = tmp_table;
                	/*Now we are to send packet*/
                	peer =InstanceLookup(myip->ip,"GGSNGtp");

			//and tunnel has already been created
			//now send it directly
			//add GTP header to packet and transmit it to GGSN
			gtp_h = (GTP_header *)p->pkt_malloc(sizeof(struct GTP_header));
			gtp_h->Version = 1;
			gtp_h->PT = 1;
			gtp_h->E = 0;
			gtp_h->S = 0;
			gtp_h->PN = 0;
			gtp_h->msg_type = 240;
			gtp_h->TEID = myip->ip;
			
                        /*calcute delay time for packet which has been activated*/
                        //simulate_delay_time = 1100 + rand()%150;
                        simulate_delay_time = delay_time + first_delay_time + rand()%v_delay_time;

			MILLI_TO_TICK(t_simulate_delay_time, simulate_delay_time);
			sendlog(pkt,t_simulate_delay_time,band_width);
			
                        s_delay->delay_time = t_simulate_delay_time;
                        tmp2 = (char *)s_delay;
                        p->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));
			free(s_delay);
			
                        peer =InstanceLookup(myip->ip,"GGSNGtp");
                        BASE_OBJTYPE(type);
                        type = POINTER_TO_MEMBER(GgsnGtp, recvbottom);
                        setObjEvent(pkt, GetCurrentTime()+t_simulate_delay_time, 0,
                        peer, type,pkt->DataInfo_);
	
		}
	found =0;
	free(myip);
	myip = NULL;
	return(1);
	}
	return(1);
	
}

/*ip_structure *SgsnGtp::NK_create_PDP_req(ePacket_ *pkt)
{
	Packet *p;
	int	echomsg;
	struct activate_type *msg;
	struct ip_structure *myip=(ip_structure *)malloc(sizeof(struct ip_structure));
	NslObject *peer;
	//open the file and search the specific GGSN IP
	p = (Packet *)pkt->DataInfo_;
        msg = (struct activate_type *)p->pkt_getinfo("flag");
	peer = InstanceLookup(msg->ms_ip,"GGSNGtp");					
	bzero(myip,sizeof(struct ip_structure));
	strcpy(myip->control,"NK_create_PDP_r");
	myip->msisdn = msg->msisdn;			
	myip->imsi = msg->imsi;
	myip->nsapi = msg->nsapi;
	myip->qos = msg->qos;
	myip->tlli = msg->tlli;
	myip->ip = RAI;	
	//get this node's ip i.e. sgsn's ip
        echomsg = ((GgsnGtp*)peer)->recvconbottom(myip);
        if(echomsg < 0)
	{
        	if(DEBUG_LEVEL >= MSG_ERROR)
			printf("SgsnGtp::NK_create_PDP_req()::connection created by GGSN side unsuccessfully!!\n");        
        }
	return(myip);
	
        
}*/
ip_structure *SgsnGtp::create_PDP_req(ePacket_ *pkt)
{
	int found =0,echomsg;//0 :not be found 1:find 
	Packet *p;
	struct in_addr in_address;
	char str_ip[50],r_line[100];
	char *tmp_store = new char[50];
	char str[3][50];
	int i=0;
	struct activate_type *msg;
	FILE *iptable;
	int nsapi=0;
	struct ip_structure *myip = (ip_structure *)malloc(sizeof(ip_structure));
	int qos=0,dt_qos=0;
	ulong ggsnip=0,dt_ggsnip=0;	
	NslObject *peer;
	bzero(myip,sizeof(struct ip_structure));
	bzero(tmp_store,sizeof(tmp_store));
        bzero(str,sizeof(str));
	/*open the file and search the specific GGSN IP*/
	p = (Packet *)pkt->DataInfo_;
        msg = (struct activate_type *)p->pkt_getinfo("flag");
	iptable = fopen(cmd_FILEPATH,"r");
	while(!feof(iptable))
	{
		i = 0;
		bzero(r_line,sizeof(r_line));
		bzero(str_ip,sizeof(str_ip));

		if (fgets(r_line,100,iptable) == NULL)
		{
			break;
		}

		tmp_store = strtok(r_line,"\t\n");
		sprintf(str[i],"%s",tmp_store);
                //printf("SgsnGtp::nsapi::%s\n",str[i]);
		while(1)
                {
                        i++;
                        tmp_store = strtok(NULL,"\t\n");
			if (!tmp_store)
				break;
                        sprintf(str[i],"%s",tmp_store);

                 }
		nsapi = atol(str[0]);
		qos = atoi(str[1]);
		sprintf(str_ip,"%s",str[2]);
		if(nsapi == 0)
		{
			inet_aton(str_ip,&in_address);
			dt_ggsnip = (ulong)in_address.s_addr;
		}
		if(msg->nsapi == nsapi && msg->qos == qos)
		{
			//build tunnel according to the mapped GGSN IP
			inet_aton(str_ip,&in_address);
			ggsnip = (ulong)in_address.s_addr;
			//invoke GGSN control socket to serve SGSN
				
			peer = InstanceLookup(ggsnip,"GGSNGtp");
			strcpy(myip->control,"create_PDP_r");
			myip->msisdn = msg->msisdn;
			myip->imsi = msg->imsi;
			myip->nsapi = msg->nsapi;
			myip->qos = qos;
			myip->tlli = msg->tlli;
			myip->ip = RAI;
			myip->ms_ip = msg->ms_ip;
			//get this node's ip i.e. sgsn's ip
                        echomsg = ((GgsnGtp*)peer)->recvconbottom(myip);
                        if(echomsg <0)
			{
                        	 if(DEBUG_LEVEL >= MSG_ERROR)
					printf("SgsnGtp::create_PDP_req::connectoin created by GGSN was made unsuccessfully!!\n");
			
			}
			myip->ip = ggsnip;
			found = 1 ;		
			break;

		}
		
	}
	
	fclose(iptable);
	if(found == 0)
	{	
		
		peer = InstanceLookup(dt_ggsnip,"GGSNGtp");
		strcpy(myip->control,"create_PDP_r");
		myip->msisdn = msg->msisdn;
		myip->imsi = msg->imsi;
		myip->nsapi = msg->nsapi;
		myip->qos = dt_qos;
		myip->tlli = msg->tlli;
		myip->ip = RAI;
		myip->ms_ip = msg->ms_ip;
		//get this node's ip i.e. sgsn's ip
                echomsg = ((GgsnGtp*)peer)->recvconbottom(myip);
                if(echomsg <0)
		{
                	 if(DEBUG_LEVEL >= MSG_ERROR)
			printf("SgsnGtp::create_PDP_req::connectoin created by GGSN was made unsuccessfully!!\n");
		}
		myip->ip = dt_ggsnip;
		return(myip);
	}
	return(myip);

}

