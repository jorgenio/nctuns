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

#include "mac802_16j_NT_pmprs.h"
#include <hash_map>

using namespace std;
using namespace MR_Common_NT;
using namespace MR_MacAddress_NT;
using namespace MR_Timer_NT;
using namespace MR_MSbase_NT;
using namespace MR_RSbase_NT;
using namespace MR_OFDMAMapIE_NT;

MODULE_GENERATOR( mac802_16j_NT_PMPRS);

mac802_16j_NT_PMPRS::mac802_16j_NT_PMPRS(uint32_t type, uint32_t id, plist* pl,
		const char* name) :
	mac802_16RSbase(type, id, pl, name) {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::mac802_16j_NT_PMPRS State: %x",
			get_nid(), State);
	vBind("LinkMode", &LinkMode);
	vBind("CSTYPE", &CSTYPE);

	_maxqlen = 10000;
	LinkMode = NULL;

	// Spec 10.1 Table 342
	LostULinterval = 600; // ms
	LostDLinterval = 600; // ms
	LostRMAPinterval = 600; // ms (apec 16j Table 554)
	PeriodRangingInterval = 0; // unit in frames
	HORetransmitInterval = 0; // unit in frames
	ScanReqInterval = 2; // s

	ServerLevelPredict = 0;
	HOReqRetryAvailable = 0;
	RngLastStatus = Init;
	RngCodeRegion = RS_INITIAL_RANGING;
	TimingAdjust = 0;
	ifsavedmm = NULL;

	brConnection = NULL;
	_ULAccessSymbols = 0;
	_DLAccessSymbols = 0;
	_ULRelaySymbols = 0;
	_DLRelaySymbols = 0;
	relay_ind = false;
	RelayULallocStartTime = 0;

	//    uint64_t    ticks = 0;

	pScheduler = new RS_16j_NT_Scheduler(this);
	NeighborMRBSList = new NeighborMRBSs_NT();
	ScanFlag = false;
	assert(pScheduler && NeighborMRBSList);

	broadcastConnection = new BroadcastConnection(broadcastCID);
	initRangingConnection = new ManagementConnection(initRangingCID);
	assert(broadcastConnection && initRangingConnection);

	BASE_OBJTYPE(mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T1);
	timer_mngr()->set_func_t(1u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T2);
	timer_mngr()->set_func_t(2u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T3);
	timer_mngr()->set_func_t(3u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T6);
	timer_mngr()->set_func_t(6u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T8);
	timer_mngr()->set_func_t(8u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T12);
	timer_mngr()->set_func_t(12u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T18);
	timer_mngr()->set_func_t(18u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T42);
	timer_mngr()->set_func_t(42u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T62);
	timer_mngr()->set_func_t(62u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, T67);
	timer_mngr()->set_func_t(67u, this, mem_func);

	timerLostDL = new timerObj;
	timerLostUL = new timerObj;
	timerSendRngCode = new timerObj;
	timerSendRngReq = new timerObj;
	timerSendDL = new timerObj;
	timerSendUL = new timerObj;
	timerHORetransmit = new timerObj;
	timerPeriodRanging = new timerObj;
	timerSendScanReq = new timerObj;
	timerStartScan = new timerObj;
	timerStartHO = new timerObj;

	assert(timerLostDL && timerLostUL && timerSendRngCode && timerSendRngReq &&
			timerSendUL && timerHORetransmit && timerPeriodRanging && timerSendScanReq &&
			timerStartScan && timerStartHO);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, LostDLMAP);
	timerLostDL->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, LostULMAP);
	timerLostUL->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, SendRNGCODE);
	timerSendRngCode->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, SendRNGREQ);
	timerSendRngReq->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, ULPacketScheduling);
	timerSendUL->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, DLPacketScheduling);
	timerSendDL->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, HOmsgRetransmit);
	timerHORetransmit->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, StartPeriodRanging);
	timerPeriodRanging->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, SendMOB_SCNREQ);
	timerSendScanReq->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, StartScanNbrMRBS_NTs);
	timerStartScan->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPRS, StartHandover);
	timerStartHO->setCallOutObj(this, mem_func);

	// Spec 16e. Table 338
	fecSNR[QPSK_1_2] = 5;
	fecSNR[QPSK_3_4] = 8;
	fecSNR[QAM16_1_2] = 10.5;
	fecSNR[QAM16_3_4] = 14;
	fecSNR[QAM64_1_2] = 16;
	fecSNR[QAM64_2_3] = 18;
	fecSNR[QAM64_3_4] = 20;

	UCDCfgCount = 0;
	DCDCfgCount = 0;

	for (int i = 0; i < 16; i++) {
		DCDProfile[i].used = 0;
		UCDProfile[i].used = 0;
	}

	/* register variable */
	REG_VAR("MAC", address()->buf());
	REG_VAR("DCDProfile", &DCDProfile);
	REG_VAR("UCDProfile", &UCDProfile);
}

mac802_16j_NT_PMPRS::~mac802_16j_NT_PMPRS() {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::~mac802_16j_NT_PMPRS State: %x",
			get_nid(), State);

	while (!MnConnections.empty()) {
		delete *(MnConnections.begin());
		MnConnections.erase(MnConnections.begin());
	}

	while (!DtConnections.empty()) {
		delete *(DtConnections.begin());
		DtConnections.erase(DtConnections.begin());
	}

	while (!ReceiverList.empty()) {
		delete *(ReceiverList.begin());
		ReceiverList.erase(ReceiverList.begin());
	}

	while (!ULRelayMnConnections.empty()) {
		delete *(ULRelayMnConnections.begin());
		ULRelayMnConnections.erase(ULRelayMnConnections.begin());
	}

	while (!ULRelayDtConnections.empty()) {
		delete *(ULRelayDtConnections.begin());
		ULRelayDtConnections.erase(ULRelayDtConnections.begin());
	}

	while (!ULRelayReceiverList.empty()) {
		delete *(ULRelayReceiverList.begin());
		ULRelayReceiverList.erase(ULRelayReceiverList.begin());
	}

	while (!DLRelayMnConnections.empty()) {
		delete *(ULRelayMnConnections.begin());
		ULRelayMnConnections.erase(ULRelayMnConnections.begin());
	}

	while (!DLRelayDtConnections.empty()) {
		delete *(DLRelayDtConnections.begin());
		DLRelayDtConnections.erase(DLRelayDtConnections.begin());
	}

	while (!DLRelayReceiverList.empty()) {
		delete *(ULRelayReceiverList.begin());
		DLRelayReceiverList.erase(DLRelayReceiverList.begin());
	}

	delete pScheduler;
	delete NeighborMRBSList;
	delete timerSendRngCode;
	delete timerSendRngReq;
	delete timerSendDL;
	delete timerSendUL;
	delete timerLostDL;
	delete timerLostUL;
	delete timerHORetransmit;
	delete timerPeriodRanging;
	delete timerSendScanReq;
	delete timerStartScan;
	delete timerStartHO;
	delete ifsavedmm;
	delete initRangingConnection;
	delete broadcastConnection;
}

int mac802_16j_NT_PMPRS::init() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::init State: %x", get_nid(),
			State);
	uint64_t tick_interval = 0;

	State = NTRS_Init;
	AttrARQ = 0;
	NE_success = false;
	rs_br_hdr_rsp = false;
	mr_coderep_rsp = false;

	MILLI_TO_TICK(tick_interval, 500); // 500 ms
	timer_mngr()->set_interval_t(18u, tick_interval);

	_frameDuCode = GET_REG_VAR(get_port(), "frameDuCode", int *);

	MILLI_TO_TICK(tick_interval, frameDuration_NT[*_frameDuCode]); // frameDuration_NT: 5 ms
	timerSendDL->start(tick_interval, tick_interval); // period timer

	NeighborMRBSList->ServingBSchID
			= ((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->getChannelID();
	NeighborMRBSList->NBRADV_CfgCount = 0;

	return mac802_16j_NT::init();
}

int mac802_16j_NT_PMPRS::recv(ePacket_ *epkt) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::recv State: %x", get_nid(),
			State);
	Packet *recvBurst = NULL;
	char *adaptedBurst = NULL;
	char *ptr = NULL;
	struct mgmt_msg *recvmm = NULL;
	unsigned int crc = 0;
	uint64_t tick = 0;
	int cid = 0;
	int len = 0;
	int plen = 0;
	int burstLen = 0;
	struct hdr_generic *hg = NULL;
	int nid = 0;
	int pid = 0;
	mac802_16j_NT_PMPMS *pMS = NULL;
	ManagementConnection *pConn = NULL;
	DataConnection *pDtConn = NULL;
	ifmgmt *ifmm_relay = NULL;
	bool deleteFlag = true;

	// Get received burst
	recvBurst = (Packet *) epkt->DataInfo_;
	assert(epkt && recvBurst);

	// Free event memory, but not include DataInfo_ (recvBurst)
	epkt->DataInfo_ = NULL;
	freePacket(epkt);

	// Save phyInfo
	memcpy(&LastSignalInfo, recvBurst->pkt_getinfo("phyInfo"),
			sizeof(LastSignalInfo));
	adaptedBurst = (char *) recvBurst->pkt_sget();
	burstLen = recvBurst->pkt_getlen();

	nid = LastSignalInfo.nid;
	pid = LastSignalInfo.pid;

	if (LastSignalInfo.SNR < 0)
		LastSignalInfo.SNR = 0;

	// MS cdma ranging
	if ((State & NTRS_Op) && (LastSignalInfo.uiuc == CDMA_BWreq_Ranging)) {
		// set MS's Access Station
		pMS = getMS(nid, pid);

		if (!pMS->AS) {
			//printf("\e[1;35mMS(%d)'s Access Station is RS!!!\e[0m\n",nid);
			pMS->AS = this;
		}

		if (pMS->AS != this) {
			//printf("\e[1;35mMS(%d)'s Access Station is not RS!!!\e[0m\n",nid);
			delete recvBurst;
			return 1;
		}
		//printf("\e[1;35mTime:%llu RS[%d] process Node[%d] CDMA_BWreq_Ranging\e[0m\n",GetCurrentTime(),get_nid(),LastSignalInfo.nid);

		uint8_t codeIndex = 0;
		uint8_t code[18] = "";
		uint8_t usage = 0;

		//printf("Time:%llu\tRS(%d)::%s()\n", GetCurrentTime(),get_nid(),__func__);
		if (LastSignalInfo.symOffset < (_ULAccessSymbols / 3) * 2) // Initial or Handover Ranging Region
		{
			if (random() % 2 == 0) {
				memcpy(code, adaptedBurst, 18);
			} else {
				memcpy(code, adaptedBurst + 18, 18);
			}
		} else {
			memcpy(code, adaptedBurst, 18);
		}

		codeIndex = getCodeIndex(code, &usage);

		if (codeIndex == 255) {
			return 0;
		} else // MS INITIAL_RANGING
		{
			//printf("Time:%llu receive MS[%d] rngcode frameStartTime=%llu\n",GetCurrentTime(),LastSignalInfo.nid,frameStartTime);

			RangingCodeList.push_back(new RangingObject(codeIndex, frameNumber
					& 0xFF, LastSignalInfo.symOffset, LastSignalInfo.chOffset,
					usage, GetCurrentTime()));

			//printf("rangingCodeIndex= %d\n",codeIndex);
			//printf("rangingFrameNumber = %d\n",frameNumber & 0xFF);
			//printf("rangingSymbol = %d\n",LastSignalInfo.symOffset);
			//printf("rangingSubchannel = %d\n",LastSignalInfo.chOffset);
			//printf("rangingUsage = %d\n\n",usage);
		}

		SendRS_BR_header();

		delete recvBurst;
		return 1;
	}
#if 0
	/*
	* When RS is scanning, we measure the SNR value and ignore the received packet
	*/
	if (ScanFlag == true)
	{
		NbrMRBS_NT *pNbrMRBS_NT = NeighborMRBSList->getNbrbyChID(LastSignalInfo.ChannelID);

		if (pNbrMRBS_NT == NULL)
		{
			fprintf(stderr, "RS(%d):%s() Warning: not existed channel ID(%d)\n", get_nid(), __func__, LastSignalInfo.ChannelID);
			exit(1);
		}

		pNbrMRBS_NT->CINR = (int) (pNbrMRBS_NT->CINR * 0.8 + LastSignalInfo.SNR * 0.2); // weighted SNR computation
		//printf("pNbrMRBS_NT->CINR=%2d(Serving=%2f)  LastSignalInfo.ChannelID=%2d\n", pNbrMRBS_NT->CINR, NeighborMRBSList->ServingCINR, LastSignalInfo.ChannelID);

		delete recvBurst;

		((OFDMA_PMPRS_NT *)sendtarget_->bindModule())->skipBufferedPkt();

		return 1;
	}
	else
	{
		// save the SNR value of serving BS
		NeighborMRBSList->ServingCINR = NeighborMRBSList->ServingCINR * 0.8 + LastSignalInfo.SNR * 0.2; // weighted SNR computation
	}
#endif
	for (ptr = adaptedBurst; ptr + sizeof(struct hdr_generic) < adaptedBurst
			+ burstLen; ptr += plen) {
		// Get generic mac header
		hg = (struct hdr_generic *) ptr;
		cid = GHDR_GET_CID(hg);
		plen = GHDR_GET_LEN(hg);

		// None
		if (hcs_16j_NT(ptr, sizeof(struct hdr_generic)) || plen == 0) {
			break;
		}

		// Save total_length
		len = plen;

		// CRC check
		if (hg->ci == 1) {
			len -= 4;
			crc = crc32_16j_NT(0, ptr, len);
			if (memcmp(&crc, ptr + len, 4) != 0) {
				printf("RS(%d) CRC Error (%08x)\n", get_nid(), crc);
				continue;
			}
		}

		//    printf("mac802_16j_NT_PMPRS::recv Time:%llu cid=%d\n",GetCurrentTime(),cid);
		// Broadcast CID
		if (cid == broadcastCID) {
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			// DL-MAP
			if (recvmm->type == MG_RMAP) {
				_ULAccessSymbols = LastSignalInfo.ULAccessSym;
				_DLAccessSymbols = LastSignalInfo.DLAccessSym;
				_DLRelaySymbols = LastSignalInfo.DLRelaySym;
				_ULRelaySymbols = LastSignalInfo.ULRelaySym;

				procRMAP(recvmm, cid, len - sizeof(struct hdr_generic));
				MILLI_TO_TICK(tick, LostRMAPinterval);
				timerLostDL->start(tick, 0); // when recv R-MAP, reset timerLostDL
				if ((State & NTRS_DLParam) == 0) {
					State |= NTRS_DLParam;
				}
			}

			// RS doesn't synchronized to downlink  ==>  continue.
			if ((State & NTRS_DLParam) == 0) {
				continue;
			}

			// DCD
			if (recvmm->type == MG_DCD) {
				procDCD(recvmm, cid, len - sizeof(struct hdr_generic));
				resetTimerT(1u); // when recv DCD, reset T1
			}

			// UCD
			if (recvmm->type == MG_UCD) {
				procUCD(recvmm, cid, len - sizeof(struct hdr_generic));
				resetTimerT(12u); // reset T12

				if ((State & NTRS_ULParam) == 0) {
					State |= NTRS_ULParam;

					resetTimerT(2u); // reset T2
					MILLI_TO_TICK(tick, LostULinterval);
					timerLostUL->start(tick, 0); // when recv UCD, start timerLostUL

					RngTime = RngStart - 1;
					RngLastStatus = Rerange; // Rerange
					WaitForRangingOpportunity();
				}
			}

			// UL-MAP
			if (recvmm->type == MG_ULMAP) {
				procULMAP(recvmm, cid, len - sizeof(struct hdr_generic));
				MILLI_TO_TICK(tick, LostULinterval);
				timerLostUL->start(tick, 0); // when recv UL-MAP, reset timerLostUL
			}

			// MOB_NBR-ADV
			if (recvmm->type == MG_MOB_NBRADV) {
				procMOB_NBRADV(recvmm, cid, len - sizeof(struct hdr_generic));
			}
		} else if (cid == initRangingCID) // Initial Ranging CID
		{
			//printf("mac802_16j_NT_PMPRS::recv Time:%llu cid=%d\n",GetCurrentTime(),cid);
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			if (recvmm->type == MG_RNGREQ) {
				if ((State & NTRS_Op) && (LastSignalInfo.uiuc == CDMA_Alloc_IE)) {
					pMS = getMS(nid, pid);

					if (pMS->AS != this) {
						//printf("\e[1;35mMS(%d)'s Access Station is not RS!!!\e[0m\n",nid);
						delete recvBurst;
						return 1;
					} else {
						//printf("\e[36mTime:%llu RS[%d]::procRNGREQ() from MS[%d]\e[0m\n",GetCurrentTime(),get_nid(),nid);
						procRNGREQ(recvmm, cid, len
								- sizeof(struct hdr_generic));
					}
				}
			}

			// RNG-RSP
			if (recvmm->type == MG_RNGRSP) {
				if (procRNGRSP(recvmm, cid, len - sizeof(struct hdr_generic))
						== 1) // success when RNG-RSP is for RNG-REQ
				{
					State |= NTRS_Ranging;
					if ((State & NTRS_Negcap) == 0) {
						SendSBCREQ();
						//printf("MnConnections.size = %d\n",MnConnections.size());
					}
				}
			}
		} else if (cid == BasicCID) // MAC PDU for this RS
		{
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			// RS_AccessMAP
			if (recvmm->type == MG_RS_AccessMAP) {
				procRS_AccessMAP(recvmm, cid, len - sizeof(struct hdr_generic));
			}

			// SBC-RSP
			if (recvmm->type == MG_SBCRSP) // Fig.66
			{
				timer_mngr()->cancel_t(18u);
				delete ifsavedmm;
				ifsavedmm = NULL;
				if (procSBCRSP(recvmm, cid, len - sizeof(struct hdr_generic))) // Response OK
				{
					State |= NTRS_Negcap;
					/*
					* Here, we skip procedure RS authorization and key exchange (Spec 6.3.9.8).
					*/
					SendREGREQ();
				}
			}

			// MOB_SCN-RSP
			if (recvmm->type == MG_MOB_SCNRSP) {
				procMOB_SCNRSP(recvmm, cid, len - sizeof(struct hdr_generic));
			}

			// MOB_BSHO-RSP
			if (recvmm->type == MG_MOB_BSHORSP) {
				timer_mngr()->cancel_t(42u);
				timerHORetransmit->cancel();

				delete ifsavedmm;
				ifsavedmm = NULL;
				procMOB_BSHORSP(recvmm, cid, len - sizeof(struct hdr_generic));
			}

			// MOB_BSHO-REQ
			if (recvmm->type == MG_MOB_BSHOREQ) {
				; // not implement
			}

			if (recvmm->type == MG_RNGRSP) {
				procRNGRSP_relay(recvmm, cid, len - sizeof(struct hdr_generic));
			}
		} else if (cid == PriCID) // MAC PDU for this RS
		{
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			// REG-RSP
			if (recvmm->type == MG_REGRSP) // Fig.69
			{
				timer_mngr()->cancel_t(6u);

				delete ifsavedmm;
				ifsavedmm = NULL;

				if (procREGRSP(recvmm, cid, len - sizeof(struct hdr_generic))) // Response OK
				{
					State |= NTRS_Register;

					// Start Period Ranging
					//MILLI_TO_TICK(tick, PeriodRangingInterval * frameDuration_NT[frameDuCode()]);
					//timerPeriodRanging->start(tick, 0);

					// Start Scanning Neighbor BSs Timer
					//SEC_TO_TICK(tick, ScanReqInterval);
					//timerSendScanReq->start(tick, tick); // FIXME

					timer_mngr()->reset_t(67u);
					if (0) // is Managed RS?
					{
						// Establish Secondary Management Connection
						// Establish IP connectivity
					}
				}
			}

			// DSA-REQ
			if (recvmm->type == MG_DSAREQ && (State & NTRS_Register)) {
				procDSAREQ(recvmm, cid, len - sizeof(struct hdr_generic));
			}

			// DSA-ACK
			if (recvmm->type == MG_DSAACK) {
				timer_mngr()->cancel_t(8u);

				delete ifsavedmm;
				ifsavedmm = NULL;

				procDSAACK(recvmm, cid, len - sizeof(struct hdr_generic));
				if ((State & NTRS_Prov) == 0) {
					State |= NTRS_Prov;
				}
			}

			if (recvmm->type == MG_UCD) {
				ifmm_relay = new ifmgmt((uint8_t *) recvmm, len
						- sizeof(struct hdr_generic));
				broadcastConnection->Insert(ifmm_relay);
			}

			if (recvmm->type == MG_DCD) {
				ifmm_relay = new ifmgmt((uint8_t *) recvmm, len
						- sizeof(struct hdr_generic));
				broadcastConnection->Insert(ifmm_relay);
			}

			if (recvmm->type == MG_RS_ConfigCMD) {
				timer_mngr()->cancel_t(67u);
				timerLostDL->cancel();
				resetTimerT(1u); // when recv DCD, reset T1
				resetTimerT(12u); // when recv UCD, reset T12

				if (procRS_ConfigCMD(recvmm, cid, len
						- sizeof(struct hdr_generic))) {
					State |= NTRS_Op;
					SendMR_GenericACK();
					printf(
							"\e[33m\n===============RS(%d)==IF=PROGRAM=RUN=TO=HERE=THAT'S=NETWORK=ENTRY=OK===============\e[0m\n",
							get_nid());

					NE_success = true;
				}
			}
		} else if ((State & NTRS_Op) && (getULRelayMnConn(cid)
				|| getULRelayDtConn(cid))) {

			//printf("Time:%llu RS(%d) receive cid=%d\n",GetCurrentTime(),get_nid(),cid);
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));
			if (cid <= 2*MAXMS + 2*MAXRS) {
				if (LastSignalInfo.burst_type == UL_ACCESS) {
					//printf("getULRelayMnConn (cid=%d)\n",cid);
					pConn = getULRelayMnConn(cid);
					if (!pConn) {
						printf("getULRelayMnConn error!!!\n");
						exit(1);
					}
					ifmm_relay = new ifmgmt((uint8_t*) recvmm, len
							- sizeof(struct hdr_generic));
					pConn->Insert(ifmm_relay);
				} else if (LastSignalInfo.burst_type == DL_RELAY) {
					//printf("\e[1;35mgetDLRelayMnConn (cid=%d)\e[0m\n",cid);

					pConn = getDLRelayMnConn(cid);
					if (!pConn) {
						printf("getDLRelayMnConn error!!!\n");
						exit(1);
					}
					ifmm_relay = new ifmgmt((uint8_t*) recvmm, len
							- sizeof(struct hdr_generic));
					pConn->Insert(ifmm_relay);
					if (recvmm->type == MG_DSAREQ) {
						//printf("recvmm->type == MG_DSAREQ\n");
						procDSAREQ_relay(recvmm, cid, len
								- sizeof(struct hdr_generic));
					}
				} else {
					printf("get Relay Connection error!!!\n");
					exit(1);
				}
			} else {

				if ((getULRelayDtConn(cid) != NULL)
						&& LastSignalInfo.burst_type == UL_ACCESS) {
					//printf("Time:%llu RS(%d) receive cid=%d (UL)\n",GetCurrentTime(),get_nid(),cid);
					vector<ConnectionReceiver *>::iterator iter;
					char *ptr2 = NULL;

					for (iter = ULRelayReceiverList.begin(); iter
							!= ULRelayReceiverList.end(); iter++) {
						if ((*iter)->getCid() == cid) {
							(*iter)->insert(hg, len);
							while ((ptr2 = (*iter)->getPacket(len)) != NULL) {
								Packet *pkt = asPacket(ptr2, len);
								//pkt->pkt_addinfo("way", "u", sizeof(char));
								//pkt->pkt_addinfo("cid", (char *) &cid,
								//		sizeof(int));
								//pkt->pkt_addinfo("len", (char *) &len,
								//		sizeof(int));
								pkt->pkt_setflow(PF_SEND);
								//ePacket_ *deliver_epkt = createEvent();
								//deliver_epkt->DataInfo_ = pkt;
								//put(deliver_epkt, recvtarget_);

								#ifdef RDV
								struct ip* ip = (struct ip *) ptr2;

								logRidvan(
								  WARN,
								  "RS UL ethernet packet received packet type:%d src:%s dst:%s",
								  hg->type, ipToStr(ip->ip_src), ipToStr(
								  ip->ip_dst));
								int myCid = 0;

								hash_map<uint32_t, int>::const_iterator viter;
								viter = routingTable.find(ip->ip_dst);
								if (viter != routingTable.end()) {

								  logRidvan(
									    WARN,
									    "routingTable[ethernet->ip_dst]=%d",
									    viter->second);
								  myCid = viter->second;

								  logRidvan(WARN, "getcid %d myCid %d",
								  GHDR_GET_CID(hg), myCid);
								  pDtConn = getDLRelayDtConn(myCid);
								} else
								  #endif
								  pDtConn = getULRelayDtConn(cid);

								if (pDtConn->nf_pending_packet()
								<= static_cast<size_t> (_maxqlen)) {
								      pDtConn->Insert(pkt);
								      deleteFlag = false;
								}
								if (deleteFlag) {
								printf(">>>RS(%d)::Drop Pakcet<<<\n",
								get_nid());
								delete pkt;
								}

							}
							break;
						}
					}
					if (iter == ULRelayReceiverList.end()) {
						printf(
								"Time:%llu RS(%d) can't receive this packet of CID(%d)\n",
								GetCurrentTime(), get_nid(), cid);
					}
					//	}
				} else if ((getDLRelayDtConn(cid) != NULL)
						&& LastSignalInfo.burst_type == DL_RELAY) {
					//logRidvan(WARN,"RS DL ethernet packet received packet type:%d src:%s dst:%s",hg->type,ipToStr(ethernet->ip_src),ipToStr(ethernet->ip_dst));

					//printf("Time:%llu RS(%d) receive cid=%d (DL)\n",GetCurrentTime(),get_nid(),cid);
					vector<ConnectionReceiver *>::iterator iter;
					char *ptr2 = NULL;
					for (iter = DLRelayReceiverList.begin(); iter
							!= DLRelayReceiverList.end(); iter++) {
						if ((*iter)->getCid() == cid) {
							(*iter)->insert(hg, len);
							while ((ptr2 = (*iter)->getPacket(len)) != NULL) {

								struct ip* ip = (struct ip *) ptr2;

								logRidvan(
										WARN,
										"RS DL ethernet packet received packet type:%d src:%s dst:%s",
										hg->type, ipToStr(ip->ip_src), ipToStr(
												ip->ip_dst));

								logRidvan(WARN, "routingTable[ip->ip_dst]=%d",
										routingTable[ip->ip_dst]);
								Packet *pkt = asPacket(ptr2, len);
								//pkt->pkt_addinfo("way", "d", sizeof(char));
								//pkt->pkt_addinfo("cid", (char *) &cid,
								//		sizeof(int));
								//pkt->pkt_addinfo("len", (char *) &len,
								//		sizeof(int));
								pkt->pkt_setflow(PF_SEND);
								//ePacket_ *deliver_epkt = createEvent();
								//deliver_epkt->DataInfo_ = pkt;
								//put(deliver_epkt, recvtarget_);

								pDtConn = getDLRelayDtConn(cid);
								routingTable[ip->ip_dst] = cid;
								if (pDtConn->nf_pending_packet()
								<= static_cast<size_t> (_maxqlen)) {
								  pDtConn->Insert(pkt);
								  deleteFlag = false;
								}
								if (deleteFlag) {
								  printf(">>>RS(%d)::Drop Pakcet<<<\n",
								  get_nid());
								  delete pkt;
								}

							}
							break;
						}
					}
					if (iter == DLRelayReceiverList.end()) {
						printf(
								"Time:%llu RS(%d) can't receive this packet of CID(%d)\n",
								GetCurrentTime(), get_nid(), cid);
					}
				} else {
					printf("get Relay Connection error!!!\n");
					exit(1);
				}
			}
		} else // Secondary CID or Transport CID
		{
			//printf("cid = %d\n",cid);
		}
	} // end for loop

	delete recvBurst;
	return 1;
}

int mac802_16j_NT_PMPRS::send(ePacket_ *epkt) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::send State: %x", get_nid(),
			State);
	DataConnection *pDtConn = NULL;
	Packet *pkt = (Packet *) epkt->DataInfo_;
	epkt->DataInfo_ = NULL;
	//Packet *pkt;
	freePacket(epkt);

	int cid = *((int *) pkt->pkt_getinfo("cid"));
	char way = *((char *) pkt->pkt_getinfo("way"));
	bool deleteFlag = true;

	if (way == 'd') {
		pDtConn = getDLRelayDtConn(cid);
		if (pDtConn->nf_pending_packet() <= static_cast<size_t> (_maxqlen)) {
			pDtConn->Insert(pkt);
			deleteFlag = false;
		}
		if (deleteFlag) {
			printf(">>>RS(%d)::Drop Pakcet<<<\n", get_nid());
			delete pkt;
		}
	} else {
		pDtConn = getULRelayDtConn(cid);
		if (pDtConn->nf_pending_packet() <= static_cast<size_t> (_maxqlen)) {
			pDtConn->Insert(pkt);
			deleteFlag = false;
		}
		if (deleteFlag) {
			printf(">>>RS(%d)::Drop Pakcet<<<\n", get_nid());
			delete pkt;
		}
	}
	return 1;
}

Connection *mac802_16j_NT_PMPRS::getConnection(uint16_t cid) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::getConnection State: %x",
			get_nid(), State);
	vector<ManagementConnection *>::iterator iter1;
	vector<DataConnection *>::iterator iter2;

	for (iter1 = MnConnections.begin(); iter1 != MnConnections.end(); iter1++) {
		Connection *conn = *iter1;
		if (conn->cid == cid)
			return conn;
	}
	for (iter2 = DtConnections.begin(); iter2 != DtConnections.end(); iter2++) {
		Connection *conn = *iter2;
		if (conn->cid == cid)
			return conn;
	}
	return NULL;
}

void mac802_16j_NT_PMPRS::PushPDUintoConnection(uint16_t cid, ifmgmt *pdu) {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::PushPDUintoConnection State: %x",
			get_nid(), State);
	vector<ManagementConnection *>::iterator iter;
	ManagementConnection *pConn = NULL;

	if (cid == initRangingCID) {
		initRangingConnection->Insert(pdu);
		return;
	}

	for (iter = MnConnections.begin(); iter != MnConnections.end(); iter++) {
		pConn = (ManagementConnection *) *iter;
		if (pConn->cid == cid) {
			//printf("Time:%llu mac802_16j_NT_PMPRS::%s cid=%d\n",GetCurrentTime(),__func__,cid);
			pConn->Insert(pdu);
			return;
		}
	}
}

/*
* for sending RNG-REQ at CDMA allocation interval
*/
uint64_t mac802_16j_NT_PMPRS::computeSendTime(int numSlots, int duration,
		int fastime) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::computeSendTime State: %x",
			get_nid(), State);
	int64_t tick = 0;
	int64_t tadj = 0;
	int64_t adjus = 0;
	int nPSs = 0;

	nPSs = fastime;
	MICRO_TO_TICK(tick, PSsToMicro(nPSs));

	adjus = (int64_t) (TimingAdjust * Tb() / 1024.0);
	tadj = (int64_t) (adjus * 1000.0 / TICK); // TICK_TO_MICRO
	tick -= (GetCurrentTime() - LastSignalInfo.frameStartTime);

	if (tick + tadj > 0) {
		return tick + tadj;
	} else {
		return 0;
	}
}

/*
* for sending ranging code at contention slots
*/
uint64_t mac802_16j_NT_PMPRS::computeSendTime(
		const struct OFDMA_ULMAP_IE_12 &ie_12, int txopOffset, int fastime) {
	int64_t tick = 0;
	int64_t tadj = 0;
	int64_t adjus = 0;
	int nPSs = 0;

	nPSs = fastime + symbolsToPSs(ie_12.symOff + txopOffset);
	MICRO_TO_TICK(tick, PSsToMicro(nPSs));

	adjus = (int64_t) (TimingAdjust * Tb() / 1024.0); // us
	tadj = (int64_t) (adjus * 1000.0 / TICK); // TICK_TO_MICRO
	tick -= (GetCurrentTime() - LastSignalInfo.frameStartTime); // absolute waiting time

	if (tick + tadj > 0) {
		return tick + tadj;
	} else {
		return 0;
	}
}

/*
* Periodically transmit downlink burst.
*/
void mac802_16j_NT_PMPRS::DLPacketScheduling() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::DLPacketScheduling State: %x",
			get_nid(), State);

	if (!(State & NTRS_Op)) {
		return;
	}
	ePacket_ *epkt = NULL;
	BurstInfo *burstInfo = new BurstInfo;

	if (rs_br_hdr_rsp == true) {
		handleRangingCode();
		ClearRNGCodeList(&RangingCodeList);
		rs_br_hdr_rsp = false;
	}

	frameStartTime = GetCurrentTime();

	//printf("%s(%d) frameStartTime = %llu\n",__func__,__LINE__,frameStartTime);

	burstInfo->Collection = pScheduler->DL_Scheduling();

	burstInfo->flag = PF_SEND;
	burstInfo->type = DL_ACCESS;

	if (burstInfo->Collection != NULL) {
		//    printf("===== Time:%llu RS(%d)::%s() send DL burst =====\n", GetCurrentTime(), get_nid(), __func__);
		epkt = createEvent();
		epkt->DataInfo_ = burstInfo;
		put(epkt, sendtarget_);
	} else {
		delete burstInfo;
	}
	//ClearRNGCodeList(&RangingCodeList);
}

/*
* Periodically transmit uplink burst.
*/
void mac802_16j_NT_PMPRS::ULPacketScheduling() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::ULPacketScheduling State: %x",
			get_nid(), State);
	ePacket_ *epkt = NULL;
	BurstInfo *burstInfo = new BurstInfo;

	//printf("===== Time:%llu RS(%d)::%s() send UL burst =====\n", GetCurrentTime(), get_nid(), __func__);
	burstInfo->Collection = pScheduler->UL_Scheduling();
	burstInfo->flag = PF_SEND;
	burstInfo->type = UL_RELAY;

	if (burstInfo->Collection != NULL) {
		epkt = createEvent();
		epkt->DataInfo_ = burstInfo;
		put(epkt, sendtarget_);
	} else {
		delete burstInfo;
	}
}

/*
* Wait for DCD timeout
*/
void mac802_16j_NT_PMPRS::T1() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T1 State: %x", get_nid(),
			State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);
}

/*
* Wait for Broadcast Ranging timeout
*/
void mac802_16j_NT_PMPRS::T2() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T2 State: %x", get_nid(),
			State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);
}

/*
* Ranging Response reception timeout following the
* transmission of a Ranging Request
*/
void mac802_16j_NT_PMPRS::T3() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T3 State: %x", get_nid(),
			State);

	printf("Time:%llu RS(%d)::%s()\n", GetCurrentTime(), get_nid(), __func__);

	if (RngCodeRegion == RS_INITIAL_RANGING || RngCodeRegion
			== HANDOVER_RANGING) // FIXME
	{
		RngLastStatus = Rerange;
	} else if (RngCodeRegion == PERIOD_RANGING || RngCodeRegion
			== RS_DEDICATED_CODES) {
		RngLastStatus = Continue;
	} else {
		;
	}
	//TimingAdjust -= 11; // FIXME: if the distance between BS and RS, the ranging packet will be dropped.
	WaitForRangingOpportunity();
}

/*
* Wait for registration response
*/
void mac802_16j_NT_PMPRS::T6() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T6 State: %x", get_nid(),
			State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm
			= new ifmgmt(saved_msg, ifsavedmm->getLen(), ifsavedmm->getFLen());

	resetTimerT(6u);
}

/*
* Wait for DSA/DSC Acknowledge timeout
*/
void mac802_16j_NT_PMPRS::T8() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T8 State: %x", get_nid(),
			State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm
			= new ifmgmt(saved_msg, ifsavedmm->getLen(), ifsavedmm->getFLen());

	resetTimerT(8u);
}

/*
* Wait for UCD timeout
*/
void mac802_16j_NT_PMPRS::T12() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T12 State: %x", get_nid(),
			State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);
}

/*
* Wait for SBC-RSP timeout
*/
void mac802_16j_NT_PMPRS::T18() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T18 State: %x", get_nid(),
			State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm
			= new ifmgmt(saved_msg, ifsavedmm->getLen(), ifsavedmm->getFLen());

	resetTimerT(18u);
}

/*
* MOB_HO-IND timeout when sent with HO_IND_type = 0b10
*/
void mac802_16j_NT_PMPRS::T42() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T42 State: %x", get_nid(),
			State);

	printf("\nTime:%llu:RS(%d)::%s() ... HO holding down timeout... \n",
			GetCurrentTime(), get_nid(), __func__);

	delete ifsavedmm;
}

void mac802_16j_NT_PMPRS::T62() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T62 State: %x", get_nid(),
			State);
	printf(
			"\nTime:%llu:RS(%d)::%s() Timeout of receiving the allocation of RNG-RSP after sending RS_BR_header\n",
			GetCurrentTime(), get_nid(), __func__);
}

void mac802_16j_NT_PMPRS::T67() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::T67 State: %x", get_nid(),
			State);

	printf(
			"\nTime:%llu:RS(%d)::%s()Timeout of waiting RS_Config-CMD from MR-BS as skip neighborBS_Index measurement\n",
			GetCurrentTime(), get_nid(), __func__);
}

/*
* Wait for DL-MAP timeout
*/
void mac802_16j_NT_PMPRS::LostDLMAP() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::LostDLMAP State: %x",
			get_nid(), State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);
}

/*
* Wait for UL-MAP timeout
*/
void mac802_16j_NT_PMPRS::LostULMAP() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::LostULMAP State: %x",
			get_nid(), State);

	printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(),
			get_nid(), __func__);
}

/*
* Handover Retransmission Timeout
*/
void mac802_16j_NT_PMPRS::HOmsgRetransmit() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::HOmsgRetransmit State: %x",
			get_nid(), State);
	uint64_t tick = 0;

	printf("\n\e[1;31mTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\e[0m\n",
			GetCurrentTime(), get_nid(), __func__);

	// Retries Available ?
	if (HOReqRetryAvailable > 0) {
		PushPDUintoConnection(lastCID, ifsavedmm);

		ifsavedmm = new ifmgmt(saved_msg, ifsavedmm->getLen(),
				ifsavedmm->getFLen());

		MILLI_TO_TICK(tick, HORetransmitInterval
				* frameDuration_NT[frameDuCode()]);
		timerHORetransmit->start(tick, 0);

		// Decrement HO-REQ-Retries available
		HOReqRetryAvailable--;
	} else {
		printf("HO-RSP retries exhausted\n");
	}
}

/*
* Start Waiting for Period Ranging Opportunity
*/
void mac802_16j_NT_PMPRS::StartPeriodRanging() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::StartPeriodRanging State: %x",
			get_nid(), State);

	RngTime = RngStart - 1;
	RngLastStatus = Continue; // continue
	RngCodeRegion = PERIOD_RANGING;
	WaitForRangingOpportunity();
}

/*
* Start Scanning Status, change the channel ID and ignore the incoming packets
*/
void mac802_16j_NT_PMPRS::StartScanNbrMRBS_NTs() {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::StartScanNbrMRBS_NTs State: %x",
			get_nid(), State);
	uint8_t chID = 0;
	uint64_t tick = 0;

	//printf("\n\e[1;31mTime:%llu:RS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	if (NeighborMRBSList->ScanTimes != NeighborMRBSList->ScanDuration) // continue scanning
	{
		chID = NeighborMRBSList->getNextScanChID();

		// Continue scanning
		if (((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->getChannelID()
				== chID) {
			fprintf(stderr, "Warning: scanning channel ID(%d) is the same\n",
					chID);
			exit(1);
		}

		//printf("Time:%llu\tRS(%d) set channel ID to (%d)\n", GetCurrentTime(), get_nid(), chID);

		((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->setChannelID(chID);
		NeighborMRBSList->ScanningChID = chID;

		MICRO_TO_TICK(tick, symbolsPerFrame() * Ts() + (TTG() + RTG()) * PS()); // one frame
		timerStartScan->start(tick, 0);

		NeighborMRBSList->ScanTimes++;
		ScanFlag = true;

		//timer_mngr()->cancel_t(3u);
	} else // back to serving channel
	{
		NeighborMRBSList->ScanTimes = 0;
		NeighborMRBSList->ScanIteration--;

		// End scanning, back to serving BS's Channel ID
		chID = NeighborMRBSList->ServingBSchID;
		((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->setChannelID(chID);
		//printf("Time:%llu\tRS(%d) set channel ID to (%d)\n", GetCurrentTime(), get_nid(), chID);

		ScanFlag = false;

		/*
		* Check handover conditions
		*/
		if (NeighborMRBSList->checkHOterms() == true) {
			timerSendScanReq->cancel();
			SendMOB_MSHOREQ(); // Send MOB_MSHO-REQ
		} else {
			if (NeighborMRBSList->ScanIteration != 0) // FIXME
			{
				// back to normal operation and schedule the next scanning
				double frameTime = symbolsPerFrame() * Ts() + (TTG() + RTG()
						+ TimingAdjust) * PS(); // us
				int frameNum = NeighborMRBSList->InterleavingInterval;

				MICRO_TO_TICK(tick, frameTime * frameNum);
				timerStartScan->start(tick, 0);
			}
		}
	}
}

/*
* When RS performs handover, these registers need to clear
*/
void mac802_16j_NT_PMPRS::StartHandover() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::StartHandover State: %x",
			get_nid(), State);

	//printf("\e[1;35mTime:%llu RS(%d)::%s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	// Cancel timers
	timerSendRngCode->cancel();
	timerSendUL->cancel();
	timerPeriodRanging->cancel();
	timerSendScanReq->cancel();
	timerStartScan->cancel();

	// Change Channel ID
	((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->setChannelID(
			NeighborMRBSList->getTargetBS()->PreambleIndex);
	//printf("Time:%llu\tRS(%d) set channel ID to (%d)\n", GetCurrentTime(), get_nid(), NeighborMRBSList->getTargetBS()->PreambleIndex);

	// Clear Registers
	myDIUC = 0;
	State = NTRS_Init;
	RngLastStatus = Init;
	RngCodeRegion = HANDOVER_RANGING;
	TimingAdjust = 0;
	ScanFlag = false;
	BasicCID = 0;
	PriCID = 0;
	SecCID = 0;

	while (!MnConnections.empty()) {
		delete *(MnConnections.begin());
		MnConnections.erase(MnConnections.begin());
	}

	while (!DtConnections.empty()) {
		delete *(DtConnections.begin());
		DtConnections.erase(DtConnections.begin());
	}

	while (!ReceiverList.empty()) {
		delete *(ReceiverList.begin());
		ReceiverList.erase(ReceiverList.begin());
	}

	if (ifsavedmm != NULL) {
		delete ifsavedmm;
		ifsavedmm = NULL;
	}

	delete NeighborMRBSList;
	NeighborMRBSList = new NeighborMRBSs_NT();
	NeighborMRBSList->NBRADV_CfgCount = 0;
	NeighborMRBSList->ServingBSchID
			= ((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->getChannelID();

	delete pScheduler;
	pScheduler = new RS_16j_NT_Scheduler(this);

	printf(
			"\n\e[33m=======%d==IF=PROGRAM=RUN=TO=HERE=THAT'S=RS=START=NETWORK=RE-ENTRY=PROCEDURE===========\e[0m\n\n",
			get_nid());
}

/*
*  Wait for Initial Ranging opportunity (Fig.60)
*/
void mac802_16j_NT_PMPRS::WaitForRangingOpportunity() {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::WaitForRangingOpportunity State: %x",
			get_nid(), State);

	if (++RngTime > RngEnd) {
		printf("Rng Exhausted\n");
		return;
	}
	RngCount = random() % (int) powl(2, RngTime);
	//printf("RngCount = %d, RngTime = %d\n", RngCount, RngTime);
}

/*
*    Processing DCD
*/
int mac802_16j_NT_PMPRS::procDCD(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procDCD State: %x", get_nid(),
			State);

	ifmgmt *ifmm = NULL;
	ifTLV *tmptlv = NULL;
	uint8_t type = 0;
	uint8_t diuc = 0;
	uint8_t value = 0;
	uint8_t fCfgCount = 0;
	uint8_t rsv = 0;
	int tlen = 0;
	uint8_t profilebuffer[128] = "";
	uint8_t hop_count = 0;

	//printf("\nTime:%llu:mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(), __func__);

	// Spec 16e. Table 15
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 2);
	ifmm->extractBitField(8, &rsv); // rsv
	ifmm->extractBitField(8, &fCfgCount); // Configuration change count

	if (fCfgCount == ((DCDCfgCount + 1) % 256)) {
		DCDCfgCount = fCfgCount;

		// clear
		for (diuc = 0; diuc < 16; diuc++) {
			DCDProfile[diuc].used = 0;
		}

		// set
		while ((type = ifmm->getNextType()) != 0) {
			if (type == 1) // DCD burst profiles
			{
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(profilebuffer);

				diuc = profilebuffer[0];
				tmptlv = new ifTLV(profilebuffer + 1, tlen - 1);
				DCDProfile[diuc].used = 1;

				while ((type = tmptlv->getNextType()) != 0) {
					tmptlv->getNextValue(&value);
					switch (type) {
					case 150:
						DCDProfile[diuc].fec = value;
						break;
					}
				}
				delete tmptlv;
			} else if (type == 50) // Support HO
			{
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(&_HO_support);
			} else if (type == 64) //end-to-end metric
			{
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(&hop_count);
				//printf("hop_count = %d\n",hop_count);
			}
		}
	}
	delete ifmm;

	return 1;
}

/*
*    Processing UCD
*/
int mac802_16j_NT_PMPRS::procUCD(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procUCD State: %x", get_nid(),
			State);

	uint8_t profilebuffer[128] = "";
	ifmgmt *ifmm = NULL;
	ifTLV *tmptlv = NULL;
	uint8_t type = 0;
	uint8_t uiuc = 0;
	uint8_t value = 0;
	uint8_t fCfgCount = 0;
	uint8_t fRngStart = 0;
	uint8_t fRngEnd = 0;
	uint8_t fReqStart = 0;
	uint8_t fReqEnd = 0;
	int tlen = 0;

	//printf("\nTime:%llu:mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 5);
	ifmm->extractBitField(8, &fCfgCount);
	ifmm->extractBitField(8, &fRngStart);
	ifmm->extractBitField(8, &fRngEnd);
	ifmm->extractBitField(8, &fReqStart);
	ifmm->extractBitField(8, &fReqEnd);

	RngStart = fRngStart;
	RngEnd = fRngEnd;
	ReqStart = fReqStart;
	ReqEnd = fReqEnd;

	if (fCfgCount == (UCDCfgCount + 1) % 256) {
		UCDCfgCount = fCfgCount;
		for (uiuc = 0; uiuc < 16; uiuc++) {
			UCDProfile[uiuc].used = 0;
		}
		while ((type = ifmm->getNextType()) != 0) {
			if (type == 1) {
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(profilebuffer);
				uiuc = profilebuffer[0];
				tmptlv = new ifTLV(profilebuffer + 1, tlen - 1);
				UCDProfile[uiuc].used = 1;
				while ((type = tmptlv->getNextType()) != 0) {
					tmptlv->getNextValue(&value);
					switch (type) {
					case 150:
						UCDProfile[uiuc].fec = value;
						break;

					case 151:
						UCDProfile[uiuc].rangRatio = value;
						break;
					}
				}
				delete tmptlv;
				continue;
			}
		}
	}
	delete ifmm;

	return 1;
}

/*
* Processing DL-MAP
*/
int mac802_16j_NT_PMPRS::procRMAP(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procRMAP State: %x",
			get_nid(), State);

	char fdcdcount = 0;
	uint8_t frameDuration_NT = 0;
	uint8_t numSymbols = 0;
	ifmgmt *ifmm = NULL;
	uint8_t *tmp_ies = NULL;
	//	ntfyDLMAP   *ntfyCmd_access = NULL; // DL access MAP IE info.
	ntfyDLMAP *ntfyCmd_relay = NULL; // DL relay MAP IE info.
	relay_ind = false;

	struct OFDMA_DLMAP_IE_other *ie_other = NULL;
	OFDMA_DLMAP_IE *dlmap_ies = NULL;
	dlmapinfo info;

	// 16j add
	struct OFDMA_DLMAP_IE_13 *ie_13;

	//printf("\nTime:%llu:mac802_16j_NT_PMPRS(%d)::%s()\n", GetCurrentTime(), get_nid(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1); // len - 1, because the first field is type
	ifmm->extractBitField(8, (uint8_t *) &frameDuration_NT); // 1 byte
	ifmm->extractBitField(24, (uint8_t *) &frameNumber); // 3
	ifmm->extractBitField(8, (uint8_t *) &fdcdcount); // 1
	ifmm->extractBitField(48, (uint8_t *) ServingBSID); // 6
	ifmm->extractBitField(8, (uint8_t *) &numSymbols); // 1

	((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->setDLsymbols(numSymbols);

	int DLMAP_field = 13; // 12 + 1 (type)

	if (State && NTRS_DLParam) // if synchronized
	{
		uint8_t diuc = 0;
		uint8_t numCid = 0;
		int leftBits = 0;

		tmp_ies = new uint8_t[len - DLMAP_field];
		memset(tmp_ies, 0, len - DLMAP_field);
		ifmm->extractBitField((len - DLMAP_field) * 8, tmp_ies); // IEs

		dlmap_ies = new OFDMA_DLMAP_IE(tmp_ies, len - DLMAP_field);
		leftBits = (len - DLMAP_field) * 8; // to bits

		while (1) {
			if (leftBits == 0 || leftBits == 4) // 0:end or 4:padding
			{
				break;
			}

			diuc = 0;
			numCid = 0;

			dlmap_ies->extractField(4, &diuc);

			switch (diuc) {
			case Gap_PAPR_reduction: // diuc=13 ==> relay zone indicator
				ie_13 = new OFDMA_DLMAP_IE_13;
				memset(ie_13, 0, sizeof(OFDMA_DLMAP_IE_13));
				ie_13->diuc = diuc;
				dlmap_ies->extractField(8, &ie_13->symOff);
				dlmap_ies->extractField(7, &ie_13->chOff);
				dlmap_ies->extractField(7, &ie_13->numSym);
				dlmap_ies->extractField(7, &ie_13->numCh);
				dlmap_ies->extractField(1, &ie_13->papr_or_safety);
				dlmap_ies->extractField(1, &ie_13->sounding);
				dlmap_ies->extractField(1, &ie_13->relay_zone_ind);
				relay_ind = true;
				leftBits -= 36;
				delete ie_13;
				break;

			case Extended_DIUC_2: // Not Implement
				break;

			case Extended_DIUC: // Not Implement
				break;

			default: //others
				dlmap_ies->extractField(8, &numCid);
				ie_other = new OFDMA_DLMAP_IE_other;

				memset(ie_other, 0, sizeof(OFDMA_DLMAP_IE_other));
				ie_other->cid = new uint16_t[numCid];
				memset(ie_other->cid, 0, numCid);

				// clear
				for (int i = 0; i < numCid; i++) {
					ie_other->cid[i] = 0;
				}

				// Structure
				ie_other->diuc = diuc;
				ie_other->numCid = numCid;
				for (int i = 0; i < numCid; i++) {
					dlmap_ies->extractField(16, (uint8_t *) &ie_other->cid[i]);
				}

				dlmap_ies->extractField(8, &ie_other->symOff);
				dlmap_ies->extractField(6, &ie_other->chOff);
				dlmap_ies->extractField(3, &ie_other->boosting);
				dlmap_ies->extractField(7, &ie_other->numSym);
				dlmap_ies->extractField(6, &ie_other->numCh);
				dlmap_ies->extractField(2, &ie_other->repeCode);

				leftBits -= (44 + numCid * 16);
				break;
			}

			if (ie_other != NULL) {
				info.diuc = ie_other->diuc;
				info.symOff = ie_other->symOff;
				info.chOff = ie_other->chOff;
				info.numSym = ie_other->numSym;
				info.numCh = ie_other->numCh;
				info.repeCode = ie_other->repeCode;

				if (relay_ind == true) {
					if (!ntfyCmd_relay) {
						ntfyCmd_relay = new ntfyDLMAP;
						ntfyCmd_relay->relay_ind = relay_ind;
					}
					ntfyCmd_relay->info.push_back(info);
				} else {
					;
				}

				delete[] ie_other->cid;
				delete ie_other;
				ie_other = NULL;
			} else {
				; // other IEs not implement
			}

		} /* End while */

		if (ntfyCmd_relay)
			SendNTFYtoPHY(ntfyCmd_relay);

		delete[] tmp_ies;
		delete dlmap_ies;
	} else {
		((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->skipBufferedPkt();
	}

	delete ifmm;

	return 1;
}

/*
* Processing UL-MAP
*/
int mac802_16j_NT_PMPRS::procULMAP(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procULMAP State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	uint16_t ie_cid = 0;
	uint8_t ie_uiuc = 0;
	int leftBits = 0;
	char rsv = 0;
	char fucdcount = 0;
	int fastime = 0; // Allocation Start Time
	int contSymbols = 0;
	uint8_t *tmp_ies = NULL;
	uint64_t tick = 0;
	OFDMA_ULMAP_IE *ulmap_ies = NULL;
	bool flag_IHie = false;
	int numSlots = 0;

	//printf("Time:%llu RS(%d)::%s\n", GetCurrentTime(), get_nid(), __func__);

	// UL-MAP
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);
	ifmm->extractBitField(8, (uint8_t *) &rsv);
	ifmm->extractBitField(8, (uint8_t *) &fucdcount);
	ifmm->extractBitField(32, (uint8_t *) &fastime); // UL Allocation Start Time (in PS)
	ifmm->extractBitField(8, (uint8_t *) &ULsymbols);

	RelayULallocStartTime = fastime;

	//((OFDMA_PMPRS_NT *)sendtarget_->bindModule())->setULsymbols(ULsymbols);

	// UL-MAP IE
	tmp_ies = new uint8_t[len - 8];
	memset(tmp_ies, 0, len - 8);
	ifmm->extractBitField((len - 8) * 8, tmp_ies); // IEs

	ulmap_ies = new OFDMA_ULMAP_IE(tmp_ies, len - 8);
	leftBits = (len - 8) * 8;

	while (1) {
		if (leftBits == 0 || leftBits == 4) // 0:end or 4:padding
		{
			break;
		}
		ie_cid = 0;
		ie_uiuc = 0;

		ulmap_ies->extractField(16, (uint8_t *) &ie_cid);
		ulmap_ies->extractField(4, &ie_uiuc);

		switch (ie_uiuc) {
		case FAST_FEEDBACK: // not implement
			break;

		case Extended_UIUC_2: // not implement
			break;

		case CDMA_BWreq_Ranging: // contention subchannel
			if (flag_IHie == false) {
				memset(&savedIHie, 0, sizeof(ULMAP_IE_u));

				savedIHie.ie_12.cid = ie_cid;
				savedIHie.ie_12.uiuc = ie_uiuc;
				ulmap_ies->extractField(8, &savedIHie.ie_12.symOff);
				ulmap_ies->extractField(7, &savedIHie.ie_12.chOff);
				ulmap_ies->extractField(7, &savedIHie.ie_12.numSym);
				ulmap_ies->extractField(7, &savedIHie.ie_12.numCh);
				ulmap_ies->extractField(2, &savedIHie.ie_12.rangMethod);
				ulmap_ies->extractField(1, &savedIHie.ie_12.rangIndicator);
				leftBits -= 52;

				flag_IHie = true;

				if (RngLastStatus == Rerange) // just synchronize to UL
				{
					int rangOpportunity = 0;
					int rangSymbol = 0;

					if (savedIHie.ie_12.rangMethod == 0) // 0: Ranging over two symbols
					{
						rangSymbol = 2;
					} else // 2: Ranging over four symbols
					{
						rangSymbol = 4;
					}

					rangOpportunity = savedIHie.ie_12.numSym / rangSymbol;
					timer_mngr()->cancel_t(2u);

					if (RngCount >= rangOpportunity) {
						RngCount -= rangOpportunity;
					} else // Send Initial Ranging Code at Initial Ranging Region
					{
						tick = computeSendTime(savedIHie.ie_12, RngCount
								* rangSymbol, fastime);
						RngSymOffset = savedIHie.ie_12.symOff + RngCount
								* rangSymbol;
						RngChOffset = 0;
						timerSendRngCode->start(tick, 0);

						uint64_t tick1;
						MICRO_TO_TICK(tick1, PSsToMicro(RelayULallocStartTime));
						tick1 += LastSignalInfo.frameStartTime;
						//printf("\n[%d]Time:%llu RS(%d)::%s | fNumber=%d fSTime=%llu RULaSTime = %llu\n",__LINE__,GetCurrentTime(),get_nid(),__func__,frameNumber & 0xFF,LastSignalInfo.frameStartTime,tick1);

						//printf("[%d]Time:%llu RS(%d) timerSendRngCode->start (Initial)\n", __LINE__, GetCurrentTime(), get_nid());

					}
				}

				contSymbols += savedIHie.ie_12.numSym;
			} else {
				memset(&savedBPie, 0, sizeof(ULMAP_IE_u));

				savedBPie.ie_12.cid = ie_cid;
				savedBPie.ie_12.uiuc = ie_uiuc;
				ulmap_ies->extractField(8, &savedBPie.ie_12.symOff);
				ulmap_ies->extractField(7, &savedBPie.ie_12.chOff);
				ulmap_ies->extractField(7, &savedBPie.ie_12.numSym);
				ulmap_ies->extractField(7, &savedBPie.ie_12.numCh);
				ulmap_ies->extractField(2, &savedBPie.ie_12.rangMethod);
				ulmap_ies->extractField(1, &savedBPie.ie_12.rangIndicator);
				leftBits -= 52;

				if (RngLastStatus == Continue) // continue status: ranging at Period Ranging Region
				{
					int rangOpportunity = 0;
					int rangSymbol = 0;

					if (savedBPie.ie_12.rangMethod == 2) // 2: Ranging over one symbols
					{
						rangSymbol = 1;
					} else // 3: Ranging over three symbols
					{
						rangSymbol = 3;
					}

					rangOpportunity = savedBPie.ie_12.numSym / rangSymbol;

					if (RngCount >= rangOpportunity) {
						RngCount -= rangOpportunity;
					} else // Send Initial(Period) Ranging Code at Period Ranging Region
					{
						tick = computeSendTime(savedBPie.ie_12, RngCount
								* rangSymbol, fastime);
						RngSymOffset = savedBPie.ie_12.symOff + RngCount
								* rangSymbol;
						RngChOffset = 0;
						timerSendRngCode->start(tick, 0);
						//printf("(%d)Time:%llu RS(%d) timerSendRngCode->start (Period)\n", __LINE__, GetCurrentTime(), get_nid());
					}
				}
			}

			break;

		case PAPR_Safety_zone: // not implement
			break;

		case CDMA_Alloc_IE:
			memset(&savedALie, 0, sizeof(ULMAP_IE_u));

			savedALie.ie_14.cid = ie_cid;
			savedALie.ie_14.uiuc = ie_uiuc;
			ulmap_ies->extractField(6, &savedALie.ie_14.duration);
			ulmap_ies->extractField(4, &savedALie.ie_14.uiuc_trans);
			ulmap_ies->extractField(2, &savedALie.ie_14.repeCode);
			ulmap_ies->extractField(4, &savedALie.ie_14.frameIndex);
			ulmap_ies->extractField(8, &savedALie.ie_14.rangCode);
			ulmap_ies->extractField(8, &savedALie.ie_14.rangSym);
			ulmap_ies->extractField(7, &savedALie.ie_14.rangCh);
			ulmap_ies->extractField(1, &savedALie.ie_14.bwReq);
			//ulmap_ies->extractField( 16,&savedALie.ie_14.relayRScid);
			//printf("RS(%d):relayRScid = %d\n", get_nid(), savedALie.ie_14.relayRScid);
			leftBits -= 60;

			if (RngLastStatus == CodeSuccess || RngLastStatus == CodeWaitRSP) // ranging code success
			{

				if (((savedCode.rangingCodeIndex & 0xFF)
						!= (savedALie.ie_14.rangCode & 0xFF))
						|| ((savedCode.rangingSymbol & 0xFF)
								!= (savedALie.ie_14.rangSym & 0xFF))
						|| ((savedCode.rangingSubchannel & 0x3F)
								!= (savedALie.ie_14.rangCh & 0x3F))
						|| ((savedCode.rangingFrameNumber & 0x0F)
								!= (savedALie.ie_14.frameIndex & 0x0F))) // Discard this IE
				{
					; // skip
				} else // Send RNG-REQ at CDMA allocation slots
				{
					if (RngCodeRegion == RS_INITIAL_RANGING || RngCodeRegion
							== HANDOVER_RANGING) {
						// Record access RS basic cid.
						//RelayRSbasicid = savedALie.ie_14.relayRScid;
						tick = computeSendTime(numSlots,
								savedALie.ie_14.duration, fastime);
						timerSendRngReq->start(tick, 0);
						//printf("Time:%llu RS(%d) timerSendRngReq->start at %llu\n", GetCurrentTime(), get_nid(), GetCurrentTime() + tick);
					}
				}
			}

			numSlots += savedALie.ie_14.duration;
			break;

		case Extended_UIUC: // not implement
			break;

		default:
			ULMAP_IE_u tmpULie;
			memset(&tmpULie, 0, sizeof(ULMAP_IE_u));

			tmpULie.ie_other.cid = ie_cid;
			tmpULie.ie_other.uiuc = ie_uiuc;
			ulmap_ies->extractField(10, (uint8_t *) &tmpULie.ie_other.duration);
			ulmap_ies->extractField(2, &tmpULie.ie_other.repeCode);
			leftBits -= 32;

			if (ie_cid == BasicCID) // Grant for this RS, save to savedULie
			{
				memcpy(&savedULie, &tmpULie, sizeof(ULMAP_IE_u));
				MICRO_TO_TICK(tick, (TTG() + symbolsToPSs(_ULAccessSymbols))
						* PS()); // schedule sending time
				timerSendUL->start(tick, 0);
			}

			numSlots += tmpULie.ie_other.duration;
			break;
		}
	}

	delete ulmap_ies;
	delete[] tmp_ies;
	delete ifmm;

	return 1;
}

/*
*  Processing MOB_NBR-ADV
*/
int mac802_16j_NT_PMPRS::procMOB_NBRADV(struct mgmt_msg *recvmm, int cid,
		int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procMOB_NBRADV State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	uint8_t skipOptField = 0;
	uint8_t fragmentIndex = 0;
	uint8_t totalFragment = 0;
	uint8_t N_neighbors = 0;
	uint8_t loopLen = 0;
	uint8_t phyProfileID = 0;
	uint8_t CfgCount = 0;

	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);
	ifmm->extractBitField(8, &skipOptField);

	if ((skipOptField & 0x01) == 0) // skipOptField[0]
	{
		uint8_t operatorID[6] = "";
		ifmm->extractBitField(24, operatorID + 3);
		//printf("operatorID:%d %d %d %d %d %d\n", operatorID[0], operatorID[1], operatorID[2], operatorID[3], operatorID[4], operatorID[5]);
	}

	ifmm->extractBitField(8, &CfgCount);

	if (CfgCount != NeighborMRBSList->NBRADV_CfgCount) {
		NeighborMRBSList->NBRADV_CfgCount = CfgCount;

		ifmm->extractBitField(4, &fragmentIndex);
		ifmm->extractBitField(4, &totalFragment);
		ifmm->extractBitField(8, &N_neighbors);

		for (int i = 0; i < N_neighbors; i++) {
			NbrMRBS_NT *pNbrMRBS_NT = NULL;
			uint8_t Neighbor_BSID[6] = "";
			uint8_t FA_Index = 0;
			uint8_t BS_EIRP = 0;

			ifmm->extractBitField(8, &loopLen);
			loopLen -= 1;

			ifmm->extractBitField(8, &phyProfileID);
			loopLen -= 1;

			if (((phyProfileID & 0x40) >> 6) == 1) {
				ifmm->extractBitField(8, &FA_Index);
				//printf("FA_Index:%d\n", FA_Index);
				loopLen -= 1;
			}

			if (((phyProfileID & 0x10) >> 4) == 1) {
				ifmm->extractBitField(8, &BS_EIRP);
				//printf("BS_EIRP:%d\n", BS_EIRP);
				loopLen -= 1;
			}

			if ((skipOptField & 0x02) == 0) {
				ifmm->extractBitField(24, Neighbor_BSID + 3); // only LSB 24 bits
				pNbrMRBS_NT = NeighborMRBSList->getNbrbyBSID(Neighbor_BSID);

				if (pNbrMRBS_NT == NULL) // if new entry
				{
					printf("### Add new entry\n");
					//printf("### RS(%d):serving: %x:%x:%x:%x:%x:%x\n", get_nid(), ServingBSID[0], ServingBSID[1], ServingBSID[2], ServingBSID[3], ServingBSID[4], ServingBSID[5]);
					printf("### RS(%d):NBRNBR : %x:%x:%x:%x:%x:%x\n",
							get_nid(), Neighbor_BSID[0], Neighbor_BSID[1],
							Neighbor_BSID[2], Neighbor_BSID[3],
							Neighbor_BSID[4], Neighbor_BSID[5]);

					pNbrMRBS_NT = new NbrMRBS_NT(-1, Neighbor_BSID);
					pNbrMRBS_NT->Index = i;
					NeighborMRBSList->nbrBSs_Index.push_back(pNbrMRBS_NT);
				}
				//printf("Neighbor_BSID:%d %d %d %d %d %d\n", Neighbor_BSID[0], Neighbor_BSID[1], Neighbor_BSID[2], Neighbor_BSID[3], Neighbor_BSID[4], Neighbor_BSID[5]);
				loopLen -= 3;
			}

			ifmm->extractBitField(8, &pNbrMRBS_NT->PreambleIndex);
			printf("### RS(%d):ChID   : %d\n\n", get_nid(),
					pNbrMRBS_NT->PreambleIndex);
			loopLen -= 1;

			if ((skipOptField & 0x04) == 0) {
				ifmm->extractBitField(8, &pNbrMRBS_NT->HO_Optimize);
				loopLen -= 1;
			}

			if ((skipOptField & 0x08) == 0) {
				ifmm->extractBitField(8, &pNbrMRBS_NT->SchServSupport);
				loopLen -= 1;
			}

			ifmm->extractBitField(4, &pNbrMRBS_NT->DCDCfgCount);
			ifmm->extractBitField(4, &pNbrMRBS_NT->UCDCfgCount);
			loopLen -= 1;

			/*
			* Here we need to record the index of the nerghbor BS in MOB_NBR-ADV message
			*/
			pNbrMRBS_NT->Index = i;

			if (loopLen == 0) {
				continue;
			} else {
				/*
				* TLV Encoded Neighbor Infotmation
				*/
			}
		}
	}

	delete ifmm;

	return 1;
}

/*
*  Processing RNG-RSP (Fig.61)
*/
int mac802_16j_NT_PMPRS::procRNGRSP(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procRNGRSP State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int type = 0;
	int ftimingadj = 0;
	char fstatus = 1;
	char fmac[6] = "";
	uint16_t fbasicid = 0;
	//uint16_t frsbasicid = 0;
	uint64_t fpricid = 0;
	uint8_t rsv = 0;
	uint32_t rangAttr = 0;
	uint64_t tick = 0;
	bool cdmaFlag = false;

	//printf("Time:%llu RS(%d) %s()\n", GetCurrentTime(), get_nid(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 1);
	ifmm->extractBitField(8, &rsv);

	while ((type = ifmm->getNextType()) != 0) {
		//printf("Type = %d\n", type);
		switch (type) {
		case 1: // both
			ifmm->getNextValue(&ftimingadj);
			//printf("\e[1;36mTime:%llu RS(%d) Timing Adjust = %d\e[0m\n", GetCurrentTime(), get_nid(), ftimingadj);
			break;

		case 4: // both
			ifmm->getNextValue(&fstatus);
			//printf("Status = %d\n", fstatus);
			break;

		case 8: // only appear at replying RNG-REQ
			ifmm->getNextValue(fmac);
			cdmaFlag = false;
			//printf("mac=%x %x %x %x %x %x\n", fmac[0], fmac[1], fmac[2], fmac[3], fmac[4], fmac[5]);
			break;

		case 9: // only appear at replying RNG-REQ
			ifmm->getNextValue(&fbasicid);
			//printf("Basic CID = %d\n", fbasicid);
			break;

		case 10: // only appear at replying RNG-REQ
			ifmm->getNextValue(&fpricid);
			//printf("Primary CID = %lld\n", fpricid);
			break;

		case 17: // both
			ifmm->getNextValue(&ServerLevelPredict);
			//printf("Service Level Prediction = %d\n", ServerLevelPredict);
			break;

		case 26: // both
			ifmm->getNextValue(&PeriodRangingInterval);
			//printf("Period Ranging Interval = %d\n", PeriodRangingInterval);
			break;

		case 99:
			//    ifmm->getNextValue(&frsbasicid);
			//printf("RS(%d)frsbasicID = %d , RelayRSbasicid = %d\n",get_nid(), frsbasicid, RelayRSbasicid);

		case 150: // only appear at replying CDMA Ranging Code
			ifmm->getNextValue(&rangAttr);
			cdmaFlag = true;
			break;
		}
	}

	delete ifmm;

	if (cdmaFlag == true) // RNG-RSP because of CDMA
	{
		if (((savedCode.rangingCodeIndex & 0xFF) != ((rangAttr >> 8) & 0xFF))
				|| ((savedCode.rangingSymbol & 0x3FF) != ((rangAttr >> 22)
						& 0x3FF)) || ((savedCode.rangingSubchannel & 0x3F)
				!= ((rangAttr >> 16) & 0x3F)) || ((savedCode.rangingFrameNumber
				& 0xFF) != ((rangAttr & 0xFF)))) // Discard RNG-RSP not to this RS
		{
			return 0;
		}

		TimingAdjust += ftimingadj;
		RngLastStatus = fstatus;
		timer_mngr()->cancel_t(3u);
		timerSendRngCode->cancel();

		if (RngLastStatus == Abort) // 2:Abort
		{
			return 0;
		} else if (RngLastStatus == Continue) // 1:Continue
		{
			// Ranging at Period Ranging Region
			RngTime = RngStart - 1;
			WaitForRangingOpportunity();
			//printf("RS(%d) Period Ranging Continue! (ftimingadj = %d) TimingAdjust = %d\n", get_nid(), ftimingadj, TimingAdjust);
			return 0;
		} else // 3:Success
		{
			RngLastStatus = CodeSuccess; // ranging code success
			if (RngCodeRegion == RS_INITIAL_RANGING || RngCodeRegion
					== HANDOVER_RANGING) {
				//printf("\e[1;35mTime:%llu RS(%d) Initial or Handover Ranging Success!\e[0m\n", GetCurrentTime(), get_nid()); // BS shall allocate a txop at CDMA_Allocation_IE
			} else if (RngCodeRegion == PERIOD_RANGING) {
				MILLI_TO_TICK(tick, PeriodRangingInterval
						* frameDuration_NT[frameDuCode()]);
				timerPeriodRanging->start(tick, 0);
				//printf("Time:%llu RS(%d) Period Ranging Success! (restart at %llu)\n", GetCurrentTime(), get_nid(), tick);
			} else if (RngCodeRegion == RS_DEDICATED_CODES) {
				;
			} else {
				printf("RS(%d) Unknown Ranging!\n", get_nid());
			}

			return 0;
		}
	} else // RNG-RSP because of RNG-REQ
	{
		if (memcmp(fmac, address()->buf(), 6)) // Discard RNG-RSP that is not to this RS
			return 0;

		TimingAdjust += ftimingadj;
		RngLastStatus = fstatus;
		timer_mngr()->cancel_t(3u);
		timerSendRngReq->cancel();
		timerSendRngCode->cancel();

		if (RngLastStatus == Abort) // 2:Abort
		{
			return 0;
		}

		BasicCID = fbasicid;
		PriCID = fpricid;

		if (RngLastStatus == Continue) // 1:Continue
		{
			RngTime = RngStart - 1;
			WaitForRangingOpportunity();
			return 0;
		} else // 3:Success
		{
			//printf("Time:%llu RS(%d) Ranging Success (recv RNG-RSP after sending RNG-REQ)!\n", GetCurrentTime(),get_nid());

			MnConnections.push_back(new ManagementConnection(BasicCID));
			MnConnections.push_back(new ManagementConnection(PriCID));
			return 1; // ranging complete
		}
	}
}

void mac802_16j_NT_PMPRS::procRNGRSP_relay(struct mgmt_msg *recvmm, int cid,
		int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procRNGRSP_relay State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	ifmgmt *ifmm_send = NULL;
	int type = 0;
	int ftimingadj = 0;
	char fstatus = 1;
	char fmac[6] = "";
	uint16_t fbasicid = 0;
	//uint16_t frsbasicid = 0;
	uint64_t fpricid = 0;
	uint8_t rsv = 0;
	uint32_t rangAttr = 0;
	//uint64_t tick       = 0;
	bool cdmaFlag = false;
	uint8_t fmsnid = 0;
	uint8_t fmspid = 0;

	//printf("\n\e[1;35mTime:%llu RS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 1);
	ifmm->extractBitField(8, &rsv);

	ifmm_send = new ifmgmt(MG_RNGRSP, 1);
	ifmm_send->appendField(1, rsv);

	while ((type = ifmm->getNextType()) != 0) {
		//printf("Type = %d\n", type);
		switch (type) {
		case 1: // both
			ifmm->getNextValue(&ftimingadj);
			ifmm_send->appendTLV(1, 4, ftimingadj);
			//printf("\e[1;36mTime:%llu RS(%d) Timing Adjust = %d\e[0m\n", GetCurrentTime(), get_nid(), ftimingadj);
			break;

		case 4: // both
			ifmm->getNextValue(&fstatus);
			ifmm_send->appendTLV(4, 1, fstatus);
			//printf("Status = %d\n", fstatus);
			break;

		case 8: // only appear at replying RNG-REQ
			ifmm->getNextValue(fmac);
			cdmaFlag = false;
			ifmm_send->appendTLV(8, 6, fmac);
			//printf("mac=%x %x %x %x %x %x\n", fmac[0], fmac[1], fmac[2], fmac[3], fmac[4], fmac[5]);
			break;

		case 9: // only appear at replying RNG-REQ
			ifmm->getNextValue(&fbasicid);
			ifmm_send->appendTLV(9, 2, fbasicid);
			//printf("Basic CID = %d\n", fbasicid);
			break;

		case 10: // only appear at replying RNG-REQ
			ifmm->getNextValue(&fpricid);
			ifmm_send->appendTLV(10, 2, fpricid);
			//printf("Primary CID = %lld\n", fpricid);
			break;

		case 17: // both
			ifmm->getNextValue(&ServerLevelPredict);
			ifmm_send->appendTLV(17, 1, ServerLevelPredict);
			//printf("Service Level Prediction = %d\n", ServerLevelPredict);
			break;

		case 26: // both
			ifmm->getNextValue(&PeriodRangingInterval);
			ifmm_send->appendTLV(26, 2, PeriodRangingInterval);
			//printf("Period Ranging Interval = %d\n", PeriodRangingInterval);
			break;

		case 99:
			//    ifmm->getNextValue(&frsbasicid);
			//printf("RS(%d)frsbasicID = %d , RelayRSbasicid = %d\n",get_nid(), frsbasicid, RelayRSbasicid);

		case 100:
			ifmm->getNextValue(&fmsnid);
			//printf("ms nid = %d\n",fmsnid);
			break;

		case 101:
			ifmm->getNextValue(&fmspid);
			//printf("ms pid = %d\n",fmspid);
			break;

		case 150: // only appear at replying CDMA Ranging Code
			ifmm->getNextValue(&rangAttr);
			cdmaFlag = true;
			break;
		}
	}

	delete ifmm;

	if (fstatus == 3) {
		//printf("Time:%llu RS create DLRelay & ULRelay MnConnections (cid = %d)\n",GetCurrentTime(),fbasicid);
		DLRelayMnConnections.push_back(new ManagementConnection(fbasicid));
		DLRelayMnConnections.push_back(new ManagementConnection(fpricid));
		ULRelayMnConnections.push_back(new ManagementConnection(fbasicid));
		ULRelayMnConnections.push_back(new ManagementConnection(fpricid));
	}

	// relay the received RNG-RSP with rangingCID
	initRangingConnection->Insert(ifmm_send);
}

int mac802_16j_NT_PMPRS::procSBCRSP(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procSBCRSP State: %x",
			get_nid(), State);

	//printf("\nTime:%llu:mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(), __func__);
	return 1;
}

int mac802_16j_NT_PMPRS::procMOB_SCNRSP(struct mgmt_msg *recvmm, int cid,
		int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procMOB_SCNRSP State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int ifmmLen = 0;
	uint8_t rsv = 0;
	uint8_t N_RecBS_Index = 0;
	uint8_t CfgChangeCnt = 0;
	uint8_t Index = 0;
	uint8_t scanType = 0;
	uint8_t NBR_BS_ID[6] = "";
	uint8_t N_RecBS_Full = 0;
	uint8_t padding = 0;
	int tmpBits = 0;

	//printf("\n\e[32mTime:%llu\e[33m RS(%d):%s\e[0m\n", GetCurrentTime(), get_nid(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);

	ifmm->extractBitField(8, &NeighborMRBSList->ScanDuration);
	ifmm->extractBitField(2, &NeighborMRBSList->ReportMode);
	ifmm->extractBitField(6, &rsv);
	ifmm->extractBitField(8, &NeighborMRBSList->ReportPeriod);
	ifmm->extractBitField(8, &NeighborMRBSList->ReportMetric);
	tmpBits += 32;

	if (NeighborMRBSList->ScanDuration != 0) {
		ifmm->extractBitField(4, &NeighborMRBSList->StartFrame);
		ifmm->extractBitField(1, &rsv);
		ifmm->extractBitField(8, &NeighborMRBSList->InterleavingInterval);
		ifmm->extractBitField(8, &NeighborMRBSList->ScanIteration);
		ifmm->extractBitField(3, &padding);
		ifmm->extractBitField(8, &N_RecBS_Index);
		tmpBits += 32;

		if (N_RecBS_Index != 0) {
			ifmm->extractBitField(8, &CfgChangeCnt);
			tmpBits += 8;
		}

		if (CfgChangeCnt == NeighborMRBSList->NBRADV_CfgCount) {
			for (int i = 0; i < N_RecBS_Index; i++) {
				ifmm->extractBitField(8, &Index);
				ifmm->extractBitField(3, &scanType);
				tmpBits += 11;

				NeighborMRBSList->nbrBSs_Index[Index]->ScanType = scanType;

				if (scanType == 0x2 || scanType == 0x3) {
					uint8_t rendezvousTime = 0;
					ifmm->extractBitField(8, &rendezvousTime);

					uint8_t CDMA_code = 0;
					ifmm->extractBitField(8, &CDMA_code);

					uint8_t TxopOffset = 0;
					ifmm->extractBitField(8, &TxopOffset);

					tmpBits += 24;
				}
			}
		}

		ifmm->extractBitField(8, &N_RecBS_Full);
		tmpBits += 8;

		for (int i = 0; i < N_RecBS_Full; i++) {
			NbrMRBS_NT *pNbrMRBS_NT = NULL;

			ifmm->extractBitField(48, NBR_BS_ID);
			ifmm->extractBitField(8, &scanType);
			tmpBits += 56;

			pNbrMRBS_NT = NeighborMRBSList->getNbrbyBSID(NBR_BS_ID);
			if (pNbrMRBS_NT == NULL)// FIXME: need??
			{
				pNbrMRBS_NT = new NbrMRBS_NT(-1, NBR_BS_ID);
				NeighborMRBSList->nbrBSs_Full.push_back(pNbrMRBS_NT);
			}

			pNbrMRBS_NT->ScanType = scanType;

			if (scanType == 0x2 || scanType == 0x3) {
				uint8_t rendezvousTime = 0;
				ifmm->extractBitField(8, &rendezvousTime);

				uint8_t CDMA_code = 0;
				ifmm->extractBitField(8, &CDMA_code);

				uint8_t TxopOffset = 0;
				ifmm->extractBitField(8, &TxopOffset);

				tmpBits += 24;
			}
		}

		if (tmpBits % 8 != 0) {
			ifmm->extractBitField(8 - tmpBits % 8, &padding);
			ifmmLen = tmpBits / 8 + 1;
		} else {
			ifmmLen = tmpBits / 8;
		}

		delete ifmm;

		/*
		* TLV encoded information
		*/
		if ((tmpBits / 8) != (len - 1)) {
			int type = 0;
			uint8_t hmac[21] = "";

			ifmm = new ifmgmt((uint8_t *) recvmm, len, ifmmLen);

			while ((type = ifmm->getNextType()) != 0) {
				switch (type) {
				case 149:
					ifmm->getNextValue(hmac);
					break;
				}
			}
			delete ifmm;
		}

		/* Schedule an timer to perform scanning */
		uint64_t tick = 0;

		MICRO_TO_TICK(tick, ULsymbols * Ts() + (TTG() + RTG() + TimingAdjust)
				* PS());

		timerStartScan->start(tick, 0);
	}

	return 1;
}

int mac802_16j_NT_PMPRS::procREGRSP(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procREGRSP State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int type = 0;
	uint8_t fmanage = 0;
	uint8_t fipm = 0;
	uint8_t farq = 0;
	uint8_t fmobile = 0;
	uint16_t fulncid = 0;
	uint16_t fdlncid = 0;
	uint16_t fclass = 0;
	uint16_t ResourceRetainTime = 0;

	//printf("\nTime:%llu:mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 0);

	while ((type = ifmm->getNextType()) != 0) {
		switch (type) {
		case 2:
			ifmm->getNextValue(&fmanage);
			break;

		case 3:
			ifmm->getNextValue(&fipm);
			break;

		case 6:
			ifmm->getNextValue(&fulncid);
			break;

		case 7:
			ifmm->getNextValue(&fclass);
			break;

		case 10:
			ifmm->getNextValue(&farq);
			break;

		case 15:
			ifmm->getNextValue(&fdlncid);
			break;

		case 28:
			ifmm->getNextValue(&ResourceRetainTime);

		case 30:
			ifmm->getNextValue(&HORetransmitInterval);
			break;

		case 31:
			ifmm->getNextValue(&fmobile);
			break;
		}
	}

	delete ifmm;

	return 1;
}

int mac802_16j_NT_PMPRS::procDSAREQ(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procDSAREQ State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	ifTLV *tmptlv = NULL;
	int type1 = 0;
	int type2 = 0;
	uint8_t fbuf[256] = "";
	uint8_t fhmac[21] = "";
	uint8_t fcsspec = 0;
	;
	uint16_t ftransid = 0;
	uint16_t fdatacid = 0;
	uint8_t fconfirm = 0;

	//printf("\e[1;35m\nTime:%llu:mac802_16j_NT_PMPRS::%s()\n\e[0m", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 2);
	ifmm->extractBitField(16, (uint8_t *) &ftransid);
	//printf("Transaction ID = %d\n", ftransid);

	while ((type1 = ifmm->getNextType()) != 0) {
		//printf("Type = %d\t", type1);
		switch (type1) {
		case 145: // Uplink service flow
			ifmm->getNextValue(fbuf);
			{
				//printf("UL Service Flow Sub-Type Length = %d\n", ifmm->getNextLen());
				tmptlv = new ifTLV(fbuf, ifmm->getNextLen());

				while ((type2 = tmptlv->getNextType()) != 0) {
					//printf(" sub-type = %d\t", type2);
					switch (type2) {
					case 2:
						tmptlv->getNextValue(&fdatacid);
						//printf("Data CID = %d\n", fdatacid);
						break;
					case 28:
						tmptlv->getNextValue(&fcsspec);
						_CSType = fcsspec;
						//printf("CS Specification = %d\n", fcsspec);
					}
				}
				delete tmptlv;
			}
			break;
		case 149:
			ifmm->getNextValue(fhmac);
			//printf("HMAC = ...\n");
			break;
		}

	}
	delete ifmm;

	if (_CSType == 3 || _CSType == 5) // Ethernet
	{
		DtConnections.push_back(new DataConnectionEthernet(fdatacid));
	} else {
		DtConnections.push_back(new DataConnection(fdatacid));
	}

	ReceiverList.push_back(new ConnectionReceiver(fdatacid));

	ifmm = new ifmgmt(MG_DSARSP, 3); // Spec 16e. 11.13
	ifmm->appendBitField(16, ftransid); // Transcation ID
	ifmm->appendBitField(8, fconfirm); // Confirmation Code: OK, Table 384
	ifmm->appendTLV(149, 21, fhmac); // HMAC

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = cid;

	PushPDUintoConnection(cid, ifmm);
	resetTimerT(8u);

	return 1;
}

int mac802_16j_NT_PMPRS::procDSAREQ_relay(struct mgmt_msg *recvmm, int cid,
		int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procDSAREQ_relay State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	ifTLV *tmptlv = NULL;
	int type1 = 0;
	int type2 = 0;
	uint8_t fbuf[256] = "";
	uint8_t fhmac[21] = "";
	uint8_t fcsspec = 0;
	;
	uint16_t ftransid = 0;
	uint16_t fdatacid = 0;
	//uint8_t fconfirm    = 0;

	//printf("\e[1;35m\nTime:%llu:mac802_16j_NT_PMPRS::%s()\n\e[0m", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 2);
	ifmm->extractBitField(16, (uint8_t *) &ftransid);
	//printf("Transaction ID = %d\n", ftransid);

	while ((type1 = ifmm->getNextType()) != 0) {
		//printf("Type = %d\t", type1);
		switch (type1) {
		case 145: // Uplink service flow
			ifmm->getNextValue(fbuf);
			{
				//printf("UL Service Flow Sub-Type Length = %d\n", ifmm->getNextLen());
				tmptlv = new ifTLV(fbuf, ifmm->getNextLen());

				while ((type2 = tmptlv->getNextType()) != 0) {
					//printf(" sub-type = %d\t", type2);
					switch (type2) {
					case 2:
						tmptlv->getNextValue(&fdatacid);
						//printf("Data CID = %d\n", fdatacid);
						break;
					case 28:
						tmptlv->getNextValue(&fcsspec);
						_CSType = fcsspec;
						//printf("CS Specification = %d\n", fcsspec);
					}
				}
				delete tmptlv;
			}
			break;
		case 149:
			ifmm->getNextValue(fhmac);
			//printf("HMAC = ...\n");
			break;
		}

	}
	delete ifmm;
	logRidvan(WARN, "rs processed DSA-REQ nid:%d cid:%d", get_nid(), fdatacid);

	ULRelayDtConnections.push_back(new DataConnection(fdatacid));
	DLRelayDtConnections.push_back(new DataConnection(fdatacid));
	ULRelayReceiverList.push_back(new ConnectionReceiver(fdatacid));
	DLRelayReceiverList.push_back(new ConnectionReceiver(fdatacid));

	return 1;
}

int mac802_16j_NT_PMPRS::procDSAACK(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procDSAACK State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int type = 0;
	uint8_t fhmac[21] = "";
	uint16_t ftransid = 0;

	//printf("\nTime:%llu:mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 2);
	ifmm->extractBitField(16, (uint8_t *) &ftransid);
	//printf("Transaction ID = %d\n", ftransid);

	while ((type = ifmm->getNextType()) != 0) {
		//printf("Type = %d\t", type);
		switch (type) {
		case 149:
			ifmm->getNextValue(fhmac);
			//printf("HMAC = ...\n");
			break;
		}

	}
	logRidvan(WARN, "rs processed DSA-ACK nid:%d cid:%d", get_nid(), cid);
	delete ifmm;
	//printf("\n\e[33m================RS(%d)==IF=PROGRAM=RUN=TO=HERE=THAT'S=NETWORK=ENTRY=OK=======================\e[0m\n\n", get_nid());

	return 1;
}

int mac802_16j_NT_PMPRS::procMOB_BSHORSP(struct mgmt_msg *recvmm, int cid,
		int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procMOB_BSHORSP State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int ifmmLen = 0;
	uint8_t Mode = 0;
	uint8_t rsv = 0;
	uint8_t padding = 0;
	int tmpBits = 0;
	uint8_t HO_operation_mode = 0;

	//printf("\n\e[1;36mTime:%llu RS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);
	//printf("========================================\n");

	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1); // Here, the (len - 1) is the maximum length of mgmt_msg, not field size
	ifmm->extractBitField(3, &Mode);
	//printf("= Mode: %d\n", Mode);
	ifmm->extractBitField(5, &rsv);
	//printf("= rsv: %d\n", rsv);
	tmpBits += 8;

	if (Mode == 0x00) {
		ifmm->extractBitField(1, &HO_operation_mode);
		//printf("= HO_operation_mode: %d\n", HO_operation_mode);

		uint8_t N_Recommended = 0;
		ifmm->extractBitField(8, &N_Recommended);
		//printf("= N_Recommended: %d\n", N_Recommended);

		uint8_t ResourceRetainFlag = 0;
		ifmm->extractBitField(1, &ResourceRetainFlag);
		//printf("= ResourceRetainFlag: %d\n", ResourceRetainFlag);
		ifmm->extractBitField(7, &rsv);

		tmpBits += 17;

		for (int i = 0; i < N_Recommended; i++) {
			uint8_t Neighbor_BSID[6] = "";
			ifmm->extractBitField(48, Neighbor_BSID);
			tmpBits += 48;

			//printf("======[%d] Neighbor_BSID: %d:%d:%d:%d:%d:%d\n", i, Neighbor_BSID[0], Neighbor_BSID[1], Neighbor_BSID[2], Neighbor_BSID[3], Neighbor_BSID[4], Neighbor_BSID[5]);
		}

		uint8_t PreambleIndex = 0;
		ifmm->extractBitField(8, &PreambleIndex);
		//printf("= PreambleIndex: %d\n", PreambleIndex);

		uint8_t ServiceLevelPredict = 0;
		ifmm->extractBitField(8, &ServiceLevelPredict);
		//printf("= ServiceLevelPredict: %d\n", ServiceLevelPredict);

		uint8_t HO_process_optimization = 0;
		ifmm->extractBitField(8, &HO_process_optimization);
		//printf("= HO_process_optimization: %d\n", HO_process_optimization);

		uint8_t Network_Assisted_HO_Support = 0;
		ifmm->extractBitField(1, &Network_Assisted_HO_Support);
		//printf("= Network_Assisted_HO_Support: %d\n", Network_Assisted_HO_Support);

		uint8_t HO_ID_Indicator = 0;
		ifmm->extractBitField(1, &HO_ID_Indicator);
		//printf("= HO_ID_Indicator: %d\n", HO_ID_Indicator);
		tmpBits += 26;

		if (HO_ID_Indicator == 1) {
			uint8_t HO_ID = 0;
			ifmm->extractBitField(8, &HO_ID);
			//printf("= HO_ID: %d\n", HO_ID);
			tmpBits += 8;
		}
		uint8_t HO_auth_Indicator = 0;
		ifmm->extractBitField(1, &HO_auth_Indicator);
		//printf("= HO_auth_Indicator: %d\n", HO_auth_Indicator);
		ifmm->extractBitField(4, &rsv);
		//printf("= rsv:%d\n", rsv);
		tmpBits += 5;

		if (HO_auth_Indicator == 1) {
			uint8_t HO_auth_support = 0;
			ifmm->extractBitField(8, &HO_auth_support);
			//printf("= HO_auth_support: %d\n", HO_auth_support);
			tmpBits += 8;
		}
	} else if (Mode == 0x01)
		;
	else if (Mode == 0x02)
		;
	else if (Mode == 0x03)
		;
	else if (Mode == 0x04)
		;
	else if (Mode == 0x05)
		;
	else if (Mode == 0x06)
		;
	else if (Mode == 0x08)
		;
	else
		;

	uint8_t ActionTime = 0;
	ifmm->extractBitField(8, &ActionTime);
	//printf("= ActionTime: %d\n", ActionTime);
	tmpBits += 8;

	if (tmpBits % 8 != 0) {
		ifmm->extractBitField(8 - tmpBits % 8, &padding);
		//printf("= padding(%d): %d\n", 8 - tmpBits % 8, padding);
		ifmmLen = tmpBits % 8 + 1;
	} else {
		ifmmLen = tmpBits % 8;
	}

	delete ifmm;

	//printf("========================================\n");
	/*
	* TLV encoded information
	*/

	SendMOB_HOIND(HO_operation_mode);

	return 1;
}

uint8_t mac802_16j_NT_PMPRS::selectDIUC(double recvSNR) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::selectDIUC State: %x",
			get_nid(), State);

	int fec = 0;

	if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0) {
		for (int i = 12; i >= 0; i--) {
			fec = DCDProfile[i].fec;
			if (DCDProfile[i].used && recvSNR >= fecSNR[fec]) {
				return i;
			}
		}
		return RobustDIUC;
	}

	if (strcasecmp(LinkMode, "QPSK1/2") == 0)
		fec = QPSK_1_2;
	else if (strcasecmp(LinkMode, "QPSK3/4") == 0)
		fec = QPSK_3_4;
	else if (strcasecmp(LinkMode, "16QAM1/2") == 0)
		fec = QAM16_1_2;
	else if (strcasecmp(LinkMode, "16QAM3/4") == 0)
		fec = QAM16_3_4;
	else if (strcasecmp(LinkMode, "64QAM1/2") == 0)
		fec = QAM64_1_2;
	else if (strcasecmp(LinkMode, "64QAM2/3") == 0)
		fec = QAM64_2_3;
	else if (strcasecmp(LinkMode, "64QAM3/4") == 0)
		fec = QAM64_3_4;
	else
		printf("%s#%d: Warning Configure String LinkMode:%s\n", __FILE__,
				__LINE__, LinkMode);

	for (int i = 0; i <= 12; i++) {
		if (DCDProfile[i].used && DCDProfile[i].fec == fec) {
			return i;
		}
	}

	return 255;
}

/*
* Send Initial or Handover Ranging Code
*/
int mac802_16j_NT_PMPRS::SendRNGCODE() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendRNGCODE State: %x",
			get_nid(), State);

	RangCodeInfo *codeInfo = NULL;
	ePacket_ *epkt = NULL;
	int codeIndex = 0;
	uint8_t *code = NULL;
	int dupFactor = 0;
	int rangMethod = 0;

	uint64_t tick1;
	MICRO_TO_TICK(tick1, PSsToMicro(RelayULallocStartTime));
	tick1 += LastSignalInfo.frameStartTime;

	//printf("[%d]Time:%llu RS(%d)::%s fNumber=%d fSTime=%llu RULaSTime=%llu***\n",__LINE__, GetCurrentTime(), get_nid(), __func__,frameNumber&0xFF,LastSignalInfo.frameStartTime,tick1);
	if (RngCodeRegion == RS_INITIAL_RANGING || RngCodeRegion
			== HANDOVER_RANGING) {
		if (savedIHie.ie_12.rangMethod == 0) {
			dupFactor = 2;
		} else if (savedIHie.ie_12.rangMethod == 1) {
			dupFactor = 4;
		} else {
			fprintf(
					stderr,
					"Warning: Impossible Point. (Initial Ranging/Handover Ranging over unknown rangMethod)\n");
			exit(1);
		}
		rangMethod = savedIHie.ie_12.rangMethod;
	} else if (RngCodeRegion == PERIOD_RANGING || RngCodeRegion == BW_REQUEST) {
		if (savedBPie.ie_12.rangMethod == 2) {
			dupFactor = 1;
		} else if (savedIHie.ie_12.rangMethod == 3) {
			dupFactor = 3;
		} else {
			fprintf(
					stderr,
					"\e[1;31mWarning: Impossible Point. (BW Request/Period Ranging over unknown rangMethod)\e[0m\n");
			exit(1);
		}
		rangMethod = savedBPie.ie_12.rangMethod;
	} else if (RngCodeRegion == RS_DEDICATED_CODES) {
		dupFactor = 1;
	} else {
		;
	}

	codeIndex = getCode(RngCodeRegion);

	if (RngCodeRegion == PERIOD_RANGING) { //offset of perodic ranging code region
		codeIndex -= 43;
	} else if (RngCodeRegion == BW_REQUEST) {
		codeIndex -= 86;
	} else if (RngCodeRegion == HANDOVER_RANGING) {
		codeIndex -= 129;
	} else if (RngCodeRegion == RS_INITIAL_RANGING) {
		codeIndex -= 172;
	} else if (RngCodeRegion == RS_DEDICATED_CODES) {
		codeIndex -= 212;
	} else // INITIAL_RANGING
	{
		; // no offset
	}

	code = new uint8_t[18 * dupFactor];
	memset(code, 0, 18 * dupFactor);

	if (RngCodeRegion == INITIAL_RANGING) {
		for (int i = 0; i < dupFactor; i++) {
			memcpy(code + 18 * i, _initRang_code_db[codeIndex], 18);
		}
	} else if (RngCodeRegion == HANDOVER_RANGING) {
		for (int i = 0; i < dupFactor; i++) {
			memcpy(code + 18 * i, _handoverRang_code_db[codeIndex], 18);
		}
	} else if (RngCodeRegion == PERIOD_RANGING) {
		for (int i = 0; i < dupFactor; i++) {
			memcpy(code + 18 * i, _periodRang_code_db[codeIndex], 18);
		}
	} else if (RngCodeRegion == BW_REQUEST) // BW request ranging
	{
		for (int i = 0; i < dupFactor; i++) {
			memcpy(code + 18 * i, _bwRequest_code_db[codeIndex], 18);
		}
	} else if (RngCodeRegion == RS_INITIAL_RANGING) {
		for (int i = 0; i < dupFactor; i++) {
			memcpy(code + 18 * i, _rs_initRang_code_db[codeIndex], 18);
		}
	} else if (RngCodeRegion == RS_DEDICATED_CODES) {
		for (int i = 0; i < dupFactor; i++) {
			memcpy(code + 18 * i, _rs_dedicated_code_db[codeIndex], 18);
		}
	}

	// save to codeInfo
	codeInfo = new RangCodeInfo(18* dupFactor , RngSymOffset, RngChOffset,
			rangMethod);
	codeInfo->flag = PF_SEND;
	codeInfo->rangingCode = code;

	// send to Phy
	epkt = createEvent();
	epkt->DataInfo_ = codeInfo;
	put(epkt, sendtarget_);

	// save at local
	savedCode.rangingFrameNumber = (frameNumber & 0xFF);
	if (RngCodeRegion == PERIOD_RANGING) {
		savedCode.rangingCodeIndex = codeIndex + 43;
	} else if (RngCodeRegion == BW_REQUEST) {
		savedCode.rangingCodeIndex = codeIndex + 86;
	} else if (RngCodeRegion == HANDOVER_RANGING) {
		savedCode.rangingCodeIndex = codeIndex + 129;
	} else if (RngCodeRegion == RS_INITIAL_RANGING) {
		savedCode.rangingCodeIndex = codeIndex + 172;
	} else if (RngCodeRegion == RS_DEDICATED_CODES) {
		savedCode.rangingCodeIndex = codeIndex + 212;
	} else // INITIAL_RANGING
	{
		savedCode.rangingCodeIndex = codeIndex;
	}
	savedCode.rangingSymbol = RngSymOffset;
	savedCode.rangingSubchannel = RngChOffset;
	savedCode.rangingUsage = RngCodeRegion;

	RngLastStatus = CodeWaitRSP; // Waiting For anonymous RNGRSP and CDMA Allocation IE
	resetTimerT(3u);

	return 1;
}

int mac802_16j_NT_PMPRS::SendRNGREQ() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendRNGREQ State: %x",
			get_nid(), State);

	int len = 0;
	int thisLen = 0;
	uint8_t rsv = 0;
	ifmgmt *ifmm = NULL;
	char *buf = NULL;
	upBurst *burst = NULL;
	BurstInfo *burstInfo = NULL;
	ePacket_ *epkt = NULL;
	vector<upBurst *> *BurstCollection = NULL;
	vector<int> pduInfo;

	myDIUC = selectDIUC(LastSignalInfo.SNR);

	printf(
			"\e[36mTime:%llu RS(%d)::%s Receive SNR=%lf dB and use diuc=%d(fec=%d)\e[0m\n",
			GetCurrentTime(), get_nid(), __func__, LastSignalInfo.SNR, myDIUC,
			DCDProfile[myDIUC].fec);

	ifmm = new ifmgmt(MG_RNGREQ, 1);
	ifmm->appendBitField(8, rsv); // Reserve
	ifmm->appendTLV(1, 1, myDIUC); // Requested Downlink Burst Profile
	ifmm->appendTLV(2, 6, address()->buf()); // RS MAC Address
	//ifmm->appendTLV(99, 2, RelayRSbasicid);     //trick TLV record access RS bcid

	if (RngLastStatus == Continue) // Continue
		ifmm->appendTLV(3, 1, 3); // Ranging Anomalies

	PushPDUintoConnection(initRangingCID, ifmm);

	// generate a burst
	burst = new upBurst(&savedALie, UCDProfile[savedALie.ie_14.uiuc_trans].fec);

	// compute total length
	pduInfo.clear();
	len = initRangingConnection->GetInformation(&pduInfo);
	thisLen = burst->toByte(savedALie.ie_14.duration);
	if (len > thisLen) {
		fprintf(
				stderr,
				"RS(%d) CDMA allocation slots is not enough to send RNG-REQ. (allocate:%d need:%d)\n",
				get_nid(), thisLen, len);
		exit(1);
	}

	// encapsulate all
	buf = new char[len];
	memset(buf, 0, len);
	burst->payload = buf;
	burst->length = initRangingConnection->EncapsulateAll(buf, len);

	BurstCollection = new vector<upBurst *> ;
	BurstCollection->push_back(burst);

	// send to PHY
	burstInfo = new BurstInfo;
	burstInfo->Collection = (vector<WiMaxBurst *> *) BurstCollection;
	burstInfo->type = RS_RNGREQ;
	burstInfo->flag = PF_SEND;

	epkt = createEvent();
	epkt->DataInfo_ = burstInfo;
	put(epkt, sendtarget_);

	RngLastStatus = REQWaitRSP; // Waiting For RNGRSP
	resetTimerT(3u);

	return 1;
}

int mac802_16j_NT_PMPRS::SendREGREQ() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendREGREQ State: %x",
			get_nid(), State);

	ifmgmt *ifmm = new ifmgmt(MG_REGREQ, 0);

	//printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS:%s()\n", GetCurrentTime(), get_nid(), __func__);

	// For PMP operation, the REG-REQ shall contain following three TLVs:
	ifmm->appendTLV(2, 1, 0U); // RS management support 11.7.2
	ifmm->appendTLV(3, 1, 0U); // IP management mode 11.7.3
	ifmm->appendTLV(6, 2, 1); // # of UL Transport CIDs supported
	ifmm->appendTLV(7, 2, 0x09); // Classification and SDU support
	ifmm->appendTLV(10, 1, AttrARQ); // ARQ Support
	ifmm->appendTLV(15, 2, 1); // # of DL Transport CIDs supported
	ifmm->appendTLV(31, 1, 0x01); // Mobility supported

	//16j Spec. 11.7.24.1 RS MAC feature support
	ifmm->appendTLV(50, 1, 0x01); //RS MAC feature suppport 11.7.24.1 , Bit#0 =1 (Centralized Scheduling mode)
	ifmm->appendTLV(51, 1, 0U); //MR MAC header and extended subheader support 11.7.24.2

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = PriCID;

	PushPDUintoConnection(PriCID, ifmm);
	resetTimerT(6u); // wait for REG-RSP

	return 1;
}

int mac802_16j_NT_PMPRS::SendSBCREQ() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendSBCREQ State: %x",
			get_nid(), State);

	ifmgmt *ifmm = new ifmgmt(MG_SBCREQ, 0);

	//printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(), get_nid(), __func__);

	ifmm->appendTLV(1, 1, 0U); // Bandwidth Allocation Support 11.8.1
	ifmm->appendTLV(2, 2, 0x2020); // Subscriber transition gaps 11.8.3.1  (TTG=RTG=90(PS)=32(us))
	ifmm->appendTLV(3, 4, 0xFFFFFFFF); // Maximum transmit power 11.8.3.2
	ifmm->appendTLV(147, 1, 0xFF); // Current transmit power 11.8.3.3

	// OFDMA specific parameters 11.8.3.7
	ifmm->appendTLV(150, 1, 0x10); // FFT-1024
	ifmm->appendTLV(151, 2, 0x0001); // 64-QAM
	ifmm->appendTLV(152, 1, 0x01); // 64-QAM
	ifmm->appendTLV(154, 1, 0U); // Permutation support

	ifmm->appendTLV(206, 1, 0xFF); //RS Maximum downlink transmit power
	ifmm->appendTLV(215, 1, 0x0A); //RSRTG (RSTTG=RSRTG=30(PS)=10(us)) 11.8.3.5.27 16j Spec 12.4.3.1.5
	ifmm->appendTLV(216, 1, 0x0A); //RSTTG (RSTTG=RSRTG=30(PS)=10(us)) 11.8.3.5.27 16j Spec 12.4.3.1.5

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = BasicCID;

	PushPDUintoConnection(BasicCID, ifmm);
	resetTimerT(18u); // wait for SBC-RSP

	return 1;
}

/*
* RS requests a scanning interval for scanning BSs
*/
int mac802_16j_NT_PMPRS::SendMOB_SCNREQ() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendMOB_SCNREQ State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int ifmmLen = 0;
	uint8_t scanDuration = 1; // in frames
	uint8_t interleaveInt = 10; // in frames
	uint8_t scanIteration = 1;
	uint8_t N_RecBS_index = NeighborMRBSList->nbrBSs_Index.size();
	uint8_t N_RecBS_Full = NeighborMRBSList->nbrBSs_Full.size();
	uint8_t padding = 0;
	int tmpBits = 0;

	//printf("\nTime:%llu:RS(%d): mac802_16j_NT_PMPRS::%s()\n", GetCurrentTime(), get_nid(), __func__);
	/*
	* Compute all field bits
	*/
	tmpBits += 32;

	if (N_RecBS_index != 0)
		tmpBits += 8;

	for (int i = 0; i < N_RecBS_index; i++)
		tmpBits += 11;

	tmpBits += 8;

	for (int i = 0; i < N_RecBS_Full; i++)
		tmpBits += 51;

	if (tmpBits % 8 != 0)
		ifmmLen = tmpBits / 8 + 1;
	else
		ifmmLen = tmpBits / 8;

	/*
	* Generate MOB_SCN-REQ Message
	*/

	// max = 5
	scanDuration = NeighborMRBSList->nbrBSs_Index.size()
			+ NeighborMRBSList->nbrBSs_Full.size();
	if (scanDuration > 5) {
		scanDuration = 5;
	} else if (scanDuration == 0) // no BSs need to scan
	{
		return 1;
	}

	ifmm = new ifmgmt(MG_MOB_SCNREQ, ifmmLen);

	ifmm->appendBitField(8, scanDuration);
	ifmm->appendBitField(8, interleaveInt);
	ifmm->appendBitField(8, scanIteration);
	ifmm->appendBitField(8, N_RecBS_index);

	if (N_RecBS_index != 0)
		ifmm->appendBitField(8, NeighborMRBSList->NBRADV_CfgCount);

	for (int i = 0; i < N_RecBS_index; i++) {
		ifmm->appendBitField(8, i);
		ifmm->appendBitField(3, NeighborMRBSList->nbrBSs_Index[i]->ScanType);
	}

	ifmm->appendBitField(8, N_RecBS_Full);

	for (int i = 0; i < N_RecBS_Full; i++) {
		ifmm->appendBitField(48, NeighborMRBSList->nbrBSs_Full[i]->addr->buf());
		ifmm->appendBitField(3, NeighborMRBSList->nbrBSs_Full[i]->ScanType);
	}

	if (tmpBits % 8 != 0)
		ifmm->appendBitField(8 - tmpBits % 8, padding);

	/*
	* TLV encoded information
	*/

	PushPDUintoConnection(BasicCID, ifmm);

	return 1;
}

/*
* MS reports the scanning results to its serving BS
*/
int mac802_16j_NT_PMPRS::SendMOB_SCNREP() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendMOB_SCNREP State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int ifmmLen = 0;
	uint8_t repMode = 0;
	uint8_t Comp_NBR_BSID_IND = 0;
	uint8_t N_curr_BSs = 0;
	uint8_t rsv = 0;
	uint8_t repMetric = 0;
	uint8_t N_BS_Index = 0;
	uint8_t N_BS_Full = 0;
	int tmpBits = 0;
	uint8_t tempBSID = 0; // soft handover is not supported
	uint8_t BS_CINR = 0;
	uint8_t BS_RSSI = 0;
	uint8_t RelativeDelay = 0;
	uint8_t BS_RTD = 0;
	uint8_t CfgChangeCnt = 0;
	uint8_t neighborBS_Index = 0;
	uint8_t neighborBS_ID[6] = "";

	/*
	* Compute all field bits
	*/

	tmpBits += (1 + 1 + 3 + 3 + 8);

	for (int i = 0; i < N_curr_BSs; i++) {
		tmpBits += (4 + 4);

		if ((repMetric & 0x1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x2) >> 1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x4) >> 2) == 1)
			tmpBits += 8;

		if (((repMetric & 0x8) >> 3) == 1)
			tmpBits += 8;
	}

	tmpBits += 8;

	if (N_BS_Index != 0)
		tmpBits += 8;

	for (int i = 0; i < N_BS_Index; i++) {
		tmpBits += 8;

		if ((repMetric & 0x1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x2) >> 1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x4) >> 2) == 1)
			tmpBits += 8;
	}

	tmpBits += 8;

	for (int i = 0; i < N_BS_Full; i++) {
		tmpBits += 48;

		if ((repMetric & 0x1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x2) >> 1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x4) >> 2) == 1)
			tmpBits += 8;
	}

	ifmmLen = tmpBits / 8;

	/*
	* Generate MOB_SCN-REP Message
	*/
	ifmm = new ifmgmt(MG_MOB_SCNREP, ifmmLen);

	ifmm->appendBitField(1, repMode);
	ifmm->appendBitField(1, Comp_NBR_BSID_IND);
	ifmm->appendBitField(3, N_curr_BSs);
	ifmm->appendBitField(3, rsv);
	ifmm->appendBitField(8, repMetric);

	for (int i = 0; i < N_curr_BSs; i++) {
		ifmm->appendBitField(4, tempBSID);
		ifmm->appendBitField(4, rsv);

		if ((repMetric & 0x1) == 1) {
			ifmm->appendBitField(8, BS_CINR);
		}

		if (((repMetric & 0x2) >> 1) == 1) {
			ifmm->appendBitField(8, BS_RSSI);
		}

		if (((repMetric & 0x4) >> 2) == 1) {
			ifmm->appendBitField(8, RelativeDelay);
		}

		if (((repMetric & 0x8) >> 3) == 1) {
			ifmm->appendBitField(8, BS_RTD);
		}
	}

	ifmm->appendBitField(8, N_BS_Index);

	if (N_BS_Index != 0) {
		ifmm->appendBitField(8, CfgChangeCnt);
	}

	for (int i = 0; i < N_BS_Index; i++) {
		ifmm->appendBitField(8, neighborBS_Index);

		if ((repMetric & 0x1) == 1) {
			ifmm->appendBitField(8, BS_CINR);
		}

		if (((repMetric & 0x2) >> 1) == 1) {
			ifmm->appendBitField(8, BS_RSSI);
		}

		if (((repMetric & 0x4) >> 2) == 1) {
			ifmm->appendBitField(8, RelativeDelay);
		}
	}

	ifmm->appendBitField(8, N_BS_Full);

	for (int i = 0; i < N_BS_Full; i++) {
		ifmm->appendBitField(48, neighborBS_ID);

		if ((repMetric & 0x1) == 1) {
			ifmm->appendBitField(8, BS_CINR);
		}

		if (((repMetric & 0x2) >> 1) == 1) {
			ifmm->appendBitField(8, BS_RSSI);
		}

		if (((repMetric & 0x4) >> 2) == 1) {
			ifmm->appendBitField(8, RelativeDelay);
		}
	}

	/*
	* TLV encoded information
	*/

	PushPDUintoConnection(PriCID, ifmm);

	return 1;
}

/*
* If RS wants to initial handover procedure, RS shall send MOB_MSHO-REQ
*/
int mac802_16j_NT_PMPRS::SendMOB_MSHOREQ() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendMOB_MSHOREQ State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int ifmmLen = 0;
	uint8_t repMetric = 0x01; // CINR (SNR)
	uint8_t N_BS_Index = 0;
	uint8_t N_BS_Full = 0;
	uint8_t N_curr_BSs = 1; // soft handover is not supported
	uint8_t ArrTimeInd = 0;
	uint8_t padding = 0;
	uint64_t tick = 0;
	int tmpBits = 0;
	uint8_t tempBSID = 0; // soft handover is not supported
	uint8_t BS_CINR = 0;
	uint8_t BS_RSSI = 0;
	uint8_t RelativeDelay = 0;
	uint8_t BS_RTD = 0;
	vector<NbrMRBS_NT *>::iterator iter;
	uint8_t ServiceLevelPredict = 3; // No service level prediction available
	uint8_t ArrivalTimeDiff = 0;

	//printf("\n\e[1;36mTime:%llu RS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	/*
	* Compute handover arget BSs
	*/
	for (iter = NeighborMRBSList->nbrBSs_Index.begin(); iter
			!= NeighborMRBSList->nbrBSs_Index.end(); iter++) {
		if ((*iter)->targetHO == true)
			N_BS_Index++;
	}

	for (iter = NeighborMRBSList->nbrBSs_Full.begin(); iter
			!= NeighborMRBSList->nbrBSs_Full.end(); iter++) {
		if ((*iter)->targetHO == true)
			N_BS_Full++;
	}

	/*
	* Compute all field bits
	*/
	tmpBits += 16;

	if (N_BS_Index != 0)
		tmpBits += 8;

	for (int i = 0; i < N_BS_Index; i++) {
		tmpBits += 16;

		if ((repMetric & 0x01) == 1)
			tmpBits += 8;

		if (((repMetric & 0x02) >> 1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x04) >> 2) == 1)
			tmpBits += 8;

		tmpBits += 4;

		if (ArrTimeInd == 1)
			tmpBits += 4;
	}

	tmpBits += 8;
	for (int i = 0; i < N_BS_Full; i++) {
		tmpBits += 56;

		if ((repMetric & 0x01) == 1)
			tmpBits += 8;

		if (((repMetric & 0x02) >> 1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x04) >> 2) == 1)
			tmpBits += 8;

		tmpBits += 4;

		if (ArrTimeInd == 1)
			tmpBits += 4;
	}

	tmpBits += 4;
	for (int i = 0; i < N_curr_BSs; i++) {
		tmpBits += 4;

		if ((repMetric & 0x01) == 1)
			tmpBits += 8;

		if (((repMetric & 0x02) >> 1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x04) >> 2) == 1)
			tmpBits += 8;

		if (((repMetric & 0x08) >> 3) == 1)
			tmpBits += 8;
	}

	if (tmpBits % 8 != 0)
		ifmmLen = tmpBits / 8 + 1;
	else
		ifmmLen = tmpBits / 8;

	/*
	* Generate Menegement Message
	*/
	ifmm = new ifmgmt(MG_MOB_MSHOREQ, ifmmLen);

	ifmm->appendBitField(8, repMetric); // Report Metric
	ifmm->appendBitField(8, N_BS_Index); // N_New_BS_Index

	if (N_BS_Index != 0) {
		ifmm->appendBitField(8, NeighborMRBSList->NBRADV_CfgCount); // Configuration change count for MOB_NBR-ADV
	}

	for (int i = 0; i < N_BS_Index; i++) {
		ifmm->appendBitField(8, i);
		ifmm->appendBitField(8,
				NeighborMRBSList->nbrBSs_Index[i]->PreambleIndex);

		if ((repMetric & 0x01) == 1) {
			ifmm->appendBitField(8, NeighborMRBSList->nbrBSs_Index[i]->CINR); // contain
		}

		if (((repMetric & 0x02) >> 1) == 1) {
			ifmm->appendBitField(8, NeighborMRBSList->nbrBSs_Index[i]->RSSI); // not implement
		}

		if (((repMetric & 0x04) >> 2) == 1) {
			ifmm->appendBitField(8,
					NeighborMRBSList->nbrBSs_Index[i]->RelativeDelay); // not implement
		}

		ifmm->appendBitField(3, ServiceLevelPredict);
		ifmm->appendBitField(1, ArrTimeInd);

		if (ArrTimeInd == 1) {
			ifmm->appendBitField(4, ArrivalTimeDiff);
		}
	}

	ifmm->appendBitField(8, N_BS_Full);

	for (int i = 0; i < N_BS_Full; i++) {
		ifmm->appendBitField(48, NeighborMRBSList->nbrBSs_Full[i]->addr->buf());
		ifmm->appendBitField(8, NeighborMRBSList->nbrBSs_Full[i]->PreambleIndex);

		if ((repMetric & 0x01) == 1) {
			ifmm->appendBitField(8, NeighborMRBSList->nbrBSs_Full[i]->CINR);
		}

		if (((repMetric & 0x02) >> 1) == 1) {
			ifmm->appendBitField(8, NeighborMRBSList->nbrBSs_Full[i]->RSSI);
		}

		if (((repMetric & 0x04) >> 2) == 1) {
			ifmm->appendBitField(8,
					NeighborMRBSList->nbrBSs_Full[i]->RelativeDelay);
		}

		ifmm->appendBitField(3, ServiceLevelPredict);
		ifmm->appendBitField(1, ArrTimeInd);

		if (ArrTimeInd == 1) {
			ifmm->appendBitField(4, ArrivalTimeDiff);
		}
	}

	ifmm->appendBitField(3, N_curr_BSs);
	ifmm->appendBitField(1, padding);

	// FIXME
	for (int i = 0; i < N_curr_BSs; i++) {
		ifmm->appendBitField(4, tempBSID);

		if ((repMetric & 0x01) == 1) {
			ifmm->appendBitField(8, BS_CINR);
		}

		if (((repMetric & 0x02) >> 1) == 1) {
			ifmm->appendBitField(8, BS_RSSI);
		}

		if (((repMetric & 0x04) >> 2) == 1) {
			ifmm->appendBitField(8, RelativeDelay);
		}

		if (((repMetric & 0x08) >> 3) == 1) {
			ifmm->appendBitField(8, BS_RTD);
		}
	}

	if (tmpBits % 8 != 0) {
		ifmm->appendBitField(8 - tmpBits % 8, padding);
	}

	/*
	* TLV encoding information
	*/

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = BasicCID;

	PushPDUintoConnection(BasicCID, ifmm);

	// Start MS_HO_retransmission_timer
	MILLI_TO_TICK(tick, HORetransmitInterval * frameDuration_NT[frameDuCode()]);
	timerHORetransmit->start(tick, 0);

	// Set HO-REQ retries available
	HOReqRetryAvailable = 3; // FIXME

	return 1;
}

int mac802_16j_NT_PMPRS::SendMOB_HOIND(uint8_t HO_operation_mode) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendMOB_HOIND State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int ifmmLen = 0;
	int tmpBits = 0;
	uint8_t rsv = 0;
	uint8_t Mode = 0;
	uint8_t padding = 0;
	NbrMRBS_NT *targetBS = NeighborMRBSList->getTargetBS();
	uint8_t HO_IND_type = 0;
	uint8_t Ranging_Params_valid = 0;
	uint8_t PreambleIndex = 0;

	//printf("\n\e[1;36mTime:%llu RS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	if (HO_operation_mode == 1) // Mandatory HO response
	{
		HO_IND_type = 0; // ==> Re-enter network at target BS
		//printf("\e[1;35m## RS(%d) starts re-enter network at target BS ... ##\e[0m\n", get_nid());
	} else // Recommended HO response
	{
		if (0) // HO cancel yes
		{
			HO_IND_type = 1; // ==> Done
			//printf("\e[1;35m## RS(%d) cancelled the handover response ... ##\e[0m\n", get_nid());
		} else // HO cancel no
		{
			HO_IND_type = 2; // ==> RS HO holding down
			resetTimerT(42u);
			//printf("\e[1;35m## RS(%d) perform handover holding down ... ##\e[0m\n", get_nid());
		}
	}

	/*
	* Compute all field bits
	*/
	tmpBits += 8;

	if (Mode == 0x0) {
		tmpBits += 8;

		if (HO_IND_type == 0)
			tmpBits += 48;
	}

	if (Mode == 0x1) {
		;
	}

	if (Mode == 0x2) {
		;
	}

	tmpBits += 8;

	if (tmpBits % 8 != 0)
		ifmmLen = tmpBits / 8 + 1;
	else
		ifmmLen = tmpBits / 8;

	/*
	* Generate MOB_HO-IND Message
	*/
	ifmm = new ifmgmt(MG_MOB_HOIND, ifmmLen);
	ifmm->appendBitField(6, rsv);
	ifmm->appendBitField(2, Mode);

	if (Mode == 0x0) // HO
	{
		ifmm->appendBitField(2, HO_IND_type);
		ifmm->appendBitField(2, Ranging_Params_valid);
		ifmm->appendBitField(4, rsv);

		if (HO_IND_type == 0) {
			ifmm->appendBitField(48, targetBS->addr->buf());
		}
	}

	if (Mode == 0x1) // MDHO/FBSS: Anchor BS update
	{
		;
	}

	if (Mode == 0x2) // MDHO/FBSS: Diversity Set update
	{
		;
	}

	if (targetBS != NULL)
		PreambleIndex = targetBS->PreambleIndex;

	ifmm->appendBitField(8, PreambleIndex);

	if (tmpBits % 8 != 0) {
		ifmm->appendBitField(8 - tmpBits % 8, padding);
	}

	/*
	* TLV encoding Information
	*/
	uint8_t hmac[21] = "";
	ifmm->appendTLV(149, 21, hmac); // HMAC/CMAC Tuple

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = BasicCID;

	PushPDUintoConnection(BasicCID, ifmm);

	/*
	* start HO timer
	*/
	if (HO_IND_type == 0) {
		uint64_t tick = 0;

		MICRO_TO_TICK(tick, TTG() * PS() + ULsymbols * Ts() + RTG() * PS());

		timerStartHO->start(tick, 0);
	}

	return 1;
}

void mac802_16j_NT_PMPRS::procRNGREQ(struct mgmt_msg *recvmm, int cid, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procRNGREQ State: %x",
			get_nid(), State);

	//printf("\nTime:%llu RS(%d)::%s() len=%d\n", GetCurrentTime(), get_nid(), __func__,len);
	ifmgmt *ifmm_req = NULL;
	ifmgmt *ifmm_relay = NULL;
	//int i                   = 0;
	int type = 0;
	int fec = 0;
	uint8_t fprofile = 1;
	uint8_t fmac[6] = "";
	uint8_t rsv = 0;
	uint8_t fanomalies = 0;
	//int nPSs                = 0;
	//uint64_t ticks          = 0;
	//double diff             = 0.0;
	//int timingAdjust        = 0;
	vector<int> pduInfo;
	//ManagementConnection *new_initRangingConn = new ManagementConnection(initRangingCID);
	mac802_16j_NT_PMPMS *pMS = NULL;

	ifmm_req = new ifmgmt((uint8_t *) recvmm, len, 1);
	ifmm_req->extractBitField(8, &rsv);

	ifmm_relay = new ifmgmt(MG_RNGREQ, 1);
	ifmm_relay->appendBitField(8, rsv);

	while ((type = ifmm_req->getNextType()) != 0) {
		switch (type) {
		case 1:
			ifmm_req->getNextValue(&fprofile);
			ifmm_relay->appendTLV(1, 1, fprofile);
			break;

		case 2:
			ifmm_req->getNextValue(fmac);
			ifmm_relay->appendTLV(2, 6, fmac);
			break;

		case 3:
			ifmm_req->getNextValue(&fanomalies);
			ifmm_relay->appendTLV(3, 1, fanomalies);
			break;

		default:
			break;
		}
	}
	ifmm_relay->appendTLV(99, 2, BasicCID); // record relay RS BasicCID
	ifmm_relay->appendTLV(100, 1, fprofile); // record MS's diuc
	ifmm_relay->appendTLV(101, 1, LastSignalInfo.nid); // record MS's nid
	ifmm_relay->appendTLV(102, 1, LastSignalInfo.pid); // record MS's pid
	ifmm_relay->appendTLV(103, 1, LastSignalInfo.ulmap_ie.ie_14.duration); // record MS's duration
	//printf("RS receive SNR = %lf db\n",LastSignalInfo.SNR);
	delete ifmm_req;

	pMS = getMS(LastSignalInfo.nid, LastSignalInfo.pid);
	pMS->SetRDIUC(fprofile);
	pMS->SetRUIUC(1); // Default: QPSK_1_2
	for (int i = 10; i >= 1; i--) {
		fec = UCDProfile[i].fec;
		if (UCDProfile[i].used && (LastSignalInfo.SNR >= fecSNR[fec])) {
			pMS->SetRUIUC(i);

			break;
		}
	}

	// Create mslist to record which MS is relay by RS
	msInfo new_ms;
	new_ms.nid = LastSignalInfo.nid;
	new_ms.pid = LastSignalInfo.pid;
	new_ms.diuc = fprofile;
	new_ms.uiuc = pMS->getRUIUC();
	mslist.push_back(new_ms);
	PushPDUintoConnection(BasicCID, ifmm_relay);
	//new_initRangingConn->Insert(ifmm_relay);
	//initRanging_relay_queue.push_back(new_initRangingConn);
}

void mac802_16j_NT_PMPRS::procRS_AccessMAP(struct mgmt_msg *recvmm, int cid,
		int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procRS_AccessMAP State: %x",
			get_nid(), State);

	//printf("Time:%llu RS(%d) receive RS_AccessMAP len=%d\n",GetCurrentTime(),get_nid(),len);
	ifmgmt *ifmm = NULL;
	OFDMA_ULMAP_IE *ulmap_ies = NULL;
	uint8_t *tmp_ies = NULL;
	int indicator = 0;
	uint8_t indicator_lsb = 0;
	uint8_t indicator_msb = 0;
	uint8_t frame_Number = 0;
	int8_t num_bw_ie = 0;
	int nBytes = 0;
	//upBurst *uburst = NULL;
	//vector <ULMAP_IE_u> recvAL;
	int ALie_count = 0;
	relay_AL.clear();

	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);
	ifmm->extractBitField(8, (uint8_t *) &indicator_lsb);
	ifmm->extractBitField(4, (uint8_t *) &indicator_msb);
	ifmm->extractBitField(4, (uint8_t *) &frame_Number);
	nBytes += 3; // include type
	indicator = indicator_lsb + (indicator_msb << 8);

	if (indicator & 0x40) {// bit 6 = 1 ==> RS_BW_alloc_IE

		uint8_t rs_bw_alloc_ie_type = 0;
		uint8_t tid = 0;
		uint8_t dlmap_idx = 0;
		uint8_t zero;

		ifmm->extractBitField(8, (uint8_t *) &num_bw_ie);

		for (int i = 0; i < num_bw_ie; i++) {
			ifmm->extractBitField(2, (uint8_t *) &rs_bw_alloc_ie_type);
			if (rs_bw_alloc_ie_type == 0) {// response for RS_BR_header
				rs_br_hdr_rsp = true;
				//printf("Time:%llu receive RS_BR_header response\n",GetCurrentTime());
				timer_mngr()->cancel_t(62u);
			}
			ifmm->extractBitField(4, (uint8_t *) &tid);
			ifmm->extractBitField(2, (uint8_t *) &zero);
			ifmm->extractBitField(8, (uint8_t *) &dlmap_idx);
		}
		nBytes += (1 + num_bw_ie * 2);
	}

	if (indicator & 0x08) { // bit 3 = 1 ==> include UL_MAP_ie (CDMA_alloc_IE)
		mr_coderep_rsp = true;
		uint16_t ie_cid = 0;
		uint8_t ie_uiuc = 0;
		uint8_t num_ulmap_ie;
		uint8_t UCD_Count;
		ifmm->extractBitField(8, (uint8_t *) &UCD_Count);
		ifmm->extractBitField(8, (uint8_t *) &num_ulmap_ie);
		nBytes += 2;
		uint8_t tmp_ies_size = len - nBytes;

		tmp_ies = new uint8_t[tmp_ies_size];
		memset(tmp_ies, 0, tmp_ies_size);
		ifmm->extractBitField(tmp_ies_size * 8, tmp_ies); // IEs

		ulmap_ies = new OFDMA_ULMAP_IE(tmp_ies, tmp_ies_size);
		int leftBits = tmp_ies_size * 8;

		while (1) {
			if (leftBits == 0 || leftBits == 4) // 0:end or 4:padding
			{
				break;
			}
			ie_cid = 0;
			ie_uiuc = 0;

			ulmap_ies->extractField(16, (uint8_t *) &ie_cid);
			ulmap_ies->extractField(4, (uint8_t *) &ie_uiuc);
			switch (ie_uiuc) {
			case FAST_FEEDBACK: // not implement
				break;

			case Extended_UIUC_2: // not implement
				break;

			case CDMA_BWreq_Ranging: // contention subchannel
				break;

			case PAPR_Safety_zone: // not implement
				break;

			case CDMA_Alloc_IE:
				ULMAP_IE_u ALie;
				memset(&ALie, 0, sizeof(struct OFDMA_DLMAP_IE_14));
				ALie.ie_14.cid = ie_cid;
				ALie.ie_14.uiuc = ie_uiuc;
				ulmap_ies->extractField(6, &ALie.ie_14.duration);
				ulmap_ies->extractField(4, &ALie.ie_14.uiuc_trans);
				ulmap_ies->extractField(2, &ALie.ie_14.repeCode);
				ulmap_ies->extractField(4, &ALie.ie_14.frameIndex);
				ulmap_ies->extractField(8, &ALie.ie_14.rangCode);
				ulmap_ies->extractField(8, &ALie.ie_14.rangSym);
				ulmap_ies->extractField(7, &ALie.ie_14.rangCh);
				ulmap_ies->extractField(1, &ALie.ie_14.bwReq);

				/*   // the following fields should be zero
				printf("RS(%d) receive frameIndex = %d\n", get_nid(), ALie.ie_14.frameIndex);
				printf("RS(%d) receive rangCode = %d\n", get_nid(), ALie.ie_14.rangCode);
				printf("RS(%d) receive rangSym = %d\n", get_nid(), ALie.ie_14.rangSym);
				printf("RS(%d) receive rangCh = %d\n", get_nid(), ALie.ie_14.rangCh);
				*/
				leftBits -= 60;
				ALie_count++;
				relay_AL.push_back(ALie);
				//printf("\e[1;35mTime:%llu RS(%d)::%s(%d) receive CDMA_Allocation_IE!!!\e[0m\n",GetCurrentTime(),get_nid(),__func__,__LINE__);
				break;

			case Extended_UIUC:
				break;

			default: // ie_other
				ULMAP_IE_u tmpULie;
				memset(&tmpULie, 0, sizeof(ULMAP_IE_u));

				tmpULie.ie_other.cid = ie_cid;
				tmpULie.ie_other.uiuc = ie_uiuc;

				ulmap_ies->extractField(10,
						(uint8_t *) &tmpULie.ie_other.duration);
				ulmap_ies->extractField(2, &tmpULie.ie_other.repeCode);

				leftBits -= 32;
				//printf("%s cid=%d\n",__func__,ie_cid);
				relay_ULie_others.push_back(tmpULie);
				break;
			}
		}
	}

	if (rs_br_hdr_rsp == true) {
		// RS can send RNG-RSP to MS
		//handleRangingCode();
		//ClearRNGCodeList(&RangingCodeList);
		//rs_br_hdr_rsp = false;
	}

	delete ulmap_ies;
	delete[] tmp_ies;
	delete ifmm;
}

int mac802_16j_NT_PMPRS::procRS_ConfigCMD(struct mgmt_msg *recvmm, int cid,
		int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::procRS_ConfigCMD State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int type = 0;
	uint8_t fhmac[21] = "";
	uint16_t ftransid = 0;
	uint8_t fframe_no = 0;
	uint16_t frs_mode = 0; //Bit#0 RS scheduling mode Bit#0 =0 ( Centralized scheduling mode)
	uint8_t feirp = 0;

	//printf("\nTime:%llu:mac802_16j_PMPRS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 3);
	ifmm->extractBitField(16, (uint8_t *) &ftransid);
	ifmm->extractBitField(8, (uint8_t *) &fframe_no);
	//  ifmm->extractBitField(48,(uint8_t *)ServingBSID);    // Base Station ID

	while ((type = ifmm->getNextType()) != 0) {
		switch (type) {
		case 1:
			ifmm->getNextValue(&frs_mode);
			//printf("\nrs operation mode = %d\n",frs_mode);
			break;

		case 5:
			ifmm->getNextValue(&feirp);
			//printf("\nRS EIRP = %d\n",feirp);
			break;

		case 149:
			ifmm->getNextValue(fhmac);
			//printf("HMAC = ...\n");
			break;

		}
	}
	delete ifmm;
	return 1;

}

int mac802_16j_NT_PMPRS::SendMR_GenericACK() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendMR_GenericACK State: %x",
			get_nid(), State);

	//printf("\nTime:%llu:rsObject::%s()\n", GetCurrentTime(), __func__);
	ifmgmt *ifmm = NULL;

	ifmm = new ifmgmt(MG_MR_GenericACK, 2);
	ifmm->appendField(2, 1234); // Transaction ID

	PushPDUintoConnection(BasicCID, ifmm);
	return 1;
}

void mac802_16j_NT_PMPRS::SendRS_BR_header() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::SendRS_BR_header State: %x",
			get_nid(), State);

	struct RS_BR_header *hdr = NULL;
	BR_HDR *br = NULL;

	hdr = new (struct RS_BR_header);
	hdr->ec = 1;
	hdr->type = 1;
	hdr->type_ext = RS_BR_hdr;
	hdr->tid_msb = (get_nid()) >> 2;
	hdr->tid_lsb = (get_nid()) % 4;
	hdr->diuc = myDIUC;
	hdr->br_msb = 0;
	hdr->br_lsb = 0;
	hdr->cid_msb = BasicCID >> 8;
	hdr->cid_lsb = BasicCID % 256;
	hdr->hcs = 0;

	br = new BR_HDR;
	br->type = type2_1;
	br->br_hdr.br_type2_1.type_ext = RS_BR_hdr;
	br->br_hdr.br_type2_1.hdr = (const void *) hdr;

	brConnection->Insert(br);

	//printf("Time:%llu insert RS_BR_header(%d bytes) BR_HDR(%d bytes)\n",GetCurrentTime(),sizeof(RS_BR_header),sizeof(BR_HDR));

	timer_mngr()->reset_t(62u);
}

void mac802_16j_NT_PMPRS::saveDLMAP(char *dstbuf, int len) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::saveDLMAP State: %x",
			get_nid(), State);

	((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->saveDLMAP(dstbuf, len);
}

void mac802_16j_NT_PMPRS::saveULsymbols(int Nsymbols) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::saveULsymbols State: %x",
			get_nid(), State);

	((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->setULsymbols(Nsymbols);
}

void mac802_16j_NT_PMPRS::saveDLAccessSymbols(int Nsymbols) {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::saveDLAccessSymbols State: %x",
			get_nid(), State);

	((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->setDLAccessSymbols(Nsymbols);
}

void mac802_16j_NT_PMPRS::saveULAccessSymbols(int Nsymbols) {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::saveULAccessSymbols State: %x",
			get_nid(), State);

	((OFDMA_PMPRS_NT *) sendtarget_->bindModule())->setULAccessSymbols(Nsymbols);
}

void mac802_16j_NT_PMPRS::handleRangingCode() {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::handleRangingCode State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	uint32_t rangAttribute = 0;
	int timingAdjust = 0;
	int nPSs = 0;
	uint64_t ticks = 0;
	int ticks_diff = 0;
	double diff = 0.0;
	uint8_t rsv = 0;
	list<RangingObject *>::iterator its1, its2;
	uint64_t frame_delay_ticks = 0;
	uint8_t num_init_rng = 0;
	uint8_t num_ho_rng = 0;
	uint8_t num_br_rng = 0;

	//pScheduler->saved_ir_code.clear();
	MILLI_TO_TICK(frame_delay_ticks, frameDuration_NT[*_frameDuCode]); // delay one frame : 5ms
	//printf("one frame delay = %llu ticks\n",delay);

	if (RangingCodeList.empty())
		return;

	//printf("Time:%llu\tmac802_16j_NT_PMPRS::%s RangingCodeList.size=%d\n",GetCurrentTime(),__func__,RangingCodeList.size());

	for (its1 = RangingCodeList.begin(); its1 != RangingCodeList.end(); its1++) {
		for (its2 = RangingCodeList.begin(); its2 != RangingCodeList.end(); its2++) {
			if (its1 == its2)
				continue;

			if (((*its1)->rangingCodeIndex == (*its2)->rangingCodeIndex)
					&& ((*its1)->rangingSymbol == (*its2)->rangingSymbol)
					&& ((*its1)->rangingSubchannel
							== (*its2)->rangingSubchannel)
					&& ((*its1)->rangingFrameNumber
							== (*its2)->rangingFrameNumber)) {
				(*its1)->collision = true;
				(*its2)->collision = true;
			}

		}
	}

	for (its1 = RangingCodeList.begin(); its1 != RangingCodeList.end(); its1++) {
		if ((*its1)->collision == true)
			continue;

		//if ((*its1)->allocated == true)
		//    continue;

		if ((*its1)->rangingUsage == INITIAL_RANGING || (*its1)->rangingUsage
				== HANDOVER_RANGING) {
			ifmm = new ifmgmt(MG_RNGRSP, 1);
			ifmm->appendField(1, rsv);

			rangAttribute = ((*its1)->rangingSymbol & 0x3FF) << 22
					| ((*its1)->rangingSubchannel & 0x3F) << 16
					| ((*its1)->rangingCodeIndex & 0xFF) << 8
					| ((*its1)->rangingFrameNumber & 0xFF);

			// Compute time adjustment
			nPSs = pScheduler->SearchULTime((*its1)->rangingSymbol)
					+ symbolsToPSs(2); // over two symbolsToPSs
			MICRO_TO_TICK(ticks, PSsToMicro(nPSs));

			//printf("\n------ Time:%llu RS[%d]::%s() -----\n",GetCurrentTime(),get_nid(),__func__);
			//printf("rangingFrameNumber = %d\n",(*its1)->rangingFrameNumber & 0xFF);
			//printf("nPSs=%d ticks=%llu\n",nPSs,ticks);


			ticks += frameStartTime;
			if ((*its1)->recv_time < frameStartTime) {
				ticks_diff = (int) (ticks - (*its1)->recv_time
						- frame_delay_ticks);
			} else
				ticks_diff = (int) (ticks - (*its1)->recv_time);

			//printf("ticks_diff = %d\n",ticks_diff);
			//printf("frameStartTime:%llu , recv_time:%llu\n",frameStartTime,(*its1)->recv_time);

			TICK_TO_MICRO(diff, ticks_diff);
			timingAdjust = (int) (diff * 1024.0 / Tb());

			if (timingAdjust > 10 || timingAdjust < -10) {
				printf(
						"\e[1;36mTime:%llu RS(%d)::%s timingAdjust = %d (Initial)(continue)\e[0m\n",
						GetCurrentTime(), get_nid(), __func__, timingAdjust);
				ifmm->appendTLV(1, 4, timingAdjust);
				ifmm->appendTLV(4, 1, 1); // Ranging Status: 1=continue
			} else {
				//printf("\e[1;32mTime:%llu RS(%d)::%s timingAdjust = %d (Initial)(success)\e[0m\n", GetCurrentTime(), get_nid(),__func__, timingAdjust);
				ifmm->appendTLV(4, 1, 3); // Ranging Status: 3=success
				(*its1)->allocated = true; // BS shall provide BW allocation using CDMA_Allocation_IE
				if ((*its1)->rangingUsage == INITIAL_RANGING) {
					num_init_rng++;
					rngcode ir;
					ir.frameIndex = (*its1)->rangingFrameNumber;
					ir.rangCode = (*its1)->rangingCodeIndex;
					ir.rangSym = (*its1)->rangingSymbol;
					ir.rangCh = (*its1)->rangingSubchannel;
					//printf("\nir.rangCode = %d\n",ir.rangCode);
					pScheduler->saved_ir_code.push_back(ir);
					//printf("Time:%llu pScheduler->saved_ir_code.size() = %d\n",GetCurrentTime(),pScheduler->saved_ir_code.size());
				} else {
					num_ho_rng++;
				}
			}
			ifmm->appendTLV(150, 4, rangAttribute); // Ranging Code Attributes
			initRangingConnection->Insert(ifmm); // using Initial Ranging Connection
			//printf("Time:%llu %s(%d) send RNGRSP\n",GetCurrentTime(),__func__,__LINE__);
		} else if ((*its1)->rangingUsage == PERIOD_RANGING) {
			ifmm = new ifmgmt(MG_RNGRSP, 1);
			ifmm->appendField(1, rsv);

			rangAttribute = ((*its1)->rangingSymbol & 0x3FF) << 22
					| ((*its1)->rangingSubchannel & 0x3F) << 16
					| ((*its1)->rangingCodeIndex & 0xFF) << 8
					| ((*its1)->rangingFrameNumber & 0xFF);

			// Compute time adjustment
			nPSs = pScheduler->SearchULTime((*its1)->rangingSymbol)
					+ symbolsToPSs(1); // over one symbolsToPSs
			MICRO_TO_TICK(ticks, PSsToMicro(nPSs));

			if ((*its1)->recv_time < frameStartTime)
				(*its1)->recv_time += frame_delay_ticks;

			ticks += frameStartTime;
			TICK_TO_MICRO(diff, (int) (ticks - (*its1)->recv_time));
			timingAdjust = (int) (diff * 1024.0 / Tb());

			if (timingAdjust > 10 || timingAdjust < -10) {
				printf(
						"\e[1;36mTime:%llu RS(%d)::%s timingAdjust = %d (Period)\e[0m\n",
						GetCurrentTime(), get_nid(), __func__, timingAdjust);
				ifmm->appendTLV(1, 4, timingAdjust);
				ifmm->appendTLV(4, 1, 1); // Ranging Status: 1=continue
			} else {
				ifmm->appendTLV(4, 1, 3); // Ranging Status: 3=success
				(*its1)->allocated = true; // BS shall provide BW allocation using CDMA_Allocation_IE
			}
			ifmm->appendTLV(150, 4, rangAttribute); // Ranging Code Attributes

			initRangingConnection->Insert(ifmm); // using Initial Ranging Connection
			//new_initRangingConn->Insert(ifmm);
		} else if ((*its1)->rangingUsage == BW_REQUEST) {
			;
		} else {
			printf("Impossible Ranging Usage\n");
		}
	}

	//if((num_init_rng || num_ho_rng || num_br_rng) && (mr_coderep_rsp == false))
	if (num_init_rng) {
		SendMR_CodeREP_header(num_init_rng, num_ho_rng, num_br_rng, BasicCID);
	}

}

void mac802_16j_NT_PMPRS::SendMR_CodeREP_header(uint8_t num_ir, uint8_t num_hr,
		uint8_t num_br, uint16_t cid) {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::SendMR_CodeREP_header State: %x",
			get_nid(), State);
	//printf("Time:%llu\tRS(%d)::%s (num_init_rng = %d)\n",GetCurrentTime(),get_nid(),__func__,num_ir);

	struct MR_CodeREP_header *hdr = NULL;
	BR_HDR *br = NULL;

	hdr = new (struct MR_CodeREP_header);
	hdr->ht = 1;
	hdr->ec = 1;
	hdr->type = 1;
	hdr->type_ext = MR_CodeREP_hdr;
	hdr->fnum_idx_msb = (frameNumber & 0x0F) << 2;
	hdr->fnum_idx_lsb = (frameNumber & 0x0F) % 4;
	hdr->num_ir = num_ir;
	hdr->num_hr_msb = num_hr << 2;
	hdr->num_hr_lsb = num_hr % 4;
	hdr->num_br = num_br;
	hdr->cid_msb = cid << 8;
	hdr->cid_lsb = cid % 256;
	hdr->hcs = 0;

	br = new BR_HDR;
	br->type = type2_1;
	br->br_hdr.br_type2_1.type_ext = MR_CodeREP_hdr;
	br->br_hdr.br_type2_1.hdr = (const void *) hdr;

	brConnection->Insert(br);
	//printf("Time:%llu insert MR_CodeREP_header(%d bytes) BR_HDR=%d bytes\n",GetCurrentTime(),sizeof(MR_CodeREP_header),sizeof(BR_HDR));
	alloc_count = 0;
}

int mac802_16j_NT_PMPRS::procRS_MemberListUpdate(struct mgmt_msg *recvmm,
		int cid, int len) {

	logRidvan(TRACE,
			"-->%d	mac802_16j_NT_PMPRS::procRS_MemberListUpdate State: %x",
			get_nid(), State);

	ifmgmt *ifmm = NULL;
	int type = 0;
	uint16_t ftransid = 0;
	uint8_t fframe_no = 0;
	uint8_t fconfig_para = 0;
	uint16_t fcid = 0;
	int i = 0;
	list<uint16_t> cidlist_tmp;
	list<uint16_t>::iterator itel;
	list<uint16_t>::iterator itec;
	vector<ManagementConnection *>::iterator item;
	vector<DataConnection *>::iterator ited;
	vector<ConnectionReceiver *>::iterator itecr;

	ifmm = new ifmgmt((uint8_t *) recvmm, len, 3);
	ifmm->extractBitField(4, (uint8_t *) &fframe_no);
	ifmm->extractBitField(4, (uint8_t *) &fconfig_para);
	ifmm->extractBitField(16, (uint8_t *) &ftransid);

	while ((type = ifmm->getNextType()) != 0) {
		switch (type) {
		case 45:
			ifmm->getNextValue(&fcid);

			i++;
			cidlist_tmp.push_back(fcid);
			break;
		}
	}

	/* Update Member cid List */
	for (itel = cidlist_tmp.begin(); itel != cidlist_tmp.end(); itel++) {
		if ((fconfig_para & 0x04) >> 2) /*Add cid into relay connection */
		{
			for (itec = mem_cidlist.begin(); itec != mem_cidlist.end(); itec++) {
				if ((*itec) == (*itel)) {
					printf("Time%llu %s::RS(%d):cid = %d existed.\n",
							GetCurrentTime(), __func__, get_nid(), *itel);
					exit(1);
				}
			}

			/* Create relay Management CID connection */
			if ((*itel) <= 510) {
				printf(
						"\e[1;34m***RS(%d):%s::Create DL/UL Management Connection(cid= %d) into RS Manalist***\e[0m\n",
						get_nid(), __func__, (*itel));
				//DLRelayMnConnections.push_back(new ManagementConnection((*itel)));
				//ULRelayMnConnections.push_back(new ManagementConnection((*itel)));
			} else {
				printf(
						"\e[1;34m***RS(%d):%s::Create DL/UL Data Connection(cid= %d) into RS Dtlist***\e[0m\n",
						get_nid(), __func__, (*itel));
				//DLRelayDtConnections.push_back(new DataConnection((*itel)));
				//ULRelayDtConnections.push_back(new DataConnection((*itel)));

				//DLRelayReceiverList.push_back(new ConnectionReceiver((*itel)));
				//ULRelayReceiverList.push_back(new ConnectionReceiver((*itel)));
			}
			mem_cidlist.push_back((*itel));
		} else /* Remove cid from relay connection */
		{
			for (itec = mem_cidlist.begin(); itec != mem_cidlist.end(); itec++) {

			}
		}
	}
	return 1;
}

mac802_16j_NT_PMPMS *mac802_16j_NT_PMPRS::getMS(uint8_t nid, uint8_t pid) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::getMS State: %x", get_nid(),
			State);

	mac802_16j_NT_PMPMS *pMS = NULL;
	pMS = (mac802_16j_NT_PMPMS *) InstanceLookup(nid, pid,
			"MAC802_16J_NT_PMPMS");
	if (!pMS) {
		printf(
				"\e[1;35mRS(%d)::%s:Get MS(nid=%d , pid=%d)) module failed!\e[0m\n",
				get_nid(), __func__, nid, pid);
		exit(1);
	} else {
		//    printf("***RS(%d)::%s:Get MS(nid=%d , pid= %d)) module success!***\n", get_nid(), __func__,nid,pid);
		return pMS;
	}
}

ManagementConnection *mac802_16j_NT_PMPRS::getULRelayMnConn(uint16_t cid) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::getULRelayMnConn State: %x",
			get_nid(), State);

	vector<ManagementConnection *>::iterator itm;
	for (itm = ULRelayMnConnections.begin(); itm != ULRelayMnConnections.end(); itm++) {
		if ((*itm)->cid == cid)
			return (*itm);
	}
	// not found
	return NULL;
}

ManagementConnection *mac802_16j_NT_PMPRS::getDLRelayMnConn(uint16_t cid) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::getDLRelayMnConn State: %x",
			get_nid(), State);

	vector<ManagementConnection *>::iterator itm;
	for (itm = DLRelayMnConnections.begin(); itm != DLRelayMnConnections.end(); itm++) {
		if ((*itm)->cid == cid)
			return (*itm);
	}
	// not found
	return NULL;
}

DataConnection *mac802_16j_NT_PMPRS::getULRelayDtConn(uint16_t cid) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::getULRelayDtConn State: %x",
			get_nid(), State);

	vector<DataConnection *>::iterator itd;
	for (itd = ULRelayDtConnections.begin(); itd != ULRelayDtConnections.end(); itd++) {
		if ((*itd)->cid == cid)
			return (*itd);
	}
	return NULL;
}

DataConnection *mac802_16j_NT_PMPRS::getDLRelayDtConn(uint16_t cid) {

	logRidvan(TRACE, "-->%d	mac802_16j_NT_PMPRS::getDLRelayDtConn State: %x",
			get_nid(), State);

	vector<DataConnection *>::iterator itd;
	for (itd = DLRelayDtConnections.begin(); itd != DLRelayDtConnections.end(); itd++) {
		if ((*itd)->cid == cid)
			return (*itd);
	}
	return NULL;
}

