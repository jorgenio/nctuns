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

#include "ofdma_pmpms_MR.h"
#include "../mac_16j/pmp/mac802_16j_pmpms.h"

MODULE_GENERATOR(OFDMA_PMPMS_MR);

extern SLIST_HEAD(wimax_head, con_list) headOfMobileRelayWIMAX_;
extern typeTable *typeTable_;
extern char *MobileRelayWIMAXChannelCoding;

	OFDMA_PMPMS_MR::OFDMA_PMPMS_MR(uint32_t type, uint32_t id, struct plist *pl, const char *name)
:OFDMA_80216j(type, id, pl, name)
{
	/* Initial Registers */
	recvState           = Idle;
	_buffered_ePacket   = NULL;

	/* Initial receive timer */
	BASE_OBJTYPE(func);
	_recvTimer = new timerObj;
	assert(_recvTimer);
	func = POINTER_TO_MEMBER(OFDMA_PMPMS_MR, recvHandler);
	_recvTimer->setCallOutObj(this, func);
}

OFDMA_PMPMS_MR::~OFDMA_PMPMS_MR()
{
	delete _recvTimer;
}

int OFDMA_PMPMS_MR::init(void)
{
	_DCDProfile = GET_REG_VAR(get_port(), "DCDProfile", struct DCDBurstProfile *);
	_UCDProfile = GET_REG_VAR(get_port(), "UCDProfile", struct UCDBurstProfile *);

	return NslObject::init();
}

int OFDMA_PMPMS_MR::recv(ePacket_ *epkt)
{
	double          recvPower   = 0;
	double          *dist       = NULL;
	Packet          *pkt        = NULL;
	struct PHYInfo  *phyInfo    = NULL;
	struct wphyInfo *wphyinfo    = NULL;
	uint32_t nid = get_nid();
        uint32_t pid = get_port();

	/* Get packet information */
	pkt     = (Packet *) epkt->DataInfo_;
	phyInfo = (struct PHYInfo *) pkt->pkt_getinfo("phyInfo");
	dist    = (double *) pkt->pkt_getinfo("dist");
	wphyinfo = (struct wphyInfo *)pkt->pkt_getinfo("WPHY");

	assert(pkt && phyInfo && dist && wphyinfo);

	/* Check channel ID */
	if (phyInfo->ChannelID != _ChannelID)
	{
		freePacket(epkt);
		return 1;
	}

	/* Compute recvPower and SNR */
	recvPower       = wphyinfo->Pr_;
	phyInfo->SNR    = _channelModel->powerToSNR(recvPower);

	/* Check receive power and compare with sensitivity */
	if (recvPower < _RecvSensitivity)
	{
		freePacket(epkt);
		return 1;
	}
	/* Hereunder , process DL Access zone burst that forwards from BS */
	/* Check receive status */
	if (recvState != Idle && recvState != ProcTransIE)
	{
		fprintf(stderr, "\e[1;31mTime:%llu:OFDMA_PMPMS_MR(%d)::recv() PHY is busy now! Receive State (%d) Error\e[0m\n", GetCurrentTime(), get_nid(), recvState);
		exit(1);
	}

	/* Process RS transparent scheduling traffic */	
	if (phyInfo->relay_flag == true)
	{
		if (((mac802_16j_PMPMS *)InstanceLookup(nid,pid,"MAC802_16J_PMPMS"))->RelayRSbasicid != phyInfo->RSBcid)
		{
			freePacket(epkt);
			return 1;
		}

		/* Clear the previous bufferedPacket */
		if (_buffered_ePacket != NULL)
		{
			freePacket(_buffered_ePacket);
			_buffered_ePacket = NULL;
		}

		_buffered_ePacket       = epkt;
		recvState               = ProcTransIE;
		_burstPtr_trans         = pkt->pkt_sget();
		_frameEndPtr            = _burstPtr_trans + pkt->pkt_getlen();
		_frameStartTime         = GetCurrentTime();     
		phyInfo->frameStartTime = _frameStartTime;

		/* Save start recv time for log */
		uint64_t start_recv_time = GetCurrentTime();
		pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t)); //transparent zone start time + prop-agation time

		/* Simulate the transmission time of the preceding 
		 * _DLTransparent symbols 
		 */
		scheduleTimer(_recvTimer, (int)((_DLTransparentSymbols) * Ts));
	}
	else
	{
		
		/* Clear the previous bufferedPacket */
		if (_buffered_ePacket != NULL)
		{
			freePacket(_buffered_ePacket);
			_buffered_ePacket = NULL;
		}
		
		/* Save the new arrival packet and change state to ProcDLFP */
		_buffered_ePacket       = epkt;
		recvState               = ProcDLFP;
		_burstPtr               = pkt->pkt_sget();
		_frameEndPtr            = _burstPtr + pkt->pkt_getlen();
		_frameStartTime         = GetCurrentTime();	//frame start time + propagation time
		phyInfo->frameStartTime = _frameStartTime;

		/*Record  DL && UL symbol partition*/
		_ULAccessSymbols = phyInfo->ULAccessSym;
		_ULRelaySymbols  = phyInfo->ULRelaySym;
		_DLAccessSymbols = phyInfo->DLAccessSym;
		_DLTransparentSymbols = phyInfo->DLTranspSym;

		/* Save start recv time for log */
		uint64_t start_recv_time = GetCurrentTime();
		pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t));

		/* Simulate the transmission time of the preceding */
		/* Preamble and DLFP (include DL-MAP)              */
		scheduleTimer(_recvTimer, (int)((PREAMBLE + DL_PUSC) * Ts));
	}

	return 1;
}

int OFDMA_PMPMS_MR::send(ePacket_ *epkt)
{
	struct PHYInfo phyInfo;
	vector<upBurst *> *burstCollection = NULL;
	BurstInfo *burstInfo    = NULL;
	Packet *ulburst_pkt     = NULL;
	upBurst *uburst         = NULL;
	char *input             = NULL;
	char *output            = NULL;
	int slots               = 0;
	int codedLen            = 0;
	int uncodedLen          = 0;
	int fec                 = 0;
	int repe                = 0;
	int uiuc                = 0;

	/* Process the DL-MAP message from upper layer */
	if (procCmdFromMAC((Pkthdr *)epkt->DataInfo_) == 1)
	{
		epkt->DataInfo_ = NULL;
		freePacket(epkt);
		return 1;
	}

	/* Process the ranging code message from upper layer */
	if (procRNGCode((Pkthdr *)epkt->DataInfo_) == 1)
	{
		epkt->DataInfo_ = NULL;
		freePacket(epkt);
		return 1;
	}

	/* Get the burst information */
	burstInfo       = (BurstInfo *)epkt->DataInfo_;
	burstCollection = (vector<upBurst *> *)burstInfo->Collection;

	/* Check the burstCollection size */
	if (burstCollection->size() != 1)
	{
		fprintf(stderr, "\e[1;31mMS(%d)::OFDMA_PMPMS_MR::%s(): burstCollection->size() != 1\e[0m\n", get_nid(), __func__);
		exit(1);
	}

	uburst = *(burstCollection->begin());

	if (uburst->ie->_uiuc == CDMA_Alloc_IE)   /* Special message of CDMA allocation */
	{
		fec     = _UCDProfile[uburst->ulmap_ie.ie_14.uiuc].fec;
		repe    = uburst->ulmap_ie.ie_14.repeCode;
		uiuc    = uburst->ulmap_ie.ie_14.uiuc;
	}
	else   /* Normal uplink burst */
	{
		fec     = _UCDProfile[uburst->ulmap_ie.ie_other.uiuc].fec;
		repe    = uburst->ulmap_ie.ie_other.repeCode;
		uiuc    = uburst->ulmap_ie.ie_other.uiuc;
	}

	/* Get the input data and compute data length */
	uncodedLen  = uburst->length;
	codedLen    = _channelCoding->getCodedBurstLen(uncodedLen, repeat_times_MR[repe], fec);
	input       = uburst->payload;

	/* Allocate a new packet */
	ulburst_pkt = new Packet;
	assert(ulburst_pkt);
	output      = ulburst_pkt->pkt_sattach(codedLen);
	ulburst_pkt->pkt_sprepend(output, codedLen);
	ulburst_pkt->pkt_setflow(PF_RECV);

	/* Encode the uplink burst */
	slots       = _channelCoding->getNumUncodedSlotWithRepe(uncodedLen, repeat_times_MR[repe], fec);
	_channelCoding->encode(input, output, uncodedLen, slots, repeat_times_MR[repe], fec);

	/* Convey additional PHY information */
	memset(&phyInfo, 0, sizeof(struct PHYInfo));
	phyInfo.power       = _transPower;
	phyInfo.fec         = fec;
	phyInfo.uiuc        = uiuc;
	phyInfo.ChannelID   = _ChannelID;
	if (uburst->ie->_uiuc == CDMA_Alloc_IE)
	{
		phyInfo.nid = get_nid();
		phyInfo.pid = get_port();
	}
	phyInfo.RS_RNGREQ_flag = false;
	phyInfo.relay_flag  = false;
	phyInfo.flowDir = 'U';
	memcpy(&phyInfo.ulmap_ie, &uburst->ulmap_ie, sizeof(ULMAP_IE_u));
	ulburst_pkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));

	/* Convey log information */
	struct LogInfo log_info;
	memset(&log_info, 0, sizeof(struct LogInfo));
	log_info.src_nid = get_nid();
	log_info.dst_nid = 0;  /* this field will be filled in sendToSuperStation()s */
	log_info.burst_len = codedLen;
	log_info.channel_id = _ChannelID;

	/* Send the burst to BS */
	sendToSuperStations(ulburst_pkt, &log_info);
	delete ulburst_pkt;

	/* Set epkt->DataInfo_ to NULL so that freePacket() won't free it again */
	epkt->DataInfo_ = NULL;
	delete burstInfo;

	/* If the upper module wants to temporarily hold outgoing packets for   */
	/* retransmission, it is the upper module's responsibility to duplicate */
	/* another copy of this packet.                                         */
	freePacket(epkt);

	return 1;
}

void OFDMA_PMPMS_MR::sendToSuperStations(Packet *ULburst, struct LogInfo *log_info)
{
	ePacket_ *epkt              = NULL;
	struct con_list *wi         = NULL;
	double dist                 = 0.0;
	const char *nodeType        = NULL;
	struct wphyInfo *wphyinfo   = NULL;
	double currAzimuthAngle     = 0.0;
	double T_locX               = 0.0;
	double T_locY               = 0.0;
	double T_locZ               = 0.0;
	int log_flag                = (!strncasecmp(MobileRelayWIMAXLogFlag, "on", 2)) ? 1 : 0;

	wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
	assert(wphyinfo);

	GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ);
	currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

	wphyinfo->nid               = get_nid();
	wphyinfo->pid               = get_port();
	wphyinfo->bw_               = 0;
	wphyinfo->channel_          = _ChannelID;
	wphyinfo->TxPr_             = pow(10, _transPower/10) * 1e-3; // Watt
	wphyinfo->RxPr_             = 0.0;
	wphyinfo->srcX_             = T_locX;
	wphyinfo->srcY_             = T_locY;
	wphyinfo->srcZ_             = T_locZ;
	wphyinfo->currAzimuthAngle_ = currAzimuthAngle;
	wphyinfo->Pr_               = 0.0;

	ULburst->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
	free(wphyinfo);

	ULburst->pkt_setflow(PF_SEND);

	SLIST_FOREACH(wi, &headOfMobileRelayWIMAX_, nextLoc)
	{
		nodeType = typeTable_->toName(wi->obj->get_type());

		/* The receiver should not be myself */
		if (get_nid() == wi->obj->get_nid() && get_port() == wi->obj->get_port())
			continue;

		/* We send the frame to MobileRelayWimax PMPBS  and MobileRelayWimax PMPRS module*/
		if ((strcmp(nodeType, "MobileRelayWIMAX_PMPBS") != 0) && (strcmp(nodeType, "MobileRelayWIMAX_PMPRS") != 0))
			continue;

		/************ LOG START *************/
		if (log_flag)
		{
			struct logEvent *logep      = NULL;
			uint8_t burst_type          = FRAMETYPE_MobileRelayWIMAX_PMP_UABURST;
			double trans_time           = _ULAccessSymbols * Ts;
			uint64_t trans_time_in_tick = 0;

			MICRO_TO_TICK(trans_time_in_tick, trans_time);

			log_info->dst_nid = wi->obj->get_nid();

			/* log StartTX event */
			mac80216j_log_t *mac80216j_log_p1 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

			LOG_MAC_802_16j(mac80216j_log_p1, GetCurrentTime(), GetCurrentTime(), get_type(), get_nid(), StartTX,
					log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216j_log_p1->PROTO, mac80216j_log_p1->Time + START, mac80216j_log_p1);
#if LOG_DEBUG
			printf("MS%d %llu ~ %llu %d->%d (StartTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime(), log_info->src_nid, log_info->dst_nid);
#endif
			/* log SuccessTX event */
			mac80216j_log_t *mac80216j_log_p2 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

			LOG_MAC_802_16j(mac80216j_log_p2, GetCurrentTime() + trans_time_in_tick, GetCurrentTime(), get_type(), get_nid(), SuccessTX, log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216j_log_p2->PROTO, mac80216j_log_p2->Time + ENDING, mac80216j_log_p2);
#if LOG_DEBUG
			printf("MS%d %llu ~ %llu %d->%d (SuccessTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime() + trans_time_in_tick, log_info->src_nid, log_info->dst_nid);
#endif
			ULburst->pkt_addinfo("loginfo", (char *) log_info, sizeof(struct LogInfo));
		}
		/************ LOG END *************/

		epkt            = createEvent();
		epkt->DataInfo_ = ULburst->copy();
		dist            = GetNodeDistance(get_nid(), wi->obj->get_nid());

		((Packet *) epkt->DataInfo_)->pkt_addinfo("dist", (char *) &dist, sizeof(double));

		epkt->timeStamp_    = 0;
		epkt->perio_        = 0;
		epkt->calloutObj_   = wi->obj;
		epkt->memfun_       = NULL;
		epkt->func_         = NULL;
		NslObject::send(epkt);
	}
}

void OFDMA_PMPMS_MR::recvHandler()
{
	struct PHYInfo *phyInfo     = NULL;
	struct LogInfo *src_log_info= NULL;
	struct wphyInfo *wphyinfo   = NULL;
	Packet *buffered_pkt_p      = NULL;
	Packet *pkt                 = NULL;
	ePacket_ *epkt              = NULL;
	char *output                = NULL;
	double *dist                = NULL;
	double SNR                  = 0.0;
	double avgSNR               = 0.0;
	double saveSNR              = 0.0;
	double BER                  = 0.0;
	int slots                   = 0;
	int fec                     = 0;
	int repe                    = 0;
	int len                     = 0;
	int burstLen                = 0;
	uint32_t src_nid            = 0;
	struct DLFP_except_128 dl_frame_prefix;
	/* While MAC can not recognize the DLMAP after channel decoding , we should ignore this burst and reset PHY state.
 	 * We inform MAC the Packet include DLMAP information by the FLAG.
 	 */
	char DLMAP_flag; 	

	/* Get the buffered packet information */
	buffered_pkt_p  = (Packet *)_buffered_ePacket->DataInfo_;
	phyInfo         = (struct PHYInfo *) buffered_pkt_p->pkt_getinfo("phyInfo");
	dist            = (double *) buffered_pkt_p->pkt_getinfo("dist");
	src_log_info    = (struct LogInfo *) buffered_pkt_p->pkt_getinfo("loginfo");
	saveSNR         = phyInfo->SNR;

	wphyinfo = (struct wphyInfo *) buffered_pkt_p->pkt_getinfo("WPHY");

	if (wphyinfo == NULL)
	{
		printf("[%s] Error: no wphyinfo\n", __func__);
		exit(1);
	}

	/******* LOG START *******/
	int log_flag = (!strncasecmp(MobileRelayWIMAXLogFlag, "on", 2)) ? 1 : 0;

	if (src_log_info == NULL)  /* Assertion for struct LogInfo structure */
	{
		if (log_flag == 1)
		{
			fprintf(stderr, "OFDMA_PMPMS_MR::recvHandler(): source log information is lost.\n");
			exit(1);
		}
	}
	else
	{
		src_nid = src_log_info->src_nid;
	}

	uint64_t *start_recv_time = (uint64_t *) buffered_pkt_p->pkt_getinfo("srecv_t");

	if (start_recv_time == NULL)   /* Assertion for start_recv_time*/
	{
		fprintf(stderr, "OFDMA_PMPMS_MR::recvHandler(): Frame receive time is lost.\n");
		exit(1);
	}

	struct LogInfo log_info;

	memset(&log_info, 0, sizeof(struct LogInfo));

	log_info.channel_id = _ChannelID;

	/****** LOG END ***********/

	uint8_t burst_type      = 0;
	char cm_obj_name[100]   = "";
	cm *cmobj               = NULL;

	sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
	cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
	if (cmobj == NULL)   /* Check channel model related information */
	{
		printf("[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
		exit(1);
	}

	if (recvState == Idle)   /* Unsafe state transformation */
	{
		fprintf(stderr, "\e[1;31mTime:%llu:MS(%d):Error! recvState is Idle.\e[0m\n", GetCurrentTime(), get_nid());
		exit(1);
	}
	else if (recvState == ProcDLFP)    /* Process DLFP and DL-MAP */
	{
		/* Get the length of DLFP */
		repe    = REPETATION_CODEWORD_4;
		fec     = QPSK_1_2;
		len     = _channelCoding->getCodedSlotLen(fec) * repeat_times_MR[repe];

		/* Make bit error and decode DLFP */
		BER     = _channelModel->computeBER(fec, saveSNR);
		_channelModel->makeBitError(_burstPtr, len, BER);
		decodeDLFP(_burstPtr, &dl_frame_prefix);
		_burstPtr += len;

		/* Get the length of DL-MAP message */
		repe        = dl_frame_prefix.repecode;
		fec         = QPSK_1_2;
		len         = _channelCoding->getCodedSlotLen(fec) * dl_frame_prefix.len;
		burstLen    = _channelCoding->getUncodedBurstLen(len, fec);

		/********** LOG **************/
		burst_type = FRAMETYPE_MobileRelayWIMAX_PMP_DLFP;
		log_info.burst_len = burstLen;
		/********** LOG **************/

		/* Allocate a new packet to deliver DL-MAP message */
		pkt         = new Packet;
		assert(pkt);
		output      = pkt->pkt_sattach(burstLen);
		pkt->pkt_sprepend(output, burstLen);
		pkt->pkt_setflow(PF_RECV);

		/* Make bit error and decode DL-MAP message */
		BER = _channelModel->computeBER(fec, saveSNR);
		_channelModel->makeBitError(_burstPtr, len, BER);
		slots = _channelCoding->getNumCodedSlotWithRepe(len, repeat_times_MR[repe], fec);
		_channelCoding->decode(_burstPtr, output, len, slots, repeat_times_MR[repe], fec);
		_burstPtr += len;

		/* We obtain avgSNR by averaging 300 samples. This facilitates the  */
		/* handover decision in MAC, otherwise MAC will probably receive    */
		/* some extreme values and make bad decision.                       */
		avgSNR = 0;
		for (int i = 1; i <= 100; i++) 
		{
			SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
			avgSNR  += ((SNR - avgSNR) / i);
		}

		/* Convey additional PHY information */
		phyInfo->SNR = avgSNR;
		pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));
		DLMAP_flag = 'Y';
		pkt->pkt_addinfo("DLMAP", &DLMAP_flag, sizeof(char));

		/* Recover the original received SNR */
		phyInfo->SNR = saveSNR;

		/* Deliver the DL-MAP message to upper layer */
		epkt = createEvent();
		epkt->DataInfo_ = pkt;
		put(epkt, recvtarget_);
	} 
	else if (recvState == ProcAccessIE)  /* Process DL access zone bursts depend on the received DL information element */
	{
		dlmapinfo *ieInfo = NULL;

		while (_DLMapInfo_access.size() != 0)
		{
			/* Get an IE to compute the burst length */
			ieInfo      = _DLMapInfo_access.front();
			repe        = ieInfo->repeCode;
			fec         = _DCDProfile[ieInfo->diuc].fec;
			len         = _channelCoding->getCodedSlotLen(fec) * (ieInfo->numSym * ieInfo->numCh / DL_PUSC);
			burstLen    = _channelCoding->getUncodedBurstLen(len, fec);

			/********** LOG START **************/
			burst_type = FRAMETYPE_MobileRelayWIMAX_PMP_DABURST;
			log_info.burst_len += burstLen;
			/********** LOG END **************/

			/* Allocate a new packet to deliver DL burst */
			pkt         = new Packet;
			assert(pkt);
			output      = pkt->pkt_sattach(burstLen);
			pkt->pkt_sprepend(output, burstLen);
			pkt->pkt_setflow(PF_RECV);

			/* Make bit error and decode burst */
			BER = _channelModel->computeBER(fec, saveSNR);
			_channelModel->makeBitError(_burstPtr, len, BER);
			slots = ieInfo->numSym * ieInfo->numCh / DL_PUSC;
			_channelCoding->decode(_burstPtr, output, len, slots, repeat_times_MR[repe], fec);
			_burstPtr += len;

			/* We obtain avgSNR by averaging 300 samples. This facilitates the  */
			/* profile selection in MAC, otherwise MAC will probably receive    */
			/* some extreme values and make bad selection.                      */
			avgSNR = 0;
			for (int i = 1; i <= 300; i++)
			{
				SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
				avgSNR  += ((SNR - avgSNR) / i);
			}

			/* Convey additional PHY information */
			phyInfo->SNR = avgSNR;
			pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));

			/* Deliver the burst to upper layer */
			epkt = createEvent();
			epkt->DataInfo_ = pkt;
			put(epkt, recvtarget_);

			/* Remove a information element */
			delete *(_DLMapInfo_access.begin());
			_DLMapInfo_access.erase(_DLMapInfo_access.begin());
		}

		/* End processing the DL burst, free memory and change state to Idle */
		recvState = Idle;
		freePacket(_buffered_ePacket);
		_buffered_ePacket = NULL;
		_burstPtr = NULL;
	}
	else if (recvState == ProcTransIE)  /* Process DL transparent zone bursts depend on the received DL information element */
	{
		dlmapinfo *ieInfo = NULL;

		while (_DLMapInfo_transparent.size() != 0)
		{
			/* Get an IE to compute the burst length */
			ieInfo      = _DLMapInfo_transparent.front();
			repe        = ieInfo->repeCode;
			fec         = _DCDProfile[ieInfo->diuc].fec;
			len         = _channelCoding->getCodedSlotLen(fec) * (ieInfo->numSym * ieInfo->numCh / DL_PUSC);
			burstLen    = _channelCoding->getUncodedBurstLen(len, fec);

			/********** LOG START **************/
			burst_type = FRAMETYPE_MobileRelayWIMAX_PMP_DTBURST;
			log_info.burst_len += burstLen;
			/********** LOG END **************/

			/* Allocate a new packet to deliver DL burst */
			pkt         = new Packet;
			assert(pkt);
			output      = pkt->pkt_sattach(burstLen);
			pkt->pkt_sprepend(output, burstLen);
			pkt->pkt_setflow(PF_RECV);

			/* Make bit error and decode burst */
			BER = _channelModel->computeBER(fec, saveSNR);
			_channelModel->makeBitError(_burstPtr_trans, len, BER);
			slots = ieInfo->numSym * ieInfo->numCh / DL_PUSC;
			_channelCoding->decode(_burstPtr_trans, output, len, slots, repeat_times_MR[repe], fec);
			_burstPtr_trans += len;

			/* We obtain avgSNR by averaging 300 samples. This facilitates the  */
			/* profile selection in MAC, otherwise MAC will probably receive    */
			/* some extreme values and make bad selection.                      */
			avgSNR = 0;
			for (int i = 1; i <= 300; i++)
			{
				SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
				avgSNR  += ((SNR - avgSNR) / i);
			}

			/* Convey additional PHY information */
			phyInfo->SNR = avgSNR;
			pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));

			/* Deliver the burst to upper layer */
			epkt = createEvent();
			epkt->DataInfo_ = pkt;
			put(epkt, recvtarget_);

			/* Remove a information element */
			delete *(_DLMapInfo_transparent.begin());
			_DLMapInfo_transparent.erase(_DLMapInfo_transparent.begin());
		}


		recvState = Idle;
		freePacket(_buffered_ePacket);
		_buffered_ePacket = NULL;
		_burstPtr_trans   = NULL;
	}
	else    /* Unknown state transformation */
	{
		fprintf(stderr, "\e[1;31mTime:%llu:OFDMA_PMPMS_MR(%d):Unknown state\e[0m\n", GetCurrentTime(), get_nid());
		exit(1);
	}

	/********* LOG START ******** */
	if (log_flag)
	{
		struct logEvent *logep      = NULL;
		double trans_time           = 0.0;
		uint64_t start_receive_time = 0;
		uint64_t trans_time_in_tick = 0;

		log_info.dst_nid = get_nid();

		if (burst_type == FRAMETYPE_MobileRelayWIMAX_PMP_DLFP)
			trans_time = DL_PUSC * Ts;
		else if (burst_type == FRAMETYPE_MobileRelayWIMAX_PMP_DTBURST)
			trans_time = _DLTransparentSymbols * Ts; 
		else
			trans_time= (_DLAccessSymbols - DL_PUSC - PREAMBLE) * Ts;

		MICRO_TO_TICK(trans_time_in_tick, trans_time);
		start_receive_time = GetCurrentTime() - trans_time_in_tick;

		/* log StartRX event */
		mac80216j_log_t *mac80216j_log_p1 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

		LOG_MAC_802_16j(mac80216j_log_p1, start_receive_time, start_receive_time, get_type(), get_nid(), StartRX,
				src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216j_log_p1->PROTO, mac80216j_log_p1->Time + START, mac80216j_log_p1);
#if LOG_DEBUG
		printf("MS%d %llu ~ %llu %d->%d (StartRX)\n", get_nid(), start_receive_time, start_receive_time, src_nid, log_info.dst_nid);
#endif
		/* log SuccessRX event */
		mac80216j_log_t *mac80216j_log_p2 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

		LOG_MAC_802_16j(mac80216j_log_p2, start_receive_time + trans_time_in_tick, start_receive_time, get_type(), get_nid(), SuccessRX, src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216j_log_p2->PROTO, mac80216j_log_p2->Time + ENDING, mac80216j_log_p2);
#if LOG_DEBUG
		printf("MS%d %llu ~ %llu %d->%d (SuccessRX)\n", get_nid(), start_receive_time, start_receive_time + trans_time_in_tick, src_nid, log_info.dst_nid);
#endif
	}
	/********* LOG END *********/
}

void OFDMA_PMPMS_MR::decodeDLFP(char *input, struct DLFP_except_128 *dlfp_p)
{
	char *tmp       = NULL;
	int check[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
	int index       = 0;
	int max         = 0;
	int len         = 0;
	int slots       = 0;
	char dup_coded_dlfp[4][12];
	char uncoded_dlfp[8][3];

	/* Repetition decode to get the whole coded DLFPs */
	for (int i = 0;i < 4;i++)
	{
		memcpy(dup_coded_dlfp[i], input + len, _channelCoding->getCodedSlotLen(QPSK_1_2));
		len += _channelCoding->getCodedSlotLen(QPSK_1_2);
	}

	/* Decode all of the DLFPs */
	for (int i = 0;i < 4;i++)
	{
		len = sizeof(struct DLFP_except_128);
		tmp = new char [len * 2];

		slots = _channelCoding->getNumCodedSlotWithRepe(12, 4, QPSK_1_2);
		_channelCoding->decode(dup_coded_dlfp[i], tmp, 12, slots, 4, QPSK_1_2);
		memcpy(uncoded_dlfp[index], tmp, len);
		memcpy(uncoded_dlfp[index + 1], tmp + len, len);
		index += 2;

		delete [] tmp;
	}

	/* Compare all of the DLFPs */
	for (int i = 0;i < 8;i++)
	{
		for (int j = 0;j < 8;j++)
		{
			if (memcmp(uncoded_dlfp[i], uncoded_dlfp[j], 3) == 0)
			{
				check[i]++;
			}
		}
	}

	/* choose the most appropriate DLFP */
	index = 0;
	for (int i = 0;i < 8;i++)
	{
		if (check[i] > max)
		{
			max   = check[i];
			index = i;
		}
	}

	/* Save the right DLFP */
	memcpy(dlfp_p, uncoded_dlfp[index], 3);
}

int OFDMA_PMPMS_MR::procRNGCode(Pkthdr *pkthdr)
{
	Packet *ulpkt           = NULL;
	char *output            = NULL;
	RangCodeInfo *codeInfo  = (RangCodeInfo *) pkthdr;
	struct PHYInfo phyInfo;

	/* It's not the ranging code message */
	if (pkthdr->type != RNGCODE_INFO_TYPE)
	{
		return 0;
	}

	/* Allocate a new packet to send this ranging code message */
	ulpkt   = new Packet;
	assert(ulpkt);
	output  = ulpkt->pkt_sattach(codeInfo->codeLen);
	ulpkt->pkt_sprepend(output, codeInfo->codeLen);
	ulpkt->pkt_setflow(PF_RECV);
	memcpy(output, codeInfo->rangingCode, codeInfo->codeLen);

	/* Convey additional PHY information */
	memset(&phyInfo, 0, sizeof(struct PHYInfo));
	phyInfo.power       = _transPower;
	phyInfo.uiuc        = CDMA_BWreq_Ranging;
	phyInfo.ChannelID   = _ChannelID;
	phyInfo.chOffset    = codeInfo->rangChOffset;
	phyInfo.symOffset   = codeInfo->rangSymOffset;
	phyInfo.MSBcid      = codeInfo->MSBcid;
	phyInfo.d_diuc	    = codeInfo->directDIUC;
	phyInfo.ulmap_ie.ie_12.rangMethod = codeInfo->rangMethod;
	phyInfo.RS_RNGREQ_flag = false;
	ulpkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));

	/* Convey log information */
	struct LogInfo log_info;
	memset(&log_info, 0, sizeof(struct LogInfo));
	log_info.src_nid = get_nid();
	log_info.dst_nid = 0;  /* this field will be filled in sendToSuperStations() */
	log_info.burst_len = codeInfo->codeLen;
	log_info.channel_id = _ChannelID;

	/* Send the ranging code to BS */
	sendToSuperStations(ulpkt, &log_info);

	/* Free memory */
	delete codeInfo;
	delete ulpkt;

	return 1;
}

int OFDMA_PMPMS_MR::procCmdFromMAC(Pkthdr *pkthdr)
{
	ntfyDLMAP *ntfyCmd = (ntfyDLMAP *) pkthdr;
	vector<dlmapinfo>::iterator iter;
	dlmapinfo *mapinfo = NULL;
	bool flag = false;

	
	
	//printf("Time:%llu:MS(%d)%s..\n",GetCurrentTime(), get_nid(), __func__);
	/* It's not the DL-MAP meassage */
	if (pkthdr->type != NOTIFY_PHY)
	{
		return 0;
	}

	/* Save the information elements of DL access zone && transparent zone MAP message */
	if (ntfyCmd->transparent_indicator)
	{
		while(!_DLMapInfo_transparent.empty())
		{
			delete *(_DLMapInfo_transparent.begin());
			_DLMapInfo_transparent.erase(_DLMapInfo_transparent.begin());
		}

		flag = true;
		for (iter = ntfyCmd->info.begin(); iter != ntfyCmd->info.end();iter++)
                {
                        mapinfo             = new dlmapinfo;
                        mapinfo->diuc       = (*iter).diuc;
                        mapinfo->symOff     = (*iter).symOff;
                        mapinfo->chOff      = (*iter).chOff;
                        mapinfo->numSym     = (*iter).numSym;
                        mapinfo->numCh      = (*iter).numCh;
                        mapinfo->repeCode   = (*iter).repeCode;

                        _DLMapInfo_transparent.push_back(mapinfo);
                }
	}
	else
	{ 
		
		while(!_DLMapInfo_access.empty())
		{
			delete *(_DLMapInfo_access.begin());
			_DLMapInfo_access.erase(_DLMapInfo_access.begin());
		}

		for (iter = ntfyCmd->info.begin(); iter != ntfyCmd->info.end();iter++)
		{
			mapinfo             = new dlmapinfo;
			mapinfo->diuc       = (*iter).diuc;
			mapinfo->symOff     = (*iter).symOff;
			mapinfo->chOff      = (*iter).chOff;
			mapinfo->numSym     = (*iter).numSym;
			mapinfo->numCh      = (*iter).numCh;
			mapinfo->repeCode   = (*iter).repeCode;

			_DLMapInfo_access.push_back(mapinfo);
		}
	}

	/* Free memory */
	delete ntfyCmd;
	ntfyCmd   = NULL;
	/* recvState dependents on transparent zone existed or not */
	/* Schedule a timer to handle the bursts.   */
        /* Here, the delay time is dependent on     */
        /* the zone it process at this ntfyCmd.     */
        /* It might be DLAccessSymbol or DLTransprent Symbols time*/
	if (flag)
	{
		;	// MS do nothing	
	}
	else
	{
		recvState = ProcAccessIE;
		scheduleTimer(_recvTimer, (_DLAccessSymbols - DL_PUSC - PREAMBLE) * Ts);
	}

	return 1;
}

int OFDMA_PMPMS_MR::skipBufferedPkt()
{
	/* Clear the buffered packet and change state to Idle */
	//printf("Time:%llu:MS(%d)%s..\n",GetCurrentTime(), get_nid(), __func__);
	recvState = Idle;
	freePacket(_buffered_ePacket);
	_buffered_ePacket = NULL;

	return 1;
}

void OFDMA_PMPMS_MR::resetState()
{
	if (recvState != Idle)
		recvState = Idle;
}

void OFDMA_PMPMS_MR::dump_buf(int st, int en)
{
	Packet *pkt = (Packet *)_buffered_ePacket->DataInfo_;

	for(int i = st;i < en;i++)
	{
		printf("[%3d]:%8X  ", i, pkt->pkt_sget()[i]);

		if (i % 7 == 0)
			printf("\n");
	}
	printf("\n");
}
