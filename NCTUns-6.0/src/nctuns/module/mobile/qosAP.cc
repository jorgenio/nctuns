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
#include <exportStr.h>
#include <ip.h>
#include <ethernet.h>
#include <mac-802_11e.h>
#include <mobile/qosAP.h>
#include <sys/time.h>
#include <mbinder.h>


MODULE_GENERATOR(qosAP);

char	broadcast_address[6]={0xff,0xff,0xff,0xff,0xff,0xff};

qosAP::qosAP(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
	: NslObject(type, id, pl, name)
{
	AssoTable = NULL;
	AssoCount = 0;
	manage_buf_head = manage_buf_tail = 0;
	ManageBuf_len = 0;
	data_buf_head = data_buf_tail = 0;
	DataBuf_len = 0;
	BEACON_TIMEVAL = 0;
	IsQAP = 1;

	vBind("BeaconInterval", &BEACON_TIMEVAL);

	REG_VAR("ISQAP",&IsQAP);
	REG_VAR("BEACON_INTERVAL",&BEACON_TIMEVAL);
}

qosAP::~qosAP(){}


int qosAP::init() {

	u_char		*tmp_mac;
	u_int64_t	period;

        NslObject::init();

	/* get/set some variables' initial values */
        tmp_mac = GET_REG_VAR(get_port(), "MAC", u_char *);
	assert(tmp_mac);
        memcpy(mac_,tmp_mac,6);

	using_chanl = GET_REG_VAR(get_port(), "CHANNEL", int *);
	assert(using_chanl);

	/* check if the value of BEACON_TIMEVAL is reasonable, if not
           give it a reasonable value */

	if(BEACON_TIMEVAL == 0)
		BEACON_TIMEVAL = 100;
	else if(BEACON_TIMEVAL < 10)
		BEACON_TIMEVAL = 10;

	/* export variable */
	EXPORT("association-table", E_RONLY);


	BASE_OBJTYPE(type);
	/* schedule a timer for generating beacon */

	MILLI_TO_TICK(period,10);
	type = POINTER_TO_MEMBER(qosAP,GenerateBeacon);	
	GenBeaconTimer.setCallOutObj(this, type);
	GenBeaconTimer.start(period,0);

        return(1);
}

int qosAP::recv(ePacket_ *pkt) {
	hdr_mac802_11e		*mac_hdr;
        Packet                  *pkt_;
	char			*multicast_flag;	
	char			*addr;

	assert(pkt);
	assert(pkt->DataInfo_);

        pkt_ = (Packet *)pkt->DataInfo_;

        /*
         * If multicast_flag is 0, that means the packet is passed
         * from the lower module, otherwise the packet is generated
         * from this module.
         */
	multicast_flag = (char *)pkt_->pkt_getinfo("multi");
        mac_hdr = (struct hdr_mac802_11e *)pkt_->pkt_get();

	if( multicast_flag == 0 ) {

		if( mac_hdr->dh_fc.fc_from_ds == BIT_0 && 
		    mac_hdr->dh_fc.fc_to_ds == BIT_1 && 
		    mac_hdr->dh_fc.fc_type != MAC_Type_Data &&
		    mac_hdr->dh_fc.fc_subtype != MAC_Subtype_ACK ){
			freePacket(pkt);
			return(1);
		}
		else if( mac_hdr->dh_fc.fc_from_ds == BIT_0 && 
	  		 mac_hdr->dh_fc.fc_to_ds == BIT_0 && 
			 mac_hdr->dh_fc.fc_type != MAC_Type_Management &&
			 mac_hdr->dh_fc.fc_subtype != MAC_Subtype_ACK ){
				freePacket(pkt);
				return(1);
		}

		/* update association table */
		AssoEntry	*tmp_entry;
		u_int32_t	src_nodeID;

		if( mac_hdr->dh_fc.fc_subtype == MAC_Subtype_ACK) { 
			addr =  (char *)pkt_->pkt_getinfo("from");
			src_nodeID = ipv4addr_to_nodeid(macaddr_to_ipv4addr((u_char *)addr)); 
		}else	
			src_nodeID = ipv4addr_to_nodeid(macaddr_to_ipv4addr((u_char *)mac_hdr->dh_addr2));

		for(tmp_entry = AssoTable; tmp_entry != NULL; tmp_entry =
			tmp_entry->next_entry){
			if(tmp_entry->nodeID == src_nodeID){
				tmp_entry->update_time = GetNodeCurrentTime(get_nid());
				break;
			}
		}
		if( mac_hdr->dh_fc.fc_subtype == MAC_Subtype_ACK) { 
			freePacket(pkt);
			return(1);
		}	


		if(mac_hdr->dh_fc.fc_type == MAC_Type_Management) {
			if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Probe_Req) {
				ProcessChanlScanReq(pkt);
				return(1);
			}
			else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Asso_Req) {
				ProcessAsso(pkt);
				return(1);
			}
			else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_ReAsso_Req) {
				ProcessReAsso(pkt);
				return(1);
			}
			else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_DisAsso) {
				ProcessDisAsso(pkt);
				return(1);
			}
			else if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Beacon) {
				freePacket(pkt);
				return(1);
			}
			else {
				printf("qosAP %d : Receive the undefined mac802_11 management pkt\n"
					,get_nid());
				freePacket(pkt);
				return(1);
			}
		}
		else if(mac_hdr->dh_fc.fc_type == MAC_Type_Data) {

			/* filter the packet sent from the unassociated address */
			if( CheckAssoAddr((char *)mac_hdr->dh_addr2, pkt_) < 0) {
				freePacket(pkt);
				return(1);
			}

			/* transmit the broadcast packet to the wireless interface  */
			if( !bcmp(mac_hdr->dh_addr3, ETHER_BROADCAST, 6)) {
				ProcessRecvBroPkt(pkt);
			}

			/* decapsulate the 802_11 header */
			if(mac_hdr->dh_fc.fc_subtype == MAC_Subtype_Data)
				pkt_->pkt_seek(sizeof(struct hdr_mac802_11));
			else 
				pkt_->pkt_seek(sizeof(struct hdr_mac802_11e));
			return(NslObject::recv(pkt));
		}
		else {
			printf("qosAP %d : Receive the undefined mac802_11 packet !!\n",get_nid());
			freePacket(pkt);
			return(1);
		}
	}
	else{
		/* for multicast packets sent from this module to
		   the switch module which is above this module */
		return(NslObject::recv(pkt));
	}	
}


int qosAP::send(ePacket_ *pkt) {
	hdr_mac802_11e		*mac_hdr;
	Packet			*pkt_;
	struct ether_header	*eh;
	char                    *ap_module_flag;	
	int             (NslObject::*upcall)(MBinder *);
	ePacket_		*return_ep;

	assert(pkt);
	assert(pkt->DataInfo_);

	pkt_ = (Packet *)pkt->DataInfo_;

        /*
         * If ap_module_flag is 0, that means the packet is passed
         * from the upper module, otherwise the packet is generated
         * from this module.
         */
	ap_module_flag = (char *)pkt_->pkt_getinfo("mflag");

	if( ap_module_flag == 0 ) {
		if(sendtarget_->get_curqlen() > 0 || DataBuf_len > 0 || ManageBuf_len > 0)
			return(-1);
		else{
			eh = (struct ether_header *)pkt_->pkt_get();

			//if( !bcmp(eh->ether_dhost,ETHER_MULTICAST2,6) )
			//	return(1);

			if(!bcmp(eh->ether_dhost, broadcast_address,6)){
				mac_hdr = (struct hdr_mac802_11e *) pkt_->pkt_malloc(sizeof(struct hdr_mac802_11));
				mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Data;
			}else{
				mac_hdr = (struct hdr_mac802_11e *) pkt_->pkt_malloc(sizeof(struct hdr_mac802_11e));
				mac_hdr->dh_fc.fc_subtype = MAC_Subtype_QoS_Data;
			}
			mac_hdr->dh_fc.fc_type = MAC_Type_Data;
			mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion; 
			mac_hdr->dh_fc.fc_from_ds = BIT_1;
			mac_hdr->dh_fc.fc_to_ds = BIT_0;

			mac_hdr->dh_fc.fc_more_frag  = 0;
			mac_hdr->dh_fc.fc_retry      = 0;
			mac_hdr->dh_fc.fc_pwr_mgt    = 0;
			mac_hdr->dh_fc.fc_more_data  = 0;
			mac_hdr->dh_fc.fc_wep        = 0;
			mac_hdr->dh_fc.fc_order      = 0;

     	  		memcpy((char *)mac_hdr->dh_addr1,(char *)eh->ether_dhost,6);
			memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
     			memcpy((char *)mac_hdr->dh_addr3,(char *)eh->ether_shost,6);

			if( bcmp(mac_hdr->dh_addr1, ETHER_BROADCAST, 6)) {
				if( CheckAssoAddr((char *)mac_hdr->dh_addr1, pkt_) < 0) {
					freePacket(pkt);
					return(1);
				}
			}

			upcall = (int (NslObject::*)(MBinder *))&qosAP::PushBufferPkt;
			sendtarget_->set_upcall(this, upcall);
			return(put(pkt, sendtarget_));
		}
	}else{  
		/* packets from this module */
		upcall = (int (NslObject::*)(MBinder *))&qosAP::PushBufferPkt;
		sendtarget_->set_upcall(this, upcall);
		return_ep = NULL;
		return_ep = put1(pkt, sendtarget_);
		
		if(return_ep != NULL) {
			if(GET_MLEVEL(return_ep) <= MLEVEL_2) {
				/* A data packet. */
				if(ManageBuf_len < MANAGE_BUF_LENGTH) {
					DataBuf[data_buf_tail] = return_ep;
					data_buf_tail++;
					data_buf_tail = data_buf_tail %
								(DATA_BUF_LENGTH+1);
					DataBuf_len++;
				}
				else {
					printf("qosAP %d : lose data pkt !!\n",get_nid());
					freePacket(return_ep);
				}
			}
			else if(GET_MLEVEL(return_ep) == MLEVEL_3) {
				/* A management packet. */
				if(ManageBuf_len < MANAGE_BUF_LENGTH) {
					ManageBuf[manage_buf_tail] = return_ep;
					manage_buf_tail++;
					manage_buf_tail = manage_buf_tail % 
								MANAGE_BUF_LENGTH;
					ManageBuf_len++;
				}
				else {
					printf("qosAP %d : lose management pkt !!\n",get_nid());
					freePacket(return_ep);
				}
			}
			else if(GET_MLEVEL(return_ep) == MLEVEL_4) {
				/* A beacon packet. */
				freePacket(return_ep);
			}
		}

		return(1);

	}		
}

int qosAP::command(int argc, const char *argv[]) {
	
	if ( argc == 2 ) {
		if ( !strcmp(argv[0], "Get")&&(argc>=2)) {
			if (!strcmp(argv[1], "association-table")) {
				DumpAssoTable();
				return 1;
			}
		}
	}

        return(NslObject::command(argc, argv));
}

void qosAP::FlushAssoTable(){
	u_int64_t		period;
/*
	struct AssoEntry	*tmp_entry;
	u_int64_t		cur_time;

	Packet			*multicast_pkt;
	char			tmp_buf[1];
	struct ether_header	*ether_hdr;
	Event_			*multicast_ep;
*/

	MILLI_TO_TICK(period,BEACON_TIMEVAL*10);

/*	for(tmp_entry = AssoTable; tmp_entry != NULL; 
		tmp_entry = tmp_entry->next_entry){
		cur_time = GetNodeCurrentTime(get_nid());
		if(cur_time - tmp_entry->update_time > period*3) {
			if( tmp_entry == AssoTable) {
				AssoTable = tmp_entry->next_entry;
				AssoCount--;
			}
			else if ( tmp_entry->next_entry == NULL) {
				tmp_entry->pre_entry->next_entry = NULL;
				AssoCount--;
			}
			else {
				tmp_entry->pre_entry->next_entry = 
					tmp_entry->next_entry;
				tmp_entry->next_entry->pre_entry =
					tmp_entry->pre_entry;
				AssoCount--;
			}

			* send a multicast pkt to inform the switch module 
			   about the disassociated mobile node's mac address *
			
			multicast_pkt = new Packet;
			multicast_pkt->pkt_setflow(PF_RECV);
			multicast_pkt->pkt_addinfo("multi", tmp_buf, 1);	
			multicast_pkt->pkt_addinfo("DelEn", tmp_buf, 1); //Delete Entry

			ether_hdr = (struct ether_header *)
				multicast_pkt->pkt_malloc(sizeof
						(struct ether_header));

			memcpy((char *)ether_hdr->ether_dhost,
				(char *)ETHER_MULTICAST1,6);
			memcpy((char *)ether_hdr->ether_shost,
				(char *)tmp_entry->mac_addr,6);	

			BASE_OBJTYPE(multicast_type);
			multicast_type = POINTER_TO_MEMBER(qosAP,recv);
			multicast_ep = createEvent();
			setObjEvent(multicast_ep,GetNodeCurrentTime(get_nid()),
				0,this,multicast_type,(void *)multicast_pkt);

		}		
	}

	if( AssoCount != 0){
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(qosAP,FlushAssoTable);
		FlushAssoTableTimer.setCallOutObj(this, type);
		FlushAssoTableTimer.start(period, 0);
	}*/

	return;
}

void qosAP::GenerateBeacon(){

	hdr_mac802_11e_manage	*mac_hdr;
	Packet			*beacon_pkt;	
	Event_			*ep;
	char			*s_buf;
	char			tmp_buf[1];

	u_char			optional_info_flag;
	bool			WithFH;
	bool			WithDS;
	bool			WithCF;
	bool			WithIBSS;
	bool			WithTIM;
	bool			WithQBSS;
	bool			WithEDCA;
	bool			WithQoS;

	unsigned int		SSID_Len;
	unsigned int		SupportedRates_Len;
	unsigned int		PartialVirtualBitmap_Len;

	unsigned int		framebody_len;
	char			*framebody;
 
	beacon_pkt = new Packet;
	beacon_pkt->pkt_setflow(PF_SEND);
	beacon_pkt->pkt_addinfo("mflag", tmp_buf, 1);

	mac_hdr = (struct hdr_mac802_11e_manage *)
		beacon_pkt->pkt_malloc(sizeof(struct hdr_mac802_11e_manage));

	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Beacon;
	mac_hdr->dh_fc.fc_type = MAC_Type_Management;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion; 
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

        memcpy((char *)mac_hdr->dh_addr1,broadcast_address,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)mac_,6);

        /*
         * Fill the frame body of beacon frame.
         */
	WithFH = false;
	WithDS = false;
	WithCF = false;
	WithIBSS = false;
	WithTIM = true;
	WithQBSS = true;
	WithEDCA = true;
	WithQoS = true;

	optional_info_flag = SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eBeaconFrameBody(WithFH, WithDS, WithCF, WithIBSS, WithTIM, WithQBSS, WithEDCA, WithQoS);

	SSID_Len = 4;
	SupportedRates_Len = 1;
	PartialVirtualBitmap_Len = 0;
	framebody_len = Calculate80211eBeaconFrameBodyLength(optional_info_flag, SSID_Len, SupportedRates_Len, PartialVirtualBitmap_Len);

	s_buf =(char *)beacon_pkt->pkt_sattach(framebody_len);
	beacon_pkt->pkt_sprepend(s_buf, framebody_len);

	framebody = s_buf;
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);
	
	u_int64_t cur_time = GetNodeCurrentTime(get_nid());
	u_int16_t beacon_timeval = BEACON_TIMEVAL;
	u_int32_t SSID = get_nid();

	SetTimestampInto80211eBeaconFrameBody(framebody, (unsigned long long *)&cur_time);
	SetBeaconIntervalInto80211eBeaconFrameBody(framebody, (unsigned short *)&beacon_timeval); 
	SetSSIDInto80211eBeaconFrameBody(framebody, SSID_Len, (unsigned int *)&SSID);

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(qosAP,send);
	ep = createEvent();
	setObjEvent(ep,GetNodeCurrentTime(get_nid()),
		0,this,type,(void *)beacon_pkt);
	SET_MLEVEL_4(ep);

	u_char	  diff_flag;
	u_int64_t diff_time_tmp;
	u_int64_t diff_time;
	u_int64_t tick;

	diff_flag = (u_char)(random() % 2);
	diff_time_tmp = (u_int64_t)(random() % (BEACON_TIMEVAL/10));

	MILLI_TO_TICK(tick, BEACON_TIMEVAL);
	MILLI_TO_TICK(diff_time,diff_time_tmp);

	BASE_OBJTYPE(type_);
	type_ = POINTER_TO_MEMBER(qosAP,GenerateBeacon);	
	GenBeaconTimer.setCallOutObj(this, type_);
	
	if(diff_flag == 0)
		GenBeaconTimer.start(tick-diff_time,0);
	else
		GenBeaconTimer.start(tick+diff_time,0);
	return;
}


void qosAP::ProcessChanlScanReq(ePacket_ *pkt){
	
	hdr_mac802_11e_manage	*mac_hdr;
	Packet			*probe_resp_pkt;
	Event_			*ep;
	hdr_mac802_11e_manage	*MAC_HDR;
	Packet			*pkt_;
	
	char			tmp_buf[1];
	char			*s_buf;	

	u_char			optional_info_flag;
	bool			WithFH;
	bool			WithDS;
	bool			WithCF;
	bool			WithIBSS;
	bool			WithQBSS;
	bool			WithEDCA;

	unsigned int		SSID_Len;
	unsigned int		SupportedRates_Len;

	unsigned int		framebody_len;
	char			*framebody;

	unsigned int		SSID;

	assert(pkt);
	assert(pkt->DataInfo_);
	
	pkt_ = (Packet *)pkt->DataInfo_;	
	MAC_HDR = (struct hdr_mac802_11e_manage *)pkt_->pkt_get();

	probe_resp_pkt = new Packet;
	probe_resp_pkt->pkt_setflow(PF_SEND);
	probe_resp_pkt->pkt_addinfo("mflag", tmp_buf, 1);

	mac_hdr = (struct hdr_mac802_11e_manage *)
		probe_resp_pkt->pkt_malloc(sizeof(struct hdr_mac802_11e_manage));	
	
	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Probe_Resp;
	mac_hdr->dh_fc.fc_type = MAC_Type_Management;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

        memcpy((char *)mac_hdr->dh_addr1,(char *)MAC_HDR->dh_addr2,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)mac_,6);


	WithFH = false;
	WithDS = false;
	WithCF = true;
	WithIBSS = false;
	WithQBSS = true;
	WithEDCA = true;

	optional_info_flag = SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eProbeResponseFrameBody(WithFH, WithDS, WithCF, WithIBSS, WithQBSS, WithEDCA);

	SSID_Len = 4;
	SupportedRates_Len = 1;
	framebody_len = Calculate80211eProbeResponseFrameBodyLength(optional_info_flag, SSID_Len, SupportedRates_Len);

	s_buf =(char *)probe_resp_pkt->pkt_sattach(framebody_len);
	probe_resp_pkt->pkt_sprepend(s_buf, framebody_len);

	framebody = s_buf;
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	SSID = (unsigned int)get_nid();
	SetSSIDInto80211eProbeResponseFrameBody(framebody, SSID_Len, &SSID);

	BASE_OBJTYPE(type);	
	type = POINTER_TO_MEMBER(qosAP,send);
	ep = createEvent();
	setObjEvent(ep,GetNodeCurrentTime(get_nid()),0,this,type,(void *)probe_resp_pkt);
	SET_MLEVEL_3(ep);

	freePacket(pkt);
}

void qosAP::ProcessAsso(ePacket_ *pkt){
	
        hdr_mac802_11e_manage	*mac_hdr;
        Packet                  *asso_resp_pkt;
        Event_                  *ep;
	AssoEntry		*tmp_entry;
	u_char			find_flag=0;

        Packet                  *pkt_;
        hdr_mac802_11e_manage	*MAC_HDR;
	u_int32_t		nodeID;

	ether_header		*ether_hdr;
	Packet			*multicast_pkt;
	Event_			*multicast_ep;
	//Packet			*broadcast_pkt;
	//Event_			*broadcast_ep;

	u_int64_t		tick;
	char			*s_buf;
	char			tmp_buf[1];

	unsigned int		SSID_Len;
	char			*asso_req_framebody;
	void			*tmp_ptr;
	
	unsigned int		SupportedRates_Len;
	unsigned int		framebody_len;
	char			*asso_resp_framebody;
	unsigned short		AID;

	bool			ESS;
	bool			IBSS;
	bool			CFPollable;
	bool			CFPollRequest;
	bool			Privacy;
	bool			ShortPreamble;
	bool			PBCC;
	bool			ChannelAgility;
	bool			SpectrumManagement;
	bool			QoS;
	bool			ShortSlotTime;
	bool			APSD;
	bool			DSSS_OFDM;
	bool			DelayedBloackAck;
	bool			ImmediateBlockAck;

	assert(pkt);
	assert(pkt->DataInfo_);

        pkt_ = (Packet *)pkt->DataInfo_;

        MAC_HDR = (struct hdr_mac802_11e_manage *)pkt_->pkt_get();

	asso_req_framebody = pkt_->pkt_sget();
	nodeID = 0;
	SSID_Len = 0;
	tmp_ptr = GetSSIDFrom80211eAssociationRequestFrameBody(asso_req_framebody, &SSID_Len);
	if(tmp_ptr != NULL && SSID_Len != 0)
		bcopy(tmp_ptr, (void *)&nodeID, SSID_Len);

        /* update association table */
	if(AssoTable == NULL){
		tmp_entry = (struct AssoEntry *)malloc(sizeof(struct AssoEntry));
		tmp_entry->nodeID = nodeID;
		memcpy((char *)tmp_entry->mac_addr,(char *)MAC_HDR->dh_addr2,6);
		tmp_entry->asso_time = GetNodeCurrentTime(get_nid());
		tmp_entry->update_time = GetNodeCurrentTime(get_nid());
		tmp_entry->state = (u_char)3;
		tmp_entry->ip = macaddr_to_ipv4addr((u_char *)MAC_HDR->dh_addr2);
		tmp_entry->consume_thrput = 0;
		tmp_entry->old_consume_thrput = 0;		
		tmp_entry->pre_entry = NULL;
		tmp_entry->next_entry = NULL;

		AssoTable = tmp_entry;

		AssoCount++;
		if( AssoCount == 1) {
			MILLI_TO_TICK(tick,BEACON_TIMEVAL*10);

			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(qosAP,FlushAssoTable);
			FlushAssoTableTimer.setCallOutObj(this, type);
			FlushAssoTableTimer.start(tick, 0);
		}
	}
	else{
		find_flag = 0;
		for(tmp_entry = AssoTable; tmp_entry != NULL; tmp_entry =
						tmp_entry->next_entry){
			if(tmp_entry->nodeID == nodeID){
				tmp_entry->update_time = GetNodeCurrentTime(get_nid());
				find_flag = 1;
				break;
			}
		}
		
		if(find_flag == 0){
                	tmp_entry = (struct AssoEntry *)malloc(sizeof(struct AssoEntry));
                	tmp_entry->nodeID = nodeID;
			memcpy((char *)tmp_entry->mac_addr,(char *)MAC_HDR->dh_addr2,6);
                	tmp_entry->asso_time = GetNodeCurrentTime(get_nid());
			tmp_entry->update_time = GetNodeCurrentTime(get_nid());
			tmp_entry->state = (u_char)3;
			tmp_entry->ip = macaddr_to_ipv4addr((u_char *)MAC_HDR->dh_addr2);
			tmp_entry->consume_thrput = 0;
			tmp_entry->old_consume_thrput = 0;
                	tmp_entry->next_entry = AssoTable;
			AssoTable->pre_entry = tmp_entry;

                	AssoTable = tmp_entry;

			AssoCount++;
			if( AssoCount == 1) {
				MILLI_TO_TICK(tick,BEACON_TIMEVAL*10);

				BASE_OBJTYPE(type);
				type = POINTER_TO_MEMBER(qosAP,FlushAssoTable);
				FlushAssoTableTimer.setCallOutObj(this, type);
				FlushAssoTableTimer.start(tick, 0);
			}
		}
	}	


	asso_resp_pkt = new Packet;
	asso_resp_pkt->pkt_setflow(PF_SEND);
	asso_resp_pkt->pkt_addinfo("mflag", tmp_buf, 1);	

        mac_hdr = (struct hdr_mac802_11e_manage *)
		asso_resp_pkt->pkt_malloc(sizeof(struct hdr_mac802_11e_manage));

        mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Asso_Resp;
        mac_hdr->dh_fc.fc_type = MAC_Type_Management;
        mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

        memcpy((char *)mac_hdr->dh_addr1,(char *)MAC_HDR->dh_addr2,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)mac_,6);

        /*
         * Fill the frame body of association response frame.
         */
	SupportedRates_Len = 1;
	framebody_len = Calculate80211eAssociationResponseFrameBodyLength(SupportedRates_Len);

	s_buf = (char *)asso_resp_pkt->pkt_sattach(framebody_len);
	asso_resp_pkt->pkt_sprepend(s_buf, framebody_len);

        asso_resp_framebody = s_buf;
	Initialize80211eFrameBodyWithZero(asso_resp_framebody, framebody_len);

	AID = (unsigned short)get_nid();
	SetAIDInto80211eAssociationResponseFrameBody(asso_resp_framebody, &AID);

	ESS = false;
	IBSS = false;
	CFPollable = false;
	CFPollRequest = true;
	Privacy = false;
	ShortPreamble = false;
	PBCC = false;
	ChannelAgility = false;
	SpectrumManagement = false;
	QoS = true;
	ShortSlotTime = false;
	APSD = false;
	DSSS_OFDM = false;
	DelayedBloackAck = false;
	ImmediateBlockAck = false;

	SetCapabilityInfoInto80211eProbeResponseFrameBody(asso_resp_framebody, ESS, IBSS, CFPollable, CFPollRequest, Privacy, ShortPreamble, PBCC, ChannelAgility, SpectrumManagement, QoS, ShortSlotTime, APSD, DSSS_OFDM, DelayedBloackAck, ImmediateBlockAck);


	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(qosAP,send);
	ep = createEvent();
        setObjEvent(ep,GetNodeCurrentTime(get_nid()),0,this,type,(void *)asso_resp_pkt);
	SET_MLEVEL_3(ep);

	if(find_flag == 0) {

	/* Send a multicast pkt to inform the switch module about
	 * the associated mobile node's mac address.
 	 */
		multicast_pkt = new Packet;
		multicast_pkt->pkt_setflow(PF_RECV);
		multicast_pkt->pkt_addinfo("multi", tmp_buf, 1);	
		multicast_pkt->pkt_addinfo("AddEn",tmp_buf,1); // Add Entry

		ether_hdr = (struct ether_header *)
			multicast_pkt->pkt_malloc(sizeof(struct ether_header));

		memcpy((char *)ether_hdr->ether_dhost,(char *)ETHER_MULTICAST1,6);
		memcpy((char *)ether_hdr->ether_shost,(char *)MAC_HDR->dh_addr2,6);	

		BASE_OBJTYPE(multicast_type);
		multicast_type = POINTER_TO_MEMBER(qosAP,recv);
		multicast_ep = createEvent();
		setObjEvent(multicast_ep,GetNodeCurrentTime(get_nid()),
        	            0,this,multicast_type,(void *)multicast_pkt);

	/* Send a boradcast pkt to inform the DS about
	 * the association of a mobile node.
 	 */
	
	/*

		broadcast_pkt = new Packet;
		broadcast_pkt->pkt_setflow(PF_RECV);
		broadcast_pkt->pkt_addinfo("multi", tmp_buf, 1);	

		ether_hdr = (struct ether_header *)
			broadcast_pkt->pkt_malloc(sizeof(struct ether_header));

		memcpy((char *)ether_hdr->ether_dhost,(char *)ETHER_BROADCAST ,6);
		memcpy((char *)ether_hdr->ether_shost,(char *)MAC_HDR->dh_addr2,6);

		BASE_OBJTYPE(broadcast_type);
		broadcast_type = POINTER_TO_MEMBER(qosAP,recv);
		broadcast_ep = createEvent();
		setObjEvent(broadcast_ep,GetNodeCurrentTime(get_nid()),
        	            0,this,broadcast_type,(void *)broadcast_pkt);
	*/

	}

	freePacket(pkt);
}

void qosAP::ProcessReAsso(ePacket_ *pkt){

        hdr_mac802_11e_manage	*mac_hdr;
        Packet                  *reasso_resp_pkt;
        Event_                  *ep;
	AssoEntry		*tmp_entry;
	u_char			find_flag;

        Packet                  *pkt_;
        hdr_mac802_11e_manage	*MAC_HDR;
	u_int32_t		nodeID;

	char			*s_buf;
	char			tmp_buf[1];

	unsigned int		SSID_Len;
	char			*reasso_req_framebody;
	void			*tmp_ptr;

	unsigned int		SupportedRates_Len;
	unsigned int		framebody_len;
	char			*reasso_resp_framebody;
	unsigned short		AID;

	bool			ESS;
	bool			IBSS;
	bool			CFPollable;
	bool			CFPollRequest;
	bool			Privacy;
	bool			ShortPreamble;
	bool			PBCC;
	bool			ChannelAgility;
	bool			SpectrumManagement;
	bool			QoS;
	bool			ShortSlotTime;
	bool			APSD;
	bool			DSSS_OFDM;
	bool			DelayedBloackAck;
	bool			ImmediateBlockAck;

	assert(pkt);
	assert(pkt->DataInfo_);

        pkt_ = (Packet *)pkt->DataInfo_;

        MAC_HDR = (struct hdr_mac802_11e_manage *)pkt_->pkt_get();

	reasso_req_framebody = pkt_->pkt_sget();
	nodeID = 0;
	SSID_Len = 0;
	tmp_ptr = GetSSIDFrom80211eReassociationRequestFrameBody(reasso_req_framebody, &SSID_Len);
	if(tmp_ptr != NULL && SSID_Len != 0)
		bcopy(tmp_ptr, (void *)&nodeID, SSID_Len);

        /* update association table */
	if(AssoCount == 0){
		printf("Warning : qosAP %d receive an unexpected reasso request.\n"
			,get_nid());
		printf("Warning : qosAP %d put this request to asso procedure.\n"
			,get_nid());
		ProcessAsso(pkt);
		return;
	}
	else{
		find_flag = 0;
		for(tmp_entry = AssoTable; tmp_entry != NULL; tmp_entry =
						tmp_entry->next_entry){
			if(tmp_entry->nodeID == nodeID){
				tmp_entry->update_time = GetNodeCurrentTime(get_nid());
				find_flag = 1;
				break;
			}
		}

		if(find_flag == 0){
			printf("Warning : qosAP %d %s %s.\n",get_nid(),
				"receive an unexpected reasso request",
				"from an un-associated mobile station");
			printf("Warning : qosAP %d %s.\n",get_nid(),
				"put this request to asso procedure");
			ProcessAsso(pkt);
			//freePacket(pkt);
			return;
		}		
	}	


	reasso_resp_pkt = new Packet;
	reasso_resp_pkt->pkt_setflow(PF_SEND);
	reasso_resp_pkt->pkt_addinfo("mflag", tmp_buf, 1);	

        mac_hdr = (struct hdr_mac802_11e_manage *)
		reasso_resp_pkt->pkt_malloc(sizeof(struct hdr_mac802_11e_manage));

        mac_hdr->dh_fc.fc_subtype = MAC_Subtype_ReAsso_Resp;
        mac_hdr->dh_fc.fc_type = MAC_Type_Management;
        mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

        memcpy((char *)mac_hdr->dh_addr1,(char *)MAC_HDR->dh_addr2,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)mac_,6);

        /*
         * Fill the frame body of association response frame.
         */
	SupportedRates_Len = 1;
	framebody_len = Calculate80211eReassociationResponseFrameBodyLength(SupportedRates_Len);
	s_buf = (char *)reasso_resp_pkt->pkt_sattach(framebody_len);
	reasso_resp_pkt->pkt_sprepend(s_buf, framebody_len);

	reasso_resp_framebody = s_buf;
	Initialize80211eFrameBodyWithZero(reasso_resp_framebody, framebody_len);

	AID = (unsigned short)get_nid();
	SetAIDInto80211eReassociationResponseFrameBody(reasso_resp_framebody, &AID);

	ESS = false;
	IBSS = false;
	CFPollable = false;
	CFPollRequest = true;
	Privacy = false;
	ShortPreamble = false;
	PBCC = false;
	ChannelAgility = false;
	SpectrumManagement = false;
	QoS = true;
	ShortSlotTime = false;
	APSD = false;
	DSSS_OFDM = false;
	DelayedBloackAck = false;
	ImmediateBlockAck = false;

	SetCapabilityInfoInto80211eReassociationResponseFrameBody(reasso_resp_framebody, ESS, IBSS, CFPollable, CFPollRequest, Privacy, ShortPreamble, PBCC, ChannelAgility, SpectrumManagement, QoS, ShortSlotTime, APSD, DSSS_OFDM, DelayedBloackAck, ImmediateBlockAck);


	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(qosAP,send);
	ep = createEvent();
        setObjEvent(ep,GetNodeCurrentTime(get_nid()),0,this,type,
			(void *)reasso_resp_pkt);
	SET_MLEVEL_3(ep);

	freePacket(pkt);
}

void qosAP::ProcessDisAsso(ePacket_ *pkt){

	Packet			*pkt_;
	hdr_mac802_11e_manage	*mac_hdr;
	struct AssoEntry	*tmp_entry;

	hdr_mac802_11e_manage	*disasso_mac_hdr;
	Packet			*disasso_pkt;
	Event_			*disasso_ep;

	ether_header		*ether_hdr;
	Packet			*multicast_pkt;
	Event_			*multicast_ep;

	char			tmp_buf[1];

	assert(pkt);
	assert(pkt->DataInfo_);

	pkt_ = (Packet *)pkt->DataInfo_;	
	mac_hdr = (struct hdr_mac802_11e_manage *)pkt_->pkt_get();


	for(tmp_entry = AssoTable; tmp_entry != NULL; 
		tmp_entry = tmp_entry->next_entry){

		if( !bcmp(mac_hdr->dh_addr2, tmp_entry->mac_addr, 6) ) {
			if( tmp_entry == AssoTable) {
				AssoTable = tmp_entry->next_entry;
				AssoCount--;
			}
			else if ( tmp_entry->next_entry == NULL) {
				tmp_entry->pre_entry->next_entry = NULL;
				AssoCount--;
			}
			else {
				tmp_entry->pre_entry->next_entry = 
					tmp_entry->next_entry;
				tmp_entry->next_entry->pre_entry =
					tmp_entry->pre_entry;
				AssoCount--;
			}

			/* send a disassociation response 
			   back to the mobile node */

			disasso_pkt = new Packet;
			disasso_pkt->pkt_setflow(PF_SEND);
			disasso_pkt->pkt_addinfo("mflag", tmp_buf, 1);

			disasso_mac_hdr = (struct hdr_mac802_11e_manage *)
			 disasso_pkt->pkt_malloc(sizeof(struct hdr_mac802_11e_manage));			

			disasso_mac_hdr->dh_fc.fc_subtype = MAC_Subtype_DisAsso;
			disasso_mac_hdr->dh_fc.fc_type = MAC_Type_Management;
			disasso_mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
			disasso_mac_hdr->dh_fc.fc_from_ds = BIT_0;
			disasso_mac_hdr->dh_fc.fc_to_ds = BIT_0;

			memcpy((char *)disasso_mac_hdr->dh_addr1,
				(char *)mac_hdr->dh_addr2,6);
			memcpy((char *)disasso_mac_hdr->dh_addr2,(char *)mac_,6);
			memcpy((char *)disasso_mac_hdr->dh_addr3,(char *)mac_,6);
			
			BASE_OBJTYPE(disasso_type);
			disasso_type = POINTER_TO_MEMBER(qosAP,send);
			disasso_ep = createEvent();
			
			setObjEvent(disasso_ep,GetNodeCurrentTime(get_nid()),
				0,this,disasso_type,(void *)disasso_pkt);
			SET_MLEVEL_3(disasso_ep);


			/* send a multicast pkt to inform the switch module 
			   about the disassociated mobile node's mac address */
			
			multicast_pkt = new Packet;
			multicast_pkt->pkt_setflow(PF_RECV);
			multicast_pkt->pkt_addinfo("multi", tmp_buf, 1);	
			multicast_pkt->pkt_addinfo("DelEn", tmp_buf, 1); //Delete Entry

			ether_hdr = (struct ether_header *)
				multicast_pkt->pkt_malloc(sizeof
						(struct ether_header));

			memcpy((char *)ether_hdr->ether_dhost,
				(char *)ETHER_MULTICAST1,6);
			memcpy((char *)ether_hdr->ether_shost,
				(char *)tmp_entry->mac_addr,6);	

			BASE_OBJTYPE(multicast_type);
			multicast_type = POINTER_TO_MEMBER(qosAP,recv);
			multicast_ep = createEvent();
			setObjEvent(multicast_ep,GetNodeCurrentTime(get_nid()),
				0,this,multicast_type,(void *)multicast_pkt);

			break;
		}		
	}

	freePacket(pkt);
}

void qosAP::PushBufferPkt(){
	ePacket_		*pkt;

	if(ManageBuf_len > 0){
		pkt = ManageBuf[manage_buf_head];
		manage_buf_head++;
		manage_buf_head = manage_buf_head % MANAGE_BUF_LENGTH;
		ManageBuf_len--;
		put(pkt, sendtarget_);
	}
	else if(DataBuf_len > 0){
		pkt = DataBuf[data_buf_head];
		data_buf_head++;
		data_buf_head = data_buf_head % (DATA_BUF_LENGTH+1);
		DataBuf_len--;
		put(pkt, sendtarget_);
	}
	else{
		return;
	}
}

void qosAP::DumpAssoTable(){

	struct ExportStr	*ExpStr;
	u_int32_t		row,column;
	AssoEntry		*tmp_entry;
	char			tmpBuf[30];

	ExpStr = new ExportStr(8);

	for(tmp_entry = AssoTable; tmp_entry != NULL; 
		tmp_entry = tmp_entry->next_entry){

		row = ExpStr->Add_row();
		column = 1;

		sprintf(tmpBuf,"%d",tmp_entry->nodeID);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		macaddr_to_str(tmp_entry->mac_addr, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		ipv4addr_to_str(tmp_entry->ip, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		sprintf(tmpBuf,"%s",getNodeName(tmp_entry->nodeID));
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		sprintf(tmpBuf,"%x",tmp_entry->state);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		sprintf(tmpBuf,"%f",(double)tmp_entry->asso_time / 10000000.0);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		sprintf(tmpBuf,"%f",((double)GetNodeCurrentTime(get_nid()) 
				- (double)tmp_entry->update_time) / 10000000.0);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		sprintf(tmpBuf,"%f",(double)(GetNodeCurrentTime(get_nid()) 
				- (double)tmp_entry->asso_time) / 10000000.0);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\n");
	}

	EXPORT_GET_SUCCESS(ExpStr);
}

int qosAP::CheckAssoAddr(char *addr, Packet *pkt_){
	struct AssoEntry	*tmp_entry;
	
	for(tmp_entry = AssoTable; tmp_entry != NULL; 
		tmp_entry = tmp_entry->next_entry){
		if(!bcmp(addr,tmp_entry->mac_addr,6)) {
			return(1);
		}
	}

	return(-1);
}

void qosAP::ProcessRecvBroPkt(ePacket_ *pkt){
	ePacket_			*epkt;
	Packet				*pkt_;
	struct hdr_mac802_11e		*mac_hdr;
	char				tmp_buf[1];

	epkt = pkt_copy(pkt);
	pkt_ = (Packet *)epkt->DataInfo_;				
	pkt_->pkt_setflow(PF_SEND);
	pkt_->pkt_addinfo("mflag", tmp_buf, 1);

	mac_hdr = (struct hdr_mac802_11e *)pkt_->pkt_get();
	memcpy((char *)mac_hdr->dh_addr1,(char *)mac_hdr->dh_addr3,6);
	
	if(!bcmp(mac_hdr->dh_addr1, broadcast_address,6))
		mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Data;
	else
		mac_hdr->dh_fc.fc_subtype = MAC_Subtype_QoS_Data;
	mac_hdr->dh_fc.fc_type = MAC_Type_Data;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion; 
	mac_hdr->dh_fc.fc_from_ds = BIT_1;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

	mac_hdr->dh_fc.fc_more_frag  = 0;
	mac_hdr->dh_fc.fc_retry      = 0;
	mac_hdr->dh_fc.fc_pwr_mgt    = 0;
	mac_hdr->dh_fc.fc_more_data  = 0;
	mac_hdr->dh_fc.fc_wep        = 0;
	mac_hdr->dh_fc.fc_order      = 0;

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(qosAP,send);
        setObjEvent(epkt,GetNodeCurrentTime(get_nid()),0,this,type,(void *)pkt_);
	SET_MLEVEL_2(epkt);
}


