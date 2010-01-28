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
#include <assert.h>
#include <object.h>
#include <packet.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <ethernet.h>
#include <ip.h>
#include <mac-802_11.h>
#include <mobile/mobilenode.h>
#include <mbinder.h>

MODULE_GENERATOR(MoblNode);

MoblNode::MoblNode(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
	: NslObject(type, id, pl, name)
{
        manage_buf_head = manage_buf_tail = 0;
        ManageBuf_len = 0;	
	DataBuf_len = 0;
	DataBuf = NULL;
	comm_status = NO_STATUS;
	scan_mode = NOT_SET_SCAN;

	vBind("OperationMode", &Operation);
	vBind("ScanMode", &Scan_Mode);
}

MoblNode::~MoblNode(){}


int MoblNode::init() {

	u_char		*tmp_mac;
	int		i;

        NslObject::init();

	action = NO_ACTION;
	power_status = POWER_UP;
	checkAPconn_flag = 0;
	ap_index = 0;
	for( i = 1; i <= CHANNEL_NUM; i++ )
		ap_table[i] = NULL;
	
	using_chanl = GET_REG_VAR(get_port(), "CHANNEL", int *);
	assert(using_chanl);
	tmp_mac = GET_REG_VAR(get_port(), "MAC", u_char *);
	assert(tmp_mac);
	memcpy(mac_,tmp_mac,6);

	/* set some variables' initial values */
	if(Operation && !strcmp(Operation,"AD_HOC"))
		comm_status = AD_HOC;
	else if(Operation && !strcmp(Operation,"INFRASTRUCTURE"))
		comm_status = INFRASTRUCTURE;

	if(Scan_Mode && !strcmp(Scan_Mode,"ACTIVE_SCAN")) {
		scan_mode = ACTIVE_SCAN;
		original_scan = ACTIVE_SCAN;
	}
	else if(Scan_Mode && !strcmp(Scan_Mode,"PASSIVE_SCAN")) {
		scan_mode = PASSIVE_SCAN;
		original_scan = PASSIVE_SCAN;
	}

		
	/* Set the default values for the comm_status and scan_mode 
	   if these two variables are not set regular values */

	if(comm_status == NO_STATUS)
		comm_status = AD_HOC;
	else if(comm_status == INFRASTRUCTURE && scan_mode == NOT_SET_SCAN)
		scan_mode = ACTIVE_SCAN;

	
	if(comm_status == AD_HOC){
		action = DATA_FLOW;
	}
	else if(comm_status == INFRASTRUCTURE && scan_mode == ACTIVE_SCAN){
		*using_chanl = 0;
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(MoblNode,ActiveChanlScan);
		ActiveScanTimer.setCallOutObj(this, type);
		ActiveScanTimer.start(1,0);
	}
	else if(comm_status == INFRASTRUCTURE && scan_mode == PASSIVE_SCAN){
		*using_chanl = 0;
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(MoblNode,PassiveChanlScan);
		PassiveScanTimer.setCallOutObj(this, type);
		PassiveScanTimer.start(1,0);
	}
        return(1);
}

int MoblNode::recv(ePacket_ *pkt) {

	Packet			*pkt_;
	hdr_mac802_11		*mac_hdr;

	assert(pkt);
	assert(pkt->DataInfo_);

	pkt_ = (Packet *)pkt->DataInfo_;
	mac_hdr = (struct hdr_mac802_11 *)pkt_->pkt_get();

	if( comm_status == AD_HOC ) {
		if( mac_hdr->dh_fc.fc_from_ds != BIT_0 ||
			mac_hdr->dh_fc.fc_to_ds != BIT_0) {
			freePacket(pkt);
			return(1);
		}
		
		/* 
		 * In current implementation, no management packet is issued
                 * in ad hoc mode. Therefore, no corresponding procedure is
                 * implemented for dealing with this kind of packets. They 
                 * are blocked here.
		 */
		if( mac_hdr->dh_fc.fc_type == MAC_Type_Management ) {
			freePacket(pkt);
			return(1);
		}
	}
	else if( comm_status == INFRASTRUCTURE ) {
		if( mac_hdr->dh_fc.fc_from_ds == BIT_1 &&
		    mac_hdr->dh_fc.fc_to_ds == BIT_0 &&
		    mac_hdr->dh_fc.fc_type != MAC_Type_Data ) {
			freePacket(pkt);
			return(1);
		}
		else if( mac_hdr->dh_fc.fc_from_ds == BIT_0 &&
			 mac_hdr->dh_fc.fc_to_ds == BIT_0 &&
			 mac_hdr->dh_fc.fc_type != MAC_Type_Management ) {
				freePacket(pkt);
				return(1);
		}
		else if( mac_hdr->dh_fc.fc_from_ds == BIT_0 &&
			 mac_hdr->dh_fc.fc_to_ds == BIT_0 &&
			 mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Probe_Req ) {
				freePacket(pkt);
				return(1);
		}
	}

	if(mac_hdr->dh_fc.fc_type == MAC_Type_Management){
		if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Probe_Resp){
			FindAccessPoint(pkt);
			return(1);
		}
		else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Asso_Resp){
			action = DATA_FLOW;

			u_int64_t period;
			MILLI_TO_TICK(period,500);

			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(MoblNode,ReAssociation);
			ReAssoTimer.setCallOutObj(this, type);
			ReAssoTimer.start(period,0);

			freePacket(pkt);
			return(1);
		}
		else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_ReAsso_Resp){
			action = DATA_FLOW;

			freePacket(pkt);
			return(1);
		}
		else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_DisAsso){
			action = NO_ACTION;

			freePacket(pkt);
			return(1);
		}
		else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Beacon){
			if(comm_status == INFRASTRUCTURE && 
				original_scan == ACTIVE_SCAN &&
				action != ACT_CHANNEL_SCAN)
					ProcessBeacon(pkt);
			else if(comm_status == INFRASTRUCTURE && 
				original_scan == PASSIVE_SCAN &&
				action != PAS_CHANNEL_SCAN)
					ProcessBeacon(pkt);
			else if(comm_status == INFRASTRUCTURE &&
				action == PAS_CHANNEL_SCAN)
					ProcessBeaconDuringPassiveScan(pkt);			
			else {
				freePacket(pkt);
			}
               		return(1);
        	}
		else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_ApInfo){
			if(action != DATA_FLOW){
				freePacket(pkt);
				return(1);
			}
			return(1);
		}
		else {
			printf("MoblNode %d : Receive the undefined mac802_11 management pkt\n"
				,get_nid());
			freePacket(pkt);
			return(1);
		}
	}
	else if(mac_hdr->dh_fc.fc_type == MAC_Type_Data){
		/* decapsulate the 802_11 */
		if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_QoS_Data)
			pkt_->pkt_seek(sizeof(struct hdr_mac802_11e));
		else
			pkt_->pkt_seek(sizeof(struct hdr_mac802_11));

	        return(NslObject::recv(pkt));
	}
	else {
		printf("MoblNode %d : Receive the undefined mac802_11 packet !!\n",get_nid());
		freePacket(pkt);
		return(1);
	}
}


int MoblNode::send(ePacket_ *pkt) {
	hdr_mac802_11		*mac_hdr;
	Packet			*pkt_;
	struct ether_header	*eh;
	char			*mobile_module_flag;
	ePacket_		*return_ep;
	int             (NslObject::*upcall)(MBinder *);

	assert(pkt);
	assert(pkt->DataInfo_);

	pkt_ = (Packet *)pkt->DataInfo_;

	/* 
	 * If mobile_module_flag is 0, that means the packet is passed 
	 * from the upper module, otherwise the packet is generated
	 * from this module.
	 */
	mobile_module_flag = (char *)pkt_->pkt_getinfo("mflag");

	if( mobile_module_flag == 0 ) {
		if( sendtarget_->get_curqlen() > 0 || DataBuf_len > 0 
			|| ManageBuf_len > 0 || action != DATA_FLOW )
				return(-1);
		else{
			// hwchu 20060225
			if (comm_status == INFRASTRUCTURE && active_ap == NULL)
			{
				printf("Error: MN %d loses active_ap\n",get_nid());
				return(-1);
			}

			eh = (struct ether_header *)pkt_->pkt_get();
			assert(eh);
			mac_hdr = (struct hdr_mac802_11 *)
				pkt_->pkt_malloc(sizeof(struct hdr_mac802_11));
			assert(mac_hdr);

			mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Data;
			mac_hdr->dh_fc.fc_type = MAC_Type_Data;
			mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;

			mac_hdr->dh_fc.fc_more_frag  = 0;
			mac_hdr->dh_fc.fc_retry      = 0;
			mac_hdr->dh_fc.fc_pwr_mgt    = 0;
			mac_hdr->dh_fc.fc_more_data  = 0;
			mac_hdr->dh_fc.fc_wep        = 0;
			mac_hdr->dh_fc.fc_order      = 0;

			if( comm_status == AD_HOC ) {
				mac_hdr->dh_fc.fc_from_ds = BIT_0;
				mac_hdr->dh_fc.fc_to_ds = BIT_0;

     	  			memcpy((char *)mac_hdr->dh_addr1,(char *)
						eh->ether_dhost,6);
       				memcpy((char *)mac_hdr->dh_addr2,
						(char *)eh->ether_shost,6);
			}
			else if( comm_status == INFRASTRUCTURE ) {
				mac_hdr->dh_fc.fc_from_ds = BIT_0;
				mac_hdr->dh_fc.fc_to_ds = BIT_1;

     	  			memcpy((char *)mac_hdr->dh_addr1,(char *)
						active_ap->ap_mac_addr,6);
       				memcpy((char *)mac_hdr->dh_addr2,
						(char *)eh->ether_shost,6);
				memcpy((char *)mac_hdr->dh_addr3,
						(char *)eh->ether_dhost,6);
			}

			upcall = (int (NslObject::*)(MBinder *))&MoblNode::PushBufferPkt;
			sendtarget_->set_upcall(this, upcall);
			return(put(pkt, sendtarget_));
		}
	}
	else {
		/* packets from this module */

		upcall = (int (NslObject::*)(MBinder *))&MoblNode::PushBufferPkt;
		sendtarget_->set_upcall(this, upcall);
		return_ep = put1(pkt, sendtarget_);
		
		if(return_ep != NULL) {
			if(GET_MLEVEL(return_ep) <= MLEVEL_2) {
				/* A data packet. */
				assert(DataBuf == NULL);
				DataBuf = return_ep;
				DataBuf_len++;
			}
			else if(GET_MLEVEL(return_ep) == MLEVEL_3) {
				/* A management packet. */
				if(ManageBuf_len < MANAGE_BUF_LENGTH) {
					ManageBuf[manage_buf_tail] = return_ep;
					manage_buf_tail++;
					manage_buf_tail = manage_buf_tail % MANAGE_BUF_LENGTH;
					ManageBuf_len++;
				}
				else {
					printf("MoblNode %d : Lose management pkt !!\n",get_nid());
					freePacket(return_ep);
				}
			}
		}

		return(1);

	}
}

void MoblNode::ActiveChanlScan(){
	
	hdr_mac802_11		*mac_hdr;
	Packet			*probe_pkt;	
	Event_			*ep;
	char			broadcast_addr[6]={0xff,0xff,0xff,
						   0xff,0xff,0xff};
	char			*s_buf;
	char			tmp_buf[1];

	unsigned int		SSID_Len;
	unsigned int		SupportedRates_Len;

	unsigned int		framebody_len;
	char			*framebody;
	
	unsigned int		SSID;

	action = ACT_CHANNEL_SCAN;
	
	probe_pkt = new Packet;
	probe_pkt->pkt_setflow(PF_SEND);
	probe_pkt->pkt_addinfo("mflag", tmp_buf, 1);

	mac_hdr = (struct hdr_mac802_11 *)
			probe_pkt->pkt_malloc(sizeof(struct hdr_mac802_11));

	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Probe_Req;
	mac_hdr->dh_fc.fc_type = MAC_Type_Management;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion; 
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

	memcpy((char *)mac_hdr->dh_addr1,broadcast_addr,6);
	memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,broadcast_addr,6);

	/*
	 * Fill the frame body of probe request frame.
	 */
	SSID_Len = 4;
	SupportedRates_Len = 1;
	framebody_len = Calculate80211ProbeRequestFrameBodyLength(SSID_Len, SupportedRates_Len);

	s_buf =(char *)probe_pkt->pkt_sattach(framebody_len);
	probe_pkt->pkt_sprepend(s_buf,framebody_len);

	framebody = s_buf;
	InitializeFrameBodyWithZero(framebody, framebody_len);

	SSID = (unsigned int)get_nid();
	SetSSIDInto80211ProbeRequestFrameBody(framebody, SSID_Len, &SSID);


	BASE_OBJTYPE(type);
	if(comm_status == INFRASTRUCTURE && scan_mode == ACTIVE_SCAN){
		if(++(*using_chanl) > CHANNEL_NUM){
			scan_mode = STOP_SCAN;
			if(ap_index > 0){
				SelectActiveAP();
				Association();
			}
			else{
				/* retry every 1 sec */
				*using_chanl = 0;
				scan_mode = ACTIVE_SCAN;

				u_int64_t retry_interval;
				SEC_TO_TICK(retry_interval,1);

				type = POINTER_TO_MEMBER(MoblNode,ActiveChanlScan);
				ActiveScanTimer.setCallOutObj(this, type);
				ActiveScanTimer.start(retry_interval,0);

				return;
			}
		}
		else{
			type = POINTER_TO_MEMBER(MoblNode,send);
			ep = createEvent();
			setObjEvent(ep,GetNodeCurrentTime(get_nid()),
				    0,this,type,(void *)probe_pkt);
			SET_MLEVEL_3(ep);
		}
	}
	
	if(scan_mode != STOP_SCAN){
		u_int64_t wait_interval;
		SEC_TO_TICK(wait_interval, 0.05);   /* wait 0.05 sec for response */

		type = POINTER_TO_MEMBER(MoblNode,ActiveChanlScan);
		ActiveScanTimer.setCallOutObj(this, type);
		ActiveScanTimer.start(wait_interval,0);
	}

	return;
}


void MoblNode::PassiveChanlScan(){

	action = PAS_CHANNEL_SCAN;

	BASE_OBJTYPE(type);
	if(comm_status == INFRASTRUCTURE && scan_mode == PASSIVE_SCAN){
		if(++(*using_chanl) > CHANNEL_NUM){
			scan_mode = STOP_SCAN;
			if(ap_index > 0){
				SelectActiveAP();
				Association();
			}
			else{
				/* retry every 1 sec */
				*using_chanl = 0;
				scan_mode = PASSIVE_SCAN;

				u_int64_t retry_interval;
				SEC_TO_TICK(retry_interval,1);

				type = POINTER_TO_MEMBER(MoblNode,PassiveChanlScan);
				PassiveScanTimer.setCallOutObj(this, type);
				PassiveScanTimer.start(retry_interval,0);

				return;
			}
		}
	}
	
	if(scan_mode != STOP_SCAN){
		u_int64_t wait_interval;
		SEC_TO_TICK(wait_interval, 0.05);   /* wait 0.05 sec for response */

		type = POINTER_TO_MEMBER(MoblNode,PassiveChanlScan);
		PassiveScanTimer.setCallOutObj(this, type);
		PassiveScanTimer.start(wait_interval, 0);
	}

}

void MoblNode::FindAccessPoint(ePacket_ *pkt){

        hdr_mac802_11           *mac_hdr;
        Packet                  *pkt_;
	struct ap_list		*ap,*pre_ap;
	struct ap_list		*tmp_ap;
	char			*RSSI;

	char			*framebody;
	unsigned int		SSID_Len;
	unsigned int		SSID;
	void			*tmp_ptr;

	assert(pkt);
	assert(pkt->DataInfo_);
	 
	pkt_ = (Packet *)pkt->DataInfo_;
	mac_hdr = (struct hdr_mac802_11 *)pkt_->pkt_get();

	framebody = pkt_->pkt_sget();
	assert(framebody);

	SSID_Len = 0;
	tmp_ptr = GetSSIDFrom80211ProbeResponseFrameBody(framebody, &SSID_Len);
	if(tmp_ptr != NULL && SSID_Len != 0) 
		bcopy(tmp_ptr, (void *)&SSID, SSID_Len);
	

	for( ap = ap_table[*using_chanl];;pre_ap = ap, ap = ap->next_ap) {
		if( ap != NULL && !bcmp(ap->ap_mac_addr, mac_hdr->dh_addr3, 6)) {
			RSSI = (char *)pkt_->pkt_getinfo("RSSI");
			memcpy((char *)&ap->RSSI,(char *)RSSI,8);

			ap->ap_id = SSID;		
			ap->channel = *using_chanl;

			break;
		}
		else if( ap == NULL) {
			tmp_ap = (struct ap_list *)malloc(sizeof(struct ap_list));

			memcpy((char *)tmp_ap->ap_mac_addr,(char *)
					mac_hdr->dh_addr3,6);

			RSSI = (char *)pkt_->pkt_getinfo("RSSI");
			memcpy((char *)&tmp_ap->RSSI,(char *)RSSI,8);

			tmp_ap->ap_id = SSID;
			tmp_ap->channel = *using_chanl;
			tmp_ap->beacon_timestamp = GetCurrentTime();
			tmp_ap->beacon_timeval = 100;  /* ms, default */

			tmp_ap->pre_ap = NULL;
			tmp_ap->next_ap = NULL;

			if(ap_table[*using_chanl] == NULL)
				ap_table[*using_chanl] = tmp_ap;
			else{
				pre_ap->next_ap = tmp_ap;
				tmp_ap->pre_ap = pre_ap;
			}

			ap_index++;
			break;
		}
	}

	freePacket(pkt);
}

void MoblNode::SelectActiveAP(){
	int			i;
	struct ap_list		*best_ap;
        struct ap_list		*ap;

	double			RSSI 			= 0;

	for( i=1; i<=CHANNEL_NUM; i++ ) {
		for( ap = ap_table[i]; ap != NULL ; ap = ap->next_ap) {
			if( ap->RSSI > RSSI ) {
				best_ap = ap;
				*using_chanl = ap->channel;
				RSSI = ap->RSSI;
			}
		}	
	}

	active_ap = best_ap;
}

void MoblNode::Association(){
	
	hdr_mac802_11		*mac_hdr;
	Packet			*asso_pkt;
	Event_			*ep;
	
	char			*s_buf;
	char			tmp_buf[1];

	unsigned int		SSID_Len;
	unsigned int		SupportedRates_Len;

	unsigned int		framebody_len;
	char			*framebody;
	unsigned int		SSID;

	//assert(active_ap);
	if(active_ap == NULL) {
		printf("Error: MN %d loses active_ap when association.\n",get_nid());
		return;
	}

	action = ASSO;

	asso_pkt = new Packet;
	asso_pkt->pkt_setflow(PF_SEND);
	asso_pkt->pkt_addinfo("mflag", tmp_buf, 1);

	mac_hdr = (struct hdr_mac802_11 *)
			asso_pkt->pkt_malloc(sizeof(struct hdr_mac802_11));
	
	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Asso_Req;
	mac_hdr->dh_fc.fc_type = MAC_Type_Management;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

        memcpy((char *)mac_hdr->dh_addr1,(char *)active_ap->ap_mac_addr,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)active_ap->ap_mac_addr,6);	


	/*
	 * Fill the frame body of association request frame
	 */
	SSID_Len = 4;
	SupportedRates_Len = 1;
	framebody_len = Calculate80211AssociationRequestFrameBodyLength(SSID_Len, SupportedRates_Len);

	s_buf =(char *)asso_pkt->pkt_sattach(framebody_len);
	asso_pkt->pkt_sprepend(s_buf,framebody_len);

	framebody = s_buf;
	InitializeFrameBodyWithZero(framebody, framebody_len);

	SSID = (unsigned int)get_nid();
	SetSSIDInto80211AssociationRequestFrameBody(framebody, SSID_Len, &SSID);


	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(MoblNode,send);
	ep = createEvent();
	setObjEvent(ep,GetNodeCurrentTime(get_nid()),0,this,type,(void *)asso_pkt);
	SET_MLEVEL_3(ep);

	u_int64_t timeval;
	SEC_TO_TICK(timeval, 0.1);

	type = POINTER_TO_MEMBER(MoblNode,RetryAssociation);
	RetryAssoTimer.setCallOutObj(this, type);
	RetryAssoTimer.start(timeval,0);
}

void MoblNode::RetryAssociation() {
	if(action == ASSO) Association();
}

void MoblNode::ReAssociation(){

	hdr_mac802_11		*mac_hdr;
	Packet			*reasso_pkt;
	Event_			*ep;
	
	char			*s_buf;
	char			tmp_buf[1];

	unsigned int		SSID_Len;
	unsigned int		SupportedRates_Len;

	unsigned int		framebody_len;
	char			*framebody;
	u_char			CurrentAPAddress[6];
	unsigned int		SSID;

	if( active_ap == NULL ) return;
	
	action = REASSO;

	reasso_pkt = new Packet;
	reasso_pkt->pkt_setflow(PF_SEND);
	reasso_pkt->pkt_addinfo("mflag", tmp_buf, 1);

	mac_hdr = (struct hdr_mac802_11 *)
			reasso_pkt->pkt_malloc(sizeof(struct hdr_mac802_11));
	
	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_ReAsso_Req;
	mac_hdr->dh_fc.fc_type = MAC_Type_Management;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

	memcpy((char *)mac_hdr->dh_addr1,(char *)active_ap->ap_mac_addr,6);
	memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)active_ap->ap_mac_addr,6);

	/*
	 * Fill the frame body of reassociation request frame
	 */
	SSID_Len = 4;
	SupportedRates_Len = 1;
	framebody_len = Calculate80211ReassociationRequestFrameBodyLength(SSID_Len, SupportedRates_Len);

	s_buf =(char *)reasso_pkt->pkt_sattach(framebody_len);
	reasso_pkt->pkt_sprepend(s_buf,framebody_len);

	framebody = s_buf;
	InitializeFrameBodyWithZero(framebody, framebody_len);

	memcpy((char *)CurrentAPAddress, (char *)active_ap->ap_mac_addr,6);	
	SetCurrentAPAddressInto80211ReassociationRequestFrameBody(framebody, (void *)CurrentAPAddress);

	SSID = (unsigned int)get_nid();
	SetSSIDInto80211ReassociationRequestFrameBody(framebody, SSID_Len, &SSID);


	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(MoblNode,send);
	ep = createEvent();
	setObjEvent(ep,GetNodeCurrentTime(get_nid()),0,this,type,(void *)reasso_pkt);
	SET_MLEVEL_3(ep);

	u_int64_t timeval;
	MILLI_TO_TICK(timeval,active_ap->beacon_timeval*10);

	type = POINTER_TO_MEMBER(MoblNode,ReAssociation);
	ReAssoTimer.setCallOutObj(this, type);
	ReAssoTimer.start(timeval,0);

	return;
}

void MoblNode::DisAssociation(){

	hdr_mac802_11		*mac_hdr;
	Packet			*disasso_pkt;
	Event_			*ep;
	
	char			*s_buf;
	char			tmp_buf[1];

	unsigned int		framebody_len;
	char			*framebody;
	
	unsigned short		ReasonCode;

	if( active_ap == NULL ) return;
	action = DISASSO;

	disasso_pkt = new Packet;
	disasso_pkt->pkt_setflow(PF_SEND);
	disasso_pkt->pkt_addinfo("mflag", tmp_buf, 1);

	mac_hdr = (struct hdr_mac802_11 *)
			disasso_pkt->pkt_malloc(sizeof(struct hdr_mac802_11));
	
	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_DisAsso;
	mac_hdr->dh_fc.fc_type = MAC_Type_Management;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

        memcpy((char *)mac_hdr->dh_addr1,(char *)active_ap->ap_mac_addr,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)active_ap->ap_mac_addr,6);	


	/*
	 * Fill the frame body of disassociation frame
	 */

	framebody_len = Calculate80211DisassociationFrameBodyLength();

	s_buf =(char *)disasso_pkt->pkt_sattach(framebody_len);
	disasso_pkt->pkt_sprepend(s_buf,framebody_len);

	framebody = s_buf;
	InitializeFrameBodyWithZero(framebody, framebody_len);

	ReasonCode = 0;
	SetReasonCodeInto80211DisassociationFrameBody(framebody, &ReasonCode);


	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(MoblNode,send);
	ep = createEvent();
	setObjEvent(ep,GetNodeCurrentTime(get_nid()),0,this,type,
				(void *)disasso_pkt);
	SET_MLEVEL_3(ep);

	/* remove the ap info entry from ap table */
	if( active_ap == ap_table[*using_chanl] ) {
		ap_table[*using_chanl] = active_ap->next_ap;
		if( active_ap->next_ap != NULL )
			ap_table[*using_chanl]->pre_ap = NULL;
	}
	else {
		active_ap->pre_ap->next_ap = active_ap->next_ap;
		if( active_ap->next_ap != NULL )
			active_ap->next_ap->pre_ap = active_ap->pre_ap;
	}

	ap_index--;
	active_ap = NULL;
	ReAssoTimer.cancel();
	CheckApConnTimer.cancel();

}

void MoblNode::ProcessBeacon(ePacket_ *pkt){
	Packet			*pkt_;
	struct hdr_mac802_11	*mac_hdr;

	char			*framebody;
	unsigned short		BeaconInterval;
	unsigned int		SSID_Len;
	unsigned int		SSID;
	void			*tmp_ptr;


	assert(pkt);
	assert(pkt->DataInfo_);

	pkt_ = (Packet *)pkt->DataInfo_;

	mac_hdr = (struct hdr_mac802_11 *)pkt_->pkt_get();
	assert(mac_hdr);
	framebody = pkt_->pkt_sget();
	assert(framebody);

	SSID_Len = 0;
	tmp_ptr = GetSSIDFrom80211BeaconFrameBody(framebody, &SSID_Len);
	if(tmp_ptr != NULL && SSID_Len != 0) 
		bcopy(tmp_ptr, (void *)&SSID, SSID_Len);
	

	if( active_ap == NULL || 
	    (bcmp(active_ap->ap_mac_addr, mac_hdr->dh_addr3, 6) &&
	     SSID != active_ap->ap_id) ) {
		freePacket(pkt);
		return;
	}

	if( action == DATA_FLOW){

		u_int64_t cur_time = GetNodeCurrentTime(get_nid());
		memcpy(&active_ap->beacon_timestamp,&cur_time,8);
		
		GetBeaconIntervalFrom80211BeaconFrameBody(framebody, &BeaconInterval);
		memcpy(&active_ap->beacon_timeval, &BeaconInterval, 2);

		if( checkAPconn_flag == 0){
			u_int64_t tick;
			MILLI_TO_TICK(tick,active_ap->beacon_timeval*4);

			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(MoblNode,CheckApConnectivity);
			CheckApConnTimer.setCallOutObj(this, type);
			CheckApConnTimer.start(tick, 0);

			checkAPconn_flag = 1;
		}

		freePacket(pkt);
	}		
	else
		freePacket(pkt);
}

void MoblNode::ProcessBeaconDuringPassiveScan(ePacket_ *pkt) {
	FindAccessPoint(pkt);
}

void MoblNode::PushBufferPkt(){
	ePacket_		*pkt;
	int			(NslObject::*upcall)(MBinder *);

	if(ManageBuf_len > 0){
		pkt = ManageBuf[manage_buf_head];
		manage_buf_head++;
		manage_buf_head = manage_buf_head % MANAGE_BUF_LENGTH;
		ManageBuf_len--;

		put(pkt, sendtarget_);
	}
	else if(DataBuf_len > 0){
		pkt = DataBuf;
		DataBuf = NULL;
		DataBuf_len--;

		upcall = (int (NslObject::*)(MBinder *))&MoblNode::PushBufferPkt;
		sendtarget_->set_upcall(this, upcall);
		put(pkt, sendtarget_);
	}
	else{
		return;
	}
}

void MoblNode::CheckApConnectivity(){

	if(active_ap == NULL) return;

	u_int64_t cur_time = GetNodeCurrentTime(get_nid());
	u_int64_t beacon_timeval;
	MILLI_TO_TICK(beacon_timeval,active_ap->beacon_timeval);


	if( cur_time - active_ap->beacon_timestamp >
		5 * beacon_timeval) {

		/* remove the ap info entry from ap table */
		if( active_ap == ap_table[*using_chanl] ) {
			ap_table[*using_chanl] = active_ap->next_ap;
			if( active_ap->next_ap != NULL )
				ap_table[*using_chanl]->pre_ap = NULL;
		}
		else {
			active_ap->pre_ap->next_ap = active_ap->next_ap;
			if( active_ap->next_ap != NULL )
				active_ap->next_ap->pre_ap = active_ap->pre_ap;
		}

		ap_index--;
		active_ap = NULL;
		ReAssoTimer.cancel();

		BASE_OBJTYPE(type);
		if ( original_scan == ACTIVE_SCAN ) {
			*using_chanl = 0;			
			action = ACT_CHANNEL_SCAN;  // stop the data flow
			scan_mode = ACTIVE_SCAN;
			
			type = POINTER_TO_MEMBER(MoblNode,ActiveChanlScan);
			ActiveScanTimer.setCallOutObj(this, type);
			ActiveScanTimer.start(beacon_timeval, 0);
		}
		else if( original_scan == PASSIVE_SCAN ) {
			*using_chanl = 0;			
			action = PAS_CHANNEL_SCAN;  // stop the data flow
			scan_mode = PASSIVE_SCAN;
			
			type = POINTER_TO_MEMBER(MoblNode,PassiveChanlScan);
			PassiveScanTimer.setCallOutObj(this, type);
			PassiveScanTimer.start(beacon_timeval, 0);
		}

		checkAPconn_flag = 0;
	}
	else {
		u_int64_t timeval;
		MILLI_TO_TICK(timeval,active_ap->beacon_timeval*4);

		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(MoblNode,CheckApConnectivity);
		CheckApConnTimer.setCallOutObj(this, type);
		CheckApConnTimer.start(timeval, 0);

		return;
	}

	return;
}

