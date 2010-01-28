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

#include <time.h>
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
#include <object.h>
#include "GprsGmm.h"
#include <gprs/include/debug.h>
#include <gprs/GprsSw/GprsSw.h>

extern HLR                 HLR_;

MODULE_GENERATOR(GprsGmm);

gmm_::gmm_(GprsGmm *upper)
{
	uplayer = upper;
	//initial SGSN's IP

	//initial MS's status
	status = 0;


}
gmm_::~gmm_()
{

}
int gmm_::initial_HLR(HLR_record *H_rec)
{
	H_rec->tlli=0;
	H_rec->nsapi=0;
	H_rec->tid=0;
	H_rec->ggsnip=0;
	H_rec->sgsnip=0;
	H_rec->qos=0;
	H_rec->imsi=0;
	H_rec->ptmsi=0;
	H_rec->msisdn=0;
	H_rec->ca=0;
	H_rec->ra=0;
	return(1);
}

int GprsGmm::freeBss_message(bss_message *p)
{
        if(p->rlc_option)
                free(p->rlc_option);
        if(p->llc_option)
                free(p->llc_option);
        if(p->sndcp_option)
                free(p->sndcp_option);
        if(p->rlc_header)
                free(p->rlc_header);
        if(p->mac_header)
                free(p->mac_header);
        if(p->llc_header)
                free(p->llc_header);
        if(p->sndcp_header)
                free(p->sndcp_header);
        if(p->bssgp_header)
                free(p->bssgp_header);
        if(p->user_data)
                free(p->user_data);
        if(p->packet)
                free(p->packet);
        return(1);

}

int gmm_::send_PDP_req(uchar qos,ushort nsapi,ulong imsi,ulong tlli, const char *control)
{
	/*MS-initiated PDP context Modification Procedure
	send update PDP context request to SGSN and GGSN to create PDP context
	*/
	struct activate_type *msg = (activate_type *)malloc(sizeof(struct activate_type));
	bzero(msg,sizeof(struct activate_type));
	sprintf(msg->control,"%s",control);
	msg->qos = qos;
	msg->imsi = 0;
	//we send attach request message to the SGSN
	if(!strcmp(control,"modify_PDP_r"))
	{
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
		bzero(gmmop,sizeof(GmmOption));
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::send_PDP_req()::update control=%s\n",msg->control);
		struct bss_message *p1= new bss_message;
		bzero(p1,sizeof(struct bss_message));
		ePacket_ *pkt1=createEvent();
		pkt1->DataInfo_ = p1;
		msg->nsapi = 0;	
		msg->tlli = tlli;
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
  	        llcop->tlli = tlli;
		llcop->qos = 1;

		p1->llc_option = (char *)llcop;
        	p1->llc_option_len = sizeof(struct LLCOption);
		p1->user_data = (char *)msg;
		p1->user_data_len = sizeof(struct activate_type);
		gmmop->imsi = imsi;
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(struct GmmOption);
		p1->flag = PF_SEND;
		return(uplayer->GprsObject::put(pkt1, uplayer->sendtarget_));
		//return(uplayer->NslObject::send(pkt1));
	}
	else
	{
		Packet *p1= new Packet;
		ePacket_ *pkt1=createEvent();
		const char *tmp1 = "flag";
		char *tmp2;
		pkt1->DataInfo_ = p1;
		msg->nsapi = nsapi;	
		tmp2 = (char *)msg;
		//when we call attach , we change status to ready
		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
		p1->pkt_setflow(PF_RECV);
		free(msg);
		return(uplayer->NslObject::recv(pkt1));	
	}
		
}
int gmm_::SGSN_context_req(ePacket_ *pkt)
{
	/*
	 *New SGSN asks the PDP context of old SGSN
	*/
	struct route_msg *msg=NULL,*msg1=NULL;
	struct bss_message *p;
	Packet *p1 = new Packet;
	ePacket_ *pkt1=createEvent();
	const char *tmp1 = "flag";
	char *tmp2;
	p = (bss_message *)pkt->DataInfo_;
	pkt1->DataInfo_ = p1;
	msg = (struct route_msg *)p->user_data;
	msg1 = (route_msg *)malloc(sizeof(struct route_msg));
	bzero(msg1,sizeof(struct route_msg));
	strcpy(msg1->control,"sgsn_context_r");
	msg1->ptmsi = msg->ptmsi;
	msg1->ptmsi_sig = msg->ptmsi_sig;
	msg1->tlli = msg->tlli;
	msg1->imsi = msg->imsi;
	msg1->TEID = HLR_.getSGSNIP(msg->imsi);
	msg1->CI = msg->CI;
	msg1->RAI = msg->RAI;
	tmp2 = (char *)msg1;
	p1->pkt_addinfo(tmp1,tmp2,sizeof(struct route_msg));
	p1->pkt_setflow(PF_RECV);
	free(msg1);
	uplayer->freeBss_message(p);
	free(pkt);
	return(uplayer->NslObject::recv(pkt1));		
	
	
}

int gmm_::update_location_rep(ePacket_ *pkt)
{
	NslObject *peer;
	int peers;
	Packet *p1 = new Packet;
        ePacket_ *pkt1 = createEvent();
        pkt1->DataInfo_ = p1 ;
	const char *tmp1 = "flag";
	char *tmp2;
	time_t the_time;
	struct control_msg *msg;
	struct ms_storage *msg1;
	struct HLR_record *H_rec = (HLR_record *)malloc(sizeof(struct HLR_record));
	ulong tmp_imsi;
	Packet *p;
	p = (Packet *)pkt->DataInfo_;
	msg = (struct control_msg *)p->pkt_getinfo("flag");
	
	// and now we generate a new ptmsi for MS
	the_time = time((time_t *)0);
	msg->ptmsi = (ulong)the_time;
	//and we need to generate new p-tmsi signature
	the_time = time((time_t *)0);
	msg->ptmsi_sig=(ulong)the_time+1;
	//now add the random ptmsi to HLR
	bzero(H_rec,sizeof(HLR_record));
	//initial_HLR(H_rec);
	H_rec->imsi = msg->imsi;
	H_rec->ptmsi=msg->ptmsi;
	tmp_imsi = HLR_.modify_record(H_rec);
	if(tmp_imsi == 0)
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("modify the HLR record unsuccessfully!!\n");
	}
		
	/*Now we are to update new VLR*/
	msg1  = (struct ms_storage *)msg;
	peer = InstanceLookup(uplayer->getRAI(),"GprsSw");
       	peers = ((GprsSw *)peer)->update_VLR(msg1);
       	
	
	/*Now we are to cancel old record in old VLR*/
	peer = InstanceLookup(msg->TEID,"GprsSw");					
	peers = ((GprsSw *)peer)->cancel_old_VLR_rd(msg->TEID,msg);
	strcpy(msg->control,"delete_PDP_r");
	tmp2 = (char *)msg;
        p1->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));
	p1->pkt_setflow(PF_RECV);
        free(H_rec);
	freePacket(pkt);
	//return(uplayer->put(pkt1, recvtarget_));
	return(uplayer->NslObject::recv(pkt1));		
}

int gmm_::Indi_rep(ePacket_ *pkt)
{
	struct HLR_record *H_rec = (HLR_record *)malloc(sizeof(struct HLR_record));
	struct control_msg *msg1=NULL;
	struct bss_message *p;
	Packet *p1= new Packet;
	ePacket_ *pkt1=new ePacket_;
	const char *tmp1 = "flag";
	char *tmp2;
	ulong 	tmp_imsi;
	pkt1->DataInfo_ = p1;
	p = (bss_message *)pkt->DataInfo_;
	msg1 = (struct control_msg *)p->user_data;
	//msg1 = (struct control_msg *)p->pkt_getinfo("flag");
	bzero(H_rec,sizeof(HLR_record));
	//initial_HLR(H_rec);
	H_rec->sgsnip = HLR_.getSGSNIP(msg1->RAI);
	H_rec->imsi=msg1->imsi;
	H_rec->ra=uplayer->getRAI();
	/*we add ms'ip to HLR which we can use for the downward packet*/
	H_rec->ms_ip = msg1->ms_ip;
	H_rec->tlli = msg1->tlli;
	H_rec->nsapi = msg1->nsapi;
	H_rec->qos = msg1->qos; 
	tmp_imsi = HLR_.modify_record(H_rec);
	msg1->imsi = tmp_imsi;
	if(tmp_imsi == 0)
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::Indi_rep()::modify the HLR record unsuccessfully!!\n");
	}
	bzero(msg1->control,sizeof(char[30])); 
	strcpy(msg1->control,"update_location_req");
	tmp2 = (char *)msg1;
	p1->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));
	p1->pkt_setflow(PF_RECV);
	free(H_rec);
	uplayer->freeBss_message(p);
        free(pkt);	
	return(uplayer->NslObject::recv(pkt1));
	
	
}

int gmm_::detach(ePacket_ *pkt)
{
	struct detach_type *msg=NULL,*msg1=NULL;
	struct bss_message *p;
	Packet *p1= new Packet;
	ePacket_ *pkt1=createEvent();
	const char *tmp1 = "flag";
	char *tmp2;
	p = (bss_message *)pkt->DataInfo_;
	msg = (struct detach_type *)p->user_data;
	msg1 = (struct detach_type *)malloc(sizeof(struct detach_type));
	pkt1->DataInfo_ = p1;
	bzero(msg1,sizeof(struct detach_type));
	strcpy(msg1->control,"delete_PDP_r");
	msg1->imsi = msg->imsi;
	msg1->type = msg->type;
	msg1->nsapi = msg->nsapi;
        msg1->qos = msg->qos;
        msg1->ptmsi = msg->ptmsi;
        msg1->ptmsi_sig = msg->ptmsi_sig;
	msg1->tlli = msg->tlli;
	tmp2 = (char *)msg1;
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("GprsGmm::detach()::control=%s imsi=%ld\n",msg1->control,msg1->imsi);
	p1->pkt_addinfo(tmp1,tmp2,sizeof(struct detach_type));
	p1->pkt_setflow(PF_RECV);
	free(msg1);
	uplayer->freeBss_message(p);
	free(pkt);
	//we send detach request message to the SGSN
	return(uplayer->NslObject::recv(pkt1));	
}

int gmm_::attach(ePacket_ *pkt)
{
	//Packet *p;
	Packet *p1= new Packet;
	ePacket_ *pkt1=createEvent();
	struct bss_message *p;
	time_t the_time;
	NslObject *peer;
	int peers;
	ulong tmp_imsi;
	struct HLR_record *H_rec=(HLR_record *)malloc(sizeof(struct HLR_record));
	const char *tmp1 = "flag";
	char *tmp2;
	ms_storage *msg=NULL,*msg2=NULL;
	control_msg *msg1=NULL;
	p = (bss_message *)pkt->DataInfo_;
	msg = (struct ms_storage *)p->user_data;
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("GprsGmm::attach()::Now at gmm_ attach!!\n");
	if(msg->ptmsi != 0 || msg->imsi != 0)
	{
		//this means we have valid ptmsi or valid imsi 
		//now we compare RAI with this sgsn's rai
		if(msg->RAI == uplayer->getRAI() || msg->RAI==0)
		{
			/*
			 *in the original SGSN-->this means we haven't changed the SGSN
			 *and RAI ==0 means that this is the first fime we do attach
			 * and we update the messages in HLR
			*/
			if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("GprsGmm::attach()::at the same SGSN!!\n");
			H_rec = (HLR_record *)malloc(sizeof(struct HLR_record));
			bzero(H_rec,sizeof(HLR_record));
			if(msg->ptmsi != 0)
				H_rec->ptmsi=msg->ptmsi;
			if(msg->imsi != 0)
				H_rec->imsi=msg->imsi;
			H_rec->ra=uplayer->getRAI();
			/*we add ms'ip to HLR which we can use for the downward packet*/
			H_rec->ms_ip = msg->ms_ip; 
			H_rec->tlli = msg->tlli;
			H_rec->nsapi = msg->nsapi;
			H_rec->qos = msg->qos;
			H_rec->msisdn = msg->msisdn;
			tmp_imsi = HLR_.modify_record(H_rec);
			if(tmp_imsi == 0)
			{
				if(DEBUG_LEVEL >= MSG_DEBUG)
					printf("GprsGmm::attach()::modify the HLR record unsuccessfully!!\n");
			}
			the_time = time((time_t *)0);
			msg->ptmsi = (ulong)the_time;
			//printf("new ptmsi=%ld\n",msg->ptmsi);
			//and we need to generate new p-tmsi signature
			the_time = time((time_t *)0);
			msg->ptmsi_sig=(ulong)the_time+1;
			//now add the random ptmsi to HLR
			bzero(H_rec,sizeof(HLR_record));
			//initial_HLR(H_rec);
			H_rec->imsi = msg->imsi;
			H_rec->ptmsi=msg->ptmsi;
			tmp_imsi = HLR_.modify_record(H_rec);
			if(tmp_imsi == 0)
			{
				if(DEBUG_LEVEL >= MSG_DEBUG)
					printf("GprsGmm::attach()::modify the HLR record unsuccessfully!!\n");
			}
			/*
			 * Now we are to update messages in VLR
			*/
			peer = InstanceLookup(uplayer->getRAI(),"GprsSw");
       			peers = ((GprsSw *)peer)->update_VLR(msg);
			/*
			 * we generate new packet and transmit it to SGSN 
			 * to delete the PDP context which is already activated 
			 * before attach
			*/
			pkt1->DataInfo_ = p1;
			msg2 = (ms_storage *)malloc(sizeof(struct ms_storage));
			bzero(msg2,sizeof(struct ms_storage));
			strcpy(msg2->control,"delete_PDP_r");
			msg2->type = msg->type;
			msg2->imsi = msg->imsi;
			msg2->ptmsi = msg->ptmsi;
			msg2->ptmsi_sig = msg->ptmsi_sig;
			msg2->nsapi = msg->nsapi;
       			msg2->qos = msg->qos;
       			/*initial the values we will not use*/
       			msg2->msisdn = msg->msisdn;
       			msg2->tlli = msg->tlli;
       			msg2->ms_ip = msg->ms_ip;
			tmp2 = (char *)msg2;
			p1->pkt_addinfo(tmp1,tmp2,sizeof(struct ms_storage));
			p1->pkt_setflow(PF_RECV);
			free(H_rec);
			free(msg2);
			uplayer->freeBss_message(p);
			free(pkt);
			return(uplayer->NslObject::recv(pkt1));	
				
		}
		else
		{
			//in the new SGSN
			//send Identification Request to original one
			pkt1->DataInfo_ = p1;
			msg1 = (control_msg *)malloc(sizeof(struct control_msg));
			bzero(msg1,sizeof(struct control_msg));
			strcpy(msg1->control,"indi_reqst");
			msg1->TEID = HLR_.getSGSNIP(msg->RAI);
			if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("GprsGmm::attach()::at different SGSN!!\n");
			if(msg->ptmsi != 0)
			{
				msg1->ptmsi = msg->ptmsi;
				msg1->ptmsi_sig = msg->ptmsi_sig;
				msg1->CI = msg->CI;
				/*initial the value*/
				msg1->nsapi = msg->nsapi;
        			msg1->qos = msg->qos;
        			msg1->type = msg->type; 
        			msg1->ms_ip = msg->ms_ip;    
        			msg1->tlli = msg->tlli;
        			msg1->msisdn = msg->msisdn;
        			msg1->imsi = msg->imsi;
        			msg1->TID = 0;
				tmp2 = (char *)msg1;
				p1->pkt_addinfo(tmp1,tmp2,sizeof(struct control_msg));
				p1->pkt_setflow(PF_RECV);
				uplayer->freeBss_message(p);
				free(pkt);
				return(uplayer->NslObject::recv(pkt1));	
			}
			else
			{
				/*this means we use imsi no ptmsi*/
				return(Indi_rep(pkt));
			}
				
		}
				
		
		
	}
	else
	{
		if(DEBUG_LEVEL >= MSG_ERROR)
			printf("GprsGmm::attach()::we don't have valid ptmsi and imsi so we can't do attach!!\n");	
		uplayer->freeBss_message(p);
		free(pkt);
		return(-1);
	}
	
}
sndcp_::sndcp_(GprsGmm *upper)
{
	uplayer = upper;
	
}


/*int sndcp_::LLestablishreq(ushort *nsapi,uchar *qos)
{
	Packet *p = new Packet;
	ePacket_ *pkt = createEvent();
        
	struct control_msg *msg;
	char *tmp1="flag",*tmp2;
	pkt->DataInfo_ = p ;

	msg = (control_msg *)malloc(sizeof(struct control_msg));
	strcpy(msg->control,"llestablishreqstdata");// the LL_unitdata_request control signal which needs
        // response from link layer
        msg->nsapi = *(nsapi);
        
        msg->qos=*(qos);
	//printf("nsapi=%d at LLestablishreq\n",msg->nsapi);
	tmp2 = (char *)msg;
        //add control msg in PT_INFO
	//printf("add the pkt info\n");
        p->pkt_addinfo(tmp1,tmp2,5);
	free(msg);
        //printf("end of addinfo.\n");
	return(uplayer->GprsObject::put(pkt, uplayer->sendtarget_));
	//return( uplayer->NslObject::send(pkt));
	
}

int sndcp_::snsmactivateindi(ushort *nsapi, uchar *qos)
{
	return(LLestablishreq(nsapi,qos));
} 

int sndcp_::snsmmodifyindi(ushort *nsapi, uchar *qos)
{
	
        return(LLestablishreq(nsapi,qos));

}



int sndcp_::snsmdeactivateindi(ushort *nsapi, uchar *qos)
{
	struct control_msg *msg;
	char *tmp1="flag",*tmp2;
	ePacket_ *pkt= createEvent();
	Packet *p= new Packet;
	pkt->DataInfo_=p;

	msg = (control_msg *)malloc(sizeof(struct control_msg));
	strcpy(msg->control,"llreleasereqstlocal");
	//release the nsapi's link        
	msg->nsapi = *(nsapi);
	msg->qos = *(qos);
	tmp2 = (char *)msg;
	 //add control msg in PT_INFO
        p->pkt_addinfo(tmp1,tmp2,4);
	free(msg);
	return(uplayer->GprsObject::put(pkt, uplayer->sendtarget_));
        //return(uplayer->NslObject::send(pkt)); 
}
*/


GprsGmm::~GprsGmm()
{
	MS_status *tmp_status_p,*tmp_status;
	Packet_table *tmp_pkt_p,*tmp_pkt;
	tmp_status_p = ms_state_head;
	tmp_pkt_p = pkt_table_h;
	/*free MS status*/
	while(tmp_status_p != NULL)
	{
		tmp_status = tmp_status_p;
		tmp_status_p = tmp_status_p->next;
		free(tmp_status);
	}
	/*free Packet table*/
	while(tmp_pkt_p != NULL)
	{
		tmp_pkt = tmp_pkt_p;
		tmp_pkt_p = tmp_pkt_p->next;
		free(tmp_pkt);
	}
	free(L_status);
	//free(ms_state);
	//free(pkt_table);
	delete(sndcp1_);
	delete(gmm1_);
	delete(snsm1_);
}
GprsGmm::GprsGmm(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
	: GprsObject(type, id, pl, name)
{
	
	
	/*initial variables added by cmchou*/
	//ms_state=(MS_status *)malloc(sizeof(struct MS_status));
	L_status=(link_status *)malloc(sizeof(struct link_status));
	qos = 1; //set default qos value
 	sndcp1_ = new sndcp_(this);
	gmm1_ = new gmm_(this);
	snsm1_ = new snsm_(this);
	control=0;
	nsapi=0;
	confirm=0;
	indication=0;
	//initial link status
	L_status->qos =0;
 	L_status->nsapi =0;
	L_status->linkstatus = 0;
	L_status->next = NULL;
	head = L_status;
	//initial MS status
	ms_state_head = NULL;
	//initial current state
	c_state =0;
	//initial packet table's contents
        pkt_table_h = NULL;
}

ulong GprsGmm::getRAI()
{
	return(RAI);
}
int GprsGmm::init()
{

	NslObject *peer;
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("This is GprsGmm::init() at node %d\n",get_nid());
	peer = InstanceLookup(get_nid(),0,"GprsSw");
	RAI =  ((GprsSw *)peer)->getSGSNIP();
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("GprsGmm::init::GprsGmm's RAI=%ld\n",RAI);
	return(1);
}
int GprsGmm::Attach_acpt()
{
	if(attempt_count<=4)
	{
		
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		struct bss_message *p1= new bss_message;
        	ePacket_ *pkt1 = createEvent();
        	pkt1->DataInfo_ = p1 ;
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
		bzero(gmmop,sizeof(GmmOption));
        	struct attach_accept *msg1 = (attach_accept *)malloc(sizeof(struct attach_accept));
		bzero(p1,sizeof(struct bss_message));
		bzero(msg1,sizeof(struct attach_accept));
		strcpy(msg1->control,"attach_accept");
        	time_t the_time = time((time_t *)0);
		msg1->ptmsi = (ulong)the_time;
		msg1->ptmsi_sig = (ulong)the_time+1;
		msg1->tlli = (ulong)the_time+2;
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
        	llcop->tlli = msg1->tlli;
		llcop->qos = 1;

        	p1->llc_option = (char *)llcop;
        	p1->llc_option_len = sizeof(struct LLCOption);
		p1->user_data = (char *)msg1;	
        	p1->user_data_len = sizeof(struct attach_accept);
		gmmop->imsi=imsi;
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(struct GmmOption);
		p1->flag = PF_SEND;
        	
        	BASE_OBJTYPE(type);
		u_int64_t wait_interval;
		SEC_TO_TICK(wait_interval, 6);   /* wait 6 sec for response */
		type = POINTER_TO_MEMBER(GprsGmm,Attach_acpt);
		T3350.setCallOutObj(this, type);
		T3350.start(wait_interval, 0);	
		attempt_count++;
		return(GprsObject::put(pkt1, sendtarget_));	
        }
        attempt_count = 0;
	if(DEBUG_LEVEL >= MSG_ERROR)
        	printf("GprsGmm::Attach_acpt::we don't accept attach_complete msg within the defined time!!\n");
	return(-1);
}
/*int GprsGmm::NK_detach(int type1,ulong tlli)
{
	if(attempt_count<=4)
	{
		//char *tmp2,*tmp1="flag";
		//Packet *p1 = new Packet;
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		struct bss_message *p1= new bss_message;
        	ePacket_ *pkt1 = createEvent();
        	pkt1->DataInfo_ = p1 ;
		bzero(p1,sizeof(struct bss_message));
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
		bzero(gmmop,sizeof(GmmOption));
        	NK_detach_type *msg = (NK_detach_type *)malloc(sizeof(struct NK_detach_type));
		bzero(msg,sizeof(struct NK_detach_type));
		strcpy(msg->control,"NK_gprs_detach_req");
		msg->type = type1; //2:network detach 3:reattach type
		msg->tlli = tlli;
		p1->user_data = (char *)msg;
		p1->user_data_len = sizeof(struct NK_detach_type);
		gmmop->imsi = imsi;
		p1->sndcp_option=(char *)gmmop;
		p1->sndcp_option_len = sizeof(struct GmmOption);
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
        	llcop->tlli = tlli;
		llcop->qos = 1;

        	p1->llc_option = (char *)llcop;
        	p1->llc_option_len = sizeof(struct LLCOption);
		p1->flag = PF_SEND;	
		BASE_OBJTYPE(type);
		u_int64_t wait_interval;
		SEC_TO_TICK(wait_interval, 6);   //wait for 6 sec
		if(type1 == 2)
			type = POINTER_TO_MEMBER(GprsGmm,mid_NK_detach);
		else
			type =POINTER_TO_MEMBER(GprsGmm,mid_NK_detach_attach);
		T3322.setCallOutObj(this, type);
		T3322.start(wait_interval, 0);
		return(GprsObject::put(pkt1, sendtarget_));
		//return(NslObject::send(pkt1));
	}
	attempt_count = 0;
	if(DEBUG_LEVEL >= MSG_ERROR)
	{
		printf("GprsGmm::NK_detach()::timer is time out and its attempt_count is over four times!!\n");
	}
	return(-1);
}
int GprsGmm::mid_NK_detach()
{
	struct ip_array *search_result;
	search_result = HLR_.search_record(&imsi);
	return(NK_detach(2,search_result->tlli)); //pure network detach without reattach procedures
}
int GprsGmm::mid_NK_detach_attach()
{
	struct ip_array *search_result;
	search_result = HLR_.search_record(&imsi);
	return(NK_detach(3,search_result->tlli)); //we do attach after the completion of detach
}
*/
int GprsGmm::route_area_update_ap(struct route_msg *rt_msg)
{
	struct route_msg *rt_msg1 = (route_msg *)malloc(sizeof(route_msg));
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	int retmsg;
	ulong tmp_imsi,pass_imsi;
	ePacket_ *pkt1=createEvent();
	struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
	struct bss_message *p1= new bss_message;
	bzero(gmmop,sizeof(GmmOption));
	bzero(rt_msg1,sizeof(route_msg));
	HLR_record *H_rec = (HLR_record *)malloc(sizeof(struct HLR_record));;
	NslObject *peer;
	//struct route_msg *rt_msg;
	pkt1->DataInfo_ = p1;
	//rt_msg = (struct route_msg *)p->pkt_getinfo("flag");
	//gmm1_->initial_HLR(H_rec);
	bzero(p1,sizeof(struct bss_message));
	bzero(H_rec,sizeof(HLR_record));
	//H_rec->sgsnip=HLR_.getSGSNIP(rt_msg->imsi);
	H_rec->sgsnip = rt_msg->RAI;
	H_rec->imsi=rt_msg->imsi;
	H_rec->ca=rt_msg->CI;
	H_rec->ra=rt_msg->RAI;
	/*the new SGSN informs the HLR of the change of SGSN by sending Update Location to HLR*/
	tmp_imsi = HLR_.modify_record(H_rec);
	if(tmp_imsi == 0)
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::route_area_update_ap()::::modify the HLR record unsuccessfully!!\n");
	}
	time_t the_time = time((time_t *)0);
	rt_msg->ptmsi = (ulong)the_time;
	rt_msg->ptmsi_sig = (ulong)the_time + 1;
	//Now we are to modify new VLR
	peer =InstanceLookup(RAI,"GprsSw");
        retmsg=((GprsSw *)peer)->Location_Update_req(rt_msg);
              
	//Now we are to delete old record in old VLR
	pass_imsi = rt_msg->imsi;
	peer =InstanceLookup(rt_msg->TEID,"GprsSw");
	retmsg=((GprsSw *)peer)->Delete_old_VLR_rd(pass_imsi);
        if(retmsg < 0)
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
        		printf("GprsGmm::route_area_update_ap()::delete data fail!!\n");
	}
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = rt_msg->tlli;
	llcop->qos = 1;

        p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);
	memcpy(rt_msg1,rt_msg,sizeof(struct route_msg));
	p1->user_data = (char *)rt_msg1;
	p1->user_data_len = sizeof(struct route_msg);
	gmmop->imsi = rt_msg->imsi;
	p1->sndcp_option = (char *)gmmop;
	p1->sndcp_option_len = sizeof(struct GmmOption);
	p1->flag = PF_SEND;
	free(H_rec);
	return(GprsObject::put(pkt1, sendtarget_));
	//return(NslObject::send(pkt1));	
}
int GprsGmm::paging_rp(struct Paging_resp *P_resp)
{
	int found=0;
	struct Packet_table *temp_table;
	temp_table = pkt_table_h;
	//struct Paging_resp *P_resp;
	struct MS_status *temp_ms;
	temp_ms = ms_state_head;
	while(temp_ms != NULL)
	{
		if(temp_ms->imsi == P_resp->imsi)
		{
			temp_ms->status = 2;
			found = 1;
			break;
		}	
		temp_ms = temp_ms->next;
	}
	if(found ==0)
	{
		if(DEBUG_LEVEL >= MSG_ERROR)
		printf("GprsGmm::paging_rp()::we didn't find ms's status !!\n");
		return(-1);
	}
	while(temp_table!=NULL)
	{
		if(temp_table->imsi == P_resp->imsi)
		{
			
			struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
			struct bss_message *p1= new bss_message;
			struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
			bzero(gmmop,sizeof(GmmOption));
			bzero(p1,sizeof(struct bss_message));
			//char *pure_data;
       	 		ePacket_ *pkt1=createEvent();
       	 		//ip     *ip_header;
			//tcphdr *tcp_header;
			Packet	*pt;
			pt = (Packet *)temp_table->pkt;
			//pure_data = (char *)(pt->pkt_sget() + sizeof(struct ip)+sizeof(struct tcphdr));	
       	 		pkt1->DataInfo_ = p1;
       	 		p1->packet = temp_table->pkt;
       	 		p1->user_data = (char *)pt->pkt_sget();
       	 		p1->user_data_len = pt->pkt_getlen();
			gmmop->imsi = P_resp->imsi;
			bzero(llcop,sizeof(LLCOption));
			strcpy(llcop->control,"LLGMM_TRIGGER");
        		llcop->tlli = P_resp->tlli;
			llcop->qos = 1;

        		p1->llc_option = (char *)llcop;
        		p1->llc_option_len = sizeof(struct LLCOption);
			p1->sndcp_option = (char *)gmmop;
			p1->sndcp_option_len = sizeof(struct GmmOption);
			p1->flag = PF_SEND;
			return(GprsObject::put(pkt1, sendtarget_));
		}
		temp_table = temp_table->next;
	}
	if(DEBUG_LEVEL >= MSG_ERROR)
		printf("GprsGmm::paging_rp()::failure of retriving pakcet from download buffer!!\n");
	return(-1);
}

int GprsGmm::set_in_standby(struct in_standby *standby)
{
	int found=0;
	struct MS_status *temp_ms,*temp_ms1;
	temp_ms = ms_state_head;
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("GprsGmm::set_in_standby()!!\n");
	/*Now we store ms's status to the table */
        while(temp_ms!= NULL)
        {
        	if(temp_ms->imsi == standby->imsi)
        	{
        		//if we indeed find it in MS's status table,
        		//we change its status
			if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("***GprsGmm::set_in_standby()::we have found record and set the standby state!!***\n");
        		temp_ms->status = 1;//standby state;
        		found = 1;
        	}
        	temp_ms = temp_ms->next;
        }
        if(found==0)
        {
        	//this means that we don't find any record in MS's status table
        	//and now we add it to tail of it
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("***GprsGmm::set_in_standby()::we don't find record and add the standby record!!***\n");
		temp_ms1 = (MS_status *)malloc(sizeof(struct MS_status));
		bzero(temp_ms1,sizeof(MS_status));
        	temp_ms1->imsi = standby->imsi;
        	temp_ms1->status = 1;//standby state
		temp_ms1->next = ms_state_head;
		ms_state_head = temp_ms1;
        }
        else
        	found=0;
	return(1);
}
int GprsGmm::intra_area_update_req(struct intra_route_msg *intra_rt_msg)
{
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
	struct HLR_record *H_rec = (HLR_record *)malloc(sizeof(struct HLR_record));
	struct bss_message *p1= new bss_message;
	struct route_accept *rt_msg;
	struct route_msg *route_m = (route_msg *)malloc(sizeof(struct route_msg));

    int echomsg;
	NslObject *peer;
	ulong	tmp_imsi;
	ePacket_ *pkt1=createEvent();
	bzero(gmmop,sizeof(GmmOption));

    //char *tmp1="flag",*tmp2="";
	pkt1->DataInfo_ = p1;
	bzero(route_m,sizeof(struct route_msg));
	bzero(p1,sizeof(struct bss_message));

    rt_msg = (route_accept *)malloc(sizeof(struct route_accept));
	bzero(rt_msg,sizeof(struct route_accept));

    //gmm1_->initial_HLR(H_rec);
	bzero(H_rec,sizeof(HLR_record));
	H_rec->imsi=intra_rt_msg->imsi;
	H_rec->ca=intra_rt_msg->CI;
	tmp_imsi = HLR_.modify_record(H_rec);

    if(tmp_imsi == 0) {

        if(DEBUG_LEVEL >= MSG_DEBUG)
            printf("modify the HLR record unsuccessfully!!\n");

    }

    time_t the_time = time((time_t *)0);
	rt_msg->ptmsi = (ulong)the_time;
	rt_msg->ptmsi_sig = (ulong)the_time+1;
	intra_rt_msg->ptmsi = rt_msg->ptmsi;
	intra_rt_msg->ptmsi_sig = rt_msg->ptmsi_sig;

	//relay packet to VLR to change its old Cell Id to new Cell Id
	//fill in the messages which we want to use to update VLR
	route_m->imsi = intra_rt_msg->imsi;
	route_m->ptmsi = intra_rt_msg->ptmsi;
	route_m->ptmsi_sig = intra_rt_msg->ptmsi_sig;
	route_m->CI = intra_rt_msg->CI;
	route_m->RAI = 0;
	//route_m->port_id = get_port();
	peer = InstanceLookup(RAI,"GprsSw");
        echomsg = ((GprsSw*)peer)->Location_Update_req(route_m);
	free(route_m);

	strcpy(rt_msg->control,"route_area_update_accept");
	rt_msg->imsi = intra_rt_msg->imsi;
	rt_msg->tlli = intra_rt_msg->tlli;
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = intra_rt_msg->tlli;
	llcop->qos = 1;

    p1->llc_option = (char *)llcop;
    p1->llc_option_len = sizeof(LLCOption);
	p1->user_data = (char *)rt_msg;
	p1->user_data_len = sizeof(struct route_accept);
	gmmop->imsi = intra_rt_msg->imsi;
	p1->sndcp_option = (char *)gmmop;
	p1->sndcp_option_len = sizeof(struct GmmOption);
	p1->flag = PF_SEND;
	free(H_rec);
	return(GprsObject::put(pkt1, sendtarget_));
}
int GprsGmm::attach_complete(struct attach_accept *msg1)
{
	//stop the attach_complete timer
	struct MS_status *temp_ms=NULL,*temp_ms1=NULL;
	int found=0;
	T3350.cancel();
	c_state = 2; //GMM-REGISTERED state
	attempt_count = 0;
	/*Now we store ms's status to the table */
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("GprsGmm::attach complete!!\n");
	temp_ms = ms_state_head;
        while(temp_ms!= NULL)
        {
        	if(temp_ms->imsi == msg1->imsi)
        	{
        		//if we indeed find it in MS's status table,
        		//we change its status
        		temp_ms->status = 2;//ready state;
        		found = 1;
        	}
        	temp_ms = temp_ms->next;
        }
        if(found==0)
        {
        	//this means that we don't find any record in MS's status table
        	//and now we add it to tail of it
		temp_ms1 = (MS_status *)malloc(sizeof(struct MS_status));
        	bzero(temp_ms1,sizeof(MS_status));
		temp_ms1->imsi = msg1->imsi;
        	temp_ms1->status = 2;//ready state
		temp_ms1->next = ms_state_head;
		ms_state_head = temp_ms1;
        }
	return(1);
}
/*
int GprsGmm::NK_detach_acpt(struct detach_accept *dtch_accept)
{
	int found=0;
	struct MS_status *temp_ms,*temp_ms1;
	//stop the Network detach request
	T3322.cancel();
	attempt_count = 0;
	c_state = 0;
	temp_ms = ms_state_head;
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("GprsGmm::NK_detach_acpt()::Network detach accepted!!\n");
	while(temp_ms!= NULL)
        {
        	if(temp_ms->imsi == dtch_accept->imsi)
        	{
        		//if we indeed find it in MS's status table,
        		//we change its status
        		temp_ms->status = 0;//idle state;
        		found = 1;
        	}
        	temp_ms = temp_ms->next;
        }
        if(found==0)
        {
        	//this means that we don't find any record in MS's status table
        	//and now we add it to tail of it
		temp_ms1 = (MS_status *)malloc(sizeof(struct MS_status));
        	temp_ms1->imsi = dtch_accept->imsi;
        	temp_ms1->status = 0;//idle state
		temp_ms1->next = ms_state_head;
		ms_state_head = temp_ms1;
        }
	return(1);
}
*/
int GprsGmm::send(ePacket_ *pkt) 
{
	
	Packet *p;
	struct ms_storage *msg;
	assert(pkt&&pkt->DataInfo_);
	p = (Packet *)pkt->DataInfo_;
	msg = (struct ms_storage *)p->pkt_getinfo("flag");
	
	if(strcmp(msg->control,"route_area_update_accept")==0)
	{
		//because of successful request, we are to 
		//inform the HLR of the change of SGSN
		int echomsg;
		route_msg *msg1 = (struct route_msg *)p->pkt_getinfo("flag");
		echomsg = route_area_update_ap(msg1);
		freePacket(pkt);
		return(echomsg);
	}
	else if(strcmp(msg->control,"iden_req")==0)
	{
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		ePacket_ *pkt1 = createEvent();
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
		struct bss_message *p1= new bss_message;
		bzero(p1,sizeof(bss_message));
		bzero(gmmop,sizeof(GmmOption));
		upper_msg *uper_msg = (upper_msg *)malloc(sizeof(struct upper_msg));
		memcpy(uper_msg->control,msg->control,sizeof(msg->control));
		//uper_msg->control = msg->control;
		pkt1->DataInfo_ = p1 ;
		p1->user_data = (char *)uper_msg;
		p1->user_data_len = sizeof(struct upper_msg);
		gmmop->imsi = msg->imsi;
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(struct GmmOption);
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
        	llcop->tlli = msg->tlli;
		llcop->qos = 1;

        	p1->llc_option = (char *)llcop;
        	p1->llc_option_len = sizeof(struct LLCOption);
		p1->flag = PF_SEND;
		freePacket(pkt);
		return(GprsObject::put(pkt1, sendtarget_));
		//return(NslObject::send(pkt1));
	}
	else if(strcmp(msg->control,"update_PDP_r")==0)
	{
		/*SGSN-Initiated PDP Context Modification Procedure*/
		qos = msg->qos;
		nsapi = msg->nsapi;
		const char *control_msg = "sgsn_update_PDP_r";
		freePacket(pkt);
		return(gmm1_->send_PDP_req(qos,nsapi,msg->imsi,msg->tlli,control_msg));
		
	}
	else if(!strcmp(msg->control,"update_PDP_r_s"))
	{
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
                ePacket_ *pkt1 = createEvent();
                struct bss_message *p1= new bss_message;
                pkt1->DataInfo_ = p1 ;
                bzero(p1,sizeof(struct bss_message));
                struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
                struct activate_type *update_msg = (activate_type *)malloc(sizeof(struct activate_type));
                bzero(update_msg,sizeof(struct activate_type));
                bzero(gmmop,sizeof(GmmOption));
                memcpy(update_msg->control,msg->control,sizeof(msg->control));
                update_msg->nsapi = msg->nsapi;
		update_msg->qos = msg->qos;
		bzero(llcop,sizeof(LLCOption));
                strcpy(llcop->control,"LLGMM_TRIGGER");
                llcop->tlli = msg->tlli;
		llcop->qos = 1;

                p1->llc_option = (char *)llcop;
                p1->llc_option_len = sizeof(struct LLCOption);
                p1->user_data = (char *)update_msg;
                p1->user_data_len = sizeof(struct activate_type);
                gmmop->imsi = msg->imsi;
                p1->sndcp_option = (char *)gmmop;
                p1->sndcp_option_len = sizeof(GmmOption);
                p1->flag = PF_SEND;
                freePacket(pkt);
                return(GprsObject::put(pkt1, sendtarget_));

		
	}
	else if(!strcmp(msg->control,"activate_PDP_accept") || !strcmp(msg->control,"activate_accept_qos_n")
		|| !strcmp(msg->control,"update_PDP_r_f")|| !strcmp(msg->control,"deactivate_PDP_r_s")
		|| !strcmp(msg->control,"activate_error_ip") || !strcmp(msg->control,"sgsn_context_r_f"))
	{
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		ePacket_ *pkt1 = createEvent();
		struct bss_message *p1= new bss_message;
		pkt1->DataInfo_ = p1 ;
		bzero(p1,sizeof(struct bss_message));
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
		struct return_msg *rt_msg = (return_msg *)malloc(sizeof(struct return_msg));
		bzero(rt_msg,sizeof(struct return_msg));
		bzero(gmmop,sizeof(GmmOption));
		memcpy(rt_msg->control,msg->control,sizeof(msg->control));
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
        	llcop->tlli = msg->tlli;
		llcop->qos = 1;

		p1->llc_option = (char *)llcop;
        	p1->llc_option_len = sizeof(struct LLCOption);
		p1->user_data = (char *)rt_msg;
		p1->user_data_len = sizeof(struct return_msg);
		gmmop->imsi = msg->imsi;
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(GmmOption);
		p1->flag = PF_SEND;
		freePacket(pkt);
		return(GprsObject::put(pkt1, sendtarget_));
	}
	else if(strcmp(msg->control,"sgsn_update_PDP_r_f")==0)
	{
		if(DEBUG_LEVEL >= MSG_ERROR)
		printf("SGSN-initiated PDP context fail!!\n");
		freePacket(pkt);
		return(1);
	}
	else if(strcmp(msg->control,"sgsn_update_PDP_r_s")==0)
	{
		qos = msg->qos;
		nsapi = msg->nsapi;
		const char *control_msg = "modify_PDP_r";
		freePacket(pkt);
		return(gmm1_->send_PDP_req(qos,nsapi,msg->imsi,msg->tlli,control_msg));
	}
	/*else if(!strcmp(msg->control ,"NK_gprs_detach") || !strcmp(msg->control,"NK_gprs_detach_attach"))
	{
		
		imsi = msg->imsi;
		if(!strcmp(msg->control,"NK_gprs_detach"))
			return(mid_NK_detach());
		else
			return(mid_NK_detach_attach());
		
	}*/
	else if(strcmp(msg->control ,"update_location_resp")==0)
	{
		return(gmm1_->update_location_rep(pkt));
	}
	else if(strcmp(msg->control ,"indi_resp")==0)
	{
		return(gmm1_->Indi_rep(pkt));
		
		
	}	
	else if(!strcmp(msg->control , "delete_PDP_rp_s") && msg->type==0)	
	{
		
		/*
		we send attach accept request to MS and wait for its attach complete msg.
		at this time, we set T3350 timer and when it expires, 
		we retransmit the request until four times
		*/
		imsi = msg->imsi;
		attempt_count++;
		ulong tmp_imsi;
		int ifexist = 1;
		struct HLR_record *H_rec = (HLR_record *)malloc(sizeof(struct HLR_record));
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		BASE_OBJTYPE(type);
		u_int64_t wait_interval;
		SEC_TO_TICK(wait_interval, 6);   /* wait 6 sec for response */
		type = POINTER_TO_MEMBER(GprsGmm,Attach_acpt);
		T3350.setCallOutObj(this, type);
		T3350.start(wait_interval, 0);
		//the max value of attempt_count is four and when it over four,we stop it
		//0:attach
		//generate new packet and transmit it to the MS
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::Delete PDP Context successfully because of records in SGSN or not in!!\n");
		struct bss_message *p1= new bss_message;
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
        	ePacket_ *pkt1 = createEvent();
        	pkt1->DataInfo_ = p1 ;
		bzero(gmmop,sizeof(GmmOption));
		bzero(p1,sizeof(struct bss_message));
        	struct attach_accept *msg1 = (attach_accept *)malloc(sizeof(struct attach_accept));
		bzero(msg1,sizeof(struct attach_accept));
		strcpy(msg1->control,"attach_accept");
        	time_t the_time = time((time_t *)0);
		msg1->ptmsi = (ulong)the_time;
		msg1->ptmsi_sig = (ulong)the_time+1;
		msg1->RAI = RAI;
		strcpy(llcop->control,"LLGMM_ASSIGN");
		/*generate local TLLI*/
		while(ifexist)
		{
			llcop->tlli = rand() | 0x00000003;
			/*search if the new tlli conflicted with the tllis which have already been generated before*/
			ifexist = HLR_.see_if_exist(llcop->tlli);
		}
		llcop->oldtlli = msg->tlli;
		llcop->qos = 1;

		msg1->tlli = llcop->tlli;
		/*modify tlli and ptmsi record in HLR*/
		bzero(H_rec,sizeof(HLR_record));
		H_rec->imsi = msg->imsi;
		H_rec->tlli = llcop->tlli;
        	H_rec->ptmsi=msg1->ptmsi;
        	tmp_imsi = HLR_.modify_record(H_rec);
		free(H_rec);
		p1->user_data = (char *)msg1;	
		p1->user_data_len = sizeof(struct attach_accept);
		gmmop->imsi = msg->imsi;
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(GmmOption);
		p1->llc_option = (char *)llcop;
		p1->llc_option_len = sizeof(struct LLCOption);
		p1->flag = PF_SEND;
		freePacket(pkt);
		return(GprsObject::put(pkt1, sendtarget_));
        	
		
	}
	/*else if(!strcmp(msg->control,"update_location_resp_f"))	
	{
		//0:attach
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		struct bss_message *p1= new bss_message;
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
        	ePacket_ *pkt1 = createEvent();
        	struct return_msg *msg1;
		bzero(p1,sizeof(struct bss_message));
        	bzero(gmmop,sizeof(GmmOption));
		pkt1->DataInfo_ = p1 ;
        	msg1 = (return_msg *)malloc(sizeof(struct return_msg));
		bzero(msg1,sizeof(struct return_msg));
		strcpy(msg1->control,"attach_not_accept");
		p1->user_data = (char *)msg1;	
        	p1->user_data_len = sizeof(struct return_msg);
		gmmop->imsi = msg->imsi;
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(GmmOption);
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGER");
        	llcop->tlli = msg->tlli;
		llcop->qos = 1;

        	p1->llc_option = (char *)llcop;
        	p1->llc_option_len = sizeof(struct LLCOption);
		p1->flag = PF_SEND;
        	freePacket(pkt);
		return(GprsObject::put(pkt1, sendtarget_));
	}*/
	else if(!strcmp(msg->control ,"delete_PDP_rp_s") && msg->type==1)	
	{
		//1:detach
		/*Before we transmit it to the MS , we transmit it to the VLR
		 *to tell the VLR we want to do GPRS Detach...  
		*/
		int found=0;
		int retmsg=0;
		ulong	pass_imsi=0;
		struct detach_type *msg1=NULL;
		struct return_msg *msg2=NULL;
		NslObject *peer;
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		msg1 = (struct detach_type *)p->pkt_getinfo("flag");
		pass_imsi = msg1->imsi;
		peer = InstanceLookup(RAI,"GprsSw");	
		retmsg = ((GprsSw*)peer)->Delete_old_VLR_rd(pass_imsi);
		
		if(retmsg < 0)
		{
			if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("GprsGmm::modifing the VLR record unsuccessfully!!\n");
		}
		//generate new packet and transmit it to the MS
		//printf("Delete PDP Context successfully!!\n");
		struct MS_status *temp_ms,*temp_ms1;
		struct bss_message *p1= new bss_message;
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
        	ePacket_ *pkt1 = createEvent();
        	pkt1->DataInfo_ = p1 ;
        	msg2 = (return_msg *)malloc(sizeof(struct return_msg));
		bzero(p1,sizeof(struct bss_message));
		bzero(msg2,sizeof(struct return_msg));
		bzero(gmmop,sizeof(GmmOption));
		strcpy(msg2->control,"detach_accept");
        	c_state = 0;//0:GMM-DEREGISTERED state
		gmmop->imsi = msg1->imsi;
		bzero(llcop,sizeof(LLCOption));
                strcpy(llcop->control,"LLGMM_TRIGGER");
                llcop->tlli = msg1->tlli;
		llcop->qos = 1;

		/*this llcop1 is for release Downlink detach*/
		struct LLCOption *llcop1 = (LLCOption *)malloc(sizeof(LLCOption));
                bzero(llcop1,sizeof(struct LLCOption));
                strcpy(llcop1->control,"LLGMM_RESET");
                llcop1->oldtlli = 0xffffffff;
                llcop1->tlli = msg1->tlli;

		p1->llc_option = (char *)llcop;
                p1->llc_option_len = sizeof(struct LLCOption);
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(GmmOption);
		p1->flag = PF_SEND;
		p1->user_data = (char *)msg2;	
        	p1->user_data_len = sizeof(struct return_msg);
        	temp_ms = ms_state_head;
        	while(temp_ms!= NULL)
        	{
        		if(temp_ms->imsi == msg1->imsi)
        		{
        			//if we indeed find it in MS's status table,
        			//we change its status
        			temp_ms->status = 0;//idle state;
        			found = 1;
				break;
        		}
        		temp_ms = temp_ms->next;
        	}
        	if(found==0)
        	{
        		//this means that we don't find any record in MS's status table
        		//and now we add it to tail of it
			temp_ms1 = (MS_status *)malloc(sizeof(struct MS_status));
        		bzero(temp_ms1,sizeof(struct MS_status));
			temp_ms1->imsi = msg1->imsi;
        		temp_ms1->status = 0;//idle state
			temp_ms1->next = ms_state_head;
			ms_state_head = temp_ms1;
        	}
        	else
        		found=0;	
        	freePacket(pkt);
		GprsObject::put(pkt1, sendtarget_);
		
		struct bss_message *p2= new bss_message;
		ePacket_ *pkt2 = createEvent();
                pkt2->DataInfo_ = p2 ;
                bzero(p2,sizeof(struct bss_message));
		p2->llc_option = (char *)llcop1;
                p2->llc_option_len = sizeof(struct LLCOption);
		p2->flag = PF_SEND;
		return(GprsObject::put(pkt2, sendtarget_));
		
		
	}
	else if((!strcmp(msg->control , "delete_PDP_rp_s") && msg->type==2) || (!strcmp(msg->control , "delete_PDP_rp_s") && msg->type==3))	
	{
		//2:Network detach
		//generate new packet and transmit it to the MS
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::Delete PDP Context successfully!!\n");
		struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
		struct bss_message *p1= new bss_message;
		struct NK_detach_type *nk_detach;
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
        	ePacket_ *pkt1 = createEvent();
        	pkt1->DataInfo_ = p1 ;
		bzero(p1,sizeof(struct bss_message));
		bzero(gmmop,sizeof(GmmOption));
        	nk_detach = (NK_detach_type *)malloc(sizeof(struct NK_detach_type));
		bzero(nk_detach,sizeof(struct NK_detach_type));
		strcpy( nk_detach->control,"NK_detach_accept");
        	nk_detach->type = msg->type; 
        	nk_detach->ptmsi = msg->ptmsi;
        	nk_detach->ptmsi_sig = msg->ptmsi_sig;
		nk_detach->tlli = msg->tlli;
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
                llcop->tlli = msg->tlli;
		llcop->qos = 1;

                p1->llc_option = (char *)llcop;
                p1->llc_option_len = sizeof(struct LLCOption);
        	c_state = 0;//0:GMM-DEREGISTERED state
		p1->user_data = (char *)nk_detach;	
		p1->user_data_len = sizeof(struct NK_detach_type);
		gmmop->imsi = msg->imsi;
		p1->sndcp_option = (char *)gmmop;
		p1->sndcp_option_len = sizeof(GmmOption);
        	p1->flag = PF_SEND;
        	freePacket(pkt);
		return(GprsObject::put(pkt1, sendtarget_));
        	//return( NslObject::send(pkt1));
		
	}
	else
	{
		//before we send packet to the MS, 
		//we must see if it is in ready state
		//if not(in standby state) we send paging request
		int found=0;
		struct ip_array *search_result;
        	struct MS_status *temp_status;
        	temp_status = ms_state_head;
		search_result = HLR_.search_record(&msg->imsi);
		while(temp_status != NULL)
        	{
        		if(temp_status->imsi == search_result->imsi)
        		{
        			if(temp_status->status==1)
        			{
        				found = 1;
					break;
        			}
				if(temp_status->status==0)
				{
					found = 2;
					break;
				}
        		}
        		temp_status = temp_status->next;
        	}
        	if(found==1)
        	{
			
			struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
			struct bss_message *p1= new bss_message;
			struct Packet_table *tmp_pkt;
			struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
        		struct Paging_ps *msg1;
			msg1 = (Paging_ps *)malloc(sizeof(struct Paging_ps));
			bzero(gmmop,sizeof(GmmOption));
			bzero(msg1,sizeof(struct Paging_ps));
			/*put the packet into pakcet buffer and retrive and send it 
        		  after we get paging response
        		*/
			tmp_pkt = (Packet_table *)malloc(sizeof(struct Packet_table));
			bzero(tmp_pkt,sizeof(Packet_table));
        		tmp_pkt->pkt = (Packet *)pkt->DataInfo_;
        		tmp_pkt->imsi = msg->imsi;
			tmp_pkt->next = pkt_table_h;
			pkt_table_h = tmp_pkt;

			pkt->DataInfo_ = (bss_message *)p1;
			strcpy(msg1->control,"paging_req");
			msg1->type = 6;
			msg1->imsi = msg->imsi;
			msg1->RAI = search_result->ra;
			msg1->qos = search_result->qos;
			msg1->ptmsi = search_result->ptmsi;
			msg1->tlli = search_result->tlli;
			bzero(p1,sizeof(struct bss_message));
			p1->user_data = (char *)msg1;	
			p1->user_data_len = sizeof(struct Paging_ps);
			gmmop->imsi = msg->imsi;
			bzero(llcop,sizeof(LLCOption));
			strcpy(llcop->control,"LLGMM_TRIGGER");
        		llcop->tlli = search_result->tlli;
			llcop->qos = 1;

        		p1->llc_option = (char *)llcop;
       			p1->llc_option_len = sizeof(struct LLCOption);
			p1->sndcp_option = (char *)gmmop;
			p1->sndcp_option_len = sizeof(GmmOption);
			p1->flag = PF_SEND;
			if(DEBUG_LEVEL >= MSG_DEBUG)
			{
			printf("***GprsGmm::we have found that ms is not in ready state!!***\n");
			printf("***GprsGmm::we are to send packet to MS to awake it!!***\n");
        		}
			/*set timer(T3313) which is network-dependent to MS */
			return(GprsObject::sendManage(pkt));
			//return(GprsObject::put(pkt, sendtarget_));	
        	}
        	else if(found==0)
		{
			/*this means ms is in ready state and we can send packet to it*/
			if(DEBUG_LEVEL >= MSG_DEBUG)
				printf("GprsGmm::we are to send pure packet from outer network to MS side!!\n");
			if (!pkt->DataInfo_) return -1;
			struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
			Packet *p;
			struct GmmOption *gmmop = (GmmOption *)malloc(sizeof(struct GmmOption));
			bzero(gmmop,sizeof(GmmOption));
			bss_message *p1= (bss_message*)malloc(sizeof (bss_message));
			bzero(p1,sizeof(bss_message));
			p = (Packet *)pkt->DataInfo_;
        		pkt->DataInfo_ = p1;
			p1->user_data = (char *)p->pkt_sget();
        		p1->user_data_len = p->pkt_getlen();
			gmmop->imsi = msg->imsi;
			bzero(llcop,sizeof(LLCOption));
			strcpy(llcop->control,"LLGMM_TRIGGER");
        		llcop->tlli = search_result->tlli;
			llcop->qos = 3;

        		p1->llc_option = (char *)llcop;
        		p1->llc_option_len = sizeof(struct LLCOption);
			p1->sndcp_option = (char *)gmmop;
			p1->sndcp_option_len = sizeof(GmmOption);
        		p1->packet = p;
			p1->flag = PF_SEND;
			return(GprsObject::sendManage(pkt));

		}
		/*MS is not attach yet so we drop this packet!!*/
		freePacket(pkt);
		return(1);
	}
	return 1;
}

int GprsGmm::recv(ePacket_ *pkt) 
{ 	
	
	struct bss_message *p;
	//struct link_status *path,*prev,*next,*current;
	struct LLC_msg *msg=NULL;                             
        assert(pkt&&pkt->DataInfo_);
	p = (bss_message *)pkt->DataInfo_;
	msg = (struct LLC_msg *)p->user_data;
	//path = head;
	//prev = head;
	
	
	if(!strcmp(msg->control , "iden_resp"))
	{
		return(gmm1_->attach(pkt));
	}
	else if(!strcmp(msg->control,"paging_resp"))
	{
		/*we are to retrive packet in the buffer and send it to MS
		  and we need to change MS's state at the same time
		*/
		int echomsg;
		struct Paging_resp *P_resp=NULL;
		P_resp = (struct Paging_resp *)p->user_data;
		echomsg = paging_rp(P_resp);
		freeBss_message(p);
		free(pkt);
		return(echomsg);
		
	}
	else if(!strcmp(msg->control,"route_area_update_complete"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::Routing area update is completed!!\n");
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control,"modify_PDP_accept"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::Modify PDP ocntext request accepted!!\n");
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control,"in_standby_state"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::we are to change state to standby state!!\n");
		struct in_standby *standby=NULL;
		standby = (struct in_standby *) p->user_data;
		set_in_standby(standby);
		freeBss_message(p);
		free(pkt);
		return(1);

	}
	else if(!strcmp(msg->control,"deactivate_PDP_r"))
	{
		struct deactivate_type *deac_type=NULL,*deac_type1=NULL;
		Packet *p1= new Packet;
		ePacket_ *pkt1 = createEvent();
		const char *tmp1="flag";
		char *tmp2;
		pkt1->DataInfo_ = p1;
		deac_type1 = (deactivate_type *)malloc(sizeof(struct deactivate_type));
		bzero(deac_type1,sizeof(struct deactivate_type));
		deac_type = (deactivate_type *)p->user_data;
		strcpy(deac_type1->control,"deactivate_PDP_r");
		deac_type1->nsapi = deac_type->nsapi;
		deac_type1->qos = deac_type->qos;
		deac_type1->imsi = deac_type->imsi;
		deac_type1->tlli = deac_type->tlli;
		tmp2 = (char *)deac_type1;
		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct deactivate_type));
		p1->pkt_setflow(PF_RECV);
		free(deac_type1);
		freeBss_message(p);
		free(pkt);
		return(NslObject::recv(pkt1));

	}
	else if(!strcmp(msg->control,"update_PDP_r"))
	{
		struct activate_type *update_t=NULL,*update_t1=NULL;
		Packet *p1= new Packet;
		ePacket_ *pkt1=createEvent();
		const char *tmp1="flag";
		char *tmp2;
		pkt1->DataInfo_ = p1;
		update_t1 = (activate_type *)malloc(sizeof(struct activate_type));
		bzero(update_t1,sizeof(struct activate_type));
		update_t = (activate_type *)p->user_data;
		memcpy(update_t1,update_t,sizeof(activate_type));
		strcpy(update_t1->control,"update_PDP_r");
		tmp2 = (char *)update_t1;
		if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("GprsGmm::update_PDP_r::update control=%s nsapi=%d qos=%d\n",update_t1->control,update_t1->nsapi,update_t1->qos);
		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
		p1->pkt_setflow(PF_RECV);
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::PID::%llu\n",p1->pkt_getpid());
		free(update_t1);
		freeBss_message(p);
		free(pkt);
		//return(put(pkt1, recvtarget_));
		return(NslObject::recv(pkt1));

	}
	//else if(!strcmp(msg->control,"NK_activate_PDP_r") || !strcmp(msg->control ,"activate_PDP_r"))
	else if(!strcmp(msg->control ,"activate_PDP_r"))
	{
		//struct ms_storage *ms_s,*ms_s1;
		struct activate_type *act_type=NULL,*act_type1=NULL;
		Packet *p1= new Packet;
		ePacket_ *pkt1=createEvent();
		const char *tmp1="flag";
		char *tmp2;
		pkt1->DataInfo_=p1;
		act_type1 = (activate_type *)malloc(sizeof(struct activate_type));
		bzero(act_type1,sizeof(struct activate_type));
		pkt1->DataInfo_ = p1;
		act_type = (activate_type *)p->user_data;
		if(!strcmp(msg->control,"NK_activate_PDP_r"))
			strcpy(act_type1->control,"NK_create_PDP_r");
		else
			strcpy(act_type1->control,"create_PDP_r");
		act_type1->nsapi = act_type->nsapi;
		act_type1->qos = act_type->qos;
		act_type1->imsi = act_type->imsi;
		act_type1->msisdn = act_type->msisdn;
		act_type1->tlli = act_type->tlli;
		act_type1->ms_ip = act_type->ms_ip;
		tmp2 = (char *)act_type1;
		//when we call attach , we change status to ready
		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
		p1->pkt_setflow(PF_RECV);
		free(act_type1);
		freeBss_message(p);
		free(pkt);
		//return(put(pkt1, recvtarget_));
		return(NslObject::recv(pkt1));
	}
	else if(!strcmp(msg->control , "intra_route_area_update_rq"))
	{
		int echomsg=0;
		struct intra_route_msg *intra_rt_msg=NULL;
		intra_rt_msg = (struct intra_route_msg *)p->user_data;
		echomsg = intra_area_update_req(intra_rt_msg);
		freeBss_message(p);
		free(pkt);
		return(echomsg);
	}
	else if(!strcmp(msg->control ,"inter_route_area_update_rq"))
	{
		return(gmm1_->SGSN_context_req(pkt));
	}
	else if(!strcmp(msg->control, "gprs_imsi_attach"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::we accept a attach request at Sgsn's Gmm side!!\n");
		return(gmm1_->attach(pkt));

	}
	else if(!strcmp(msg->control , "attach_complete"))
	{
		struct attach_accept *msg1;
		msg1 = (struct attach_accept *)p->user_data;
		attach_complete(msg1);
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	/*else if(!strcmp(msg->control , "NK_detach_accept"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("GprsGmm::NK_detach accept!!\n");
		struct detach_accept *dtch_accept=NULL;
		dtch_accept = (struct detach_accept *)p->user_data;
		NK_detach_acpt(dtch_accept);
		freeBss_message(p);
		free(pkt);
		return(1);
	}*/
	else if(!strcmp(msg->control,"MS_gprs_detach"))
	{
		return(gmm1_->detach(pkt));
	}
	/*else if(!strcmp(msg->control,"NK_gprs_detach"))
	{
		return(gmm1_->detach(pkt));
	}
	else if(!strcmp(msg->control,"llestablishindify"))
	{
		if(msg->status != 0 )
                {
                        //establishment of this link is successful
                        return(1);
                }
                else
                {

                        return(-1);
                }

	}
	else if(!strcmp(msg->control,"llreleaseindify"))
	{
			//receive LL_release_identify control msg from LLC
			//delete the whole nsapi link
			while(path != NULL)
			{
				current = path;
				path = path->next;
				free(current);
			}
			return(1);


	}

	else if(!strcmp(msg->control ,"lldataindify"))
        {
			//receive LL_data_identify control msg from LLC
			if(msg->status != 0 )
                        {
                                //establishment of this link is successful
				//return(put(pkt, recvtarget_));
                                return(NslObject::recv(pkt));
                        }
                        else
                        {
				freeBss_message(p);
				free(pkt);
                                return(-1);
                        }

	}

	else if(!strcmp(msg->control,"llunitdataindify"))
        {
			//receive LL_unitdata_identify

			//when we receive this kind of packet that means it
			//was received successfully by LLC layer

			//return(put(pkt, recvtarget_));
			return(NslObject::recv(pkt));

	}
	else if(!strcmp(msg->control ,"llstatusindify"))
        {
			//receive LL_status_identify
			//it means some error occurs in LLC layer
			//remove the link which has some problems
			 //delete the link which nsapi is mapped to
                        while(path != NULL)
                        {
                                if(path->nsapi == msg->nsapi && path->qos == msg->qos)
                                {
                                        current = path;
                                        next = path->next;
                                        path = prev;
                                        path->next = next;
                                        free(current);
                                }
                                prev = path;
                                path = path->next;

                        }
			//return message to snsm layer
			strcpy(msg->control,"snsmstatusreqst");
			//return(put(pkt, recvtarget_));
			return(NslObject::recv(pkt));

	}*/
	else
	{
		/*this means that we accept a pure data packet and
		* we are trasmitting it to SGSN. However,before we
		* transmit it to SGSN, we must convert its packet
		* format in normal TCP/IP format
		*/

		Packet	*p1;
		struct activate_type *ctl_msg;
		const char *tmp1="flag";
		char *tmp2;
		ctl_msg = (struct activate_type *)p->sndcp_option;
        //if ( ctl_msg ) /* revised by jclin on 09/26/2004 */
	    strcpy(ctl_msg->control,"pure_data");

		p1 = p->packet;
		pkt->DataInfo_ = (Packet *)p1;
		tmp2 = (char *)ctl_msg;
		p1->pkt_addinfo(tmp1,tmp2,sizeof(struct activate_type));
		p1->pkt_setflow(PF_RECV);

        if ( p->user_data ) {
            delete p->user_data;
            p->user_data = NULL;
        }

        if ( p->packet )
            p->packet = NULL;

		if(p->rlc_option)
            delete p->rlc_option;
        if(p->llc_option)
            delete p->llc_option;
        if(p->sndcp_option)
            delete p->sndcp_option;
        if(p->rlc_header)
            delete p->rlc_header;
        if(p->mac_header)
            delete p->mac_header;
        if(p->llc_header)
            delete p->llc_header;
        if(p->sndcp_header)
            delete p->sndcp_header;
        if(p->bssgp_header)
            delete p->bssgp_header;

        delete p;

        return(NslObject::recv(pkt));


	}
	return 1;

 }
