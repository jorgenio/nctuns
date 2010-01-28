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

#ifndef __GMMAG_h__
#define __GMMAG_h__
#include <gprs/GtpGmm/HLR/HLR.h>

extern HLR                 HLR_;

int SgsnGtp::sendpacket(ePacket_ *pkt)
{
	Packet *p=NULL;
	struct control_msg *msg=NULL;
	p = (Packet *)pkt->DataInfo_;
	msg = (struct control_msg *)p->pkt_getinfo("flag");
	recvlog(pkt,msg->delay_time,band_width);
	return(sendManage(pkt));
}
ulong SgsnGtp::Indi_resp_b(control_msg *msg)
{
	return(HLR_.search_imsi(&msg->ptmsi));

}
ulong SgsnGtp::Indi_req(control_msg *msg)
{
	//struct sockaddr_in sin;
	//int rt,s,retcode,peers;
	ulong imsi;
	NslObject* peer;
	peer = InstanceLookup(msg->TEID,"SGSNGtp");
        imsi = ((SgsnGtp*)peer)->Indi_resp_b(msg);
	if(imsi == 0)
	{  	
		//printf("SgsnGtp::IMSI not known!!\n");
		
		 // If the MS is unknown in both the old and new SGSN, 
		 // the SGSN sends an Identity Request to the MS
		return(0);              
	}
	else
		//printf("SgsnGtp::Indi_req Request Accepted!!\n");
	return(imsi);

}
SGSN_table *SgsnGtp::SGSN_context_resp_b(route_msg *msg)
{
	int found =0;
	struct SGSN_table *r_table=NULL,*s_table=NULL;
	s_table = sgsn_head_t;
	r_table = (struct SGSN_table *)malloc(sizeof(SGSN_table));
	bzero(r_table,sizeof(struct SGSN_table));
	/*
	 *Now we search its PDP context in SGSN table 
	 and connect to its GGSN to change its record in GGSN table
	*/
	while(s_table!=NULL)
        {
                if(s_table->imsi==msg->imsi)
                {
			r_table->imsi =s_table->imsi;
			r_table->TLLI =s_table->TLLI;
			r_table->NSAPI = s_table->NSAPI;
			r_table->TID = s_table->TID;
			r_table->GGSNIP = s_table->GGSNIP;
			r_table->qos =s_table->qos;
			found = 1;
			break;		
			
                }
                s_table = s_table->next;
        }
        if(found)
        {
        	/*we indeed found its SGSN record in SGSN table 
        	 *and now return this record
        	 */
        	 //printf("SgsnGtp::we found this SGSN record in SGSN table!!\n");
        	 return(r_table);
        }
        //printf("SgsnGtp::we don't found this SGSN record in SGSN table!!\n");
        r_table->imsi = 0;//this indicates that we didn't find this record
        return(r_table);
	
}

int SgsnGtp::SGSN_context_req(ePacket_ *pkt)
{
	/*
	we use ptmsi as the key to search for PDP context in old SGSN table
	*/
	Packet *p;
	struct route_msg *msg=NULL;
	//struct sockaddr_in sin;
	struct SGSN_table *s_table=NULL,*tmp_table=NULL,*current_table=NULL;
	int retcode=0;
	int peers,found = 0;
	//int rt,s,retcode,peers;
	//struct return_msg *r_msg;				    
	NslObject* peer;
	peers = -1;
	p = (Packet *)pkt->DataInfo_;
	msg = (struct route_msg *)p->pkt_getinfo("flag");
	
	peer = InstanceLookup(msg->TEID,"SGSNGtp");
        s_table = ((SgsnGtp*)peer)->SGSN_context_resp_b(msg);
        current_table = sgsn_head_t;
        if(s_table->imsi > 0)
        {
        	/*
        	*we obtain old SGSN's record successfully
        	*and now we are to connect to GGSN and change its record
        	*/
        	while(current_table != NULL)
        	{
        		if(current_table->imsi == s_table->imsi)
        		{
        			current_table->ptmsi = msg->ptmsi;
        			current_table->TLLI = s_table->TLLI;
        			current_table->NSAPI = s_table->NSAPI;
        			current_table->TID = s_table->TID;
        			current_table->GGSNIP = s_table->GGSNIP;
        			current_table->qos = s_table->qos;
        			found = 1;
        			break;
        		}
        		current_table = current_table->next;
        	}
        	if(!found)
      		{
        		tmp_table = (SGSN_table *)malloc(sizeof(struct SGSN_table));
        		bzero(tmp_table,sizeof(struct SGSN_table));
			tmp_table->ptmsi = msg->ptmsi;
        		tmp_table->imsi = s_table->imsi;
        		tmp_table->TLLI = s_table->TLLI;
        		tmp_table->NSAPI = s_table->NSAPI;
        		tmp_table->TID = s_table->TID;
        		tmp_table->GGSNIP = s_table->GGSNIP;
        		tmp_table->qos = s_table->qos;
        		tmp_table->next = sgsn_head_t;
        		sgsn_head_t = tmp_table;
        	}	
        	/*
        	 Now we connect to GGSN to modify its record, and 
        	 this is Update PDP context Request
        	*/
		peer = InstanceLookup(s_table->GGSNIP,"GGSNGtp");
       		retcode = ((GgsnGtp*)peer)->GGSN_context_resp_b(RAI,s_table);	
       		free(s_table);
                return(retcode);
        	
        }
        //this indicates that we didn't find this record
        free(s_table);
	return(-1);
       
        
        
	
}

int SgsnGtp::note_MS_GPRS_pt_req_b(ip_array *ip_list)
{
	/*
	receive the message and generate a new control msg to tell the MS to
	re-attach again
	*/	
	struct control_msg *msg = (control_msg *)malloc(sizeof(struct control_msg));	 
	Packet *p= new Packet;
	const char *tmp1="flag";
	char *tmp2;
	ePacket_ *pkt=createEvent();
	pkt->DataInfo_ = p;
	bzero(msg,sizeof(struct control_msg));
	//fill in msg into control message and send it to MS to revoke it
	strcpy(msg->control,"note_MS_prest_r");
	msg->imsi = ip_list->imsi;

	tmp2 = (char *)msg;
	//add control msg in PT_INFO
	p->pkt_addinfo(tmp1,tmp2,sizeof(control_msg));
	free(msg);
	return(NslObject::recv(pkt));
}
int SgsnGtp::update_location_resp_b(control_msg *msg)
{
	Packet *p1= new Packet;
	ePacket_ *pkt1=createEvent();
	pkt1->DataInfo_ = p1;
	const char *tmp1="flag";
	char *tmp2;
	tmp2 = (char *)msg;
	p1->pkt_addinfo(tmp1,tmp2,sizeof(control_msg));	
	if(Delete_PDP_req(pkt1)>0)
	{	
		freePacket(pkt1);
		return(1);	
	}
	assert(0);		
	return 1;
}

int SgsnGtp::update_location(control_msg *msg)
{
	//first we connect to peer side and see if we there are any active PDP context 
	//if indeed exists one , we delete it from SGSN table and GGSN table
	//struct sockaddr_in sin;
	int peers;
	
	NslObject* peer;
	peer = InstanceLookup(msg->TEID,"SGSNGtp");
	peers = ((SgsnGtp*)peer)->update_location_resp_b(msg);
	return(peers);	 	
	
}
#endif
