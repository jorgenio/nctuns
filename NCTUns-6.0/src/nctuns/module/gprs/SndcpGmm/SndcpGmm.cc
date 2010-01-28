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
#include <time.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <object.h>
#include <gprs/include/debug.h>
#include "SndcpGmm.h"

MODULE_GENERATOR(SndcpGmm);

gmm::~gmm()
{
	free(ms_store);
}
sndcp::~sndcp()
{
}
SndcpGmm::~SndcpGmm()
{
	delete(gmm1);
	delete(sndcp1);
	delete(snsm1);
	free(L_status);
	
}
gmm::gmm(SndcpGmm *upper)
{
	uplayer = upper;
	//initial attempt count
	attempt_count = 0;	
	CI = 0;
		
	
}
int gmm::init()
{
	//initial ms's storage data	

	ms_store = (ms_storage *)malloc(sizeof(struct ms_storage));
	bzero(ms_store,sizeof(ms_storage));
	ms_store->qos = 3;
	ms_store->msisdn = uplayer->getMSISDN();        
	ms_store->imsi =uplayer->getIMSI();
	if(DEBUG_LEVEL >= MSG_DEBUG)
	{
		printf("gmm's imsi=%ld\n",ms_store->imsi);
		printf("ms's ip=%ld\n",ms_store->ms_ip);
	
	}
	//get ms's ip and at the activation procedure we send it to SGSN and GGSN
	ms_store->ms_ip =nodeid_to_ipv4addr(uplayer->get_nid(),uplayer->get_port());
	return 1;
}

inline int gmm::change_ptmsi_sig(ulong ptmsi,ulong ptmsi_sig)
{
	ms_store->ptmsi = ptmsi;
	ms_store->ptmsi_sig = ptmsi_sig;
	return 1;
}
int SndcpGmm::freeBss_message(bss_message *p)
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
        if(p->packet) {
                free(p->packet);
                p->packet = NULL;
        }
        return(1);

}


inline int SndcpGmm::mid_detach()
{
	return(gmm1->detach("MS_gprs_detach",1));
}
inline int SndcpGmm::mid_attach()
{
	//printf("at gmm attach!!\n");
	return(gmm1->attach());
}
int gmm::Modify_PDP_req(ePacket_ *pkt)
{
	struct LLCOption* llcop   = (LLCOption *)malloc(sizeof(LLCOption));
	bss_message* p1           = new bss_message;
	ePacket_ *pkt1            = createEvent();

	struct activate_type* msg = NULL,*msg1 = NULL;

	pkt1->DataInfo_ = p1;
	bzero(p1,sizeof(struct bss_message));

	bss_message* p = (bss_message *)pkt->DataInfo_;
	msg1 = (struct activate_type *)malloc(sizeof(struct activate_type));
	bzero(msg1,sizeof(struct activate_type));

	msg           = (struct activate_type *)p->user_data;
	ms_store->qos = msg->qos;

	strcpy(msg1->control,"modify_PDP_accept");
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
    
	llcop->tlli      = ms_store->tlli;
	llcop->qos       = 1;
	llcop->req_ch_no = -1;
	llcop->req_bsic  = -1;
	llcop->req_rai   = -1;

	p1->llc_option       = (char *)llcop;
	p1->llc_option_len   = sizeof(struct LLCOption);
	p1->user_data        = (char *)msg1;
	p1->user_data_len    = sizeof(msg1->control);
	p1->sndcp_header_len = 0;
	p1->flag             = PF_SEND;

	uplayer->freeBss_message(p);
	free(pkt);
	
	return(uplayer->GprsObject::sendManage(pkt1));
	
}
inline int gmm::setRAI(ulong RAI)
{
	ms_store->RAI = RAI;
	return 1;
}
inline int gmm::get_current_attempt()
{
	return(attempt_count);
}
inline int gmm::attempt_add()
{
	attempt_count++;
	return 1;
}
int SndcpGmm::periodic()
{
	/*
	we send periodic update request to under layer to 
	inform them to do periodic routing area update request
	*/
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	upper_msg *msg;
	struct ms_storage *ms1;
	struct bss_message *p1= new bss_message;
	//Packet *p1 = new Packet;
        ePacket_ *pkt1 = createEvent();
	ms1= gmm1->get_ms_store();
        pkt1->DataInfo_ = p1 ;
	bzero(p1,sizeof(struct bss_message));
        msg = (upper_msg *)malloc(sizeof(struct upper_msg));
	bzero(msg,sizeof(struct upper_msg));
	strcpy(msg->control,"route_area_update_rq");
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
    llcop->tlli = ms1->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


	p1->llc_option = (char *)llcop;
    p1->llc_option_len = sizeof(struct LLCOption);
    p1->user_data = (char *)msg;
    p1->user_data_len = sizeof(struct upper_msg);
    p1->sndcp_header_len = 0;
	p1->flag = PF_SEND;
	return(GprsObject::sendManage(pkt1));
        
}
int SndcpGmm::updateexpire()
{
	struct upper_msg *msg = (upper_msg *)malloc(sizeof(struct upper_msg));
	bzero(msg,sizeof(struct upper_msg));
	gmm1->attempt_add();//the number of attempt attach and the max value of it is four
	if(gmm1->get_current_attempt()<=4)
	{
		struct ms_storage *ms1;
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		struct bss_message *p= new bss_message;
		ePacket_ *pkt=createEvent();
		ms1 = gmm1->get_ms_store();
		pkt->DataInfo_ = p;
		bzero(p,sizeof(struct bss_message));
		strcpy(msg->control,"route_area_update_rq");
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = ms1->tlli;
		llcop->qos = 1;
		llcop->req_ch_no = -1;
        llcop->req_bsic  = -1;
        llcop->req_rai   = -1;



                p->llc_option = (char *)llcop;
                p->llc_option_len = sizeof(struct LLCOption);
		p->user_data = (char *)msg;
		p->user_data_len = sizeof(struct upper_msg);
		p->sndcp_header_len = 0;
		p->flag = PF_SEND;
		//we send attach request message to the SGSN
		//we set timer again
		BASE_OBJTYPE(type);
		u_int64_t wait_interval;
		SEC_TO_TICK(wait_interval, 15);   /* wait for 15 sec*/
		type = POINTER_TO_MEMBER(SndcpGmm,updateexpire);
		T3330.setCallOutObj(this, type);
		T3330.start(wait_interval, 0);
		return(GprsObject::sendManage(pkt));
		//return(GprsObject::put(pkt, sendtarget_));
	}
	gmm1->reset_attempt_count();
	if(DEBUG_LEVEL >= MSG_ERROR)
	{
		printf("timer is time out and its attempt_count is over four times!!\n");
		printf("routing area update fail!!\n");
	}
	return(-1);
}
int SndcpGmm::set_cstate(int cstate)
{
	c_state = cstate;
	return 1;
}
inline ms_storage* gmm::get_ms_store()
{
	return(ms_store);
}
int SndcpGmm::readyexpire()
{
	/*
	Now we generate new packet to tell SGSN that we are in standby state
	due to the time expiring
	*/
	if(DEBUG_LEVEL >= MSG_DEBUG)
	{
		printf("***ready timer has expired in SndcpGmm!!***\n");
		printf("***we are to tell SGSN to change ms's state!!***\n");
	}
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct in_standby *standby = (in_standby *)malloc(sizeof(struct in_standby));
	struct ms_storage *ms_store1;
	struct bss_message *p1= new bss_message;
	bzero(standby,sizeof(struct in_standby));
        ePacket_ *pkt1 = createEvent();
        pkt1->DataInfo_ = p1 ;
	bzero(p1,sizeof(struct bss_message));
	T3312.cancel();
        ms_store1 = gmm1->get_ms_store();
	strcpy(standby->control,"in_standby_state");
        standby->imsi = ms_store1->imsi;
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = ms_store1->tlli;
	llcop->qos       = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


        p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)standby;
	p1->user_data_len = sizeof(struct in_standby);
	p1->flag = PF_SEND;
	status = 1;//standby state
	return(GprsObject::sendManage(pkt1));
}
inline int gmm::reset_attempt_count()
{
	attempt_count = 0;
	return 1;
}

int gmm::iden_reqst()
{
	//at this function, we send MS's imsi to SGSN
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct bss_message *p1= new bss_message;
	struct ms_storage *ms1=(ms_storage *)malloc(sizeof(ms_storage));
        ePacket_ *pkt1 = createEvent();
        pkt1->DataInfo_ = p1 ;
	bzero(p1,sizeof(struct bss_message));
	bcopy(ms_store,ms1,sizeof(ms_storage));
	strcpy(ms1->control,"iden_resp");
	ms1->type = 0;//attach type
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = ms_store->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


        p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)ms1;
	p1->user_data_len = sizeof(struct ms_storage);
	p1->sndcp_header_len =0;
	p1->flag = PF_SEND;
	//return(uplayer->GprsObject::sendManage(pkt1));
	return(uplayer->GprsObject::put(pkt1, uplayer->sendtarget_));
}

int gmm::route_area_update_req(ePacket_ *pkt) {

    struct  LLCOption*          llcop = (LLCOption *)malloc(sizeof(LLCOption));
    struct  intra_route_msg*    intra_msg;
    struct  route_msg*          msg1  = NULL;
    struct  bss_message*        p1    = new bss_message;
    int     no_of_list, found = 0;
    char    biggest = 0;

    Event*  pkt1 = createEvent();
    pkt1->DataInfo_ = p1;
    bzero(p1,sizeof(struct bss_message));

    struct  bss_message* p = (bss_message *) (pkt->DataInfo_);

    struct  preferred_list* msg = (struct preferred_list*) (p->sndcp_option);

    no_of_list = (int) (msg->num_of_list);


    //return 1;

    //printf("current CI:%d received CI:%d\n",CI,msg->list_head_p->bsic);
    //printf("current RAI:%d received RAI:%d\n",ms_store->RAI,msg->list_head_p->rai);

    if(msg->list_head_p->rai == ms_store->RAI) {

        if(CI != msg->list_head_p->bsic) {

            //first, we must find the best BS to attach

            biggest = msg->list_head_p->bsic;
            CI      = msg->list_head_p->bsic;

            //this indicates that it is intra SGSN RA Update

            intra_msg = (intra_route_msg *)malloc(sizeof(struct intra_route_msg));
            bzero(intra_msg,sizeof(struct intra_route_msg));
            ms_store->CI  = biggest;
            intra_msg->CI = biggest;
            strcpy(intra_msg->control,"intra_route_area_update_rq");

            intra_msg->ptmsi     = ms_store->ptmsi;
            intra_msg->ptmsi_sig = ms_store->ptmsi_sig;
            intra_msg->imsi      = ms_store->imsi;
            intra_msg->tlli      = ms_store->tlli;

            bzero(llcop,sizeof(LLCOption));
            strcpy(llcop->control,"LLGMM_TRIGGER");

            llcop->tlli          = ms_store->tlli;
            llcop->qos           = 1;
            llcop->req_ch_no     = -1;
            llcop->req_bsic      = biggest;
            llcop->req_rai       = msg->list_head_p->rai;

            p1->llc_option       = (char *)llcop;
            p1->llc_option_len   = sizeof(LLCOption);
            p1->user_data        = (char *)intra_msg;
            p1->user_data_len    = sizeof(struct intra_route_msg);
            p1->sndcp_header_len = 0;
            p1->flag             = PF_SEND;

            found = 1;

            /* added by jclin on 09/17/2004 */
            Event* imc_ep = NULL;
            CREATE_BSS_EVENT(imc_ep);
            bss_message* imc_ep_bmsg = reinterpret_cast<bss_message*> (imc_ep->DataInfo_);

            struct LLCOption *imc_ep_llcop = (LLCOption*) malloc (sizeof (LLCOption) );
            bzero(imc_ep_llcop, sizeof (LLCOption) );
            imc_ep_bmsg->llc_option     = reinterpret_cast<char*> (imc_ep_llcop);
            imc_ep_bmsg->llc_option_len = sizeof(LLCOption);
            imc_ep_bmsg->flag           = PF_SEND;

            strcpy(imc_ep_llcop->control,"LLGMM_TRIGGER");

            imc_ep_llcop->tlli      = ms_store->tlli;
            imc_ep_llcop->qos       = 1;
            imc_ep_llcop->req_ch_no = 0;
            imc_ep_llcop->req_bsic  = biggest;
            imc_ep_llcop->req_rai   = msg->list_head_p->rai;

            int res = (uplayer->GprsObject::put (imc_ep, uplayer->sendtarget_) );
            if ( res <= 0 ) {
                printf(
                    "gmm::route_area_update_req(): Inter-module control packet for roaming was sent with an warning.\n");
                exit(1);
            }
            /* end the section added by jclin on 09/17/2004 */

            /* added by jclin for free preferred_list */
            bss_info* elem      = msg->list_head_p;
            bss_info* elem_next = msg->list_head_p;
            for ( int i=0 ; i<no_of_list ; ++i) {

                elem = elem_next;

                if ( elem ) {

                    elem_next = elem->next;
                    delete elem;

                }

            }

            free(msg);
            p->sndcp_option     = NULL;
            p->sndcp_option_len = 0;


            uplayer->freeBss_message(p);
            free(pkt);


            return(uplayer->GprsObject::put(pkt1, uplayer->sendtarget_));

        }
        else {

            /*we don't need to do intra roamming*/

            /* added by jclin for free preferred_list */
            bss_info* elem      = msg->list_head_p;
            bss_info* elem_next = msg->list_head_p;
            for ( int i=0 ; i<no_of_list ; ++i) {

                elem = elem_next;

                if ( elem ) {

                    elem_next = elem->next;
                    delete elem;

                }

            }

            free(msg);
            p->sndcp_option     = NULL;
            p->sndcp_option_len = 0;

            uplayer->freeBss_message(p);
            free(pkt);
            return 1;

        }
    }

    if(!found) {

        //first, we must find the best bsic to attach
        biggest = 0;
        biggest = msg->list_head_p->bsic;
        //this indicates that it is inter SGSN RA Update
        msg1 = (route_msg *)malloc(sizeof(struct route_msg));
        bzero(msg1,sizeof(struct route_msg));
        ms_store->CI            = biggest;
        CI                      = biggest;
        ms_store->RAI           = msg->list_head_p->rai;
        new_RAI                 = msg->list_head_p->rai;
        msg1->CI                = biggest;
        msg1->RAI               = msg->list_head_p->rai;
        strcpy(msg1->control,"inter_route_area_update_rq");
        msg1->ptmsi             = ms_store->ptmsi;
        msg1->ptmsi_sig         = ms_store->ptmsi_sig;
        msg1->imsi              = ms_store->imsi;
        msg1->tlli              = ms_store->tlli;
        bzero(llcop,sizeof(LLCOption));
        strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli             = ms_store->tlli;
        llcop->qos              = 1;
        llcop->req_ch_no        = -1;
        llcop->req_bsic         = biggest;
        llcop->req_rai          = msg->list_head_p->rai;

        p1->llc_option          = (char *)llcop;
        p1->llc_option_len      = sizeof(LLCOption);
        p1->user_data           = (char *)msg1;
        p1->user_data_len       = sizeof(struct route_msg);
        p1->sndcp_header_len    = 0;
        p1->flag                = PF_SEND;


        /* added by jclin on 09/17/2004
        * This block of codes send a inter-module event to inform
        * the lower layer module of resetting the states of some
        * data structures before peforming roaming.
        */

        Event* imc_ep = NULL;
        CREATE_BSS_EVENT(imc_ep);
        bss_message* imc_ep_bmsg = reinterpret_cast<bss_message*> (imc_ep->DataInfo_);
        struct LLCOption *imc_ep_llcop = (LLCOption*)malloc(sizeof(LLCOption));

        bzero(imc_ep_llcop,sizeof(LLCOption));
        imc_ep_bmsg->llc_option     = reinterpret_cast<char*> (imc_ep_llcop);
        imc_ep_bmsg->flag           = PF_SEND;
        imc_ep_bmsg->llc_option_len = sizeof(LLCOption);

        strcpy(imc_ep_llcop->control,"LLGMM_TRIGGER");

        imc_ep_llcop->tlli      = ms_store->tlli;
        imc_ep_llcop->qos       = 1;
        imc_ep_llcop->req_ch_no = 0;
        imc_ep_llcop->req_bsic  = biggest;
        imc_ep_llcop->req_rai   = msg->list_head_p->rai;

        int res = (uplayer->GprsObject::put(imc_ep, uplayer->sendtarget_));
        if ( res <= 0 ) {

            printf("gmm::route_area_update_req(): Inter-module control packet for roaming was sent with an warning.\n");
            exit(1);
        }

        /* end the section added by jclin on 09/17/2004 */

        /* added by jclin for free preferred_list */
        bss_info* elem      = msg->list_head_p;
        bss_info* elem_next = msg->list_head_p;

        for ( int i=0 ; i<no_of_list ; ++i) {

            elem = elem_next;

            if ( elem ) {

                elem_next = elem->next;
                delete elem;

            }

        }

        free(msg);
        p->sndcp_option     = NULL;
        p->sndcp_option_len = 0;

        uplayer->freeBss_message(p);
        free(pkt);
        return(uplayer->GprsObject::put(pkt1, uplayer->sendtarget_));
    }

    return 1;
}

int gmm::deactivate_PDP_req()
{
	//MS-initiated PDP context Modification Procedure
	//send update PDP context request to SGSN and GGSN to create PDP context
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct bss_message *p1= new bss_message;
	ePacket_ *pkt1=createEvent();
	struct deactivate_type *deac_type = (deactivate_type *)malloc(sizeof(struct deactivate_type));
	bzero(deac_type,sizeof(struct deactivate_type));
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("Now we are to trigger a deactivation request on the MS side!!\n");
	pkt1->DataInfo_ = p1;
	bzero(p1,sizeof(struct bss_message));
	strcpy(deac_type->control,"deactivate_PDP_r");
	deac_type->nsapi = ms_store->nsapi;
	deac_type->qos = ms_store->qos;
	deac_type->imsi = ms_store->imsi;
	deac_type->tlli = ms_store->tlli;
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
    llcop->tlli = ms_store->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


	p1->llc_option = (char *)llcop;
    p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)deac_type;
	p1->user_data_len = sizeof(struct deactivate_type);
	p1->sndcp_header_len = 0;
	p1->flag = PF_SEND;
	/*when we call attach , we change status to ready
	we send attach request message to the SGSN*/
	return(uplayer->GprsObject::sendManage(pkt1));
}

int gmm::update_PDP_req(ushort nsapi,uchar qos)
{
	/*MS-initiated PDP context Modification Procedure
	send update PDP context request to SGSN and GGSN to create PDP context*/
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("Now we are to do update pdp request!!\n");
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct bss_message *p1= new bss_message;
	ePacket_ *pkt1=createEvent();
	struct activate_type *update_t = (activate_type *)malloc(sizeof(struct activate_type));
	bzero(update_t,sizeof(activate_type));
	pkt1->DataInfo_ = p1;
	bzero(p1,sizeof(struct bss_message));
	/*ms_store->nsapi = nsapi;
	ms_store->qos = qos;*/
	strcpy(update_t->control,"update_PDP_r");
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("control=%s imsi=%ld\n",update_t->control,ms_store->imsi);
	update_t->nsapi = nsapi;
	update_t->qos = qos;
	update_t->imsi = ms_store->imsi;
	update_t->tlli = ms_store->tlli;
	update_t->msisdn = ms_store->msisdn;
	update_t->ms_ip = ms_store->ms_ip;
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
    llcop->tlli = ms_store->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


	p1->llc_option = (char *)llcop;
    p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)update_t;
	p1->user_data_len = sizeof(struct activate_type);	
	p1->sndcp_header_len = 0;
	p1->flag = PF_SEND;
	//we send attach request message to the SGSN
	return(uplayer->GprsObject::sendManage(pkt1));
	//return(uplayer->GprsObject::put(pkt1, uplayer->sendtarget_));
}
/*int gmm::NK_activate_PDP_req(ePacket_ *pkt)
{
	struct NK_type *msg=NULL;
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct activate_type *act_type = (activate_type *)malloc(sizeof(struct activate_type));
	bzero(act_type,sizeof(struct activate_type));
	struct bss_message *p;
	struct bss_message *p1 = new bss_message;
	ePacket_ *pkt1=createEvent();
	p = (bss_message *)pkt->DataInfo_;
	pkt1->DataInfo_ = p1;
	bzero(p1,sizeof(struct bss_message));
	msg = (NK_type *)p->user_data;
	memcpy(act_type->control,msg->control,sizeof(msg->control));
	//act_type->control = msg->control;
	act_type->nsapi = ms_store->nsapi;
	act_type->qos = ms_store->qos;
	act_type->imsi = ms_store->imsi;
	act_type->msisdn = ms_store->msisdn;
	act_type->tlli = ms_store->tlli;
	act_type->ms_ip = msg->TEID;//save GGSN's ip here
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = ms_store->tlli;
	llcop->qos = 1;

        p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)act_type;
	p1->user_data_len = sizeof(struct activate_type);
	p1->sndcp_header_len = 0;
	p1->flag = PF_SEND;
	//when we call attach , we change status to ready
	return(uplayer->GprsObject::sendManage(pkt1));
	//return(uplayer->GprsObject::put(pkt1, uplayer->sendtarget_));
}*/
int gmm::activate_PDP_req(const char *msg)
{
	//send create PDP context request to SGSN and GGSN to create PDP context
	struct activate_type *act_type;
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct bss_message *p1 = new bss_message;
	ePacket_ *pkt1=createEvent();
	pkt1->DataInfo_ = p1;
	bzero(p1,sizeof(struct bss_message));
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("Now we are to trigger a activation request on the MS side!!\n");
	
	act_type = (activate_type *)malloc(sizeof(struct activate_type));
	bzero(act_type,sizeof(struct activate_type));
	strcpy(act_type->control,msg);
	act_type->nsapi = ms_store->nsapi;
	act_type->qos = ms_store->qos;
	act_type->imsi = ms_store->imsi;
	act_type->msisdn = ms_store->msisdn;
	act_type->tlli = ms_store->tlli;
	act_type->ms_ip = ms_store->ms_ip;
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = ms_store->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


	p1->llc_option = (char *)llcop;
    p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)act_type;
	p1->user_data_len = sizeof(struct activate_type);
	p1->sndcp_header_len = 0;
	p1->flag = PF_SEND;
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("activate control=%s imsi=%ld\n",act_type->control,act_type->imsi);
	//we send activate request message to the SGSN
	return(uplayer->GprsObject::sendManage(pkt1));
}

int SndcpGmm::set3321()
{
	/*we set the wait timer*/
	BASE_OBJTYPE(type);
	u_int64_t wait_interval;
	SEC_TO_TICK(wait_interval, 15);   /* wait 15 sec for response */
	type = POINTER_TO_MEMBER(SndcpGmm,mid_detach);
	T3321.setCallOutObj(this, type);
	T3321.start(wait_interval, 0);
	return(1);
}
int gmm::detach(const char *detach_type1,uchar type)
{
	/*
	we send detach request to SGSN'gmm and wait for its detach accept msg.
	*/
	/*
	3:GMM-DEREGISTERED-INITIATED, at this state , 
	we enter into waiting for request accomplishment status
	*/
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SndcpGmm::detach::Now we are to trigger a detach request on the MS side!!\n");
	attempt_count++;//number of attempt attach and the max value of it is four
	if(attempt_count<=4)
	{
		uplayer->set_cstate(3); 
		struct bss_message *p= new bss_message;
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		ePacket_ *pkt=createEvent();
		struct detach_type *detach_t = (detach_type *)malloc(sizeof(struct detach_type));
		bzero(detach_t,sizeof(struct detach_type));
		pkt->DataInfo_ = p;
		bzero(p,sizeof(struct bss_message));
		p->sndcp_header_len = 0;/*this indicates that it is not for LLC */
		p->user_data_len = sizeof(struct detach_type);
		sprintf(detach_t->control,"%s", detach_type1);
		//detach type
		detach_t->type = type;
		detach_t->nsapi = ms_store->nsapi;
		detach_t->qos = ms_store->qos;
	  	detach_t->imsi = ms_store->imsi;
	   	detach_t->ptmsi = ms_store->ptmsi;
	   	detach_t->ptmsi_sig = ms_store->ptmsi_sig;
		detach_t->tlli = ms_store->tlli;
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
                llcop->tlli = ms_store->tlli;
		llcop->qos = 1;
		llcop->req_ch_no = -1;
        llcop->req_bsic  = -1;
        llcop->req_rai   = -1;


		p->user_data = (char *)detach_t;
		p->llc_option = (char *)llcop;
		p->llc_option_len = sizeof(struct LLCOption);
		p->flag = PF_SEND;
		/*set T3321 timer*/
		uplayer->set3321();
		//we send attach request message to the SGSN
		return(uplayer->GprsObject::sendManage(pkt));
	}
	attempt_count = 0;
	if(DEBUG_LEVEL >= MSG_ERROR)
	{
		printf("SndcpGmm::detach()::timer is time out and its attempt_count is over four times!!\n");
		printf("SndcpGmm::detach()::detach failure because attempt_count is over four times!!\n");
	}
	return(-1);
}

int gmm::attach()
{
	
	/*
	1:GMM-REGISTERED-INITIATED, at this state , 
	we enter into waiting for request accomplishment status
	*/
	/*we don't have valid ptmsi during the first attach*/
	attempt_count++;//number of attempt attach and the max value of it is four
	if(DEBUG_LEVEL >= MSG_DEBUG)	
		printf("SndcpGmm::attach()::Now we are to trigger a attach request on the MS side!!\n");
	if(attempt_count<=4)
	{
		uplayer->set_cstate(1);
		struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
		struct bss_message *p= new bss_message;
		struct ms_storage *ms_store1 = (ms_storage *)malloc(sizeof(ms_storage));
		bcopy(ms_store,ms_store1,sizeof(ms_storage));
		bzero(p,sizeof(bss_message));
		ePacket_ *pkt=createEvent();
		pkt->DataInfo_ = p;
		strcpy(ms_store1->control,"gprs_imsi_attach");
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::attach()::control msg:%s RAI:%ld ip:%ld\n",ms_store1->control,ms_store1->RAI,ms_store1->ms_ip);
		strcpy(llcop->control,"LLGMM_ASSIGN");
		//printf("%s\n",llcop->control);
		/*generate random TLLI*/
		llcop->tlli = (rand() | 0x0000001e ) & 0xfffffffe;
		ms_store->tlli = llcop->tlli;
		llcop->oldtlli = 0xffffffff;
		llcop->qos = 1;
		llcop->req_ch_no = -1;
		llcop->req_bsic  = -1;
		llcop->req_rai   = -1;
		ms_store1->tlli = llcop->tlli;
        	p->llc_option = (char *)llcop;
        	p->llc_option_len = sizeof(struct LLCOption);
		p->user_data = (char *)ms_store1;
		p->user_data_len = sizeof(struct ms_storage);
		p->flag = PF_SEND;
		//we send attach request message to the SGSN
		return(uplayer->GprsObject::sendManage(pkt));
		//return(uplayer->GprsObject::put(pkt, uplayer->sendtarget_));
	}
	attempt_count = 0;

	if(DEBUG_LEVEL >= MSG_ERROR)
		printf("SndcpGmm::attach()::attach failure because attempt_count is over four times!!\n");
	return(-1);
}
sndcp::sndcp(SndcpGmm *upper)
{
	uplayer = upper;
	
}
/*int sndcp::LLreleasereq(uchar *fieldv,ushort nsapi,uchar qos)
{
	struct bss_message *p= new bss_message;
	ePacket_ *pkt=createEvent();
	uchar *f1;
	struct control_msg *msg;
	bzero(p,sizeof(struct bss_message));
	pkt->DataInfo_ = p;
	f1 = fieldv;
	msg = (control_msg *)malloc(sizeof(struct control_msg));
	bzero(msg,sizeof(struct control_msg));
	if(*(f1) == 0)
		strcpy(msg->control,"llreleasereqstlocal");
	else	
		strcpy(msg->control,"llreleasereqst");
	//release the nsapi's link
	msg->nsapi = nsapi;
	msg->qos = qos;
	p->user_data = (char *)msg;
	p->user_data_len = sizeof(struct control_msg);
	p->sndcp_header_len =0 ; //this indicates that it is not for LLC
	p->flag = PF_SEND;
	return(uplayer->GprsObject::sendManage(pkt));
	//return(uplayer->GprsObject::put(pkt, uplayer->sendtarget_));

}

int sndcp::snsmactivateindi(ushort nsapi, uchar qos)
{
	return(LLestablishreq(nsapi,qos));
} 

int sndcp::snsmmodifyindi(ushort nsapi, uchar qos)
{
	
        return(LLestablishreq(nsapi,qos));

}

int sndcp::snsmdeactivateindi(ushort nsapi, uchar qos)
{
	struct control_msg *msg;
	ePacket_ *pkt= createEvent();
	struct bss_message *p= new bss_message;
	pkt->DataInfo_=p;
	bzero(p,sizeof(struct bss_message));
	msg = (control_msg *)malloc(sizeof(struct control_msg));
	bzero(msg,sizeof(struct control_msg));
	strcpy(msg->control,"llreleasereqstlocal");
	//release the nsapi's link        
	msg->nsapi = nsapi;
	msg->qos = qos;
	p->sndcp_header_len =0;//this indicates that it is not for LLC
	p->user_data_len = sizeof(struct control_msg);
	p->user_data = (char *)msg;
	p->flag = PF_SEND;
	return(uplayer->GprsObject::sendManage(pkt));
	//return(uplayer->GprsObject::put(pkt, uplayer->sendtarget_));
}

int sndcp::LLestablishreq(ushort nsapi,uchar qos)
{
	struct bss_message *p= new bss_message;
	ePacket_ *pkt = createEvent();
        
	struct control_msg *msg;
	
	pkt->DataInfo_ = p ;
	bzero(p,sizeof(struct bss_message));
	msg = (control_msg *)malloc(sizeof(struct control_msg));
	bzero(msg,sizeof(struct control_msg));
	strcpy(msg->control,"llestablishreqstdata");// the LL_unitdata_request control signal which needs
 	// response from link layer
        msg->nsapi = nsapi;
        
        msg->qos=qos;
	p->user_data = (char *)msg;
	p->user_data_len = sizeof(struct control_msg);
	p->sndcp_header_len =0 ; //this indicates that it is not for LLC 
	p->flag = PF_SEND;
	return(uplayer->GprsObject::sendManage(pkt));
	
}
int sndcp::LLestablishreqpure(ushort nsapi,uchar qos)
{
        struct bss_message *p= new bss_message;
        ePacket_ *pkt = createEvent();

        struct control_msg *msg;
	bzero(p,sizeof(struct bss_message));
        pkt->DataInfo_ = p ;

        msg = (control_msg *)malloc(sizeof(struct control_msg));
	bzero(msg,sizeof(struct control_msg));
	strcpy(msg->control,"llestablishreqst");// the LL_unitdata_request controlsignal which needs
        // response from link layer
        msg->nsapi = nsapi;

        msg->qos=qos;
        p->user_data = (char *)msg;
        p->user_data_len = sizeof(struct control_msg);
        p->sndcp_header_len =0 ; //this indicates that it is not for LLC
	p->flag = PF_SEND;
	return(uplayer->GprsObject::sendManage(pkt));

}
*/
int sndcp::snunitdatareqst(ePacket_ *pkt,ushort nsapi,uchar qos,ulong tlli,link_status *head)
{
	/* struct link_status *path;
	int	links=0;
	path = uplayer->head;	
	while(path != NULL)
        {
                if(path->nsapi == nsapi && path->qos == qos)
                        links = 1;
                path = path->next;

        }
 	*/

	return(LLunitdatareq(pkt,nsapi,qos,tlli));
		
	//}
	/*else
	{
		//link has not yet been created
		printf("link has not yet been created!!\n");
		printf("qos=%d!!!\n",*(qos));
		return(LLestablishreq(nsapi,qos));
			
	}
	*/	
}
int sndcp::LLunitdatareq(ePacket_ *pkt,ushort nsapi,uchar qos,ulong tlli)
{
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));

	Event* pkt1 = createEvent();

	bss_message* bmsg_p = new bss_message;
	bzero(bmsg_p,sizeof(bss_message));
	pkt1->DataInfo_ = reinterpret_cast<bss_message*> (bmsg_p);

	Packet* p = (Packet *)pkt->DataInfo_;

	struct activate_type* msg = (activate_type *)malloc(sizeof(struct activate_type));
	bzero(msg,sizeof(struct activate_type));

	struct sndcp_unack *unack_header = (sndcp_unack *)malloc(sizeof(struct sndcp_unack));
	
	/* send control msg to LLC layer */
	strcpy(msg->control,"llunitdatareqst");

        /* the LL_unitdata_request control signal which the LL module needs */
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SndcpGmm::LLunitdatareq::control=%s nsapi=%d\n",msg->control,nsapi);

        /* response from link layer */
        msg->nsapi = nsapi;
        msg->imsi = uplayer->getIMSI();
        msg->qos=qos;
	msg->tlli = tlli;
	
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = tlli;
	llcop->qos = 3;
	llcop->req_ch_no = -1;
	llcop->req_bsic  = -1;
        llcop->req_rai   = -1;

        bmsg_p->llc_option       = (char *)llcop;
        bmsg_p->llc_option_len   = sizeof(struct LLCOption);
        bmsg_p->sndcp_option     = (char *)msg;
	bmsg_p->sndcp_option_len = sizeof(activate_type);
        bmsg_p->user_data        = (char *)p->pkt_sget();
        bmsg_p->user_data_len    = p->pkt_getlen();
#if 0	
        int app_payload_len = p->pkt_getlen();
	if (!app_payload_len) {

    	    bmsg_p->user_data        = NULL; 
	    bmsg_p->user_data_len    = 0;


	}
	else {
	    
	    char* app_payload_p      = new char[app_payload_len];
            memcpy(app_payload_p, p->pkt_sget(), app_payload_len);	    
	    bmsg_p->user_data        = app_payload_p;
	    bmsg_p->user_data_len    = app_payload_len;

	}
#endif
	bmsg_p->packet           = p;


	/* add sndcp-layer header */
	bzero(unack_header,sizeof(struct sndcp_unack));
	unack_header->X = 0;
	unack_header->F = 1;
	unack_header->T = 1;
	unack_header->M = 0;

	unack_header->NSAPI   = nsapi;
	unack_header->DCOMP   = 0;
	unack_header->PCOMP   = 0;
	unack_header->seg_no  = 0;
	unack_header->N_PDU   = 0;
	unack_header->E_N_PDU = 0;

	bmsg_p->sndcp_header     = (char *)unack_header;
	bmsg_p->sndcp_header_len = sizeof(struct sndcp_unack);
	bmsg_p->flag             = PF_SEND;

	/* send acked data packet */
	return(uplayer->GprsObject::put(pkt1, uplayer->sendtarget_));
}
/*int sndcp::sndatareqst(ePacket_ *pkt,ushort nsapi,uchar qos,link_status *head)
{
	struct link_status	*path;
	int 	links=0;
        //when other condition occurs, we wait until ack from LLC
	//search if this nsapi has created the link for it
	path = uplayer->head;
	
	while(path != NULL)
	{
		if(path->nsapi == nsapi && path->qos == qos)
			links = 1;
		path = path->next;
			
	}
	if(links)
	{
		//link has been created 
		//now to determine if we need segmentation
		//printf("link has been created!!\n");
		return(LLdatareq(pkt,nsapi,qos));
		
	}
	else
	{
	
		
		//link is not created yet
		//printf("link has not been created!!\n");
		return(LLestablishreq(nsapi,qos));
	}
}

int sndcp::LLdatareq(ePacket_ *pkt,ushort nsapi,uchar qos)
{
	struct control_msg *msg;
	struct sndcp_ack *ack_header;
	struct bss_message *p= new bss_message;
	
	msg =(struct control_msg *) malloc(sizeof(struct control_msg));
	bzero(msg,sizeof(struct control_msg));
	ack_header = (sndcp_ack *)malloc(sizeof(struct sndcp_ack));
	pkt->DataInfo_ = p;
	bzero(p,sizeof(struct bss_message));
	//send control msg to LLC layer
	strcpy(msg->control,"lldatareqst");// the LL_data_request control signal which needs
        // response from link layer
        msg->nsapi = nsapi;
        
        msg->qos= qos;
	p->user_data = (char *)msg;
	p->user_data_len = sizeof(struct control_msg);
	//add sndcp header
	ack_header->X = 0;
	ack_header->F = 1;
	ack_header->T = 0;
	ack_header->M = 0;
	ack_header->NSAPI = nsapi;
	ack_header->DCOMP = 0;
	ack_header->PCOMP = 0;
	p->sndcp_header = (char *)ack_header;
	p->sndcp_header_len = sizeof(struct sndcp_ack);
	p->flag = PF_SEND;
	//send acked data packet
	return(uplayer->GprsObject::put(pkt, uplayer->sendtarget_));
	//return(uplayer->NslObject::send(pkt));
	  	
}
*/
SndcpGmm::SndcpGmm(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
	: GprsObject(type, id, pl, name)
{
	
	vBind("msisdn", &tmp_msisdn);
	vBind("imsi", &tmp_imsi);
	vBind("actiontable",&action_table);	
	
	/* initial MS's status */
	status = 0;
	c_state = 0;
	
	/*initial variables added by cmchou*/
	TimerAction_head = NULL;
	L_status=(link_status *)malloc(sizeof(struct link_status));

	qos        = 1; //set default qos value
 	sndcp1     = new sndcp(this);
	gmm1       = new gmm(this);
	snsm1      = new snsm(this);
	control    = 0;
	nsapi      = 0;
	confirm    = 0;
	indication = 0;
	
	/* initial link status */
	L_status->qos =0;
 	L_status->nsapi =0;
	L_status->linkstatus = 0;
	L_status->next = NULL;
	head = L_status;

	/*initial user-specified time and action*/


}

int SndcpGmm::init()
{
	FILE *infile;
	char *tmp_store = new char[50];
	int tmp_time=0;
	char nodeid_name[50];
	char tmp_action[50],r_line[100],str[4][50];
	char *action_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(action_table)+1);
	int i=0,no_of_record=0,j=0;
	//ulong  whichone_nid;
	struct doingAction      *TimerAction;
	struct doingAction 	*tmpAction,*tmpAction1;
	Event_ *start_ep;
	/*get initial imsi and msisdn value*/
	msisdn1 = atol(tmp_msisdn);
	imsi1 = atol(tmp_imsi);

        sprintf(action_FILEPATH,"%s%s",GetConfigFileDir(),action_table);

	bzero(tmp_action,sizeof(tmp_action));
	bzero(nodeid_name,sizeof(nodeid_name));
	bzero(str,sizeof(str));
	infile = fopen(action_FILEPATH,"r");
	assert(infile);
	while(!feof(infile))
    	{
		 i = 0;
                 bzero(r_line,sizeof(r_line));
                 if (!(fgets(r_line,100,infile)))
			break;

		 if(DEBUG_LEVEL >= MSG_DEBUG)
		 	printf("SndcpGmm::init()::get line:%s\n",r_line);
		 tmp_store = strtok(r_line,"\t\n");
		 sprintf(str[i],"%s",tmp_store);
		 //whichone_nid = atol(str[0]);
		 sprintf(nodeid_name,"%s%i","NODE",get_nid());
		 //printf("SndcpGmm::noid=%s\n",nodeid_name);
		 //if(whichone_nid==get_nid())
		 if(!strcmp(str[0],nodeid_name))
		 {
                 	bzero(r_line,sizeof(r_line));
                 	if (!(fgets(r_line,100,infile)))
				break;

			if(DEBUG_LEVEL >= MSG_DEBUG)
		 		printf("SndcpGmm::init::get line:%s\n",r_line);
		 	tmp_store = strtok(r_line,"\t\n");
		 	sprintf(str[i],"%s",tmp_store);
		 	no_of_record = atoi(str[0]);
			for(j=0;j<no_of_record;j++)
			{
				i = 0;
		 		bzero(tmp_action,sizeof(tmp_action));
                 		bzero(r_line,sizeof(r_line));
                 		fgets(r_line,100,infile);
				if(DEBUG_LEVEL >= MSG_DEBUG)
		 			printf("SndcpGmm::init()::get line:%s\n",r_line);
		 		tmp_store = strtok(r_line,"\t\n");
		 		sprintf(str[i],"%s",tmp_store);
				//printf("SndcpGmm::time::%s\n",str[i]);
				 while(1)
		 		{
					i++;
					tmp_store = strtok(NULL,"\t\n");
                                        if (!tmp_store)
					    break;
					sprintf(str[i],"%s",tmp_store);

		 		};
				if(DEBUG_LEVEL >= MSG_DEBUG)	
					printf("time=%s\taction=%s\n",str[0],str[1]);
				tmp_time = atoi(str[0]);
    				TimerAction = (struct doingAction *)malloc(sizeof(struct doingAction));
				bzero(TimerAction,sizeof(doingAction));
				TimerAction->s_time = tmp_time;
				sprintf(TimerAction->action,"%s",str[1]);
				if(!strcmp(TimerAction->action,"QoS_Update"))
				{	
			
					update_nsapi = atoi(str[2]);
					update_qos = atoi(str[3]);
				}
    				TimerAction->next = TimerAction_head;
				TimerAction_head = TimerAction;
				
			}
			break;
		 }
		 
    	}
    	fclose(infile);
	free(action_FILEPATH);
	tmpAction = TimerAction_head;
    	/*set up timer for user-specified action*/
    	BASE_OBJTYPE(type);
	u_int64_t wait_interval;
    	while(tmpAction!=NULL)
    	{
		start_ep =  createEvent();
		if(DEBUG_LEVEL >= MSG_DEBUG)
    			printf("action=%s\n",tmpAction->action);
		if(strcmp(tmpAction->action,"Attach")==0)
    		{
			if(DEBUG_LEVEL >= MSG_DEBUG)
    					printf("Now we are at attach!!\n");
			SEC_TO_TICK(wait_interval, tmpAction->s_time); 
			type = POINTER_TO_MEMBER(SndcpGmm,gprs_imsi_attach_action);
                        setObjEvent(start_ep, wait_interval,0,this,type,(void *)NULL);

    		}
    		else if(strcmp(tmpAction->action,"Detach")==0)
    		{
    			SEC_TO_TICK(wait_interval, tmpAction->s_time); 
			type = POINTER_TO_MEMBER(SndcpGmm,MS_gprs_detach_action);
                        setObjEvent(start_ep,wait_interval,0,this,type,(void *)NULL);
    		}
    		else if(strcmp(tmpAction->action,"Association")==0)
    		{
    			SEC_TO_TICK(wait_interval, tmpAction->s_time); 
			type = POINTER_TO_MEMBER(SndcpGmm,activate_PDP_r_action);
			setObjEvent(start_ep,wait_interval,0,this,type,(void *)NULL);
			
    		}
    		else if(strcmp(tmpAction->action,"QoS_Update")==0)
    		{
    			SEC_TO_TICK(wait_interval, tmpAction->s_time); 
			type = POINTER_TO_MEMBER(SndcpGmm,update_PDP_r_action);
			setObjEvent(start_ep,wait_interval,0,this,type,(void *)NULL);
    		}
    		else if(strcmp(tmpAction->action,"Deassociation")==0)
    		{
    			SEC_TO_TICK(wait_interval, tmpAction->s_time); 
			type = POINTER_TO_MEMBER(SndcpGmm,deactivate_PDP_r_action);
			setObjEvent(start_ep,wait_interval,0,this,type,(void *)NULL);
    		}
    		else
		{
			if(DEBUG_LEVEL >= MSG_FATAL)
    				printf("SndcpGmm::init::There is no such command!!\n");
			assert(0);
		}
    		tmpAction = tmpAction->next;
    	}
    	/*free TimerAction_head space*/
    	tmpAction = TimerAction_head;
    	while(tmpAction!=NULL)
    	{
    		tmpAction1 = tmpAction->next;
		free(tmpAction);
		tmpAction = tmpAction1;
    	}
	/*initial gmm's init()*/
	gmm1->init();
	return(1);
}

inline ulong SndcpGmm::getMSISDN()
{
	return(msisdn1);
}
inline ulong SndcpGmm::getIMSI()
{
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("getIMSI=%ld\n",imsi1);
	return(imsi1);
}

int SndcpGmm::attach_acpt(struct attach_accept *atch_accept)
{
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct attach_accept *con_msg;
	ms_storage *ms1;
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("attach complete successfully!!\n");

	//now we are to stop the T3310 timer which was triggered by the time of attach
	T3310.cancel();
	
	//when we attach complete, we change status to ready
	status  = 2;//ready state 
	c_state = 2;//GMM-REGISTERED state
	
	/*give new ptmsi and ptmsi_sig for ms*/
	gmm1->setRAI(atch_accept->RAI);
	gmm1->change_ptmsi_sig(atch_accept->ptmsi,atch_accept->ptmsi_sig);

	/*we store TLLI to ms_storage structure*/
	ms1 = gmm1->get_ms_store();
	
	/*
	 * When the attach_request is accepted, the attempt_counter is to be setted to zero
	 */

	gmm1->reset_attempt_count();
	
	//Now we are in ready state and are going to set ready timer(T3314)
	
	BASE_OBJTYPE(type);
	u_int64_t wait_interval;
	SEC_TO_TICK(wait_interval, 44);   /* wait for 44 sec in ready state  */
	type = POINTER_TO_MEMBER(SndcpGmm,readyexpire);
	T3314.setCallOutObj(this, type);
	T3314.start(wait_interval, 0);
	
	/*
	 * Now we are in ready state, we set T3312 timer 
	 * which is periodic route area update timer
	 */

	SEC_TO_TICK(wait_interval, 2700);   /* wait for 45 min in ready state  */
	type = POINTER_TO_MEMBER(SndcpGmm,periodic);
	T3312.setCallOutObj(this, type);
	T3312.start(wait_interval, 2700);/*every 45 mins we trigger it again*/
	
	/*send echo msg back to SGSN*/
	ePacket_ *pkt1 = createEvent();
	struct bss_message *p1= new bss_message;
	pkt1->DataInfo_ = p1 ;
	bzero(p1,sizeof(struct bss_message));
	con_msg = (attach_accept *)malloc(sizeof(struct attach_accept));
	bzero(con_msg,sizeof(struct attach_accept));
	strcpy(con_msg->control,"attach_complete");
	con_msg->ptmsi = atch_accept->ptmsi;
	con_msg->ptmsi_sig = atch_accept->ptmsi_sig;
	con_msg->imsi = imsi1;
	strcpy(llcop->control,"LLGMM_ASSIGN");
	llcop->tlli = atch_accept->tlli;
	llcop->oldtlli = ms1->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
        llcop->req_bsic  = -1;
        llcop->req_rai   = -1;

	p1->user_data =(char *)con_msg;
	p1->user_data_len = sizeof(struct attach_accept);
	p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);

	p1->flag = PF_SEND;
	
	ms1->tlli = atch_accept->tlli;
        
	printf("SndcpGmm[%u]: sending accept completion control message. \n", get_nid());
	return(GprsObject::sendManage(pkt1));
		
}
/*int SndcpGmm::NK_detach_acpt(struct NK_detach_type *nk_detach)
{
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct detach_accept *nk_detach1;
	struct bss_message *p1= new bss_message;
        ePacket_ *pkt1 = createEvent();
        c_state = 0;//GMM-DEREGISTERED state
	//when we detach complete, we change status to idle
	status = 0;//idle state 

	if(nk_detach->type == 2) //pure Network detach without reattachment
	{
		bzero(p1,sizeof(struct bss_message));
        	pkt1->DataInfo_ = p1 ;
        	nk_detach1 = (detach_accept *)malloc(sizeof(struct detach_accept));
		bzero(nk_detach1,sizeof(struct detach_accept));
		strcpy(nk_detach1->control,"NK_detach_accept");
		nk_detach1->imsi = imsi1;
        	nk_detach1->ptmsi = nk_detach->ptmsi;
        	nk_detach1->ptmsi_sig = nk_detach->ptmsi_sig;
		nk_detach1->tlli = nk_detach->tlli;
		bzero(llcop,sizeof(LLCOption));
		strcpy(llcop->control,"LLGMM_TRIGGER");
                llcop->tlli = nk_detach->tlli;
		llcop->qos = 1;

                p1->llc_option = (char *)llcop;
                p1->llc_option_len = sizeof(struct LLCOption);
        	p1->user_data = (char *)nk_detach1;	
        	p1->user_data_len = sizeof(struct detach_accept);
        	p1->sndcp_header_len = 0; //this indicates that it is not for LLC 
		p1->flag = PF_SEND;
		return(GprsObject::sendManage(pkt1));
        }
        //freePacket(pkt);
	BASE_OBJTYPE(type);
	u_int64_t wait_interval;
	SEC_TO_TICK(wait_interval, 15);   //wait 15 sec for response
	type = POINTER_TO_MEMBER(SndcpGmm,mid_attach);
	T3310.setCallOutObj(this, type);
	T3310.start(wait_interval, 0);
	return(gmm1->attach());
}
*/
int SndcpGmm::route_area_update_acpt(struct route_accept *rt_accept)
{
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct bss_message *p1= new bss_message;
	struct return_msg *rt_msg=NULL;
	struct ms_storage *ms1=NULL;
	ePacket_ *pkt1=createEvent();
	bzero(p1,sizeof(struct bss_message));
	pkt1->DataInfo_ = p1;
	rt_msg = (struct return_msg *)malloc(sizeof(struct return_msg));
	bzero(rt_msg,sizeof(struct return_msg));
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SndcpGmm::route_area_update_acpt::route area update update accept successfully!!\n");
	//Now we are to change the ptmsi in MS
	gmm1->change_ptmsi_sig(rt_accept->ptmsi,rt_accept->ptmsi_sig);
	//when we obtain acceptence, we stop the route area update timer
	strcpy(rt_msg->control,"route_area_update_complete");
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
	ms1 = gmm1->get_ms_store();
        llcop->tlli = ms1->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


        p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)rt_msg;
	p1->user_data_len = sizeof(struct return_msg);
	p1->flag = PF_SEND;
	T3330.cancel();
	return(GprsObject::sendManage(pkt1));
}
int SndcpGmm::paging_reqst(struct Paging_ps *p_ps)
{
	/*we are send paging response msg to SGSN 
	and change MS's state to ready*/
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("***SndcpGmm::paging_reqst()::we are to send paging request to GprsGmm!!***\n");
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	struct Paging_resp *p_resp;
	const char *paging_r1 = "paging_resp";
	struct bss_message *p1= new bss_message;
       	ePacket_ *pkt1=createEvent();
	bzero(p1,sizeof(struct bss_message));
       	pkt1->DataInfo_ = p1;
       	
       	p_resp = (Paging_resp *)malloc(sizeof(struct Paging_resp));
	bzero(p_resp,sizeof(struct Paging_resp));
	sprintf(p_resp->control,"%s",paging_r1);
	p_resp->imsi =p_ps->imsi;
	p_resp->ptmsi = p_ps->ptmsi;
	p_resp->tlli = p_ps->tlli;
	bzero(llcop,sizeof(LLCOption));
	strcpy(llcop->control,"LLGMM_TRIGGER");
        llcop->tlli = p_ps->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;


        p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);
	p1->user_data = (char *)p_resp;
	p1->user_data_len = sizeof(struct Paging_resp);
	p1->sndcp_header_len =0 ; /*this indicates that it is not for LLC */
	p1->flag = PF_SEND;
	/*we are to change ms's status to ready*/
	status = 2;
	return(GprsObject::sendManage(pkt1));
}



int SndcpGmm::gprs_imsi_attach_action()
{
	/*
	we send attach request to SGSN'gmm and wait for its attach accept msg.
	at this time, we set T3310 timer and when it expires, 
	we retransmit the request until four times
	*/
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SndcpGmm::gprs_imsi_attach_action()::Now we are to do attach_action!!\n");
	BASE_OBJTYPE(type);
	u_int64_t wait_interval;
	SEC_TO_TICK(wait_interval, 15);   /* wait 15 sec for response */
	type = POINTER_TO_MEMBER(SndcpGmm,mid_attach);
	T3310.setCallOutObj(this, type);
	T3310.start(wait_interval, 0);
	return(gmm1->attach());	
}

int SndcpGmm::MS_gprs_detach_action()
{
	/*
	we send detach request to SGSN'gmm and wait for its detach accept msg.
	at this time, we set T3310 timer and when it expires, 
	we retransmit the request until four times
	*/
	const char *con_msg = "MS_gprs_detach";
	if(status==0)
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::MS_gprs_detach_action()::we have already in detach status so we don't have to do detach!!\n");
		return(0);
	}
	BASE_OBJTYPE(type);
	u_int64_t wait_interval;
	SEC_TO_TICK(wait_interval, 15);   /* wait 15 sec for response */
	type = POINTER_TO_MEMBER(SndcpGmm,mid_detach);
	T3321.setCallOutObj(this, type);
	T3321.start(wait_interval, 0);
	return(gmm1->detach(con_msg,1));
}
int SndcpGmm::activate_PDP_r_action()
{
	const char *msg="activate_PDP_r";
	if(status != 0)
		return(gmm1->activate_PDP_req(msg));
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SndcpGmm::activate_PDP_r_action()::MS is in idle mode and can't do activation!!\n");
	return 1;
}

inline int SndcpGmm::update_PDP_r_action()
{
	if(status!=0)
		return(gmm1->update_PDP_req(update_nsapi,update_qos));
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SndcpGmm::update_PDP_r_action()::we can't do update because we are in idle state!!\n");
	return 1;
}

inline int SndcpGmm::deactivate_PDP_r_action()
{
	if(status!=0)
		return(gmm1->deactivate_PDP_req());
	if(DEBUG_LEVEL >= MSG_DEBUG)
		printf("SndcpGmm::deactivate_PDP_r_action()::we can't do deactivation because we are in idle state!!\n");
	return 1;
}



int SndcpGmm::send(ePacket_ *pkt)
{

	Packet	*p;
	ip     *ip_header;
	tcphdr *tcp_header;
	udphdr *udp_header;
	struct ms_storage *ms1;

	assert(pkt&&pkt->DataInfo_);
	p = (Packet *)pkt->DataInfo_;
	ip_header = (ip *)p->pkt_sget();
	if(ip_header->ip_p == 6)
	{
		//TCP packet
		tcp_header = (tcphdr *)(p->pkt_sget() + sizeof(struct ip));
		nsapi = ntohs(tcp_header->th_dport);

	}
	if(ip_header->ip_p == 17)
	{
		//UDP packet
		udp_header = (udphdr *)(p->pkt_sget() + sizeof(struct ip));
		nsapi = ntohs(udp_header->uh_dport);

	}

	if(status == 0)
	{
		if(DEBUG_LEVEL >= MSG_ERROR)
			printf("we must do attach before send packet!!\n");
		freePacket(pkt);
		return(1);
	}

	/*we send normal packets*/
	T3314.cancel();
	//Now we are in ready state and are going to set ready timer(T3314)
	status = 2; //ready state
	BASE_OBJTYPE(type);
	u_int64_t wait_interval;
	SEC_TO_TICK(wait_interval, 44);   /* wait for 44 sec in ready state  */
	type = POINTER_TO_MEMBER(SndcpGmm,readyexpire);
	T3314.setCallOutObj(this, type);
	T3314.start(wait_interval, 0);

	ms1 = gmm1->get_ms_store();
	if(sndcp1->snunitdatareqst(pkt,nsapi,qos,ms1->tlli,head) != 1)
        {
         	//failure of sending packet
		freePacket(pkt);
                return(1);
        }

	pkt->DataInfo_ = NULL;
	freePacket(pkt);
	
	return(1);

}

int SndcpGmm::Detach_acpt()
{
	struct ms_storage *ms1;
	struct bss_message *p1= new bss_message;
	struct LLCOption *llcop = (LLCOption *)malloc(sizeof(LLCOption));
	ePacket_ *pkt1 = createEvent();
	/*
	now we are to stop the T3312 timer which was for periodic update
	*/
	T3312.cancel();
	//now we are to stop the T3310 timer which was triggered by the time of attach
	T3321.cancel();
	T3314.cancel();
	c_state = 0;//GMM-DEREGISTERED state
	//when we detach complete, we change status to idle
	status = 0;//idle state
	
	ms1 = gmm1->get_ms_store();
	
	/*
	when the attach_request is accepted, the attempt_counter is to be setted to zero
	*/
	gmm1->reset_attempt_count();

        pkt1->DataInfo_ = p1 ;
	bzero(p1,sizeof(struct bss_message));
	ms1 = gmm1->get_ms_store();
	strcpy(llcop->control,"LLGMM_RESET");
        llcop->oldtlli = 0xffffffff;
        llcop->tlli = ms1->tlli;
	llcop->qos = 1;
	llcop->req_ch_no = -1;
    llcop->req_bsic  = -1;
    llcop->req_rai   = -1;

        p1->llc_option = (char *)llcop;
        p1->llc_option_len = sizeof(struct LLCOption);
	p1->flag = PF_SEND;
	/*set MS's TLLI to zero*/
	ms1->tlli = 0;
	/*for temp*/
	//return(1);
	return(GprsObject::sendManage(pkt1));
	//return(GprsObject::put(pkt1,sendtarget_));

}


int SndcpGmm::recv(ePacket_ *pkt)
{ 	
	struct bss_message *p;

	struct link_status *path,*prev;
	struct Paging_ps *msg=NULL;
        assert(pkt&&pkt->DataInfo_);
	p = (bss_message *)pkt->DataInfo_;
	msg = (struct Paging_ps *)p->user_data;
	path = head;
	prev = head;

	//control packet

	if(!strcmp(msg->control ,"modify_PDP_r"))
	{
		return(gmm1->Modify_PDP_req(pkt));
	}
	/*else if(!strcmp(msg->control , "NK_activate_PDP_r"))
	{
		return(gmm1->NK_activate_PDP_req(pkt));
	}
	else if(!strcmp(msg->control , "NK_gprs_detach_req"))
	{
		struct NK_detach_type *NK_d_type=NULL;
		NK_d_type = (struct NK_detach_type *)p->user_data;
		//for SGSN-initiated detach request
		if(NK_d_type->type ==3)
			gmm1->detach("NK_gprs_detach",3); //reattach after we complete the detach
		else
			gmm1->detach("NK_gprs_detach",2);
		gmm1->reset_attempt_count();
		return(1);

	}
	else if(!strcmp(msg->control , "NK_detach_accept"))
	{
		//control_msg	*msg3;
		int echomsg;
		struct NK_detach_type *nk_detach=NULL;
		nk_detach = (struct NK_detach_type *)p->user_data;
		echomsg = NK_detach_acpt(nk_detach);
		freeBss_message(p);
		free(pkt);
		return(echomsg);        	

	}
	else if(!strcmp(msg->control , "NK_detach_not_accept"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::Network detach is not accepted!!\n");
		freeBss_message(p);
		free(pkt);
        	return(1);
	}*/
	else if(!strcmp(msg->control , "activate_PDP_accept"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::Activation of the PDP context request successfully\n");
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control , "activate_accept_qos_n"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
		{
			printf("SndcpGmm::Activation of the PDP context request successfully\n");
			printf("SndcpGmm::but your requested qos is not equal to APN's\n");
		}
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control , "route_area_update_accept"))
	{
		
		struct route_accept *rt_accept=NULL;
		int echomsg;
		rt_accept = (struct route_accept *)p->user_data;
		echomsg = route_area_update_acpt(rt_accept);
		freeBss_message(p);
		free(pkt);
		return(echomsg);
	}
	else if(!strcmp(msg->control , "route_area_update_rq"))
	{
		
		if(status != 0)
		{

			#ifdef __JCLIN_DISABLE__
			BASE_OBJTYPE(type);
			u_int64_t wait_interval;
			SEC_TO_TICK(wait_interval, 15);   /* wait for 15 sec*/
			type = POINTER_TO_MEMBER(SndcpGmm,updateexpire);
			T3330.setCallOutObj(this, type);
			T3330.start(wait_interval, 0);
			#endif
			return(gmm1->route_area_update_req(pkt));
		}

		if(DEBUG_LEVEL >= MSG_DEBUG)
		    printf("SndcpGmm::recv::we are in detach state so we don't need to do roamming!!\n");
		
		
		/* added by jclin on 12/29 for freeing the memory used by preferred list clearly. */		

		struct  preferred_list* msg = (struct preferred_list*) (p->sndcp_option);

    		int no_of_list = (int) (msg->num_of_list);


		/* added by jclin for free preferred_list */
                bss_info* elem      = msg->list_head_p;
                bss_info* elem_next = msg->list_head_p;
                for ( int i=0 ; i<no_of_list ; ++i) {

                    elem = elem_next;

                    if ( elem ) {

                        elem_next = elem->next;
                        delete elem;

                    }

                }

                free(msg);
                p->sndcp_option     = NULL;
                p->sndcp_option_len = 0;


		
		freeBss_message(p);
		free(pkt);
		return 1;

	}
	else if(!strcmp(msg->control , "deactivate_PDP_r_s"))
	{
		//MS receives the deactivate PDP context request accept msg
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::deactivate PDP context request accept\n");
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control ,"update_PDP_r_s"))
	{
		//MS receives the update PDP context request accept msg
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::update PDP context request successfully\n");
		ms_storage *ms1;
		struct activate_type *update_msg=NULL;
		update_msg = (struct activate_type *)p->user_data;
		ms1 = gmm1->get_ms_store();
		ms1->qos = update_msg->qos;
		ms1->nsapi = update_msg->nsapi;
		nsapi = update_msg->nsapi;
		qos = update_msg->qos;
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control,"update_PDP_r_f"))
	{
		//MS receives the update PDP context request accept msg
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::update PDP context request reject\n");
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control,"activate_error_ip"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::activate was rejected by SGSN because the specified ggsn IP is errorous!!\n");
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control,"sgsn_context_r_f"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::iner_route_area fails because of sgsn_context_req_fail!!\n");
		freeBss_message(p);
		free(pkt);
		return(1);
	}
	else if(!strcmp(msg->control , "detach_accept"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::detach complete successfully!!\n");
		freeBss_message(p);
		free(pkt);
		return(Detach_acpt());
	}
	else if(!strcmp(msg->control,"attach_accept"))
	{
		int echomsg=0;
		struct attach_accept *atch_accept=NULL;
		atch_accept = (struct attach_accept *)p->user_data;
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::attach_accepted by MS side!!\n");
		echomsg = attach_acpt(atch_accept);
		freeBss_message(p);
		free(pkt);
		return(echomsg);
	}
	else if(!strcmp(msg->control,"iden_req"))
	{
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("SndcpGmm::we didn't find imsi so we send MS's imsi to SGSN!!\n");
		int echomsg;
		echomsg = gmm1->iden_reqst();
		freeBss_message(p);
		free(pkt);
		return(echomsg);
	}
	else if(!strcmp(msg->control ,"paging_req"))
	{
		int echomsg;
		if(DEBUG_LEVEL >= MSG_DEBUG)
			printf("***SndcpGmm::we got paging request!!***\n");
		struct Paging_ps *p_ps=NULL; 	
       		p_ps = (struct Paging_ps *)p->user_data;
		echomsg = paging_reqst(p_ps);
		freeBss_message(p);
		free(pkt);
		return(echomsg);
	}
	else 
	{
                Packet  *p1;
                //struct bss_message *p2;
                //p2 = (bss_message *)pkt->DataInfo_;
                p1 = (Packet *)p->packet;
                assert(p1);
                if(p1)
                {
                        pkt->DataInfo_ = p1;
                        p1->pkt_setflow(PF_RECV);
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

                }
		if(DEBUG_LEVEL >= MSG_DEBUG)
                	printf("SndcpGmm::receive pure data:: %p\n",p1->pkt_sget());
                return(NslObject::recv(pkt));

	}
	return 1;
	        
 }
