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
#include "ofdm_pmpbs.h"

#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>


#include "../mac/structure.h"
#include "../mac/common.h"
#include "../mac/library.h"

#define VERBOSE_LEVEL MSG_WARNING
#include "../mac/verbose.h"

MODULE_GENERATOR(OFDM_PMPBS);

extern SLIST_HEAD(wimax_head, con_list) headOfWiMAX_;
extern typeTable*                       typeTable_;

OFDM_PMPBS::OFDM_PMPBS(u_int32_t type, u_int32_t id, struct plist *pl,
		       const char *name)
:OFDM_80216(type, id, pl, name)
{
	printf("%d OFDM_PMPBS::OFDM_PMPBS()\n", get_nid());

	transPower = 37.0;	//      dBm
	_ChannelID = 1;
	_RecvSensitivity = -115;
	recvState = Idle;
}

OFDM_PMPBS::~OFDM_PMPBS()
{
	printf("%d OFDM_PMPBS::~OFDM_PMPBS()\n", get_nid());
}

int OFDM_PMPBS::init(void)
{
	DCDProfile =
	    GET_REG_VAR(get_port(), "DCDProfile",
			struct DCDBurstProfile *);
	UCDProfile =
	    GET_REG_VAR(get_port(), "UCDProfile",
			struct UCDBurstProfile *);


	/* Mac 802.16 Mesh mode log */
	if (WirelessLogFlag && !strcasecmp(WiMAXLogFlag, "on")) {

		if (!ptrlogFileOpenFlag) {

			ptrlogFileOpenFlag = true;

			char *ptrFile = NULL;

			if (ptrlogFileName) {

				ptrFile = (char *) malloc(strlen
							  (GetConfigFileDir
							   ()) +
							  strlen
							  (ptrlogFileName)
							  + 1);

				sprintf(ptrFile, "%s%s",
					GetConfigFileDir(),
					ptrlogFileName);

				fptr = fopen(ptrFile, "w+");

				free(ptrFile);

			} else {
				ptrFile = (char *) malloc(strlen
							  (GetScriptName())
							  + 5);
				sprintf(ptrFile, "%s.ptr",
					GetScriptName());
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}

			if (fptr == NULL) {
				printf
				    ("Error : Cannot create packe trace file %s\n",
				     ptrFile);
				exit(-1);
			}

			Event_ *heapHandle = createEvent();
			u_int64_t time;
			MILLI_TO_TICK(time, 100);
			u_int64_t chkInt = GetCurrentTime() + time;
			setEventTimeStamp(heapHandle, chkInt, 0);

			int (*__fun) (Event_ *) =
			    (int (*)(Event_ *)) &DequeueLogFromHeap;

			setEventCallOutFunc(heapHandle, __fun, heapHandle);
			scheduleInsertEvent(heapHandle);
		}


	}

	return NslObject::init();
}

int OFDM_PMPBS::recv(Event * ep)
{
	BASE_OBJTYPE(type);
	Packet *p;
	struct PHYInfo *phyInfo;
	struct LogInfo *log_info = NULL;

	u_int64_t timeInTick = 0;
	double recvPower, *dist;
	struct wphyInfo *wphyinfo;

	p = (Packet *) ep->DataInfo_;
	phyInfo = (struct PHYInfo *) p->pkt_getinfo("phyInfo");
	dist = (double *) p->pkt_getinfo("dist");
	wphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");

	int log_flag = (!strcasecmp(WiMAXLogFlag, "on")) ? 1 : 0;

	if (log_flag) {

		log_info = (struct LogInfo *) p->pkt_getinfo("loginfo");

		if (!log_info) {

			printf
			    ("OFDM_Mesh::recv(): missing log information. \n");
			exit(1);

		}
	}


	assert(p && phyInfo && dist && wphyinfo);

//      printf("%d OFDM_PMPBS::recv() (len=%d) @ %lld\n", get_nid(), p->pkt_getlen(), GetCurrentTime());

//      printf("\tphyInfo: power=%lf, nsymbols=%d, fec=%d\n",
//              phyInfo->power, phyInfo->nsymbols, phyInfo->fec);

	if (phyInfo->ChannelID != _ChannelID) {
		freeEvent(ep);
		return 1;
	}
	//recvPower = channelModel->receivePower(phyInfo->power, *dist);
	recvPower = wphyinfo->Pr_;
	//fprintf(stderr, "========= recvPower %f, dist %f\n", recvPower, *dist);
	phyInfo->SNR = channelModel->powerToSNR(recvPower);

	if (recvPower < _RecvSensitivity) {
		freeEvent(ep);
		return 1;
	}

	if (recvState != Idle) {
		printf("\tCollided!\n");
		recvState = Collided;

		if (log_flag) {

			struct logEvent *logep;

			mac80216_log_t *mac80216_log_p1 =
			    (mac80216_log_t *)
			    malloc(sizeof(mac80216_log_t));
			mac80216_log_t *mac80216_log_p2 =
			    (mac80216_log_t *)
			    malloc(sizeof(mac80216_log_t));

			double trans_time = (log_info->nsymbols) * Ts;
			u_int64_t trans_time_in_tick;
			MICRO_TO_TICK(trans_time_in_tick, trans_time);

			/* log StartRX packets */
			u_char burst_type = FRAMETYPE_WIMAX_PMP_UBURST;

			LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
				       GetCurrentTime(), get_type(),
				       get_nid(), StartRX,
				       log_info->src_nid, get_nid(),
				       log_info->connection_id, burst_type,
				       log_info->burst_len,
				       log_info->channel_id);

			INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
				       mac80216_log_p1->Time + START,
				       mac80216_log_p1);


			if (recvState == Collided) {

				DROP_LOG_802_16(mac80216_log_p1,
						mac80216_log_p2,
						GetCurrentTime() +
						trans_time_in_tick, DropRX,
						DROP_COLL);
				printf
				    ("OFDM_PMPBS: drop a packet due to collision.\n\n");

#ifdef __JCLIN_OFDM_PMPBS_LOG_DEBUG
				dump_mac80216_logep(mac80216_log_p1);
				dump_mac80216_logep(mac80216_log_p2);
#endif

			} else {

				LOG_MAC_802_16(mac80216_log_p2,
					       GetCurrentTime() +
					       trans_time_in_tick,
					       GetCurrentTime(),
					       get_type(), get_nid(),
					       SuccessRX,
					       log_info->src_nid,
					       get_nid(),
					       log_info->connection_id,
					       burst_type,
					       log_info->burst_len,
					       log_info->channel_id);


			}


			INSERT_TO_HEAP(logep, mac80216_log_p2->PROTO,
				       mac80216_log_p2->Time + ENDING,
				       mac80216_log_p2);

		}



		freeEvent(ep);
		return 1;
	}
	//
	// Simulate the transmission time of the uplink burst.
	//
	MICRO_TO_TICK(timeInTick, (phyInfo->nsymbols) * Ts);
	timeInTick += GetCurrentTime();
	timeInTick -= 2;	// To avoid unnecessary collision.
	type = POINTER_TO_MEMBER(OFDM_PMPBS, recvHandler);

	setObjEvent(ep, timeInTick, 0, this, type, ep->DataInfo_);
	recvState = Busy;

	/* C.C. Lin:
	 * Since  OFDM_PMPBS module automatically decrements the receive transmission
	 * time, add the information of correct time point that the module starts
	 * to receive this burst into the packet. In such way, the events representing
	 * START_XXX can match its corresponding END_XXX events correctly.
	 */


	u_int64_t start_recv_time = GetCurrentTime();
	p->pkt_addinfo("srecv_t", (const char *) &start_recv_time,
		       sizeof(u_int64_t));



	struct logEvent *logep = NULL;

	mac80216_log_t *mac80216_log_p1 =
	    (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

	/* log StartRX packets */
	u_char burst_type = FRAMETYPE_WIMAX_PMP_UBURST;

	LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(), GetCurrentTime(),
		       get_type(), get_nid(), StartRX, log_info->src_nid,
		       get_nid(), log_info->connection_id, burst_type,
		       log_info->burst_len, log_info->channel_id);


	INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
		       mac80216_log_p1->Time + START, mac80216_log_p1);



//      printf("\tAt %lld, schedule a event which will be triggered at %lld (%lf us later)\n", 
//              GetCurrentTime(), timeInTick, (phyInfo->nsymbols)*Ts);

	return 1;
}

int OFDM_PMPBS::send(Event * pkt)
{
	BurstInfo *burstInfo;
	vector < downBurst * >*burstCollection;
	vector < downBurst * >::iterator it;
	struct PHYInfo phyInfo;
	downBurst *burst;
	Packet *DLframe;
	struct DLFP prefix;
	int frameLen;		// Frame length in bytes
	int nsymbols;		// Frame length in symbols
	char *input;
	char *output;
	int fec;
	int len;
	int n;

	burstInfo = (BurstInfo *) pkt->DataInfo_;
	pkt->DataInfo_ = NULL;	// We will free DataInfo_ by ourself.
	burstCollection = (vector < downBurst * >*)burstInfo->Collection;

	if (burstCollection->size() == 0) {
		printf("%s(): burstCollection is Empty!\n", __FUNCTION__);
		exit(1);
	}
//      printf("%d OFDM_PMPBS::send() burstCollection->size() = %d\n",
//              get_nid(), burstCollection->size());

	// Compute the total frame length: DLFP
	frameLen = channelCoding->getCodedBlockSize(BPSK_1_2);
	nsymbols = 1;

	// Compute the total frame length: DL bursts
	for (it = burstCollection->begin(); it != burstCollection->end();
	     it++) {
		burst = *it;
		fec = DCDProfile[burst->ie.diuc].fec;
		len = burst->length;

		n = channelCoding->getCodedBurstLen(len, fec);
		frameLen += n;
		nsymbols += (n / channelCoding->getCodedBlockSize(fec));

//              printf("\tData Burst: fec=%d len=%d coded=%d\n", fec, len, n);
	}

	if (nsymbols > symbolsPerFrame) {
		printf("[%u]%s(): DL subframe is too long!\n", get_nid(),
		       __FUNCTION__);
		exit(0);
	}
	// Allocate a new Packet object according to the frame length.
	DLframe = new Packet;
	//DLframe->pkt_setflow(PF_RECV);
	output = DLframe->pkt_sattach(frameLen);
	DLframe->pkt_sprepend(output, frameLen);

	// Convey additional PHY information.
	memset(&phyInfo, 0, sizeof(struct PHYInfo));
	phyInfo.power = transPower;
	phyInfo.ChannelID = _ChannelID;
	DLframe->pkt_addinfo("phyInfo", (const char *) &phyInfo,
			     sizeof(struct PHYInfo));

	// Build and encode the DLFP.
	buildDLFP(&prefix, burstCollection);
	n = channelCoding->encode((char *) &prefix, output, sizeof(prefix),
				  BPSK_1_2);
	output += n;

	// Encode DL bursts.
	for (it = burstCollection->begin(); it != burstCollection->end();
	     it++) {
		burst = *it;
		fec = DCDProfile[burst->ie.diuc].fec;
		len = burst->length;
		input = burst->payload;

		n = channelCoding->encode(input, output, len, fec);
		output += n;
	}
	//printf("\tframeLen = %d, nsymbols = %d\n", frameLen, nsymbols);

	/* WiMAX PMP Mode Log Functionality */


	u_int32_t logflag = (!strncasecmp(WiMAXLogFlag, "on", 2)) ? 1 : 0;

	if (logflag) {

		struct LogInfo log_info;
		memset(&log_info, 0, sizeof(struct LogInfo));

		log_info.nsymbols = nsymbols;
		log_info.burst_len = frameLen;
		log_info.channel_id = _ChannelID;
		log_info.src_nid = get_nid();

		//u_char burst_type = wimax_burst_type_mapping(log_info->burst_type);
		u_char burst_type = FRAMETYPE_WIMAX_PMP_DBURST;
		log_info.dst_nid = PHY_BROADCAST_ID;

		struct logEvent *logep;

		mac80216_log_t *mac80216_log_p1 =
		    (mac80216_log_t *) malloc(sizeof(mac80216_log_t));


		/* log StartTX event */
		LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
			       GetCurrentTime(), get_type(), get_nid(),
			       StartTX, log_info.src_nid, log_info.dst_nid,
			       0, burst_type, log_info.burst_len,
			       log_info.channel_id);

		INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
			       mac80216_log_p1->Time + START,
			       mac80216_log_p1);

		double trans_time = (log_info.nsymbols) * Ts;

		u_int64_t trans_time_in_tick;
		MICRO_TO_TICK(trans_time_in_tick, trans_time);

		mac80216_log_t *mac80216_log_p2 =
		    (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

		/* log SuccessTX event */
		LOG_MAC_802_16(mac80216_log_p2,
			       GetCurrentTime() + trans_time_in_tick,
			       GetCurrentTime(), get_type(), get_nid(),
			       SuccessTX, log_info.src_nid,
			       log_info.dst_nid, 0, burst_type,
			       log_info.burst_len, log_info.channel_id);



#ifdef __JCLIN_OFDM_PMP_LOG_DEBUG

		printf
		    ("OFDM_PMPBS::send(): insert log event into heap.\n");

		dump_mac80216_logep(mac80216_log_p1);
		dump_mac80216_logep(mac80216_log_p2);

#endif

		INSERT_TO_HEAP(logep, mac80216_log_p2->PROTO,
			       mac80216_log_p2->Time + ENDING,
			       mac80216_log_p2);

		DLframe->pkt_addinfo("loginfo", (const char *) &log_info,
				     sizeof(struct LogInfo));

	}
	// Send the DL subframe.
	sendToSSs(DLframe);
	delete DLframe;

	delete burstInfo;

	// If the upper module wants to temporarily hold outgoing packets for
	// retransmission, it is the upper module's responsibility to duplicate
	// another copy of this packet.
	freePacket(pkt);

	return 1;
}

void OFDM_PMPBS::sendToSSs(Packet * DLframe)
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
//      printf("%d OFDM_PMPBS::sendToSSs()\n", get_nid());

	//BASE_OBJTYPE(type);
	//type = POINTER_TO_MEMBER(OFDM_80216, get);

	wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
	assert(wphyinfo);

	GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ);
	currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

	wphyinfo->nid           = get_nid();
	wphyinfo->pid           = get_port();
	wphyinfo->bw_           = 0;
	wphyinfo->channel_      = _ChannelID;
	wphyinfo->TxPr_         = pow(10, transPower/10) * 1e-3;
	wphyinfo->RxPr_         = 0.0;
	wphyinfo->srcX_         = T_locX;
	wphyinfo->srcY_         = T_locY;
	wphyinfo->srcZ_         = T_locZ;
	wphyinfo->currAzimuthAngle_     = currAzimuthAngle;
	wphyinfo->Pr_		= 0.0;

	DLframe->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
	free(wphyinfo);

	DLframe->pkt_setflow(PF_SEND);
	SLIST_FOREACH(wi, &headOfWiMAX_, nextLoc) {

		nodeType = typeTable_->toName(wi->obj->get_type());

		// The receiver should not be myself
		if (get_nid() == wi->obj->get_nid() &&
		    get_port() == wi->obj->get_port())
			continue;

		// We only send the frame to SSs.
		if (strcmp(nodeType, "WIMAX_PMP_SS") != 0) {

			continue;

		}

		ep = createEvent();
		ep->DataInfo_ = DLframe->copy();	/* logstructure in PT_INFO is copied by packet::copy() */

		dist = GetNodeDistance(get_nid(), wi->obj->get_nid());
		((Packet *) ep->DataInfo_)->pkt_addinfo("dist",
							(char *) &dist,
							sizeof(double));
		// Simulate propagation
		/*
		delay = (dist / SPEED_OF_LIGHT) * 1000000;
		MICRO_TO_TICK(ticks, delay);
		ticks += GetCurrentTime();

		setObjEvent(ep, ticks, 0, wi->obj, type, ep->DataInfo_);
		*/
		ep->timeStamp_ = 0;
		ep->perio_ = 0;
		ep->calloutObj_ = wi->obj;
		ep->memfun_ = NULL;
		ep->func_ = NULL;
		NslObject::send(ep);

//              printf("\tdistance(%d->%d)=%lf, delay=%lf\n", get_nid(), wi->obj->get_nid(), dist, delay);
//              printf("\twill arrive at %lld\n", ticks);
	}
}

void OFDM_PMPBS::recvHandler(Event * pkt)
{
	Event_ *ep;
	Packet *p;

	struct wphyInfo *wphyinfo;
	struct PHYInfo *phyInfo;
	struct LogInfo *log_info;


	double *dist;
	char *input;
	char *output;
	int fec;
	int len;
	int burstLen;
	double avgSNR, SNR, saveSNR;
	double BER;
	int i;

//      printf("%d OFDM_PMPBS::recvHandler() At %lld\n", get_nid(), GetCurrentTime());

	p = (Packet *) pkt->DataInfo_;
	wphyinfo = (struct wphyInfo *) p->pkt_getinfo("WPHY");
	phyInfo = (struct PHYInfo *) p->pkt_getinfo("phyInfo");
	dist = (double *) p->pkt_getinfo("dist");

	int log_flag = (!strcasecmp(WiMAXLogFlag, "on")) ? 1 : 0;
	if (log_flag) {

		log_info = (struct LogInfo *) p->pkt_getinfo("loginfo");

		if (!log_info) {

			printf
			    ("OFDM_PMPBS::recvHandler(): missing log information. \n");
			exit(1);

		}

		struct logEvent *logep;

		mac80216_log_t *mac80216_log_p1 =
		    (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

		u_char burst_type = FRAMETYPE_WIMAX_PMP_UBURST;

		u_int64_t *start_recv_time =
		    (u_int64_t *) p->pkt_getinfo("srecv_t");

		if (recvState == Collided) {

			LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
				       *start_recv_time, get_type(),
				       get_nid(), DropRX,
				       log_info->src_nid, get_nid(),
				       log_info->connection_id, burst_type,
				       log_info->burst_len,
				       log_info->channel_id);

			mac80216_log_p1->DropReason = DROP_COLL;


		} else {

			LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
				       *start_recv_time, get_type(),
				       get_nid(), SuccessRX,
				       log_info->src_nid, get_nid(),
				       log_info->connection_id, burst_type,
				       log_info->burst_len,
				       log_info->channel_id);


		}


		INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
			       mac80216_log_p1->Time + ENDING,
			       mac80216_log_p1);

	}


	if (recvState == Collided) {
		printf("\tDropped!\n");
		recvState = Idle;
		freePacket(pkt);
		return;
	}
	//
	// Decode the burst and deliver it to the MAC layer.
	//
	input = p->pkt_sget();
	fec = phyInfo->fec;
	len = p->pkt_getlen();

	burstLen = channelCoding->getUncodedBurstLen(len, fec);

	p = new Packet;
	p->pkt_setflow(PF_RECV);
	output = p->pkt_sattach(burstLen);
	p->pkt_sprepend(output, burstLen);

	saveSNR = phyInfo->SNR;
	BER = channelModel->computeBER(fec, saveSNR);
	channelModel->makeBitError(input, len, BER);
	channelCoding->decode(input, output, len, fec);

	// We obtain avgSNR by averaging 300 samples. This facilitates the
	// profile selection in MAC, otherwise MAC will probably receive
	// some extreme values and make bad selection.
	char	cm_obj_name[100];
	cm		*cmobj;

	sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
	cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
	if(!cmobj){
		fprintf(stderr, "[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
		exit(0);
	}

	for (i = 1, avgSNR = 0; i <= 300; i++) {
		SNR = channelModel->powerToSNR(cmobj->computePr(wphyinfo));
		avgSNR += ((SNR - avgSNR) / i);
	}

	//p->pkt_addinfo("WPHY", (char *) wphyinfo, sizeof(struct wphyInfo));
	// Convey additional PHY information.
	phyInfo->SNR = avgSNR;
	p->pkt_addinfo("phyInfo", (char *) phyInfo,
		       sizeof(struct PHYInfo));

	phyInfo->SNR = saveSNR;

	ep = createEvent();
	ep->DataInfo_ = p;
	put(ep, recvtarget_);

	recvState = Idle;
	freePacket(pkt);
}

void OFDM_PMPBS::buildDLFP(struct DLFP *prefix,
			   vector < downBurst * >*burstCollection)
{
	vector < downBurst * >::iterator it;
	downBurst *burst;
	int fec, len, i;

	prefix->bs_id = 0;
	prefix->frame_num = 0;
	prefix->config_chg_cnt = 0;
	prefix->rsv = 0;

	for (it = burstCollection->begin(), i = 0;
	     it != burstCollection->end() && i < 4; it++, i++) {
		burst = *it;
		fec = DCDProfile[burst->ie.diuc].fec;
		len =
		    (burst->length /
		     channelCoding->getUncodedBlockSize(fec)) + 1;

		if (i)
			prefix->IE[i].diuc = burst->ie.diuc;
		else
			prefix->IE[i].diuc = fec;

		prefix->IE[i].has_preamble = 0;
		prefix->IE[i].len_msb = len / 256;
		prefix->IE[i].len_lsb = len % 256;
	}

	for (; i < 4; i++) {
		prefix->IE[i].diuc = 0;
		prefix->IE[i].has_preamble = 0;
		prefix->IE[i].len_msb = 0;
		prefix->IE[i].len_lsb = 0;
	}

	prefix->hcs =
	    hcs(reinterpret_cast < char *>(prefix),
		sizeof(struct DLFP) - 1);

/*	// Dump the DLFP
	printf("OFDM_PMPBS::buildDLFP() DLFP:\n");
	for (i=0; i<4; i++)
	{
		printf("\tIE[%d]: diuc=%d, len=%d\n", i, prefix->IE[i].diuc,
		DLFP_IE_GET_LEN(prefix, i));
	}
*/
}
