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

#include "ofdma_pmprs_MR.h"
MODULE_GENERATOR(OFDMA_PMPRS_MR);

extern SLIST_HEAD(wimax_head, con_list) headOfMobileRelayWIMAX_;
extern typeTable *typeTable_;
extern char *MobileRelayWIMAXChannelCoding;

	OFDMA_PMPRS_MR::OFDMA_PMPRS_MR(uint32_t type, uint32_t id, struct plist *pl, const char *name)
:OFDMA_80216j(type, id, pl, name)
{
	/* Initial Registers */
	recvState           = Idle;
	_buffered_ePacket   = NULL;
	trans_flag	    = false;

	/* Initial receive timer */
	BASE_OBJTYPE(func);
	_recvTimer = new timerObj;
	assert(_recvTimer);
	func = POINTER_TO_MEMBER(OFDMA_PMPRS_MR, recvHandler_superordinate);
	_recvTimer->setCallOutObj(this, func);
}

OFDMA_PMPRS_MR::~OFDMA_PMPRS_MR()
{
	delete _recvTimer;
}

int OFDMA_PMPRS_MR::init(void)
{
	_DCDProfile = GET_REG_VAR(get_port(), "DCDProfile", struct DCDBurstProfile *);
	_UCDProfile = GET_REG_VAR(get_port(), "UCDProfile", struct UCDBurstProfile *);

	return NslObject::init();
}

int OFDMA_PMPRS_MR::recv(ePacket_ *epkt)
{
	double          recvPower   = 0;
	double          *dist       = NULL;
	Packet          *pkt        = NULL;
	struct PHYInfo  *phyInfo    = NULL;
	struct wphyInfo *wphyinfo    = NULL;

	uint8_t symbols_time     = 0;
	uint64_t timeInTick = 0;

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
	
	//fprintf(stderr, "\e[1;41mTime:%llu:OFDMA_PMPRS_MR(%d)::recv() PHY recv packet! Receive State (%d),uiuc = %d\e[0m\n", GetCurrentTime(), get_nid(), recvState ,phyInfo->uiuc);
	/* Check receive status */
	if (recvState != Idle)
	{
		fprintf(stderr, "\e[1;31mTime:%llu:OFDMA_PMPRS_MR(%d)::recv() PHY is busy now! Receive State (%d) Error,uiuc = %d\e[0m\n", GetCurrentTime(), get_nid(), recvState ,phyInfo->uiuc);
		exit(1);
	}

	/* Clear the previous BS bufferedPacket */
	if (_buffered_ePacket != NULL)
	{
		freePacket(_buffered_ePacket);
		_buffered_ePacket = NULL;
	}

	/* Compute transmission time */
	/*process subordinate stations CDMA ranging codes*/
        if (phyInfo->uiuc == CDMA_BWreq_Ranging) // Ranging Code ,12
        {
		// RS should manage the ranging information from subordinate statation
		if (phyInfo->ulmap_ie.ie_12.rangMethod == 0)        // Initial or Handover Ranging over 2 symbols
		{
			symbols_time = 2;
		}
		else if (phyInfo->ulmap_ie.ie_12.rangMethod == 1)   // Initial or Handover Ranging over 4 symbols
		{
			symbols_time = 4;
		}
		else if (phyInfo->ulmap_ie.ie_12.rangMethod == 2)   // BW Request or Period Ranging over 1 symbol
		{
			symbols_time = 1;
		}
		else                                                // BW Request or Period Ranging over 3 symbols
		{
			symbols_time = 3;
		}

		// Save start recv time for log
                uint64_t start_recv_time = GetCurrentTime();
                pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t));

                //Simulate the transmission time of the preceding 
                MICRO_TO_TICK(timeInTick, symbols_time * Ts);
		timeInTick += GetCurrentTime();
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(OFDMA_PMPRS_MR, recvHandler_subordinate);
		setObjEvent(epkt, timeInTick, 0, this, type, epkt->DataInfo_);
        }
	else if (phyInfo->uiuc == CDMA_Alloc_IE) // subordinate RNG-REQ message (CDMA Allocation, 14)
        {

                if (phyInfo->ulmap_ie.ie_14.duration * UL_PUSC > _ULAccessSymbols)
                        symbols_time = _ULAccessSymbols;
                else
                        symbols_time = phyInfo->ulmap_ie.ie_14.duration * UL_PUSC;

		/* Save start recv time for log */
                uint64_t start_recv_time = GetCurrentTime();
                pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t));
		
                //Simulate the transmission time of the preceding 
                MICRO_TO_TICK(timeInTick, symbols_time * Ts);
		timeInTick += GetCurrentTime();
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(OFDMA_PMPRS_MR, recvHandler_subordinate);
		setObjEvent(epkt, timeInTick, 0, this, type, epkt->DataInfo_);
        }
	else if(phyInfo->uiuc > 0 && phyInfo->uiuc < 11)	/*Process subordinate stations UL traiffcs*/
	{
		symbols_time = _ULAccessSymbols;

		/* Save start recv time for log */
                uint64_t start_recv_time = GetCurrentTime();
                pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t)); // UL all-start time + propa time
			
                //Simulate the transmission time of the preceding 
                MICRO_TO_TICK(timeInTick, symbols_time * Ts);
		timeInTick += GetCurrentTime();
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(OFDMA_PMPRS_MR, recvHandler_subordinate);
		setObjEvent(epkt, timeInTick, 0, this, type, epkt->DataInfo_);
	}	
	else	/* Process DL-Subframe */
	{
		/* Save the new arrival packet and change state to ProcDLFP */
		_buffered_ePacket       = epkt;
		recvState               = ProcDLFP;
		_burstPtr               = pkt->pkt_sget();
		_frameEndPtr            = _burstPtr + pkt->pkt_getlen();
		_frameStartTime         = GetCurrentTime();
		phyInfo->frameStartTime = _frameStartTime;

		/*Record DL & UL symbol partition*/
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

void OFDMA_PMPRS_MR::recvHandler_subordinate(ePacket_ *epkt)
{
	struct PHYInfo *phyInfo     = NULL;
	struct LogInfo *src_log_info= NULL;
	struct wphyInfo *wphyinfo   = NULL;
	Packet *pkt_p      	    = NULL;
	Packet *pkt                 = NULL;
	char *output                = NULL;
	double *dist                = NULL;
	double SNR                  = 0.0;
	double avgSNR               = 0.0;
	double saveSNR              = 0.0;
	double BER                  = 0.0;
	int slots                   = 0;
	int fec                     = 0;
	int repe                    = 0;
	uint32_t src_nid            = 0;

	int uncodedLen              = 0;
	int codedLen                = 0;
	ePacket_ *deliver_epkt      = NULL;
	char *input;

	/* Get the buffered packet information */
	pkt_p  = (Packet *) epkt->DataInfo_;
	wphyinfo = (struct wphyInfo *) pkt_p->pkt_getinfo("WPHY");
	input = pkt_p->pkt_sget();
	phyInfo         = (struct PHYInfo *) pkt_p->pkt_getinfo("phyInfo");
	dist            = (double *) pkt_p->pkt_getinfo("dist");

	src_log_info    = (struct LogInfo *) pkt_p->pkt_getinfo("loginfo");
	saveSNR         = phyInfo->SNR;

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
			fprintf(stderr, "OFDMA_PMPRS_MR::recvHandler_subordinate(): source log information is lost.\n");
			exit(1);
		}
	}
	else
	{
		src_nid = src_log_info->src_nid;
	}

	uint64_t *start_recv_time = (uint64_t *) pkt_p->pkt_getinfo("srecv_t");

	if (start_recv_time == NULL)   /* Assertion for start_recv_time*/
	{
		fprintf(stderr, "OFDMA_PMPRS_MR::recvHandler_subordinate(): Frame receive time is lost.\n");
		exit(1);
	}

	struct LogInfo log_info;

	memset(&log_info, 0, sizeof(struct LogInfo));

	log_info.channel_id = _ChannelID;

	/****** LOG END ***********/

	uint8_t burst_type      = 0;

	/* We obtain avgSNR by averaging 300 samples. This facilitates the  */
	/* profile selection in MAC, otherwise MAC will probably receive    */
	/* some extreme values and make bad selection.                      */
	char cm_obj_name[100]   = "";
	cm *cmobj               = NULL;
	sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
	cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
	if (cmobj == NULL)
	{
		fprintf(stderr, "[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
		exit(0);
	}
	if ( phyInfo->uiuc == CDMA_BWreq_Ranging ) // Process subordinate stations CDMA  codes
	{
		fec= BPSK_;

                if (phyInfo->ulmap_ie.ie_12.rangMethod == 0)        // Initial or Handover Ranging over 2 symbols
                {
                        uncodedLen = 2 * 18;
                }
                else if (phyInfo->ulmap_ie.ie_12.rangMethod == 1)   // Initial or Handover Ranging over 4 symbols
                {
                        uncodedLen = 4 * 18;
                }
                else if (phyInfo->ulmap_ie.ie_12.rangMethod == 2)   // BW Request or Period Ranging over 1 symbol
                {
                        uncodedLen = 1 * 18;
                }
                else                                                // BW Request or Period Ranging over 3 symbols
                {
                        uncodedLen = 3 * 18;
                }

                //Allocate a new packet to deliver ranging code message 
                pkt     = new Packet;
                assert(pkt);
                output  = pkt->pkt_sattach(uncodedLen);
                pkt->pkt_sprepend(output, uncodedLen);
                pkt->pkt_setflow(PF_RECV);

		/********** LOG **************/
		burst_type = FRAMETYPE_MobileRelayWIMAX_PMP_UABURST;
		log_info.burst_len = uncodedLen;
		/********** LOG **************/

                // Make bit error and copy to output 
                BER = _channelModel->computeBER(fec, phyInfo->SNR);
                _channelModel->makeBitError(input, uncodedLen, BER);
                memcpy(output, input, uncodedLen);

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

                //Deliver the ranging code message to upper layer
                deliver_epkt            = createEvent();
		deliver_epkt->DataInfo_ = pkt;
                put(deliver_epkt, recvtarget_);
	}
	else /*Process subordinate stations UL traffics */ 
	{
		if (phyInfo->uiuc == CDMA_Alloc_IE)  /* Special message of CDMA allocation with RNG-REQ pdu*/
                {
                        repe = phyInfo->ulmap_ie.ie_14.repeCode;
                }
                else /* Normal uplink burst */
                {
                        repe = phyInfo->ulmap_ie.ie_other.repeCode;
                }

                /* Compute the data length */
                fec         = phyInfo->fec;
                codedLen    = pkt_p->pkt_getlen();
                uncodedLen  = _channelCoding->getUncodedBurstLen(codedLen, fec);

		/********** LOG **************/
		burst_type = FRAMETYPE_MobileRelayWIMAX_PMP_UABURST;
		log_info.burst_len = uncodedLen;
		/********** LOG **************/

                /* Allocate a new packet */
                pkt         = new Packet;
                assert(pkt);
                output      = pkt->pkt_sattach(uncodedLen);
                pkt->pkt_sprepend(output, uncodedLen);
                pkt->pkt_setflow(PF_RECV);

                /* Make bit error and decode burst */
                BER = _channelModel->computeBER(fec, phyInfo->SNR);
                _channelModel->makeBitError(input, codedLen, BER);
                slots = _channelCoding->getNumCodedSlotWithRepe(codedLen, repeat_times_MR[repe], fec);
                _channelCoding->decode(input, output, codedLen, slots, repeat_times_MR[repe], fec);

                /* We obtain avgSNR by averaging 300 samples. This facilitates the  */
                /* profile selection in MAC, otherwise MAC will probably receive    */
                /* some extreme values and make bad selection.                      */
                char cm_obj_name[100]   = "";
                cm *cmobj               = NULL;
		sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
                cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
                if (cmobj == NULL)
                {
                        fprintf(stderr, "[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
                        exit(0);
                }

                avgSNR = 0;
                for (int i = 1;i <= 300; i++)
                {
                        SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
                        avgSNR  += ((SNR - avgSNR) / i);
                }

                /* Convey additional PHY information */
                phyInfo->SNR = avgSNR;
                pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));

                /* Deliver the burst to upper layer */
                deliver_epkt            = createEvent();
                deliver_epkt->DataInfo_ = pkt;
                put(deliver_epkt, recvtarget_);
		
	}

	/********* LOG START *********/
	if (log_flag)
	{
		struct logEvent *logep      = NULL;
		double trans_time           = 0.0;
		uint64_t start_receive_time = 0;
		uint64_t trans_time_in_tick = 0;

		log_info.dst_nid = get_nid();

		if (burst_type == FRAMETYPE_MobileRelayWIMAX_PMP_UABURST)
			trans_time = _ULAccessSymbols *Ts;
		else
		{
			if (phyInfo->ulmap_ie.ie_12.rangMethod == 0)        // Initial or Handover Ranging over 2 symbols
			{
				trans_time = 2 * Ts;
			}
			else if (phyInfo->ulmap_ie.ie_12.rangMethod == 1)   // Initial or Handover Ranging over 4 symbols
			{
				trans_time = 4 * Ts;
			}
			else if (phyInfo->ulmap_ie.ie_12.rangMethod == 2)   // BW Request or Period Ranging over 1 symbol
			{
				trans_time = 1 * Ts;
			}
			else                                                // BW Request or Period Ranging over 3 symbols
			{
				trans_time = 3 * Ts;
			}
		}
		MICRO_TO_TICK(trans_time_in_tick, trans_time);
		start_receive_time = GetCurrentTime() - trans_time_in_tick;
		
		//FIXME
		/* log StartRX event */
		mac80216j_log_t *mac80216j_log_p1 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

		LOG_MAC_802_16j(mac80216j_log_p1, start_receive_time, start_receive_time, get_type(), get_nid(), StartRX,
				src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216j_log_p1->PROTO, mac80216j_log_p1->Time + START, mac80216j_log_p1);
#if LOG_DEBUG
		printf("RS%d %llu ~ %llu %d->%d (StartRX)\n", get_nid(), start_receive_time, start_receive_time, src_nid, log_info.dst_nid);
#endif
		/* log SuccessRX event */
		mac80216j_log_t *mac80216j_log_p2 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

		LOG_MAC_802_16j(mac80216j_log_p2, start_receive_time + trans_time_in_tick, start_receive_time, get_type(), get_nid(), SuccessRX,
				src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216j_log_p2->PROTO, mac80216j_log_p2->Time + ENDING, mac80216j_log_p2);
#if LOG_DEBUG
		printf("RS%d %llu ~ %llu %d->%d (SuccessRX)\n", get_nid(), start_receive_time, start_receive_time + trans_time_in_tick, src_nid, log_info.dst_nid);
#endif
	}
	/********* LOG END *********/

	/* Free memory */
        freePacket(epkt);
}

void OFDMA_PMPRS_MR::recvHandler_superordinate()
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
			fprintf(stderr, "OFDMA_PMPRS_MR::recvHandler_superordinate(): source log information is lost.\n");
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
		fprintf(stderr, "OFDMA_PMPRS_MR::recvHandler_superordinate(): Frame receive time is lost.\n");
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
                fprintf(stderr, "\e[1;31mTime:%llu:RS(%d):Error! recvState is Idle.\e[0m\n", GetCurrentTime(), get_nid());
                exit(1);
        }

	if (recvState == ProcDLFP)    /* Process DLFP and DL-MAP */
	{
		/* reset transparent zone flag */
		trans_flag = false;
		
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
	else if (recvState == ProcAccessIE)  /* Process the DL bursts depend on the received DL information element */
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
	else    /* Unknown state transformation */
        {
                fprintf(stderr, "\e[1;31mTime:%llu:OFDMA_PMPRS_MR(%d):Unknown state(%d)\e[0m\n", GetCurrentTime(), get_nid(), recvState);
                exit(1);
        }

	/********* LOG START *********/
	if (log_flag)
	{
		struct logEvent *logep      = NULL;
		double trans_time           = 0.0;
		uint64_t start_receive_time = 0;
		uint64_t trans_time_in_tick = 0;

		log_info.dst_nid = get_nid();

		if (burst_type == FRAMETYPE_MobileRelayWIMAX_PMP_DLFP)
			trans_time = DL_PUSC * Ts;
		else if (burst_type == FRAMETYPE_MobileRelayWIMAX_PMP_DABURST)
                        trans_time = (_DLAccessSymbols - DL_PUSC - PREAMBLE) * Ts;

		MICRO_TO_TICK(trans_time_in_tick, trans_time);
		start_receive_time = GetCurrentTime() - trans_time_in_tick;
		
		//FIXME
		/* log StartRX event */
		mac80216j_log_t *mac80216j_log_p1 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

		LOG_MAC_802_16j(mac80216j_log_p1, start_receive_time, start_receive_time, get_type(), get_nid(), StartRX,
				src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216j_log_p1->PROTO, mac80216j_log_p1->Time + START, mac80216j_log_p1);
#if LOG_DEBUG
		printf("RS%d %llu ~ %llu %d->%d (StartRX)\n", get_nid(), start_receive_time, start_receive_time, src_nid, log_info.dst_nid);
#endif
		/* log SuccessRX event */
		mac80216j_log_t *mac80216j_log_p2 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

		LOG_MAC_802_16j(mac80216j_log_p2, start_receive_time + trans_time_in_tick, start_receive_time, get_type(), get_nid(), SuccessRX,
				src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216j_log_p2->PROTO, mac80216j_log_p2->Time + ENDING, mac80216j_log_p2);
#if LOG_DEBUG
		printf("RS%d %llu ~ %llu %d->%d (SuccessRX)\n", get_nid(), start_receive_time, start_receive_time + trans_time_in_tick, src_nid, log_info.dst_nid);
#endif
	}
	/********* LOG END *********/
}
int OFDMA_PMPRS_MR::send(ePacket_ *epkt)
{
	struct PHYInfo phyInfo;
	vector<upBurst *> *burstCollection_UL = NULL;
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

	/* 16j added */
	uint32_t nid = get_nid();
        uint32_t pid = get_port();
	int len                 = 0;
	int n 			= 0;
	Packet *dlburst_pkt     = NULL;
	downBurst *dburst         = NULL;
	vector<downBurst *> *burstCollection_DL = NULL;

	vector<downBurst *>::iterator itd;

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

	/* Get the burst information , pocess  traffics  */
	burstInfo       = (BurstInfo *)epkt->DataInfo_;

	/* Process transparent zone DL traffic encoding */
	if (trans_flag && (burstInfo->type == DL_TRANSPARENT_TYPE))
	{
		burstCollection_DL = (vector<downBurst *> *)burstInfo->Collection;

		if (burstCollection_DL->size() == 0)
		{
			fprintf(stderr, "\e[1;31mTime:%llu:OFDMA_PMPRS_MR(%d): DL burstCollection is Empty!\e[0m\n", GetCurrentTime(), get_nid());
			exit(1);
		}

		for (itd = burstCollection_DL->begin();itd != burstCollection_DL->end();itd++)
		{
			dburst       = *itd;
			fec         = _DCDProfile[dburst->ie->_diuc].fec;
			slots       = (dburst->dlmap_ie.ie_other.numSym * dburst->dlmap_ie.ie_other.numCh / 2);
			len         = _channelCoding->getUncodedSlotLen(fec) * slots;
			n           = _channelCoding->getCodedBurstLen(len, repeat_times_MR[repe], fec);
			codedLen    += n;
		}

		dlburst_pkt = new Packet;
		assert(dlburst_pkt);
		output      = dlburst_pkt->pkt_sattach(codedLen);
		dlburst_pkt->pkt_sprepend(output, codedLen);
		dlburst_pkt->pkt_setflow(PF_RECV);
		
		/* Convey additional PHY information */
		memset(&phyInfo, 0, sizeof(struct PHYInfo));
		phyInfo.power       = _transPower;
		phyInfo.ChannelID   = _ChannelID;
		phyInfo.relay_flag  = true;
		phyInfo.flowDir = 'D';
		phyInfo.RSBcid = burstInfo->RSBcid;
		dlburst_pkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));
		
		/* Encode the DL transparent bursts */
		for (itd = burstCollection_DL->begin();itd != burstCollection_DL->end();itd++)
		{
			dburst   = *itd;
			repe    = REPETATION_CODEWORD_1;
			fec     = _DCDProfile[dburst->ie->_diuc].fec;
			len     = dburst->length;
			input   = dburst->payload;
			slots   = dburst->dlmap_ie.ie_other.numSym * dburst->dlmap_ie.ie_other.numCh / DL_PUSC;
			n       = _channelCoding->encode(input, output, len, slots, repeat_times_MR[repe], fec);
			output += n;
		}
	}
	else
	{
		burstCollection_UL = (vector<upBurst *> *)burstInfo->Collection;


		/* Check the Relay UL burstCollection size */
		if (burstCollection_UL->size() != 1)
		{
			fprintf(stderr, "\e[1;31mTime::%llu:RS(%d)::OFDMA_PMPRS_MR::%s(): UL burstCollection is Empty\e[0m\n", GetCurrentTime(),  get_nid(), __func__);
			exit(1);
		}

		uburst = *(burstCollection_UL->begin());

		if (uburst->ie->_uiuc == CDMA_Alloc_IE)   /* Special message of CDMA allocation with RNG-REQ pdu to MR-BS*/
		{
			fec     = _UCDProfile[uburst->ulmap_ie.ie_14.uiuc].fec;
			repe    = uburst->ulmap_ie.ie_14.repeCode;	//repe=0
			uiuc    = uburst->ulmap_ie.ie_14.uiuc;
		}
		else   /* Normal uplink burst */
		{
			fec     = _UCDProfile[uburst->ulmap_ie.ie_other.uiuc].fec;
			repe    = uburst->ulmap_ie.ie_other.repeCode;	//variant
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
			phyInfo.RS_RNGREQ_flag = true;	/* flag to indicate iff RNGREQ by RS */
			phyInfo.nid = nid;
			phyInfo.pid = pid;
		}
		else
			phyInfo.RS_RNGREQ_flag = false;

		phyInfo.relay_flag  = true;		
		phyInfo.flowDir = 'U';
		memcpy(&phyInfo.ulmap_ie, &uburst->ulmap_ie, sizeof(ULMAP_IE_u));
		ulburst_pkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));
	}

	/* Convey log information */
	struct LogInfo log_info;
	memset(&log_info, 0, sizeof(struct LogInfo));
	log_info.src_nid = get_nid();
	log_info.dst_nid = 0;  /* this field will be filled in sendToBS() */
	log_info.burst_len = codedLen;
	log_info.channel_id = _ChannelID;

	
	if (trans_flag && (burstInfo->type == DL_TRANSPARENT_TYPE))
	{
		/* Send the burst to subordinate stations in DL transparent zone */
		sendToSubStations(dlburst_pkt, &log_info);
		delete dlburst_pkt;
	}
	else
	{
		/* Send relay burst to BS in UL relay zone*/
		sendToBS(ulburst_pkt, &log_info);
		delete ulburst_pkt;
	}

	/* Set epkt->DataInfo_ to NULL so that freePacket() won't free it again */
	epkt->DataInfo_ = NULL;
	/* If it is transparent zone burst, We will not delete the burstInfo here 
 	 * due to it might be multiple RSs need to transmit transparent zone burst. 
         * It will be released at every BS PacketScheduling.
         */
	if (!trans_flag)
		delete burstInfo;

	/* If the upper module wants to temporarily hold outgoing packets for   */
	/* retransmission, it is the upper module's responsibility to duplicate */
	/* another copy of this packet.                                         */
	freePacket(epkt);

	return 1;
}

void OFDMA_PMPRS_MR::sendToBS(Packet *ULburst, struct LogInfo *log_info)
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

		/* We only send the frame to MobileWimax PMPBS module */
		if (strcmp(nodeType, "MobileRelayWIMAX_PMPBS") != 0)
			continue;
		
		/************ LOG START *************/
		if (log_flag)
		{
			struct logEvent *logep      = NULL;
			uint8_t burst_type          = FRAMETYPE_MobileRelayWIMAX_PMP_URBURST;
			double trans_time           = _ULRelaySymbols * Ts;
			uint64_t trans_time_in_tick = 0;

			MICRO_TO_TICK(trans_time_in_tick, trans_time);

			log_info->dst_nid = wi->obj->get_nid();

			/* log StartTX event */
			mac80216j_log_t *mac80216j_log_p1 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

			LOG_MAC_802_16j(mac80216j_log_p1, GetCurrentTime(), GetCurrentTime(), get_type(), get_nid(), StartTX,
					log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216j_log_p1->PROTO, mac80216j_log_p1->Time + START, mac80216j_log_p1);
#if LOG_DEBUG
			printf("RS%d %llu ~ %llu %d->%d (StartTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime(), log_info->src_nid, log_info->dst_nid);
#endif
			/* log SuccessTX event */
			mac80216j_log_t *mac80216j_log_p2 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

			LOG_MAC_802_16j(mac80216j_log_p2, GetCurrentTime() + trans_time_in_tick, GetCurrentTime(), get_type(), get_nid(), SuccessTX,
					log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216j_log_p2->PROTO, mac80216j_log_p2->Time + ENDING, mac80216j_log_p2);
#if LOG_DEBUG
			printf("RS%d %llu ~ %llu %d->%d (SuccessTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime() + trans_time_in_tick, log_info->src_nid, log_info->dst_nid);
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
void OFDMA_PMPRS_MR::sendToSubStations(Packet *DLburst, struct LogInfo *log_info)
{
	ePacket_ *epkt              = NULL;
        struct con_list *wi         = NULL;
        const char *nodeType        = NULL;
        double dist                 = 0.0;
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

        DLburst->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
        free(wphyinfo);

        DLburst->pkt_setflow(PF_SEND);

        SLIST_FOREACH(wi, &headOfMobileRelayWIMAX_, nextLoc)
        {
                nodeType = typeTable_->toName(wi->obj->get_type());

                /*The receiver should not be myself */
                if (get_nid() == wi->obj->get_nid() && get_port() == wi->obj->get_port())
                        continue;

                /*We only send the frame to MobileiRelayWimax PMPMS module */
                if ((strcmp(nodeType, "MobileRelayWIMAX_PMPMS") != 0))
                        continue;
 		 
		/************ LOG START *************/
		if (log_flag)
		{
			struct logEvent *logep      = NULL;
			uint8_t burst_type          = FRAMETYPE_MobileRelayWIMAX_PMP_DTBURST;
			double trans_time           = _DLTransparentSymbols * Ts;
			uint64_t trans_time_in_tick = 0;

			MICRO_TO_TICK(trans_time_in_tick, trans_time);

			log_info->dst_nid = wi->obj->get_nid();

			/* log StartTX event */
			mac80216j_log_t *mac80216j_log_p1 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

			LOG_MAC_802_16j(mac80216j_log_p1, GetCurrentTime(), GetCurrentTime(), get_type(), get_nid(), StartTX,
					log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216j_log_p1->PROTO, mac80216j_log_p1->Time + START, mac80216j_log_p1);
#if LOG_DEBUG
			printf("RS%d %llu ~ %llu %d->%d (StartTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime(), log_info->src_nid, log_info->dst_nid);
#endif
			/* log SuccessTX event */
			mac80216j_log_t *mac80216j_log_p2 = (mac80216j_log_t *)malloc(sizeof(mac80216j_log_t));

			LOG_MAC_802_16j(mac80216j_log_p2, GetCurrentTime() + trans_time_in_tick, GetCurrentTime(), get_type(), get_nid(), SuccessTX,log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216j_log_p2->PROTO, mac80216j_log_p2->Time + ENDING, mac80216j_log_p2);
#if LOG_DEBUG
			printf("RS%d %llu ~ %llu %d->%d (SuccessTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime() + trans_time_in_tick, log_info->src_nid, log_info->dst_nid);
#endif
			DLburst->pkt_addinfo("loginfo", (char *) log_info, sizeof(struct LogInfo));
		}
		/************ LOG END *************/
                epkt = createEvent();
                epkt->DataInfo_ = DLburst->copy();

                ((Packet *) epkt->DataInfo_)->pkt_addinfo("dist", (char *) &dist, sizeof(double));

                epkt->timeStamp_    = 0;
                epkt->perio_        = 0;
                epkt->calloutObj_   = wi->obj;
                epkt->memfun_       = NULL;
                epkt->func_         = NULL;
                NslObject::send(epkt);
        }
}

void OFDMA_PMPRS_MR::decodeDLFP(char *input, struct DLFP_except_128 *dlfp_p)
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

int OFDMA_PMPRS_MR::procRNGCode(Pkthdr *pkthdr)
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
	phyInfo.d_diuc      = codeInfo->directDIUC; 
	phyInfo.ulmap_ie.ie_12.rangMethod = codeInfo->rangMethod;
	phyInfo.RS_RNGREQ_flag = false;
	ulpkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));

	/* Convey log information */
	struct LogInfo log_info;
	memset(&log_info, 0, sizeof(struct LogInfo));
	log_info.src_nid = get_nid();
	log_info.dst_nid = 0;  /* this field will be filled in sendToBS() */
	log_info.burst_len = codeInfo->codeLen;
	log_info.channel_id = _ChannelID;

	/* Send the ranging code to BS */
	sendToBS(ulpkt, &log_info);

	/* Free memory */
	delete codeInfo;
	delete ulpkt;

	return 1;
}

int OFDMA_PMPRS_MR::procCmdFromMAC(Pkthdr *pkthdr)
{
	ntfyDLMAP *ntfyCmd = (ntfyDLMAP *) pkthdr;
	vector<dlmapinfo>::iterator iter;
	dlmapinfo *mapinfo = NULL;

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
		
		trans_flag = true;
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
	/* Since RS do not need transparent MAP IE information , we erase the DL transparent MAP directly */
	if (trans_flag)
        {
		while (_DLMapInfo_transparent.size() != 0)
		{
			/* Remove a information element */
                        delete *(_DLMapInfo_transparent.begin());
                        _DLMapInfo_transparent.erase(_DLMapInfo_transparent.begin());	
		}

        }
        else
        {
                recvState = ProcAccessIE;
                scheduleTimer(_recvTimer, (_DLAccessSymbols - DL_PUSC - PREAMBLE) * Ts);
        }	

	return 1;
}

int OFDMA_PMPRS_MR::skipBufferedPkt()
{
	/* Clear the buffered packet and change state to Idle */
	recvState = Idle;
	freePacket(_buffered_ePacket);
	_buffered_ePacket = NULL;

	return 1;
}

void OFDMA_PMPRS_MR::resetState()
{
        if (recvState != Idle)
                recvState = Idle;
}

void OFDMA_PMPRS_MR::dump_buf(int st, int en)
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
