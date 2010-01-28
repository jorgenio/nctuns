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

#include <assert.h>
#include <stdlib.h>
#include <object.h>

#include <config.h>
#include <maptable.h>
#include <mbinder.h>
#include <nctuns_api.h>
#include <nodetype.h>
#include <packet.h>
#include <regcom.h>
#include "GprsSw.h"

extern RegTable                 RegTable_;
extern HLR                 HLR_;

MODULE_GENERATOR(GprsSw);

GprsSw::GprsSw(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name), sgsnport(0), PortList(NULL), num_port(0), RAI(0), VLR_table_h(NULL)
{
	/* relay should not buffer any pkt in s_queue or r_queue */
        s_flowctl = DISABLED;
        r_flowctl = DISABLED;
        vBind("rai",&tmp_RAI);

}


GprsSw::~GprsSw() {
	RelayPort               *tmp_port_p,*tmp_port;
	VLR			*tmp_vlr_p,*tmp_vlr;
	/*free PortList*/
	tmp_port_p = PortList;
	while(tmp_port_p != NULL)
	{
		tmp_port = tmp_port_p;
		tmp_port_p = tmp_port_p->nextport;
		free(tmp_port);	
	}
	//free(VLR_table);
	tmp_vlr_p = VLR_table_h;
	while(tmp_vlr_p != NULL)
	{
		tmp_vlr = tmp_vlr_p;
		tmp_vlr_p = tmp_vlr_p->next;
		free(tmp_vlr);
	}
	//free(PortList);

}

int GprsSw::init() {

	RAI = atol(tmp_RAI);
	/*I use the setObjEvent to register RAI for instance lookup because
	if I don't adopt this way, there is an error occurs in the engine
	*/
	BASE_OBJTYPE(type);
        u_int64_t wait_interval;
	//printf("RAI at sw=%ld\n",RAI);
	Event_ *start_ep =  createEvent();
	SEC_TO_TICK(wait_interval, 0);
        type =POINTER_TO_MEMBER(GprsSw,initial_maptable);
        setObjEvent(start_ep,wait_interval,0,this,type,(void *)NULL);
	//printf("sw module:%d\n",this);
	return(1);  
}
int GprsSw::initial_maptable()
{
	umtbl_add(get_nid(),get_port(),0,&RAI,0,0);
	return(1);
}
/* main behaviors of relay are in send function */
ulong GprsSw::getSGSNIP()
{
	return(RAI);
}
int GprsSw::send(ePacket_ *pkt, MBinder *frm) {

        ePacket_ 		*pkt_;
	u_int32_t		i;
	int			found = 0,insgsn=0;
	VLR			*temp_VLR=NULL,*temp_VLR1=NULL;
	RelayPort		*tmpRelayPort=NULL;
	control_msg *msg=NULL;
	Packet *p;
	//printf("we are to send packet at sw side!!\n");
	//printf("send side relayport:%d\n",PortList->portNum);
	p = (Packet *)pkt->DataInfo_;
	msg = (struct control_msg *)p->pkt_getinfo("flag");
	//printf("CMCHOU:: we are to send packet at sw side %s!!\n",msg->control);
	if(!strcmp(msg->control , "delete_PDP_r") || !strcmp(msg->control,"indi_reqst")
	||!strcmp(msg->control,"update_location_req") || !strcmp(msg->control , "sgsn_context_r")
	||!strcmp(msg->control,"NK_create_PDP_r") ||!strcmp(msg->control ,  "create_PDP_r")
	||!strcmp(msg->control,"deactivate_PDP_r") ||!strcmp(msg->control , "update_PDP_r")
	||!strcmp(msg->control,"sgsn_context_r") ||!strcmp(msg->control , "sgsn_update_PDP_r")
    ||!strcmp(msg->control, "pure_data") )
	{
                //printf("packet_ind_str = %s \n" , msg->control );
		
		for ( i = 1, tmpRelayPort = PortList;
	      		
			tmpRelayPort != NULL && i <= num_port;
	      		tmpRelayPort = tmpRelayPort->nextport, i++ ) {
	      		
			if(tmpRelayPort->port && 
			    (frm->myModule() == tmpRelayPort->port->bindModule()))
	      		{
	      			temp_VLR = VLR_table_h;
				while(temp_VLR != NULL)
				{
					if(temp_VLR->imsi == msg->imsi || temp_VLR->ptmsi == msg->ptmsi)
					{
						//if(temp_VLR->bssidNum == 0)
						temp_VLR->bssidNum = tmpRelayPort->portNum;
						found = 1;
						break;
					}
					temp_VLR = temp_VLR->next;
				}
				if(found == 0)
				{
                			temp_VLR1 = (struct VLR *)malloc(sizeof(VLR));
					bzero(temp_VLR1,sizeof(struct VLR));
					temp_VLR1->imsi = msg->imsi;
					temp_VLR1->ptmsi = msg->ptmsi;
					temp_VLR1->ptmsi_sig = msg->ptmsi_sig;
					temp_VLR1->bssidNum = tmpRelayPort->portNum;
                			temp_VLR1->next = VLR_table_h;
					VLR_table_h = temp_VLR1;

				}
	      		}
		}
	}
	for ( i = 1, tmpRelayPort = PortList;
	      tmpRelayPort != NULL && i <= num_port;
	      tmpRelayPort = tmpRelayPort->nextport, i++ ) {

		//printf("module1:%d module2:%d\n",frm->myModule(),tmpRelayPort->port->bindModule());
	      	//printf("!!port=%d\n",tmpRelayPort->portNum);
		
		if(tmpRelayPort->port && 
		       tmpRelayPort->portNum == sgsnport && 
		            (frm->myModule() != tmpRelayPort->port->bindModule())) {

		        /* printf("now send packet!! port=%d\n",tmpRelayPort->portNum); */
	      		pkt_ = pkt_copy(pkt);
			put(pkt_, tmpRelayPort->port);
			break;
	      	}
	      	if(tmpRelayPort->port && tmpRelayPort->portNum == sgsnport && (frm->myModule() == tmpRelayPort->port->bindModule()))
	      	{
	      		/*this means we from the SGSN side and we want to send packet to the BSS*/
			//printf("we are to send packet to GprsGmm side!!\n");
	      		temp_VLR = VLR_table_h;
			while(temp_VLR != NULL)
			{
				//printf("temp_VLR imsi=%ld imsi=%ld\n",temp_VLR->imsi,msg->imsi);
				if(temp_VLR->imsi == msg->imsi || temp_VLR->ptmsi == msg->ptmsi)
				{
					for ( i = 1, tmpRelayPort = PortList;
	      				tmpRelayPort != NULL && i <= num_port;
	     	 			tmpRelayPort = tmpRelayPort->nextport, i++ ) {
	     	 				if(tmpRelayPort->port && tmpRelayPort->portNum == temp_VLR->bssidNum)
	     	 				{
	     	 					#ifdef __RELAY_PORT_DEBUG
							printf("GPRS SW::send(): relay port=%d\n",tmpRelayPort->portNum);
							#endif

	     	 					pkt_ = pkt_copy(pkt);
							put(pkt_, tmpRelayPort->port);
							insgsn = 1;
							break;
	     	 				}
	     	 			}
					break;
				}
				temp_VLR = temp_VLR->next;
			}
			if(!insgsn)
			{
				//printf("MS is not yet in this SGSN or not attach yet!!\n");
				freePacket(pkt);
				return(1);
			}
			break;

	      	}
	}
	//printf("we have put packet to GprsGmm at sw!!\n");
	freePacket(pkt);
        return(1);
}


int GprsSw::recv(ePacket_ *pkt) {
	return(1);

}


int GprsSw::get(ePacket_ *pkt, MBinder *frm) {

	/* change pkt flow to SEND_FLOW since relay is the top module */
	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	pkt_->pkt_setflow(PF_SEND);
	//printf("get packet!!\n");
	return send(pkt, frm);
}





int GprsSw::command(int argc, const char *argv[]) {

    NslObject                       *obj;


    if (argc != 4) {
            //printf("Syntax:  Set variable = value\n\n");
            return(-1);
    }

    bool uncanonical_module_name_flag = 0;

    /* The "." sign is not allowed as part of a module name. */
    if ( strstr(argv[3],".") ) {
        uncanonical_module_name_flag = 1;
    }
    /* The RegTable->lookup_Instance() with two parameter version
    * should use the canonical name of a module. If an illegal
    * module name is found, we should return immediately.
    * A canonical module name should start with
    * a prefix "NODE." As such, for any module names
    * without this prefix, this function simply returns
    * the original string as its output because the
    * name translation process may fail with incorrect
    * input names.
    */
    if (strncasecmp("NODE", argv[3], 4))
        uncanonical_module_name_flag = 1;

    /* A canonical module name should start with
    * a prefix "NODE." As such, for any module names
    * without this prefix, this function simply returns
    * the original string as its output because the
    * name translation process may fail with incorrect
    * input names.
    */

    /* Connectivity */
    if (!uncanonical_module_name_flag) {
        obj = RegTable_.lookup_Instance(get_nid(), argv[3]);
        if (!obj) {
                printf("No %s this Instance!\n\n", argv[3]);
                return(-1);
        }
    }
    printf("GprsSW::command(): perform command: %s %s %s %s.\n",argv[0],argv[1],argv[2],argv[3]);
    /* support port should be added here */
    MBinder			*tmpMBinder;
    RelayPort		*tmpRelayPort;
    u_int32_t		portNum;
    const char		*mname;
    if (!strncmp(argv[1], "port", 4)) {
        //if(!strncmp(argv[1], "BSSID", 5))
            sscanf(argv[1], "port%d", &portNum);
        //else
        //	portNum = 8; //this means we relay packets from BSSID1~7 to SGSN
        num_port++;
        //printf("port num=%d oldport=%d newport=%d\n",num_port,lastNum,portNum);
        tmpMBinder = new MBinder(this);
        assert(tmpMBinder);
        tmpMBinder->bind_to(obj);
        mname = tmpMBinder->bindModule()->get_name();
        if(strstr(mname,"SGSNGtp")!=NULL)
        {
            sgsnport = portNum;
        }
        tmpRelayPort = (RelayPort *)malloc(sizeof(struct RelayPort));
        bzero(tmpRelayPort,sizeof(struct RelayPort));
        tmpRelayPort->portNum = (u_int8_t)portNum;
        tmpRelayPort->port = tmpMBinder;
        tmpRelayPort->nextport = PortList;
        PortList = tmpRelayPort;
    }
    else if (!strcmp(argv[1], "sendtarget"))
        sendtarget_->bind_to(obj);
    else if(!strcmp(argv[1], "recvtarget"))
        recvtarget_->bind_to(obj);
    else {
        printf("GprsSw::command(): Invalid command: %s %s %s %s.\n",argv[0],argv[1],argv[2],argv[3]);
        return(-1);
    }
    return(1);
}

int GprsSw::update_VLR(ms_storage *msg)
{
	
	VLR *temp_VLR,*temp_VLR1;
	temp_VLR = VLR_table_h;
	int found=0;
	//printf("update VLR record!!\n");
	while(temp_VLR != NULL)
	{
		if(temp_VLR->imsi == msg->imsi)
		{
			temp_VLR->RAI = RAI;
			temp_VLR->CI = msg->CI;
			temp_VLR->bssidNum = 0;
			found=1;
			break;
		}
		temp_VLR = temp_VLR->next;
	}
	if(found==0)
	{
		//printf("NULL\n");
		temp_VLR1 = (struct VLR *)malloc(sizeof(VLR));
		bzero(temp_VLR1,sizeof(struct VLR));
		temp_VLR1->imsi = msg->imsi;
		temp_VLR1->RAI = RAI;
		temp_VLR1->CI = msg->CI;
		temp_VLR1->bssidNum = 0;
		temp_VLR1->next = VLR_table_h;
		VLR_table_h = temp_VLR1;
	}
	//printf("update_VLR!!\n");
	//printf("imsi=%ld RAI=%ld\n",msg->imsi,RAI);
	return(1);
}

int GprsSw::cancel_old_VLR_rd(ulong ip,control_msg *msg)
{
	NslObject *peer;
	int ret;
	//printf("cancel old VLR record!!\n");
	peer = InstanceLookup(ip,"GprsSw");					
        ret = ((GprsSw *)peer)->Delete_old_VLR_rd(msg->imsi);
        //close(s);
        if(ret<0)
        printf("fail of deleting old VLR record!!\n");	
        return(ret);
        
}

int GprsSw::Location_Update_req(route_msg *msg)
{
	VLR *temp_VLR=NULL;
	struct HLR_record *temp_HLR_r = (HLR_record *) malloc(sizeof(struct HLR_record));
	int	found=0;
	bzero(temp_HLR_r,sizeof(struct HLR_record));
	temp_VLR = VLR_table_h;
	//printf("Location update request in VLR!!\n");
	while(temp_VLR != NULL)
	{
		if(temp_VLR->imsi == msg->imsi)
		{
			//update location of VLR
			if(msg->RAI!=0)
				temp_VLR->RAI = msg->RAI;
			temp_VLR->CI = msg->CI;
			temp_VLR->ptmsi = msg->ptmsi;
			temp_VLR->ptmsi_sig = msg->ptmsi_sig;
			//temp_VLR->bssidNum = msg->port_id;
			found =1;
			//update location of HLR
			temp_HLR_r->imsi = msg->imsi;
			temp_HLR_r->ptmsi = msg->ptmsi;
			//temp_HLR_r->ca = msg->CI;
			HLR_.modify_record(temp_HLR_r);
			free(temp_HLR_r);
			break;
		}
		temp_VLR = temp_VLR->next;
	}
	return(found);
	
}
int GprsSw::Delete_old_VLR_rd(ulong imsi)
{
	int found = 0,find_record=0;
	//uchar alen;
	VLR *temp_VLR=NULL,*cur_VLR=NULL,*next_VLR=NULL,*pre_VLR=NULL;
	pre_VLR = VLR_table_h;
	temp_VLR = VLR_table_h;
	//printf("Delete old VLR record!!\n");
	while(temp_VLR!=NULL)
	{
		next_VLR = temp_VLR->next;
		if(temp_VLR->imsi == imsi)
		{
			found = 1;
			/*Delete the specified VLR record*/
			cur_VLR = temp_VLR;
			if(cur_VLR == VLR_table_h)
				VLR_table_h = VLR_table_h->next;
			pre_VLR->next = next_VLR;
			free(cur_VLR);
			find_record = 1;
		}
		if(!find_record)
			pre_VLR = temp_VLR;
		temp_VLR = next_VLR;
		find_record =0;
	}
	if(found)
		return(1);
	return(-1);
	
}
