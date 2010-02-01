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

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <nctuns_api.h>
#include <con_list.h>
#include <nodetype.h>
#include "ofdm_pmpss.h"

#include "../mac/structure.h"
#include "../mac/common.h"
#include "../mac/library.h"

#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>


#define VERBOSE_LEVEL MSG_WARNING
#include "../mac/verbose.h"

MODULE_GENERATOR(OFDM_PMPSS);

extern SLIST_HEAD(wimax_head, con_list) headOfWiMAX_;
extern typeTable*                       typeTable_;

OFDM_PMPSS::OFDM_PMPSS(u_int32_t type, u_int32_t id, struct plist *pl,
		       const char *name)
:OFDM_80216(type, id, pl, name)
{
	BASE_OBJTYPE(func);
	int i;

	printf("%d OFDM_PMPSS::OFDM_PMPSS()\n", get_nid());

	transPower = 23.0;	//      dBm
	_ChannelID = 1;
	_RecvSensitivity = -90;
	recvState = Idle;
	bufferedPacket = NULL;
	DLMapInfo = NULL;

	/* Initilize Timers */
	for (i = 0; i < 2; i++) {
		recvTimer[i] = new timerObj;
		assert(recvTimer[i]);
		func = POINTER_TO_MEMBER(OFDM_PMPSS, recvHandler);
		recvTimer[i]->setCallOutObj(this, func);
	}
}

OFDM_PMPSS::~OFDM_PMPSS()
{
	printf("%d OFDM_PMPSS::~OFDM_PMPSS()\n", get_nid());
}

int OFDM_PMPSS::init(void)
{
	DCDProfile =
	    GET_REG_VAR(get_port(), "DCDProfile",
			struct DCDBurstProfile *);
	UCDProfile =
	    GET_REG_VAR(get_port(), "UCDProfile",
			struct UCDBurstProfile *);

	return NslObject::init();
}

int OFDM_PMPSS::recv(Event * ep)
{
	Packet *p;
	struct PHYInfo *phyInfo;
	struct wphyInfo *wphyinfo;
	double *dist;
	double recvPower;

	p = (Packet *) ep->DataInfo_;
	phyInfo = (struct PHYInfo *) p->pkt_getinfo("phyInfo");
	dist = (double *) p->pkt_getinfo("dist");
	wphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");

	assert(p && phyInfo && dist && wphyinfo);

//      printf("%d OFDM_PMPSS::recv() (len = %d) @ %lld\n",
//              get_nid(), p->pkt_getlen(), GetCurrentTime());

//      printf("\tphyInfo: power=%lf, nsymbols=%d, fec=%d\n",
//              phyInfo->power, phyInfo->nsymbols, phyInfo->fec);

	if (phyInfo->ChannelID != _ChannelID) {
		freePacket(ep);
		return 1;
	}
	//recvPower = channelModel->receivePower(phyInfo->power, *dist);
	recvPower = wphyinfo->Pr_;
	phyInfo->SNR = channelModel->powerToSNR(recvPower);
	//printf("ss recv power %f, SNR %f, dist %f\n", recvPower, phyInfo->SNR, *dist);

	if (recvPower < _RecvSensitivity) {
		freeEvent(ep);
		return 1;
	}

	if (recvState != Idle) {
		printf
		    ("\tOFDM_PMPSS::recv() PHY is busy now! State (%d)\n",
		     recvState);
		exit(1);
	}

	if (bufferedPacket) {
		freePacket(bufferedPacket);
	}

	bufferedPacket = ep;

	recvState = ProcDLFP;
	burstPtr = p->pkt_sget();
	frameEndPtr = burstPtr + p->pkt_getlen();
	frameStartTime = GetCurrentTime();
	phyInfo->frameStartTime = frameStartTime;


	u_int64_t start_recv_time = GetCurrentTime();
	p->pkt_addinfo("srecv_t", (char *) &start_recv_time,
		       sizeof(u_int64_t));

	// Simulate the transmission time of the preceding
	// long preamble and the DLFP.
	scheduleTimer(recvTimer[0], (LONG_PREAMBLE + DLFP_LEN) * Ts);

	return 1;
}

int OFDM_PMPSS::send(Event * pkt)
{
	BurstInfo *burstInfo;
	vector < upBurst * >*burstCollection;
	struct PHYInfo phyInfo;
	Packet *ULburst;
	upBurst *burst;
	int burstLen;		// Burst length in bytes
	int nsymbols;		// Burst length in symbols
	char *input, *output;
	int fec;
	int len;

	//
	// Process commands from MAC.
	//
	if (procCmdFromMAC((Pkthdr *) pkt->DataInfo_)) {
		pkt->DataInfo_ = NULL;
		freePacket(pkt);
		return 1;
	}
	//
	// Send the uplink burst on behalf of MAC if there's no command.
	//
	burstInfo = (BurstInfo *) pkt->DataInfo_;
	burstCollection = (vector < upBurst * >*)burstInfo->Collection;

	if (burstCollection->size() != 1) {
		printf("%s(): burstCollection->size() != 1\n",
		       __FUNCTION__);
		exit(1);
	}
	// Compute the burst length.
	burst = *(burstCollection->begin());
	fec = UCDProfile[burst->ie.uiuc].fec;
	len = burst->length;
	input = burst->payload;
	burstLen = channelCoding->getCodedBurstLen(len, fec);

	if (burst->ie.uiuc == Initial_Ranging) {
		nsymbols = 5;
	} else if (burst->ie.uiuc == REQ_Full) {
		nsymbols = 2;
	} else {
		nsymbols =
		    SHORT_PREAMBLE +
		    (burstLen / channelCoding->getCodedBlockSize(fec));
	}

//      printf("%d OFDM_PMPSS::send() @ %lld uiuc=%d, fec=%d, burstLen=%d, nsymbols=%d\n",
//              get_nid(), GetCurrentTime(), burst->ie.uiuc, fec, burstLen, nsymbols);

	// Allocate a new Packet object.
	ULburst = new Packet;
	//ULburst->pkt_setflow(PF_RECV);
	output = ULburst->pkt_sattach(burstLen);
	ULburst->pkt_sprepend(output, burstLen);

	// Convey additional PHY information.
	memset(&phyInfo, 0, sizeof(struct PHYInfo));
	phyInfo.power = transPower;
	phyInfo.nsymbols = nsymbols;
	phyInfo.fec = fec;
	phyInfo.ChannelID = _ChannelID;
	memcpy(&phyInfo.ie, &burst->ie, sizeof(struct OFDM_ULMAP_IE));
	ULburst->pkt_addinfo("phyInfo", (char *) &phyInfo,
			     sizeof(struct PHYInfo));

//      printf("\tOFDM_ULMAP_IE: cid=%d, uiuc=%d, stime=%d\n",
//              phyInfo.ie.cid, phyInfo.ie.uiuc, phyInfo.ie.stime);

	// Encode the UL burst.
	channelCoding->encode(input, output, len, fec);

	/* add log information */
	struct LogInfo log_info;
	memset(&log_info, 0, sizeof(struct LogInfo));
	log_info.src_nid = get_nid();
	log_info.dst_nid = 0;	/* this field will be filled in sendToBS() */
	log_info.burst_len = burstLen;
	log_info.nsymbols = nsymbols;
	log_info.channel_id = _ChannelID;
	log_info.connection_id = 0;	/* unused so far */

	// Send the UL burst.
	sendToBS(ULburst, &log_info);
	delete ULburst;

	// Set pkt->DataInfo_ to NULL so that freePacket() won't free it again.
	pkt->DataInfo_ = NULL;
	delete burstInfo;

	// If the upper module wants to temporarily hold outgoing packets for
	// retransmission, it is the upper module's responsibility to duplicate
	// another copy of this packet.
	freePacket(pkt);

	return 1;
}

void OFDM_PMPSS::sendToBS(Packet * ULburst, struct LogInfo *log_info)
{
	Event *ep;
	struct con_list *wi;
	//double delay;		// in micro second
	double dist;
	//u_int64_t ticks;
	const char *nodeType;

	struct wphyInfo         *wphyinfo;
	double currAzimuthAngle;
	double T_locX, T_locY, T_locZ;
//      printf("%d OFDM_PMPSS::sendToBS()\n", get_nid());

	wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
	assert(wphyinfo);

	GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ);
	currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

	wphyinfo->nid           = get_nid();
	wphyinfo->pid           = get_port();
	wphyinfo->bw_           = 0;
	wphyinfo->channel_      = _ChannelID;
	wphyinfo->TxPr_         = pow(10, transPower/10) * 1e-3; // Watt
	wphyinfo->RxPr_         = 0.0;
	wphyinfo->srcX_         = T_locX;
	wphyinfo->srcY_         = T_locY;
	wphyinfo->srcZ_         = T_locZ;
	wphyinfo->currAzimuthAngle_     = currAzimuthAngle;
	wphyinfo->Pr_		= 0.0;

	ULburst->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
	free(wphyinfo);

	ULburst->pkt_setflow(PF_SEND);

	u_int32_t logflag = (!strncasecmp(WiMAXLogFlag, "on", 2)) ? 1 : 0;

	//BASE_OBJTYPE(type);
	//type = POINTER_TO_MEMBER(OFDM_80216, get);

	SLIST_FOREACH(wi, &headOfWiMAX_, nextLoc) {
		nodeType = typeTable_->toName(wi->obj->get_type());

		// The receiver should not be myself
		if (get_nid() == wi->obj->get_nid() &&
		    get_port() == wi->obj->get_port())
			continue;

		// We only send the frame to the BS.
		if (strcmp(nodeType, "WIMAX_PMP_BS") != 0) {
			continue;
		}

		log_info->dst_nid = wi->obj->get_nid();


		/* WiMAX PMP Mode Log Functionality */

		if (logflag) {

			struct logEvent *logep;
			u_char burst_type = FRAMETYPE_WIMAX_PMP_UBURST;

			mac80216_log_t *mac80216_log_p1 =
			    (mac80216_log_t *)
			    malloc(sizeof(mac80216_log_t));


			/* log StartTX event */
			LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
				       GetCurrentTime(), get_type(),
				       get_nid(), StartTX,
				       log_info->src_nid,
				       log_info->dst_nid, 0, burst_type,
				       log_info->burst_len,
				       log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
				       mac80216_log_p1->Time + START,
				       mac80216_log_p1);

			double trans_time = (log_info->nsymbols) * Ts;

			u_int64_t trans_time_in_tick;
			MICRO_TO_TICK(trans_time_in_tick, trans_time);

			mac80216_log_t *mac80216_log_p2 =
			    (mac80216_log_t *)
			    malloc(sizeof(mac80216_log_t));

			/* log SuccessTX event */
			LOG_MAC_802_16(mac80216_log_p2,
				       GetCurrentTime() +
				       trans_time_in_tick,
				       GetCurrentTime(), get_type(),
				       get_nid(), SuccessTX,
				       log_info->src_nid,
				       log_info->dst_nid, 0, burst_type,
				       log_info->burst_len,
				       log_info->channel_id);



#ifdef __JCLIN_OFDM_PMP_LOG_DEBUG

			printf
			    ("OFDM_PMPSS::send(): insert log event into heap.\n");

			dump_mac80216_logep(mac80216_log_p1);
			dump_mac80216_logep(mac80216_log_p2);

#endif

			INSERT_TO_HEAP(logep, mac80216_log_p2->PROTO,
				       mac80216_log_p2->Time + ENDING,
				       mac80216_log_p2);

			ULburst->pkt_addinfo("loginfo", (char *) log_info,
					     sizeof(struct LogInfo));

		}

		ep = createEvent();
		ep->DataInfo_ = ULburst->copy();

		dist = GetNodeDistance(get_nid(), wi->obj->get_nid());
		((Packet *) ep->DataInfo_)->pkt_addinfo("dist",
							(char *) &dist,
							sizeof(double));
		ep->timeStamp_ = 0;
		ep->perio_ = 0;
		/* set call out object */
		ep->calloutObj_ = wi->obj;
		ep->memfun_ = NULL;
		ep->func_ = NULL;
		NslObject::send(ep);
		//printf("========= ss send pkt\n");
		// Simulate propagation
		/*
		delay = (dist / SPEED_OF_LIGHT) * 1000000;
		MICRO_TO_TICK(ticks, delay);
		ticks += GetCurrentTime();

		setObjEvent(ep, ticks, 0, wi->obj, type, ep->DataInfo_);
		*/

//              printf("\tdistance(%d->%d)=%lf, delay=%lf\n", get_nid(), wi->obj->get_nid(), dist, delay);
	}

}

void OFDM_PMPSS::recvHandler()
{

	Event *ep;
	Packet *pkt;

	struct wphyInfo         *wphyinfo;
	struct PHYInfo *phyInfo;
	struct LogInfo *src_log_info;

	double *dist;
	double SNR, avgSNR;
	double BER;
	char *output;
	int diuc, fec;
	int len, burstLen, i;
	Packet *buffered_pkt_p = (Packet *) bufferedPacket->DataInfo_;

	//printf("%d OFDM_PMPSS::recvHandler() At %lld\n", get_nid(), GetCurrentTime());

	wphyinfo = (struct wphyInfo *) buffered_pkt_p->pkt_getinfo("WPHY");

	if(!wphyinfo){
		printf("[%s] Error: no wphyinfo\n", __func__);
		exit(1);
	}

	phyInfo =
	    (struct PHYInfo *) buffered_pkt_p->pkt_getinfo("phyInfo");

	dist = (double *) buffered_pkt_p->pkt_getinfo("dist");

	src_log_info =
	    (struct LogInfo *) buffered_pkt_p->pkt_getinfo("loginfo");


	/* Assertion for struct LogInfo structure */
	if (!src_log_info) {

		printf
		    ("OFDM_PMPSS::recvHandler(): source log information is lost.\n");
		exit(1);

	}

	u_int64_t *start_recv_time =
	    (u_int64_t *) buffered_pkt_p->pkt_getinfo("srecv_t");

	if (!start_recv_time) {

		printf
		    ("OFDM_PMPSS::recvHandler(): Frame receive time is lost.\n");
		exit(1);

	}


	struct LogInfo log_info;
	memset(&log_info, 0, sizeof(struct LogInfo));


	log_info.channel_id = _ChannelID;

	int log_flag = (!strcasecmp(WiMAXLogFlag, "on")) ? 1 : 0;

	u_char burst_type = 0;


	char	cm_obj_name[100];
	cm		*cmobj;

	sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
	cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
	if(!cmobj){
		printf("[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
		exit(1);
	}


	if (recvState == Idle) {
//              printf("\tState: Idle\n");

		if (DLMapInfo) {
			recvState = ProcDLMAP;
			pkt = (Packet *) bufferedPacket->DataInfo_;
			fec = DCDProfile[(unsigned char)DLMapInfo->usage].fec;
			len = DLMapInfo->end - DLMapInfo->start;
			burstPtr = pkt->pkt_sget() + DLMapInfo->byteOffset;

			// Simulate the transmission time of the burst.
			scheduleTimer(recvTimer[0], len * Ts);


			return;
			//exit(1);
		}

		printf("\tWrong state transition\n");
		exit(1);
	} else if (recvState == ProcDLFP) {
//              printf("\tState: ProcDLFP burstPtr=%p\n", burstPtr);

		// Decode DLFP.
		fec = BPSK_1_2;
		len = channelCoding->getCodedBlockSize(fec);

		SNR = phyInfo->SNR;
		BER = channelModel->computeBER(fec, SNR);
		channelModel->makeBitError(burstPtr, len, BER);
		channelCoding->decode(burstPtr, (char *) &framePrefix, len,
				      fec);
		burstPtr += len;

		if (hcs
		    (reinterpret_cast < char *>(&framePrefix),
		     sizeof(struct DLFP))) {
			printf("DLFP Header error: HCS is not match.\n");
			fflush(stdout);
			recvState = Idle;
			freePacket(bufferedPacket);
			bufferedPacket = NULL;
			return;
		}
		// Examine the 1st IE.
		fec = framePrefix.IE[0].diuc;
		len = DLFP_IE_GET_LEN(&framePrefix, 0);

		if (fec == 0 && len == 0) {
			printf("\tWrong: DLFP has no IEs\n");
			exit(1);
		}

		i = LONG_PREAMBLE + DLFP_LEN + len +
		    DLFP_IE_GET_LEN(&framePrefix,
				    1) + DLFP_IE_GET_LEN(&framePrefix,
							 2) +
		    DLFP_IE_GET_LEN(&framePrefix, 3);

		if (i > symbolsPerFrame) {
			printf("DLFP Len error: \n");
			fflush(stdout);
			recvState = Idle;
			freePacket(bufferedPacket);
			bufferedPacket = NULL;
			return;
		}

		burst_type = FRAMETYPE_WIMAX_PMP_DLFP;
		log_info.burst_len = i;

		recvState = ProcIE;	// Next state
		currProcIE = 0;	// Next IE to be processed

		// Simulate the transmission time of the 1st burst.
		scheduleTimer(recvTimer[0], len * Ts);

		u_int64_t next_start_recv_time;
		MICRO_TO_TICK(next_start_recv_time, len * Ts);
		next_start_recv_time += GetCurrentTime();
		buffered_pkt_p->pkt_addinfo("srecv_t",
					    (char *) &next_start_recv_time,
					    sizeof(u_int64_t));
	} else if (recvState == ProcIE) {
		double saveSNR;
		//printf("\tState: ProcIE burstPtr=%p\n", burstPtr);

		// Decode the burst and deliver it to the MAC layer.
		diuc = framePrefix.IE[currProcIE].diuc;
		fec = (currProcIE == 0) ? diuc : DCDProfile[diuc].fec;
		len = DLFP_IE_GET_LEN(&framePrefix, currProcIE);



		len *= channelCoding->getCodedBlockSize(fec);
		burstLen = channelCoding->getUncodedBurstLen(len, fec);

		burst_type = FRAMETYPE_WIMAX_PMP_DBURST;
		log_info.burst_len = burstLen;
		log_info.nsymbols = channelCoding->getCodedBlockSize(fec);

		//printf("\tburstPtr=%p, fec=%d, len=%d, burstLen=%d\n", burstPtr, fec, len, burstLen);
		if (burstPtr + burstLen > frameEndPtr) {
			printf
			    ("Err: burstPtr exceed boundary (%p+%x, %p)\n",
			     burstPtr, burstLen, frameEndPtr);
			recvState = Idle;
			freePacket(bufferedPacket);
			bufferedPacket = NULL;
			return;
		}

		pkt = new Packet;
		pkt->pkt_setflow(PF_RECV);
		output = pkt->pkt_sattach(burstLen);
		pkt->pkt_sprepend(output, burstLen);

		saveSNR = phyInfo->SNR;
		BER = channelModel->computeBER(fec, saveSNR);
		channelModel->makeBitError(burstPtr, len, BER);
		channelCoding->decode(burstPtr, output, len, fec);
		burstPtr += len;
//              printf("\tSNR=%lf, BER=%lf\n", SNR, BER);

		// We obtain avgSNR by averaging 100 samples. This facilitates the
		// module profile selection in MAC, otherwise MAC will probably 
		// receive some extreme values and make bad selection.

		//pkt->pkt_addinfo("WPHY", (char *) wphyinfo, sizeof(struct wphyInfo));

		for (i = 1, avgSNR = 0; i <= 100; i++) {
			SNR =
			    channelModel->powerToSNR(cmobj->computePr(wphyinfo));
			avgSNR += ((SNR - avgSNR) / i);
		}

		// Convey additional PHY information.
		phyInfo->SNR = avgSNR;
		pkt->pkt_addinfo("phyInfo", (char *) phyInfo,
				 sizeof(struct PHYInfo));

		phyInfo->SNR = saveSNR;



		ep = createEvent();
		ep->DataInfo_ = pkt;
		put(ep, recvtarget_);

		int more_burst_flag = 1;

		// Return if all IEs are processed.
		if (++currProcIE >= 4) {

			recvState = Idle;
			more_burst_flag = 0;
			//return;
		} else {

			// Examine the next IE.
			fec = framePrefix.IE[currProcIE].diuc;
			len = DLFP_IE_GET_LEN(&framePrefix, currProcIE);

			// If we encounter an empty IE, there will be no more bursts,
			if (fec == 0 && len == 0) {
				recvState = Idle;
				more_burst_flag = 0;
				//return;
			}
			// Simulate the transmission time of the next burst.
			if (more_burst_flag) {
				scheduleTimer(recvTimer[0], len * Ts);

				u_int64_t next_start_recv_time;
				next_start_recv_time += GetCurrentTime();
				buffered_pkt_p->pkt_addinfo("srecv_t",
							    (char *)
							    &next_start_recv_time,
							    sizeof
							    (u_int64_t));
			}

		}
	} else if (recvState == ProcDLMAP) {
//              printf("\tState: ProcDLMAP burstPtr=%p\n", burstPtr);

		// Decode the burst and deliver it to the MAC layer.
		fec = DCDProfile[(unsigned char)DLMapInfo->usage].fec;
		len = DLMapInfo->end - DLMapInfo->start;

		len *= channelCoding->getCodedBlockSize(fec);
		burstLen = channelCoding->getUncodedBurstLen(len, fec);

		pkt = new Packet;
		pkt->pkt_setflow(PF_RECV);
		output = pkt->pkt_sattach(burstLen);
		pkt->pkt_sprepend(output, burstLen);

		SNR = phyInfo->SNR;
		BER = channelModel->computeBER(fec, SNR);
		channelModel->makeBitError(burstPtr, len, BER);
		channelCoding->decode(burstPtr, output, len, fec);
//              printf("\tSNR=%lf, BER=%lf\n", SNR, BER);

		// We obtain avgSNR by averaging 100 samples. This facilitates the
		// profile selection in MAC, otherwise MAC will probably receive
		// some extreme values and make bad selection.

		//pkt->pkt_addinfo("WPHY", (char *) wphyinfo, sizeof(struct wphyInfo));

		for (i = 1, avgSNR = 0; i <= 100; i++) {
			SNR =
			    channelModel->powerToSNR(cmobj->computePr(wphyinfo));
			avgSNR += ((SNR - avgSNR) / i);
		}
		phyInfo->SNR = avgSNR;
//              printf("\tavgSNR=%lf\n", avgSNR);
		// Convey additional PHY information.
		pkt->pkt_addinfo("phyInfo", (char *) phyInfo,
				 sizeof(struct PHYInfo));
		phyInfo->SNR = SNR;

		burst_type = FRAMETYPE_WIMAX_PMP_DLMAP;
		log_info.burst_len = burstLen;

		ep = createEvent();
		ep->DataInfo_ = pkt;
		put(ep, recvtarget_);

		recvState = Idle;

		delete DLMapInfo;
		DLMapInfo = NULL;
	} else {
		printf("\tWrong state: unknown\n");
		exit(1);
	}

	if (log_flag) {

		/* add log information */
		log_info.dst_nid = get_nid();

		//u_char burst_type = wimax_burst_type_mapping(log_info->burst_type);            
		struct logEvent *logep;

		mac80216_log_t *mac80216_log_p1 =
		    (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

		double trans_time = 0.0;
		u_int64_t start_receive_time = 0;

		if (burst_type == FRAMETYPE_WIMAX_PMP_DLFP) {

			trans_time = DLFP_LEN * Ts;
		} else {

			trans_time = log_info.nsymbols * Ts;

		}

		u_int64_t trans_time_in_tick;
		MICRO_TO_TICK(trans_time_in_tick, trans_time);

		/* temporay codes until I make sure the transmission time 
		 * computing is exactly correct.
		 */

		if (burst_type == FRAMETYPE_WIMAX_PMP_DLFP) {

			start_receive_time =
			    GetCurrentTime() - trans_time_in_tick;

		} else {

			start_receive_time = *start_recv_time;

		}


		//printf("trans_time_intick = %llu \n", trans_time_in_tick);
		/* log StartRX event */
		LOG_MAC_802_16(mac80216_log_p1,
			       start_receive_time, start_receive_time,
			       get_type(), get_nid(), StartRX,
			       src_log_info->src_nid, log_info.dst_nid,
			       0, burst_type, log_info.burst_len,
			       log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
			       mac80216_log_p1->Time + START,
			       mac80216_log_p1);


		mac80216_log_t *mac80216_log_p2 =
		    (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

		/* log SuccessRX event */
		LOG_MAC_802_16(mac80216_log_p2,
			       start_receive_time + trans_time_in_tick,
			       start_receive_time,
			       get_type(), get_nid(), SuccessRX,
			       src_log_info->src_nid, log_info.dst_nid,
			       0, burst_type, log_info.burst_len,
			       log_info.channel_id);



		//#define __JCLIN_OFDM_PMP_LOG_DEBUG
#ifdef __JCLIN_OFDM_PMP_LOG_DEBUG

		printf
		    ("OFDM_PMPBS::recvHandler(): insert log event into heap.\n");

		dump_mac80216_logep(mac80216_log_p1);
		dump_mac80216_logep(mac80216_log_p2);

#endif

		INSERT_TO_HEAP(logep, mac80216_log_p2->PROTO,
			       mac80216_log_p2->Time + ENDING,
			       mac80216_log_p2);


	}
}


int OFDM_PMPSS::procCmdFromMAC(Pkthdr * pkthdr)
{
	ntfyDLMAP *cmd = (ntfyDLMAP *) pkthdr;
	vector < dlmapinfo >::iterator iter;
	double timeInMicro;
	int handledSymbols = LONG_PREAMBLE + DLFP_LEN;
	int elapsedSymbols;
	int i;

	if (pkthdr->type == 2) {
//              printf("%d OFDM_PMPSS::procCmdFromMAC() At %lld\n", get_nid(), GetCurrentTime());

		for (i = 0; i < 4; i++) {
			handledSymbols += DLFP_IE_GET_LEN(&framePrefix, i);
		}
		if (i == 4)
			handledSymbols--;

		// We assume there is at most one more burst to receive.
		for (iter = cmd->info.begin(); iter != cmd->info.end();
		     iter++) {
//                      printf("\tFrom %d to %d using diuc %d byteOffset %d ",
//                              (*iter).start, (*iter).end, (*iter).usage, (*iter).byteOffset);

			if ((*iter).start <= handledSymbols) {
//                              printf("(already indicated by DLFP)\n");
			} else if (DLMapInfo == NULL) {
				printf("(OK)\n");

				DLMapInfo = new dlmapinfo;
				DLMapInfo->start = (*iter).start;
				DLMapInfo->end = (*iter).end;
				DLMapInfo->usage = (*iter).usage;
				DLMapInfo->byteOffset =
				    (*iter).byteOffset +
				    channelCoding->
				    getCodedBlockSize(BPSK_1_2);
			} else {
				printf("\t(Wrong?)\n");
				exit(1);
			}
		}

		delete cmd;
		cmd = NULL;

		if (!DLMapInfo) {
			return 1;
		}

		TICK_TO_MICRO(timeInMicro,
			      (GetCurrentTime() - frameStartTime));
		elapsedSymbols = (int) (timeInMicro / Ts) + 1;
//              printf("\tframeStartTime=%lld, elapsedSymbols=%d\n", frameStartTime, elapsedSymbols);

		// Schedule a timer to handle the burst.
		scheduleTimer(recvTimer[1],
			      (DLMapInfo->start - elapsedSymbols) * Ts);

		return 1;
	}

	return 0;
}
