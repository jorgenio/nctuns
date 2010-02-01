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

#include "mac802_16e_pmpms.h"

using namespace std;
using namespace mobileCommon;
using namespace mobileMacAddress;
using namespace mobileTimer;
using namespace mobileMSbase;
using namespace mobileOFDMAMapIE;

MODULE_GENERATOR(mac802_16e_PMPMS);

	mac802_16e_PMPMS::mac802_16e_PMPMS(uint32_t type, uint32_t id, plist* pl, const char* name)
: mac802_16MSbase(type, id, pl, name)
{
	vBind("LinkMode", &LinkMode);

	_maxqlen = 2000;
	LinkMode = NULL;

	// Spec 10.1 Table 342
	LostULinterval          = 600; // ms
	LostDLinterval          = 600; // ms
	PeriodRangingInterval   = 0;   // unit in frames
	HORetransmitInterval    = 0;   // unit in frames
	ScanReqInterval         = 2;   // s

	ServerLevelPredict  = 0;
	HOReqRetryAvailable = 0;
	RngLastStatus       = Init;
	RngCodeRegion       = INITIAL_RANGING;
	TimingAdjust        = 0;
	ifsavedmm           = NULL;

	pScheduler      = new MS_16e_Scheduler(this);
	NeighborBSList  = new NeighborBSs();
	ScanFlag        = false;

	assert(pScheduler && NeighborBSList);

	BASE_OBJTYPE(mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T1);
	timer_mngr()->set_func_t(1u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T2);
	timer_mngr()->set_func_t(2u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T3);
	timer_mngr()->set_func_t(3u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T6);
	timer_mngr()->set_func_t(6u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T8);
	timer_mngr()->set_func_t(8u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T12);
	timer_mngr()->set_func_t(12u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T18);
	timer_mngr()->set_func_t(18u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, T42);
	timer_mngr()->set_func_t(42u, this, mem_func);

	timerLostDL         = new timerObj;
	timerLostUL         = new timerObj;
	timerSendRngCode    = new timerObj;
	timerSendRngReq     = new timerObj;
	timerSendUL         = new timerObj;
	timerHORetransmit   = new timerObj;
	timerPeriodRanging  = new timerObj;
	timerSendScanReq    = new timerObj;
	timerStartScan      = new timerObj;
	timerStartHO        = new timerObj;

	assert(timerLostDL && timerLostUL && timerSendRngCode && timerSendRngReq &&
			timerSendUL && timerHORetransmit && timerPeriodRanging && timerSendScanReq &&
			timerStartScan && timerStartHO);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, LostDLMAP);
	timerLostDL->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, LostULMAP);
	timerLostUL->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, SendRNGCODE);
	timerSendRngCode->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, SendRNGREQ);
	timerSendRngReq->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, PacketScheduling);
	timerSendUL->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, HOmsgRetransmit);
	timerHORetransmit->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, StartPeriodRanging);
	timerPeriodRanging->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, SendMOB_SCNREQ);
	timerSendScanReq->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, StartScanNbrBSs);
	timerStartScan->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPMS, StartHandover);
	timerStartHO->setCallOutObj(this, mem_func);

	// Spec 16e. Table 338
	fecSNR[QPSK_1_2]    = 5;
	fecSNR[QPSK_3_4]    = 8;
	fecSNR[QAM16_1_2]   = 10.5;
	fecSNR[QAM16_3_4]   = 14;
	fecSNR[QAM64_1_2]   = 16;
	fecSNR[QAM64_2_3]   = 18;
	fecSNR[QAM64_3_4]   = 20;

	UCDCfgCount = 0;
	DCDCfgCount = 0;

	for (int i = 0; i < 16; i++)
	{
		DCDProfile[i].used = 0;
		UCDProfile[i].used = 0;
	}

	/* register variable */
	REG_VAR("MAC", address()->buf());
	REG_VAR("DCDProfile", &DCDProfile);
	REG_VAR("UCDProfile", &UCDProfile);
}

mac802_16e_PMPMS::~mac802_16e_PMPMS()
{
	while(!MnConnections.empty())
	{
		delete *(MnConnections.begin());
		MnConnections.erase(MnConnections.begin());
	}

	while(!DtConnections.empty())
	{
		delete *(DtConnections.begin());
		DtConnections.erase(DtConnections.begin());
	}

	while(!ReceiverList.empty())
	{
		delete *(ReceiverList.begin());
		ReceiverList.erase(ReceiverList.begin());
	}

	delete pScheduler;
	delete NeighborBSList;
	delete timerSendRngCode;
	delete timerSendRngReq;
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
}

int mac802_16e_PMPMS::init()
{
	uint64_t tick_interval = 0;

	State   = msInit;
	AttrARQ = 0;

	initRangingConnection = new ManagementConnection(initRangingCID);

	assert(initRangingConnection);

	MILLI_TO_TICK(tick_interval, 500); // 500 ms
	timer_mngr()->set_interval_t(18u, tick_interval);

	_frameDuCode = GET_REG_VAR(get_port(), "frameDuCode", int *);

	NeighborBSList->ServingBSchID = ((OFDMA_PMPMS *)sendtarget_->bindModule())->getChannelID();
	NeighborBSList->NBRADV_CfgCount = 0;

	return mac802_16e::init();
}

int mac802_16e_PMPMS::recv(ePacket_ *epkt)
{
	Packet *recvBurst       = NULL;
	char *adaptedBurst      = NULL;
	char *ptr               = NULL;
	struct mgmt_msg *recvmm = NULL;
	unsigned int crc        = 0;
	uint64_t tick           = 0;
	int cid                 = 0;
	int len                 = 0;
	int plen                = 0;
	int burstLen            = 0;
	struct hdr_generic *hg  = NULL;

	// Get received burst
	recvBurst = (Packet *) epkt->DataInfo_;
	assert(epkt && recvBurst);

	// Free event memory, but not include DataInfo_ (recvBurst)
	epkt->DataInfo_ = NULL;
	freePacket(epkt);

	// Save phyInfo
	memcpy(&LastSignalInfo, recvBurst->pkt_getinfo("phyInfo"), sizeof(LastSignalInfo));
	adaptedBurst    = (char *) recvBurst->pkt_sget();
	burstLen        = recvBurst->pkt_getlen();

	if (LastSignalInfo.SNR < 0)
		LastSignalInfo.SNR = 0;

	/*
	 * When MS is scanning, we measure the SNR value and ignore the received packet
	 */
	if (ScanFlag == true)
	{
		NbrBS *pNbrBS = NeighborBSList->getNbrbyChID(LastSignalInfo.ChannelID);

		if (pNbrBS == NULL)
		{
			fprintf(stderr, "MS(%d):%s() Warning: not existed channel ID(%d)\n", get_nid(), __func__, LastSignalInfo.ChannelID);
			exit(1);
		}

		pNbrBS->CINR = (int) (pNbrBS->CINR * 0.8 + LastSignalInfo.SNR * 0.2); // weighted SNR computation
		//printf("pNbrBS->CINR=%2d(Serving=%2f)  LastSignalInfo.ChannelID=%2d\n", pNbrBS->CINR, NeighborBSList->ServingCINR, LastSignalInfo.ChannelID);

		delete recvBurst;

		((OFDMA_PMPMS *)sendtarget_->bindModule())->skipBufferedPkt();

		return 1;
	}
	else
	{
		// save the SNR value of serving BS
		NeighborBSList->ServingCINR = NeighborBSList->ServingCINR * 0.8 + LastSignalInfo.SNR * 0.2; // weighted SNR computation
	}

	for (ptr = adaptedBurst;ptr + sizeof(struct hdr_generic) < adaptedBurst + burstLen;ptr += plen)
	{
		// Get generic mac header
		hg      = (struct hdr_generic *) ptr;
		cid     = GHDR_GET_CID(hg);
		plen    = GHDR_GET_LEN(hg);

		// None
		if (hcs_16e(ptr, sizeof(struct hdr_generic)) || plen == 0)
		{
			break;
		}

		// Save total_length
		len = plen;

		// CRC check
		if (hg->ci == 1)
		{
			len -= 4;
			crc = crc32_16e(0, ptr, len);
			if (memcmp(&crc, ptr + len, 4) != 0)
			{
				printf("MS(%d) CRC Error (%08x)\n", get_nid(), crc);
				continue;
			}
		}

		// Broadcast CID
		if (cid == broadcastCID)
		{
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			// DL-MAP
			if (recvmm->type == MG_DLMAP)
			{
				procDLMAP(recvmm, cid, len - sizeof(struct hdr_generic));
				MILLI_TO_TICK(tick, LostDLinterval);
				timerLostDL->start(tick, 0); // when recv DL-MAP, reset timerLostDL

				if ((State & msDLParam) == 0)
				{
					State |= msDLParam;
				}
			}

			// MS doesn't synchronized to downlink  ==>  continue.
			if ((State & msDLParam) == 0)
			{
				continue;
			}

			// DCD
			if (recvmm->type == MG_DCD)
			{
				procDCD(recvmm, cid, len - sizeof(struct hdr_generic));
				resetTimerT(1u);  // when recv DCD, reset T1
			}

			// UCD
			if (recvmm->type == MG_UCD)
			{
				procUCD(recvmm, cid, len - sizeof(struct hdr_generic));
				resetTimerT(12u);  // reset T12

				if ((State & msULParam) == 0)
				{
					State |= msULParam;

					resetTimerT(2u);  // reset T2
					MILLI_TO_TICK(tick, LostULinterval);
					timerLostUL->start(tick, 0);  // when recv UCD, start timerLostUL

					RngTime       = RngStart - 1;
					RngLastStatus = Rerange;    // Rerange
					WaitForRangingOpportunity();
				}
			}

			// UL-MAP
			if (recvmm->type == MG_ULMAP)
			{
				procULMAP(recvmm, cid, len - sizeof(struct hdr_generic));

				MILLI_TO_TICK(tick, LostULinterval);
				timerLostUL->start(tick, 0);  // when recv UL-MAP, reset timerLostUL
			}

			// MOB_NBR-ADV
			if (recvmm->type == MG_MOB_NBRADV)
			{
				procMOB_NBRADV(recvmm, cid, len - sizeof(struct hdr_generic));
			}
		}
		else if (cid == initRangingCID) // Initial Ranging CID
		{
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			// RNG-RSP
			if (recvmm->type == MG_RNGRSP)
			{
				if (procRNGRSP(recvmm, cid, len - sizeof(struct hdr_generic)) == 1) // success when RNG-RSP is for RNG-REQ
				{
					State |= msRanging;
					if ((State & msNegcap) == 0)
					{
						SendSBCREQ();
					}
				}
			}
		}
		else if (cid == BasicCID)    // MAC PDU for this MS
		{
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			// SBC-RSP
			if (recvmm->type == MG_SBCRSP)    // Fig.66
			{
				timer_mngr()->cancel_t(18u);

				delete ifsavedmm;
				ifsavedmm = NULL;

				if (procSBCRSP(recvmm, cid, len - sizeof(struct hdr_generic)))    // Response OK
				{
					State |= msNegcap;
					/*
					 * Here, we skip procedure MS authorization and key exchange (Spec 6.3.9.8).
					 */
					SendREGREQ();
				}
			}

			// MOB_SCN-RSP
			if (recvmm->type == MG_MOB_SCNRSP)
			{
				procMOB_SCNRSP(recvmm, cid, len - sizeof(struct hdr_generic));
			}

			// MOB_BSHO-RSP
			if (recvmm->type == MG_MOB_BSHORSP)
			{
				timer_mngr()->cancel_t(42u);
				timerHORetransmit->cancel();

				delete ifsavedmm;
				ifsavedmm = NULL;

				procMOB_BSHORSP(recvmm, cid, len - sizeof(struct hdr_generic));
			}

			// MOB_BSHO-REQ
			if (recvmm->type == MG_MOB_BSHOREQ)
			{
				; // not implement
			}
		}
		else if (cid == PriCID)    // MAC PDU for this MS
		{
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			// REG-RSP
			if (recvmm->type == MG_REGRSP)    // Fig.69
			{
				timer_mngr()->cancel_t(6u);

				delete ifsavedmm;
				ifsavedmm = NULL;

				if (procREGRSP(recvmm, cid, len - sizeof(struct hdr_generic)))    // Response OK
				{
					State |= msRegister;

					// Start Period Ranging
					MILLI_TO_TICK(tick, PeriodRangingInterval * frameDuration[frameDuCode()]);
					timerPeriodRanging->start(tick, 0);

					// Start Scanning Neighbor BSs Timer
					SEC_TO_TICK(tick, ScanReqInterval);
					timerSendScanReq->start(tick, tick); // FIXME

					if (0)    // is Managed MS?
					{
						// Establish Secondary Management Connection
						// Establish IP connectivity
					}
				}
			}

			// DSA-REQ
			if (recvmm->type == MG_DSAREQ && (State & msRegister))
			{
				procDSAREQ(recvmm, cid, len - sizeof(struct hdr_generic));
			}

			// DSA-ACK
			if (recvmm->type == MG_DSAACK)
			{
				timer_mngr()->cancel_t(8u);

				delete ifsavedmm;
				ifsavedmm = NULL;

				procDSAACK(recvmm, cid, len - sizeof(struct hdr_generic));
				if ((State & msProv) == 0)
				{
					State |= msProv;
				}
			}
		}
		else // Secondary CID or Transport CID
		{
			vector<ConnectionReceiver *>::iterator iter;
			char *ptr2 = NULL;

			for (iter = ReceiverList.begin();iter != ReceiverList.end(); iter++)
			{
				if ((*iter)->getCid() == cid)
				{
					(*iter)->insert(hg, len);
					while ((ptr2 = (*iter)->getPacket(len)) != NULL)
					{
						ePacket_ *deliver_epkt  = createEvent();
						Packet *pkt             = asPacket(ptr2, len);

						pkt->pkt_setflow(PF_RECV);
						deliver_epkt->DataInfo_ = pkt;
						put(deliver_epkt, recvtarget_);
					}
					break;
				}
			}
		}
	} // end for loop

	delete recvBurst;
	return 1;
}

int mac802_16e_PMPMS::send(ePacket_ *epkt)
{
	vector<DataConnection *>::iterator iter;
	Packet *pkt = NULL;
	bool deleteFlag = true;

	assert(epkt);
	pkt = (Packet *)epkt->DataInfo_;
	epkt->DataInfo_ = NULL;

	for (iter = DtConnections.begin(); iter != DtConnections.end(); iter++)
	{
		if ((*iter)->nf_pending_packet() <= static_cast<size_t>(_maxqlen))
		{
			(*iter)->Insert(pkt);
			deleteFlag = false;
		}
		break;
	}

	if (deleteFlag == true)
		delete pkt;

	freePacket(epkt);

	return 1;
}

Connection *mac802_16e_PMPMS::getConnection(uint16_t cid)
{
	vector<ManagementConnection *>::iterator iter1;
	vector<DataConnection *>::iterator iter2;

	for (iter1 = MnConnections.begin(); iter1 != MnConnections.end();iter1++)
	{
		Connection *conn = *iter1;
		if (conn->cid == cid)
			return conn;
	}
	for (iter2 = DtConnections.begin(); iter2 != DtConnections.end();iter2++)
	{
		Connection *conn = *iter2;
		if (conn->cid == cid)
			return conn;
	}
	return NULL;
}


void mac802_16e_PMPMS::PushPDUintoConnection(uint16_t cid, ifmgmt *pdu)
{
	vector<ManagementConnection *>::iterator iter;
	ManagementConnection *pConn = NULL;

	if (cid == initRangingCID)
	{
		initRangingConnection->Insert(pdu);
		return;
	}

	for (iter = MnConnections.begin(); iter != MnConnections.end(); iter++)
	{
		pConn = (ManagementConnection *) *iter;
		if (pConn->cid == cid)
		{
			pConn->Insert(pdu);
			return;
		}
	}
}

/*
 * for sending RNG-REQ at CDMA allocation interval
 */
uint64_t mac802_16e_PMPMS::computeSendTime(int numSlots, int duration, int fastime)
{
	int64_t tick        = 0;
	int64_t tadj        = 0;
	int64_t adjus       = 0;
	int nPSs            = 0;

	nPSs = fastime;
	MICRO_TO_TICK(tick, PSsToMicro(nPSs));

	adjus   = (int64_t) (TimingAdjust * Tb() / 1024.0);
	tadj    = (int64_t) (adjus * 1000.0 / TICK);    // TICK_TO_MICRO
	tick    -= (GetCurrentTime() - LastSignalInfo.frameStartTime);

	if (tick + tadj > 0)
	{
		return tick + tadj;
	}
	else
	{
		return 0;
	}
}

/*
 * for sending ranging code at contention slots
 */
uint64_t mac802_16e_PMPMS::computeSendTime(const struct OFDMA_ULMAP_IE_12 &ie_12, int txopOffset, int fastime)
{
	int64_t tick    = 0;
	int64_t tadj    = 0;
	int64_t adjus   = 0;
	int nPSs        = 0;

	nPSs    = fastime + symbolsToPSs(ie_12.symOff + txopOffset);
	MICRO_TO_TICK(tick, PSsToMicro(nPSs));

	adjus   = (int64_t) (TimingAdjust * Tb() / 1024.0); // us
	tadj    = (int64_t) (adjus * 1000.0 / TICK);    // TICK_TO_MICRO
	tick    -= (GetCurrentTime() - LastSignalInfo.frameStartTime); // absolute waiting time

	if (tick + tadj > 0)
	{
		return tick + tadj;
	}
	else
	{
		return 0;
	}
}

/*
 * Periodically transmit uplink burst.
 */
void mac802_16e_PMPMS::PacketScheduling()
{
	ePacket_ *epkt          = NULL;
	BurstInfo *burstInfo    = new BurstInfo;

	//printf("===== Time:%llu MS(%d)::%s() =====\n", GetCurrentTime(), get_nid(), __func__);
	burstInfo->Collection = pScheduler->Scheduling();
	burstInfo->flag = PF_SEND;

	if (burstInfo->Collection != NULL)
	{
		epkt = createEvent();
		epkt->DataInfo_ = burstInfo;
		put(epkt, sendtarget_);
	}
	else
	{
		delete burstInfo;
	}
}

/* 
 * Wait for DCD timeout 
 */
void mac802_16e_PMPMS::T1()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);
}

/*
 * Wait for Broadcast Ranging timeout
 */
void mac802_16e_PMPMS::T2()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);
}

/*
 * Ranging Response reception timeout following the
 * transmission of a Ranging Request
 */
void mac802_16e_PMPMS::T3()
{
	printf("Time:%llu MS(%d)::%s()\n", GetCurrentTime(), get_nid(), __func__);

	if (RngCodeRegion == INITIAL_RANGING || RngCodeRegion == HANDOVER_RANGING) // FIXME
	{
		RngLastStatus = Rerange;
	}
	else if (RngCodeRegion == PERIOD_RANGING)
	{
		RngLastStatus = Continue;
	}
	else
	{
		;
	}
	//TimingAdjust -= 11; // FIXME: if the distance between BS and MS, the ranging packet will be dropped.
	WaitForRangingOpportunity();
}

/* 
 * Wait for registration response
 */
void mac802_16e_PMPMS::T6()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm = new ifmgmt(saved_msg, ifsavedmm->getLen(), ifsavedmm->getFLen());

	resetTimerT(6u);
}

/* 
 * Wait for DSA/DSC Acknowledge timeout
 */
void mac802_16e_PMPMS::T8()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm = new ifmgmt(saved_msg, ifsavedmm->getLen(), ifsavedmm->getFLen());

	resetTimerT(8u);
}

/*
 * Wait for UCD timeout
 */
void mac802_16e_PMPMS::T12()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);
}

/* 
 * Wait for SBC-RSP timeout 
 */
void mac802_16e_PMPMS::T18()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm = new ifmgmt(saved_msg, ifsavedmm->getLen(), ifsavedmm->getFLen());

	resetTimerT(18u);
}

/*
 * MOB_HO-IND timeout when sent with HO_IND_type = 0b10
 */
void mac802_16e_PMPMS::T42()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s() ... HO holding down timeout... \n", GetCurrentTime(), get_nid(), __func__);

	delete ifsavedmm;
}

/*
 * Wait for DL-MAP timeout
 */
void mac802_16e_PMPMS::LostDLMAP()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);
}

/*
 * Wait for UL-MAP timeout
 */
void mac802_16e_PMPMS::LostULMAP()
{
	printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);
}

/*
 * Handover Retransmission Timeout
 */
void mac802_16e_PMPMS::HOmsgRetransmit()
{
	uint64_t tick = 0;

	printf("\n\e[1;31mTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	// Retries Available ?
	if (HOReqRetryAvailable > 0)
	{
		PushPDUintoConnection(lastCID, ifsavedmm);

		ifsavedmm = new ifmgmt(saved_msg, ifsavedmm->getLen(), ifsavedmm->getFLen());

		MILLI_TO_TICK(tick, HORetransmitInterval * frameDuration[frameDuCode()]);
		timerHORetransmit->start(tick, 0);

		// Decrement HO-REQ-Retries available
		HOReqRetryAvailable--;
	}
	else
	{
		printf("HO-RSP retries exhausted\n");
	}
}

/*
 * Start Waiting for Period Ranging Opportunity
 */
void mac802_16e_PMPMS::StartPeriodRanging()
{
	RngTime       = RngStart - 1;
	RngLastStatus = Continue; // continue
	RngCodeRegion = PERIOD_RANGING;
	WaitForRangingOpportunity();
}

/*
 * Start Scanning Status, change the channel ID and ignore the incoming packets
 */
void mac802_16e_PMPMS::StartScanNbrBSs()
{
	uint8_t chID = 0;
	uint64_t tick = 0;

	//printf("\n\e[1;31mTime:%llu:MS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	if (NeighborBSList->ScanTimes != NeighborBSList->ScanDuration) // continue scanning
	{
		chID = NeighborBSList->getNextScanChID();

		// Continue scanning
		if (((OFDMA_PMPMS *)sendtarget_->bindModule())->getChannelID() == chID)
		{
			fprintf(stderr, "Warning: scanning channel ID(%d) is the same\n", chID);
			exit(1);
		}

		//printf("Time:%llu\tMS(%d) set channel ID to (%d)\n", GetCurrentTime(), get_nid(), chID);

		((OFDMA_PMPMS *)sendtarget_->bindModule())->setChannelID(chID);
		NeighborBSList->ScanningChID = chID;

		MICRO_TO_TICK(tick, symbolsPerFrame() * Ts() + (TTG() + RTG()) * PS()); // one frame
		timerStartScan->start(tick, 0);

		NeighborBSList->ScanTimes++;
		ScanFlag = true;

		//timer_mngr()->cancel_t(3u);
	}
	else // back to serving channel
	{
		NeighborBSList->ScanTimes = 0;
		NeighborBSList->ScanIteration--;

		// End scanning, back to serving BS's Channel ID
		chID = NeighborBSList->ServingBSchID;
		((OFDMA_PMPMS *)sendtarget_->bindModule())->setChannelID(chID);
		//printf("Time:%llu\tMS(%d) set channel ID to (%d)\n", GetCurrentTime(), get_nid(), chID);

		ScanFlag = false;

		/*
		 * Check handover conditions
		 */
		if (NeighborBSList->checkHOterms() == true)
		{
			timerSendScanReq->cancel();
			SendMOB_MSHOREQ(); // Send MOB_MSHO-REQ
		}
		else
		{
			if (NeighborBSList->ScanIteration != 0) // FIXME
			{
				// back to normal operation and schedule the next scanning
				double frameTime = symbolsPerFrame() * Ts() + (TTG() + RTG() + TimingAdjust) * PS(); // us
				int frameNum = NeighborBSList->InterleavingInterval;

				MICRO_TO_TICK(tick, frameTime * frameNum);
				timerStartScan->start(tick, 0);
			}
		}
	}
}

/*
 * When MS performs handover, these registers need to clear
 */
void mac802_16e_PMPMS::StartHandover()
{
	printf("\e[1;35mTime:%llu MS(%d)::%s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	// Cancel timers
	timerSendRngCode->cancel();
	timerSendUL->cancel();
	timerPeriodRanging->cancel();
	timerSendScanReq->cancel();
	timerStartScan->cancel();

	// Change Channel ID
	((OFDMA_PMPMS *)sendtarget_->bindModule())->setChannelID(NeighborBSList->getTargetBS()->PreambleIndex);
	//printf("Time:%llu\tMS(%d) set channel ID to (%d)\n", GetCurrentTime(), get_nid(), NeighborBSList->getTargetBS()->PreambleIndex);

	// Clear Registers
	myDIUC        = 0;
	State         = msInit;
	RngLastStatus = Init;
	RngCodeRegion = HANDOVER_RANGING;
	TimingAdjust  = 0;
	ScanFlag      = false;
	BasicCID      = 0;
	PriCID        = 0;
	SecCID        = 0;

	while(!MnConnections.empty())
	{
		delete *(MnConnections.begin());
		MnConnections.erase(MnConnections.begin());
	}

	while(!DtConnections.empty())
	{
		delete *(DtConnections.begin());
		DtConnections.erase(DtConnections.begin());
	}

	while(!ReceiverList.empty())
	{
		delete *(ReceiverList.begin());
		ReceiverList.erase(ReceiverList.begin());
	}

	if (ifsavedmm != NULL)
	{
		delete ifsavedmm;
		ifsavedmm = NULL;
	}

	delete NeighborBSList;
	NeighborBSList = new NeighborBSs();
	NeighborBSList->NBRADV_CfgCount = 0;
	NeighborBSList->ServingBSchID = ((OFDMA_PMPMS *)sendtarget_->bindModule())->getChannelID();

	delete pScheduler;
	pScheduler = new MS_16e_Scheduler(this);

	printf("\n=======%d==IF=PROGRAM=RUN=TO=HERE=THAT'S=MS=START=NETWORK=RE-ENTRY=PROCEDURE===========\n\n", get_nid());
}

/*
 *  Wait for Initial Ranging opportunity (Fig.60)
 */
void mac802_16e_PMPMS::WaitForRangingOpportunity()
{
	if (++RngTime > RngEnd)
	{
		printf("Rng Exhausted\n");
		return;
	}
	RngCount = random() % (int) powl(2, RngTime);
	//printf("RngCount = %d, RngTime = %d\n", RngCount, RngTime);
}

/*
 *    Processing DCD
 */
int mac802_16e_PMPMS::procDCD(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm        = NULL;
	ifTLV *tmptlv       = NULL;
	uint8_t type        = 0;
	uint8_t diuc        = 0;
	uint8_t value       = 0;
	uint8_t fCfgCount   = 0;
	uint8_t rsv         = 0;
	int tlen            = 0;
	uint8_t profilebuffer[128] = "";

	//printf("\nTime:%llu:mac802_16e_PMPMS::%s()\n", GetCurrentTime(), __func__);

	// Spec 16e. Table 15
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 2);
	ifmm->extractBitField( 8, &rsv);         // rsv
	ifmm->extractBitField( 8, &fCfgCount);   // Configuration change count

	if (fCfgCount == ((DCDCfgCount + 1) % 256))
	{
		DCDCfgCount = fCfgCount;

		// clear
		for (diuc = 0; diuc < 16; diuc++)
		{
			DCDProfile[diuc].used = 0;
		}

		// set
		while ((type = ifmm->getNextType()) != 0)
		{
			if (type == 1) // DCD burst profiles
			{
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(profilebuffer);

				diuc = profilebuffer[0];
				tmptlv = new ifTLV(profilebuffer + 1, tlen - 1);
				DCDProfile[diuc].used = 1;

				while ((type = tmptlv->getNextType()) != 0)
				{
					tmptlv->getNextValue(&value);
					switch (type) {
						case 150:
							DCDProfile[diuc].fec = value;
							break;
					}
				}
				delete tmptlv;
			}
			else if (type == 50) // Support HO
			{
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(&_HO_support);
			}
		}
	}
	delete ifmm;

	return 1;
}

/*
 *    Processing UCD
 */
int mac802_16e_PMPMS::procUCD(struct mgmt_msg *recvmm, int cid, int len)
{
	uint8_t profilebuffer[128] = "";
	ifmgmt *ifmm        = NULL;
	ifTLV *tmptlv       = NULL;
	uint8_t type         = 0;
	uint8_t uiuc         = 0;
	uint8_t value        = 0;
	uint8_t fCfgCount    = 0;
	uint8_t fRngStart    = 0;
	uint8_t fRngEnd      = 0;
	uint8_t fReqStart    = 0;
	uint8_t fReqEnd      = 0;
	int tlen            = 0;

	//printf("\nTime:%llu:mac802_16e_PMPMS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 5);
	ifmm->extractBitField( 8, &fCfgCount);
	ifmm->extractBitField( 8, &fRngStart);
	ifmm->extractBitField( 8, &fRngEnd);
	ifmm->extractBitField( 8, &fReqStart);
	ifmm->extractBitField( 8, &fReqEnd);

	RngStart    = fRngStart;
	RngEnd      = fRngEnd;
	ReqStart    = fReqStart;
	ReqEnd      = fReqEnd;

	if (fCfgCount == (UCDCfgCount + 1) % 256)
	{
		UCDCfgCount = fCfgCount;
		for (uiuc = 0; uiuc < 16; uiuc++)
		{
			UCDProfile[uiuc].used = 0;
		}
		while ((type = ifmm->getNextType()) != 0)
		{
			if (type == 1)
			{
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(profilebuffer);
				uiuc = profilebuffer[0];
				tmptlv = new ifTLV(profilebuffer + 1, tlen - 1);
				UCDProfile[uiuc].used = 1;
				while ((type = tmptlv->getNextType()) != 0)
				{
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
int mac802_16e_PMPMS::procDLMAP(struct mgmt_msg *recvmm, int cid, int len)
{
	char        fdcdcount       = 0;
	uint8_t     frameDuration   = 0;
	uint8_t     numSymbols      = 0;
	ifmgmt      *ifmm           = NULL;
	uint8_t     *tmp_ies        = NULL;
	ntfyDLMAP   *ntfyCmd        = NULL;

	struct OFDMA_DLMAP_IE_other *ie_other   = NULL;
	OFDMA_DLMAP_IE *dlmap_ies               = NULL;
	dlmapinfo   info;

	//printf("\nTime:%llu:mac802_16e_PMPMS(%d)::%s()\n", GetCurrentTime(), get_nid(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1); // len - 1, because the first field is type
	ifmm->extractBitField( 8, (uint8_t *)&frameDuration);
	ifmm->extractBitField(24, (uint8_t *)&frameNumber);
	ifmm->extractBitField( 8, (uint8_t *)&fdcdcount);
	ifmm->extractBitField(48, (uint8_t *)ServingBSID);
	ifmm->extractBitField( 8, (uint8_t *)&numSymbols);

	((OFDMA_PMPMS *)sendtarget_->bindModule())->setDLsymbols(numSymbols);

	if (State && msDLParam) // if synchronized
	{
		uint8_t diuc    = 0;
		uint8_t numCid  = 0;
		int leftBits    = 0;

		tmp_ies = new uint8_t [len - 13];
		memset(tmp_ies, 0, len - 13);
		ifmm->extractBitField((len - 13) * 8, tmp_ies); // IEs

		dlmap_ies = new OFDMA_DLMAP_IE(tmp_ies, len - 13);
		leftBits = (len - 13) * 8;  // to bits

		while(1)
		{
			if(leftBits == 0 || leftBits == 4) // 0:end or 4:padding
			{
				break;
			}

			diuc = 0;
			numCid = 0;

			dlmap_ies->extractField( 4, &diuc);

			switch(diuc) {
				case Gap_PAPR_reduction: // Skip
					break;

				case Extended_DIUC_2: // Not Implement
					break;

				case Extended_DIUC: // Not Implement
					break;

				default: //others
					dlmap_ies->extractField( 8, &numCid);
					ie_other        = new OFDMA_DLMAP_IE_other;

					memset(ie_other, 0, sizeof(OFDMA_DLMAP_IE_other));
					ie_other->cid   = new uint16_t [numCid];
					memset(ie_other->cid, 0, numCid);

					// clear
					for (int i = 0;i < numCid;i++)
					{
						ie_other->cid[i] = 0;
					}

					// Structure
					ie_other->diuc      = diuc;
					ie_other->numCid    = numCid;
					for (int i = 0;i < numCid;i++)
					{
						dlmap_ies->extractField(16, (uint8_t *)&ie_other->cid[i]);
					}

					dlmap_ies->extractField( 8, &ie_other->symOff);
					dlmap_ies->extractField( 6, &ie_other->chOff);
					dlmap_ies->extractField( 3, &ie_other->boosting);
					dlmap_ies->extractField( 7, &ie_other->numSym);
					dlmap_ies->extractField( 6, &ie_other->numCh);
					dlmap_ies->extractField( 2, &ie_other->repeCode);

					leftBits -= (44 + numCid * 16);
					break;
			}

			if(ie_other != NULL)
			{
				info.diuc       = ie_other->diuc;
				info.symOff     = ie_other->symOff;
				info.chOff      = ie_other->chOff;
				info.numSym     = ie_other->numSym;
				info.numCh      = ie_other->numCh;
				info.repeCode   = ie_other->repeCode;

				if(ntfyCmd == NULL)
				{
					ntfyCmd = new ntfyDLMAP;
				}
				ntfyCmd->info.push_back(info);

				delete [] ie_other->cid;
				delete ie_other;
				ie_other = NULL;
			}
			else
			{
				; // other IEs not implement
			}

		} /* End while */

		if (ntfyCmd != NULL)
		{
			SendNTFYtoPHY(ntfyCmd);
		}

		delete [] tmp_ies;
		delete dlmap_ies;
	}
	else
	{
		((OFDMA_PMPMS *)sendtarget_->bindModule())->skipBufferedPkt();
	}

	delete ifmm;

	return 1;
}

/*
 * Processing UL-MAP
 */
int mac802_16e_PMPMS::procULMAP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt          *ifmm       = NULL;
	uint16_t        ie_cid      = 0;
	uint8_t         ie_uiuc     = 0;
	int             leftBits    = 0;
	char            rsv         = 0;
	char            fucdcount   = 0;
	int             fastime     = 0;        // Allocation Start Time
	int             contSymbols = 0;
	uint8_t         *tmp_ies    = NULL;
	uint64_t        tick        = 0;
	OFDMA_ULMAP_IE  *ulmap_ies  = NULL;
	bool            flag_IHie   = false;
	int             numSlots    = 0;

	//printf("Time:%llu MS(%d) %s()\n", GetCurrentTime(), get_nid(), __func__);

	// UL-MAP
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);
	ifmm->extractBitField( 8, (uint8_t *)&rsv);
	ifmm->extractBitField( 8, (uint8_t *)&fucdcount);
	ifmm->extractBitField(32, (uint8_t *)&fastime);    // UL Allocation Start Time (in PS)
	ifmm->extractBitField( 8, (uint8_t *)&ULsymbols);
	((OFDMA_PMPMS *)sendtarget_->bindModule())->setULsymbols(ULsymbols);

	// UL-MAP IE
	tmp_ies = new uint8_t [len - 8];
	memset(tmp_ies, 0, len - 8);
	ifmm->extractBitField((len - 8)*8, tmp_ies); // IEs

	ulmap_ies = new OFDMA_ULMAP_IE(tmp_ies, len - 8);
	leftBits = (len - 8) * 8;

	while(1)
	{
		if (leftBits == 0 || leftBits == 4) // 0:end or 4:padding
		{
			break;
		}
		ie_cid  = 0;
		ie_uiuc = 0;

		ulmap_ies->extractField(16, (uint8_t *)&ie_cid);
		ulmap_ies->extractField( 4, &ie_uiuc);

		switch(ie_uiuc) {
			case FAST_FEEDBACK: // not implement
				break;

			case Extended_UIUC_2: // not implement
				break;

			case CDMA_BWreq_Ranging: // contention subchannel
				if (flag_IHie == false)
				{
					memset(&savedIHie, 0, sizeof(ULMAP_IE_u));

					savedIHie.ie_12.cid  = ie_cid;
					savedIHie.ie_12.uiuc = ie_uiuc;
					ulmap_ies->extractField( 8, &savedIHie.ie_12.symOff);
					ulmap_ies->extractField( 7, &savedIHie.ie_12.chOff);
					ulmap_ies->extractField( 7, &savedIHie.ie_12.numSym);
					ulmap_ies->extractField( 7, &savedIHie.ie_12.numCh);
					ulmap_ies->extractField( 2, &savedIHie.ie_12.rangMethod);
					ulmap_ies->extractField( 1, &savedIHie.ie_12.rangIndicator);
					leftBits -= 52;

					flag_IHie = true;

					if (RngLastStatus == Rerange)  // just synchronize to UL
					{
						int rangOpportunity = 0;
						int rangSymbol      = 0;

						if (savedIHie.ie_12.rangMethod == 0) // 0: Ranging over two symbols
						{
							rangSymbol = 2;
						}
						else // 2: Ranging over four symbols
						{
							rangSymbol = 4;
						}

						rangOpportunity = savedIHie.ie_12.numSym / rangSymbol;
						timer_mngr()->cancel_t(2u);

						if (RngCount >= rangOpportunity)
						{
							RngCount -= rangOpportunity;
						}
						else  // Send Initial Ranging Code at Initial Ranging Region
						{
							tick          = computeSendTime(savedIHie.ie_12, RngCount * rangSymbol, fastime);
							RngSymOffset  = savedIHie.ie_12.symOff + RngCount * rangSymbol;
							RngChOffset   = 0;
							timerSendRngCode->start(tick, 0);
							//printf("(%d)Time:%llu MS(%d) timerSendRngCode->start (Initial)\n", __LINE__, GetCurrentTime(), get_nid());
						}
					}

					contSymbols += savedIHie.ie_12.numSym;
				}
				else
				{
					memset(&savedBPie, 0, sizeof(ULMAP_IE_u));

					savedBPie.ie_12.cid  = ie_cid;
					savedBPie.ie_12.uiuc = ie_uiuc;
					ulmap_ies->extractField( 8, &savedBPie.ie_12.symOff);
					ulmap_ies->extractField( 7, &savedBPie.ie_12.chOff);
					ulmap_ies->extractField( 7, &savedBPie.ie_12.numSym);
					ulmap_ies->extractField( 7, &savedBPie.ie_12.numCh);
					ulmap_ies->extractField( 2, &savedBPie.ie_12.rangMethod);
					ulmap_ies->extractField( 1, &savedBPie.ie_12.rangIndicator);
					leftBits -= 52;

					if (RngLastStatus == Continue)  // continue status: ranging at Period Ranging Region
					{
						int rangOpportunity = 0;
						int rangSymbol      = 0;

						if (savedBPie.ie_12.rangMethod == 2)  // 2: Ranging over one symbols
						{
							rangSymbol = 1;
						}
						else // 3: Ranging over three symbols
						{
							rangSymbol = 3;
						}

						rangOpportunity = savedBPie.ie_12.numSym / rangSymbol;

						if (RngCount >= rangOpportunity)
						{
							RngCount -= rangOpportunity;
						}
						else  // Send Initial(Period) Ranging Code at Period Ranging Region
						{
							tick          = computeSendTime(savedBPie.ie_12, RngCount * rangSymbol, fastime);
							RngSymOffset  = savedBPie.ie_12.symOff + RngCount * rangSymbol;
							RngChOffset   = 0;
							timerSendRngCode->start(tick, 0);
							//printf("(%d)Time:%llu MS(%d) timerSendRngCode->start (Period)\n", __LINE__, GetCurrentTime(), get_nid());
						}
					}
				}

				break;

			case PAPR_Safety_zone:  // not implement
				break;

			case CDMA_Alloc_IE:
				memset(&savedALie, 0, sizeof(ULMAP_IE_u));

				savedALie.ie_14.cid  = ie_cid;
				savedALie.ie_14.uiuc = ie_uiuc;
				ulmap_ies->extractField( 6, &savedALie.ie_14.duration);
				ulmap_ies->extractField( 4, &savedALie.ie_14.uiuc_trans);
				ulmap_ies->extractField( 2, &savedALie.ie_14.repeCode);
				ulmap_ies->extractField( 4, &savedALie.ie_14.frameIndex);
				ulmap_ies->extractField( 8, &savedALie.ie_14.rangCode);
				ulmap_ies->extractField( 8, &savedALie.ie_14.rangSym);
				ulmap_ies->extractField( 7, &savedALie.ie_14.rangCh);
				ulmap_ies->extractField( 1, &savedALie.ie_14.bwReq);
				leftBits -= 60;

				if (RngLastStatus == CodeSuccess || RngLastStatus == CodeWaitRSP)  // ranging code success
				{
					if (((savedCode.rangingCodeIndex   & 0xFF) != (savedALie.ie_14.rangCode   & 0xFF)) ||
							((savedCode.rangingSymbol      & 0xFF) != (savedALie.ie_14.rangSym    & 0xFF)) ||
							((savedCode.rangingSubchannel  & 0x3F) != (savedALie.ie_14.rangCh     & 0x3F)) ||
							((savedCode.rangingFrameNumber & 0x0F) != (savedALie.ie_14.frameIndex & 0x0F))) // Discard this IE
					{
						; // skip
					}
					else    // Send RNG-REQ at CDMA allocation slots
					{
						if (RngCodeRegion == INITIAL_RANGING || RngCodeRegion == HANDOVER_RANGING)
						{
							tick = computeSendTime(numSlots, savedALie.ie_14.duration, fastime);
							timerSendRngReq->start(tick, 0);
							//printf("Time:%llu MS[%d] timerSendRngReq->start at %llu\n", GetCurrentTime(), get_nid(), GetCurrentTime() + tick);
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

				tmpULie.ie_other.cid  = ie_cid;
				tmpULie.ie_other.uiuc = ie_uiuc;
				ulmap_ies->extractField(10, (uint8_t *)&tmpULie.ie_other.duration);
				ulmap_ies->extractField( 2, &tmpULie.ie_other.repeCode);
				leftBits -= 32;

				if (ie_cid == BasicCID) // Grant for this MS, save to savedULie
				{
					memcpy(&savedULie, &tmpULie, sizeof(ULMAP_IE_u));
					MICRO_TO_TICK(tick, TTG() * PS()); // schedule sending time
					timerSendUL->start(tick, 0);
				}

				numSlots += tmpULie.ie_other.duration;
				break;
		}
	}

	delete ulmap_ies;
	delete [] tmp_ies;
	delete ifmm;

	return 1;
}

/*
 *  Processing MOB_NBR-ADV
 */
int mac802_16e_PMPMS::procMOB_NBRADV(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm            = NULL;
	uint8_t skipOptField    = 0;
	uint8_t fragmentIndex   = 0;
	uint8_t totalFragment   = 0;
	uint8_t N_neighbors     = 0;
	uint8_t loopLen         = 0;
	uint8_t phyProfileID    = 0;
	uint8_t CfgCount        = 0;

	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);
	ifmm->extractBitField( 8, &skipOptField);

	if ((skipOptField & 0x01) == 0)  // skipOptField[0]
	{
		uint8_t operatorID[6] = "";
		ifmm->extractBitField(24, operatorID + 3);
		//printf("operatorID:%d %d %d %d %d %d\n", operatorID[0], operatorID[1], operatorID[2], operatorID[3], operatorID[4], operatorID[5]);
	}

	ifmm->extractBitField( 8, &CfgCount);

	if (CfgCount != NeighborBSList->NBRADV_CfgCount)
	{
		NeighborBSList->NBRADV_CfgCount = CfgCount;

		ifmm->extractBitField( 4, &fragmentIndex);
		ifmm->extractBitField( 4, &totalFragment);
		ifmm->extractBitField( 8, &N_neighbors);

		for (int i = 0;i < N_neighbors;i++)
		{
			NbrBS *pNbrBS               = NULL;
			uint8_t Neighbor_BSID[6]    = "";
			uint8_t FA_Index            = 0;
			uint8_t BS_EIRP             = 0;

			ifmm->extractBitField( 8, &loopLen);
			loopLen -= 1;

			ifmm->extractBitField( 8, &phyProfileID);
			loopLen -= 1;

			if (((phyProfileID & 0x40) >> 6) == 1)
			{
				ifmm->extractBitField( 8, &FA_Index);
				//printf("FA_Index:%d\n", FA_Index);
				loopLen -= 1;
			}

			if (((phyProfileID & 0x10) >> 4) == 1)
			{
				ifmm->extractBitField( 8, &BS_EIRP);
				//printf("BS_EIRP:%d\n", BS_EIRP);
				loopLen -= 1;
			}

			if ((skipOptField & 0x02) == 0)
			{
				ifmm->extractBitField(24, Neighbor_BSID + 3); // only LSB 24 bits
				pNbrBS = NeighborBSList->getNbrbyBSID(Neighbor_BSID);

				if (pNbrBS == NULL) // if new entry
				{
					printf("### Add new entry\n");
					printf("### MS(%d):serving: %x:%x:%x:%x:%x:%x\n", get_nid(), ServingBSID[0], ServingBSID[1], ServingBSID[2], ServingBSID[3], ServingBSID[4], ServingBSID[5]);
					printf("### MS(%d):NBRNBR : %x:%x:%x:%x:%x:%x\n", get_nid(), Neighbor_BSID[0], Neighbor_BSID[1], Neighbor_BSID[2], Neighbor_BSID[3], Neighbor_BSID[4], Neighbor_BSID[5]);

					pNbrBS = new NbrBS(-1, Neighbor_BSID);
					pNbrBS->Index = i;
					NeighborBSList->nbrBSs_Index.push_back(pNbrBS);
				}
				//printf("Neighbor_BSID:%d %d %d %d %d %d\n", Neighbor_BSID[0], Neighbor_BSID[1], Neighbor_BSID[2], Neighbor_BSID[3], Neighbor_BSID[4], Neighbor_BSID[5]);
				loopLen -= 3;
			}

			ifmm->extractBitField( 8, &pNbrBS->PreambleIndex);
			printf("### MS(%d):ChID   : %d\n\n", get_nid(), pNbrBS->PreambleIndex);
			loopLen -= 1;

			if ((skipOptField & 0x04) == 0)
			{
				ifmm->extractBitField( 8, &pNbrBS->HO_Optimize);
				loopLen -= 1;
			}

			if ((skipOptField & 0x08) == 0)
			{
				ifmm->extractBitField( 8, &pNbrBS->SchServSupport);
				loopLen -= 1;
			}

			ifmm->extractBitField( 4, &pNbrBS->DCDCfgCount);
			ifmm->extractBitField( 4, &pNbrBS->UCDCfgCount);
			loopLen -= 1;

			/*
			 * Here we need to record the index of the nerghbor BS in MOB_NBR-ADV message
			 */
			pNbrBS->Index = i;

			if (loopLen == 0)
			{
				continue;
			}
			else
			{
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
int mac802_16e_PMPMS::procRNGRSP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm        = NULL;
	int type            = 0;
	int ftimingadj      = 0;
	char fstatus        = 1;
	char fmac[6]        = "";
	uint16_t fbasicid   = 0;
	uint64_t fpricid    = 0;
	uint8_t rsv         = 0;
	uint32_t rangAttr   = 0;
	uint64_t tick       = 0;
	bool cdmaFlag       = false;

	//printf("Time:%llu MS(%d) %s()\n", GetCurrentTime(), get_nid(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 1);
	ifmm->extractBitField( 8, &rsv);

	while ((type = ifmm->getNextType()) != 0)
	{
		//printf("Type = %d\n", type);
		switch (type) {
			case 1: // both
				ifmm->getNextValue(&ftimingadj);
				//printf("\e[1;36mTime:%llu MS(%d) Timing Adjust = %d\e[0m\n", GetCurrentTime(), get_nid(), ftimingadj);
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

			case 150: // only appear at replying CDMA Ranging Code
				ifmm->getNextValue(&rangAttr);
				cdmaFlag = true;
				break;
		}
	}

	delete ifmm;

	if (cdmaFlag == true)  // RNG-RSP because of CDMA
	{
		if (((savedCode.rangingCodeIndex   & 0xFF)  != ((rangAttr >> 8) & 0xFF))   ||
				((savedCode.rangingSymbol      & 0x3FF) != ((rangAttr >> 22) & 0x3FF)) ||
				((savedCode.rangingSubchannel  & 0x3F)  != ((rangAttr >> 16) & 0x3F))  ||
				((savedCode.rangingFrameNumber & 0xFF)  != ((rangAttr & 0xFF)))) // Discard RNG-RSP not to this MS 
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
		}
		else if (RngLastStatus == Continue) // 1:Continue
		{
			// Ranging at Period Ranging Region
			RngTime = RngStart - 1;
			WaitForRangingOpportunity();
			//printf("MS(%d) Period Ranging Continue! (ftimingadj = %d) TimingAdjust = %d\n", get_nid(), ftimingadj, TimingAdjust);
			return 0;
		}
		else    // 3:Success
		{
			RngLastStatus = CodeSuccess; // ranging code success
			if (RngCodeRegion == INITIAL_RANGING || RngCodeRegion == HANDOVER_RANGING)
			{
				//printf("Time:%llu MS(%d) Initial or Handover Ranging Success!\n", GetCurrentTime(), get_nid()); // BS shall allocate a txop at CDMA_Allocation_IE
			}
			else if (RngCodeRegion == PERIOD_RANGING)
			{
				MILLI_TO_TICK(tick, PeriodRangingInterval * frameDuration[frameDuCode()]);
				timerPeriodRanging->start(tick, 0);
				//printf("Time:%llu MS(%d) Period Ranging Success! (restart at %llu)\n", GetCurrentTime(), get_nid(), tick);
			}
			else
			{
				printf("MS(%d) Unknown Ranging!\n", get_nid());
			}

			return 0;
		}
	}
	else // RNG-RSP because of RNG-REQ
	{
		if (memcmp(fmac, address()->buf(), 6))    // Discard RNG-RSP that is not to this MS
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

		if (RngLastStatus == Continue)    // 1:Continue
		{
			//            printf("MS(%d) Continue!\n", get_nid());
			RngTime = RngStart - 1;
			WaitForRangingOpportunity();
			return 0;
		}
		else    // 3:Success
		{
			//            printf("MS(%d) Success!\n", get_nid());
			MnConnections.push_back(new ManagementConnection(BasicCID));
			MnConnections.push_back(new ManagementConnection(PriCID));
			return 1;  // ranging complete
		}
	}
}

int mac802_16e_PMPMS::procSBCRSP(struct mgmt_msg *recvmm, int cid, int len)
{
	//printf("\nTime:%llu:mac802_16e_PMPMS::%s()\n", GetCurrentTime(), __func__);
	return 1;
}

int mac802_16e_PMPMS::procMOB_SCNRSP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm            = NULL;
	int ifmmLen             = 0;
	uint8_t rsv             = 0;
	uint8_t N_RecBS_Index   = 0;
	uint8_t CfgChangeCnt    = 0;
	uint8_t Index           = 0;
	uint8_t scanType        = 0;
	uint8_t NBR_BS_ID[6]    = "";
	uint8_t N_RecBS_Full    = 0;
	uint8_t padding         = 0;
	int tmpBits             = 0;

	//printf("\n\e[32mTime:%llu\e[33m MS(%d):%s\e[0m\n", GetCurrentTime(), get_nid(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);

	ifmm->extractBitField( 8, &NeighborBSList->ScanDuration);
	ifmm->extractBitField( 2, &NeighborBSList->ReportMode);
	ifmm->extractBitField( 6, &rsv);
	ifmm->extractBitField( 8, &NeighborBSList->ReportPeriod);
	ifmm->extractBitField( 8, &NeighborBSList->ReportMetric);
	tmpBits += 32;

	if (NeighborBSList->ScanDuration != 0)
	{
		ifmm->extractBitField( 4, &NeighborBSList->StartFrame);
		ifmm->extractBitField( 1, &rsv);
		ifmm->extractBitField( 8, &NeighborBSList->InterleavingInterval);
		ifmm->extractBitField( 8, &NeighborBSList->ScanIteration);
		ifmm->extractBitField( 3, &padding);
		ifmm->extractBitField( 8, &N_RecBS_Index);
		tmpBits += 32;

		if (N_RecBS_Index != 0)
		{
			ifmm->extractBitField( 8, &CfgChangeCnt);
			tmpBits += 8;
		}

		if (CfgChangeCnt == NeighborBSList->NBRADV_CfgCount)
		{
			for (int i = 0;i < N_RecBS_Index;i++)
			{
				ifmm->extractBitField( 8, &Index);
				ifmm->extractBitField( 3, &scanType);
				tmpBits += 11;

				NeighborBSList->nbrBSs_Index[Index]->ScanType = scanType;

				if (scanType == 0x2 || scanType == 0x3)
				{
					uint8_t rendezvousTime = 0;
					ifmm->extractBitField( 8, &rendezvousTime);

					uint8_t CDMA_code = 0;
					ifmm->extractBitField( 8, &CDMA_code);

					uint8_t TxopOffset = 0;
					ifmm->extractBitField( 8, &TxopOffset);

					tmpBits += 24;
				}
			}
		}

		ifmm->extractBitField( 8, &N_RecBS_Full);
		tmpBits += 8;

		for (int i = 0;i < N_RecBS_Full;i++)
		{
			NbrBS *pNbrBS = NULL;

			ifmm->extractBitField(48, NBR_BS_ID);
			ifmm->extractBitField( 8, &scanType);
			tmpBits += 56;

			pNbrBS = NeighborBSList->getNbrbyBSID(NBR_BS_ID);
			if (pNbrBS == NULL)// FIXME: need??
			{
				pNbrBS = new NbrBS(-1, NBR_BS_ID);
				NeighborBSList->nbrBSs_Full.push_back(pNbrBS);
			}

			pNbrBS->ScanType = scanType;

			if (scanType == 0x2 || scanType == 0x3)
			{
				uint8_t rendezvousTime = 0;
				ifmm->extractBitField( 8, &rendezvousTime);

				uint8_t CDMA_code = 0;
				ifmm->extractBitField( 8, &CDMA_code);

				uint8_t TxopOffset = 0;
				ifmm->extractBitField( 8, &TxopOffset);

				tmpBits += 24;
			}
		}

		if (tmpBits % 8 != 0)
		{
			ifmm->extractBitField(8 - tmpBits % 8, &padding);
			ifmmLen = tmpBits / 8 + 1;
		}
		else
		{
			ifmmLen = tmpBits / 8;
		}

		delete ifmm;

		/*
		 * TLV encoded information
		 */
		if ((tmpBits / 8) != (len - 1))
		{
			int type = 0;
			uint8_t hmac[21] = "";

			ifmm = new ifmgmt((uint8_t *) recvmm, len, ifmmLen);

			while ((type = ifmm->getNextType()) != 0)
			{
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

		MICRO_TO_TICK(tick, ULsymbols * Ts() + (TTG() + RTG() + TimingAdjust) * PS());

		timerStartScan->start(tick, 0);
	}

	return 1;
}

int mac802_16e_PMPMS::procREGRSP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm     = NULL;
	int type         = 0;
	uint8_t fmanage  = 0;
	uint8_t fipm     = 0;
	uint8_t farq     = 0;
	uint8_t fmobile  = 0;
	uint16_t fulncid = 0;
	uint16_t fdlncid = 0;
	uint16_t fclass  = 0;
	uint16_t ResourceRetainTime = 0;

	//printf("\nTime:%llu:mac802_16e_PMPMS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 0);

	while ((type = ifmm->getNextType()) != 0)
	{
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

int mac802_16e_PMPMS::procDSAREQ(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm        = NULL;
	ifTLV *tmptlv       = NULL;
	int type1           = 0;
	int type2           = 0;
	uint8_t fbuf[256]   = "";
	uint8_t fhmac[21]   = "";
	uint8_t fcsspec     = 0;;
	uint16_t ftransid   = 0;
	uint16_t fdatacid   = 0;
	uint8_t fconfirm    = 0;

	//printf("\nTime:%llu:mac802_16e_PMPMS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 2);
	ifmm->extractBitField(16, (uint8_t *)&ftransid);
	//printf("Transaction ID = %d\n", ftransid);

	while ((type1 = ifmm->getNextType()) != 0)
	{
		//printf("Type = %d\t", type1);
		switch (type1) {
			case 145: // Uplink service flow
				ifmm->getNextValue(fbuf);
				{
					//printf("UL Service Flow Sub-Type Length = %d\n", ifmm->getNextLen());
					tmptlv = new ifTLV(fbuf, ifmm->getNextLen());

					while ((type2 = tmptlv->getNextType()) != 0)
					{
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

	if (_CSType == 3 || _CSType == 5)   // Ethernet
	{
		DtConnections.push_back(new DataConnectionEthernet(fdatacid));
	}
	else
	{
		DtConnections.push_back(new DataConnection(fdatacid));
	}

	ReceiverList.push_back(new ConnectionReceiver(fdatacid));

	ifmm = new ifmgmt(MG_DSARSP, 3);     // Spec 16e. 11.13
	ifmm->appendBitField(16, ftransid);  // Transcation ID
	ifmm->appendBitField( 8, fconfirm);  // Confirmation Code: OK, Table 384
	ifmm->appendTLV(149, 21, fhmac);     // HMAC

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = cid;

	PushPDUintoConnection(cid, ifmm);
	resetTimerT(8u);

	return 1;
}

int mac802_16e_PMPMS::procDSAACK(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm        = NULL;
	int type            = 0;
	uint8_t fhmac[21]   = "";
	uint16_t ftransid   = 0;

	//printf("\nTime:%llu:mac802_16e_PMPMS::%s()\n", GetCurrentTime(), __func__);
	ifmm = new ifmgmt((uint8_t *) recvmm, len, 2);
	ifmm->extractBitField(16, (uint8_t *)&ftransid);
	//printf("Transaction ID = %d\n", ftransid);

	while ((type = ifmm->getNextType()) != 0)
	{
		//printf("Type = %d\t", type);
		switch (type) {
			case 149:
				ifmm->getNextValue(fhmac);
				//printf("HMAC = ...\n");
				break;
		}

	}

	delete ifmm;
	printf("\n================%d==IF=PROGRAM=RUN=TO=HERE=THAT'S=NETWORK=ENTRY=OK=======================\n\n", get_nid());

	return 1;
}

int mac802_16e_PMPMS::procMOB_BSHORSP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm    = NULL;
	int ifmmLen     = 0;
	uint8_t Mode    = 0;
	uint8_t rsv     = 0;
	uint8_t padding = 0;
	int tmpBits     = 0;
	uint8_t HO_operation_mode = 0;

	//printf("\n\e[1;36mTime:%llu MS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);
	//printf("========================================\n");

	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1); // Here, the (len - 1) is the maximum length of mgmt_msg, not field size
	ifmm->extractBitField( 3, &Mode);
	//printf("= Mode: %d\n", Mode);
	ifmm->extractBitField( 5, &rsv);
	//printf("= rsv: %d\n", rsv);
	tmpBits += 8;

	if (Mode == 0x00)
	{
		ifmm->extractBitField( 1, &HO_operation_mode);
		//printf("= HO_operation_mode: %d\n", HO_operation_mode);

		uint8_t N_Recommended = 0;
		ifmm->extractBitField( 8, &N_Recommended);
		//printf("= N_Recommended: %d\n", N_Recommended);

		uint8_t ResourceRetainFlag = 0;
		ifmm->extractBitField( 1, &ResourceRetainFlag);
		//printf("= ResourceRetainFlag: %d\n", ResourceRetainFlag);
		ifmm->extractBitField( 7, &rsv);

		tmpBits += 17;

		for (int i = 0;i < N_Recommended;i++)
		{
			uint8_t Neighbor_BSID[6] = "";
			ifmm->extractBitField(48, Neighbor_BSID);
			tmpBits += 48;

			//printf("======[%d] Neighbor_BSID: %d:%d:%d:%d:%d:%d\n", i, Neighbor_BSID[0], Neighbor_BSID[1], Neighbor_BSID[2], Neighbor_BSID[3], Neighbor_BSID[4], Neighbor_BSID[5]);
		}

		uint8_t PreambleIndex = 0;
		ifmm->extractBitField( 8, &PreambleIndex);
		//printf("= PreambleIndex: %d\n", PreambleIndex);

		uint8_t ServiceLevelPredict = 0;
		ifmm->extractBitField( 8, &ServiceLevelPredict);
		//printf("= ServiceLevelPredict: %d\n", ServiceLevelPredict);

		uint8_t HO_process_optimization = 0;
		ifmm->extractBitField( 8, &HO_process_optimization);
		//printf("= HO_process_optimization: %d\n", HO_process_optimization);

		uint8_t Network_Assisted_HO_Support = 0;
		ifmm->extractBitField( 1, &Network_Assisted_HO_Support);
		//printf("= Network_Assisted_HO_Support: %d\n", Network_Assisted_HO_Support);

		uint8_t HO_ID_Indicator = 0;
		ifmm->extractBitField( 1, &HO_ID_Indicator);
		//printf("= HO_ID_Indicator: %d\n", HO_ID_Indicator);
		tmpBits += 26;

		if (HO_ID_Indicator == 1)
		{
			uint8_t HO_ID = 0;
			ifmm->extractBitField( 8, &HO_ID);
			//printf("= HO_ID: %d\n", HO_ID);
			tmpBits += 8;
		}
		uint8_t HO_auth_Indicator = 0;
		ifmm->extractBitField( 1, &HO_auth_Indicator);
		//printf("= HO_auth_Indicator: %d\n", HO_auth_Indicator);
		ifmm->extractBitField( 4, &rsv);
		//printf("= rsv:%d\n", rsv);
		tmpBits += 5;

		if (HO_auth_Indicator == 1)
		{
			uint8_t HO_auth_support = 0;
			ifmm->extractBitField( 8, &HO_auth_support);
			//printf("= HO_auth_support: %d\n", HO_auth_support);
			tmpBits += 8;
		}
	}
	else if (Mode == 0x01)
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
	ifmm->extractBitField( 8, &ActionTime);
	//printf("= ActionTime: %d\n", ActionTime);
	tmpBits += 8;

	if (tmpBits % 8 != 0)
	{
		ifmm->extractBitField( 8 - tmpBits % 8, &padding);
		//printf("= padding(%d): %d\n", 8 - tmpBits % 8, padding);
		ifmmLen = tmpBits % 8 + 1;
	}
	else
	{
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

uint8_t mac802_16e_PMPMS::selectDIUC(double recvSNR)
{
	int fec = 0;

	if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
	{
		for (int i = 12; i >= 0; i--)
		{
			fec = DCDProfile[i].fec;
			if (DCDProfile[i].used && recvSNR >= fecSNR[fec])
			{
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
		printf("%s#%d: Warning Configure String LinkMode:%s\n", __FILE__, __LINE__, LinkMode);

	for (int i = 0; i <= 12; i++)
	{
		if (DCDProfile[i].used && DCDProfile[i].fec == fec)
		{
			return i;
		}
	}

	return 255;
}

/*
 * Send Initial or Handover Ranging Code
 */
int mac802_16e_PMPMS::SendRNGCODE()
{
	RangCodeInfo *codeInfo  = NULL;
	ePacket_ *epkt          = NULL;
	int codeIndex           = 0;
	uint8_t *code           = NULL;
	int dupFactor           = 0;
	int rangMethod          = 0;

	//    printf("Time:%llu MS(%d)::%s() RngCodeRegion=%d\n", GetCurrentTime(), get_nid(), __func__, RngCodeRegion);

	if (RngCodeRegion == INITIAL_RANGING || RngCodeRegion == HANDOVER_RANGING)
	{
		if (savedIHie.ie_12.rangMethod == 0)
		{
			dupFactor = 2;
		}
		else if (savedIHie.ie_12.rangMethod == 1)
		{
			dupFactor = 4;
		}
		else
		{
			fprintf(stderr, "Warning: Impossible Point. (Initial Ranging/Handover Ranging over unknown rangMethod)\n");
			exit(1);
		}
		rangMethod = savedIHie.ie_12.rangMethod;
	}
	else if (RngCodeRegion == PERIOD_RANGING || RngCodeRegion == BW_REQUEST)
	{
		if (savedBPie.ie_12.rangMethod == 2)
		{
			dupFactor = 1;
		}
		else if (savedIHie.ie_12.rangMethod == 3)
		{
			dupFactor = 3;
		}
		else
		{
			fprintf(stderr, "\e[1;31mWarning: Impossible Point. (BW Request/Period Ranging over unknown rangMethod)\e[0m\n");
			exit(1);
		}
		rangMethod = savedBPie.ie_12.rangMethod;
	}
	else
	{
		;
	}

	codeIndex   = getCode(RngCodeRegion);
	if (RngCodeRegion == PERIOD_RANGING)    //offset of perodic ranging code region
                codeIndex   -= 64;
        else if (RngCodeRegion == HANDOVER_RANGING)
                codeIndex   -= 192;
	code = new uint8_t [18 * dupFactor];
	memset(code, 0, 18 * dupFactor);

	if (RngCodeRegion == PERIOD_RANGING)
        {
                for (int i = 0;i < dupFactor;i++)
                {
                        memcpy(code + 18 * i, _periodRang_code_db[codeIndex], 18);
                }
        }
        else if (RngCodeRegion == HANDOVER_RANGING)
        {
                for (int i = 0;i < dupFactor;i++)
                {
                        memcpy(code + 18 * i, _handoverRang_code_db[codeIndex], 18);
                }
        }
        else
        {
		for (int i = 0;i < dupFactor;i++)
		{
			memcpy(code + 18 * i, _initRang_code_db[codeIndex], 18);
		}
	}

	// save to codeInfo
	codeInfo                = new RangCodeInfo;
	codeInfo->flag          = PF_SEND;
	codeInfo->rangingCode   = code;
	codeInfo->codeLen       = 18 * dupFactor;
	codeInfo->rangSymOffset = RngSymOffset;
	codeInfo->rangChOffset  = RngChOffset;
	codeInfo->rangMethod    = rangMethod;

	// send to Phy
	epkt                    = createEvent();
	epkt->DataInfo_         = codeInfo;
	put(epkt, sendtarget_);

	// save at local
	savedCode.rangingFrameNumber    = (frameNumber & 0xFF);
	if (RngCodeRegion == PERIOD_RANGING)
                savedCode.rangingCodeIndex      = codeIndex + 64;
        else if (RngCodeRegion == HANDOVER_RANGING)
                savedCode.rangingCodeIndex      = codeIndex + 192;
        else
                savedCode.rangingCodeIndex      = codeIndex;
	savedCode.rangingSymbol         = RngSymOffset;
	savedCode.rangingSubchannel     = RngChOffset;
	savedCode.rangingUsage          = RngCodeRegion;

	RngLastStatus = CodeWaitRSP;    // Waiting For anonymous RNGRSP and CDMA Allocation IE
	resetTimerT(3u);

	return 1;
}

int mac802_16e_PMPMS::SendRNGREQ()
{
	int len                 = 0;
	int thisLen             = 0;
	uint8_t rsv             = 0;
	ifmgmt *ifmm            = NULL;
	char *buf               = NULL;
	upBurst *burst          = NULL;
	BurstInfo *burstInfo    = NULL;
	ePacket_ *epkt          = NULL;
	vector<upBurst *> *BurstCollection = NULL;
	vector<int> pduInfo;

	//    printf("Time:%llu:MS(%d) %s()\n", GetCurrentTime(), get_nid(), __func__);
	myDIUC = selectDIUC(LastSignalInfo.SNR);
	printf("\e[36mTime:%llu MS(%d) Receive SNR=%lf dB and use diuc=%d(fec=%d)\e[0m\n", GetCurrentTime(), get_nid(), LastSignalInfo.SNR, myDIUC, DCDProfile[myDIUC].fec);

	ifmm = new ifmgmt(MG_RNGREQ, 1);
	ifmm->appendBitField( 8, rsv);              // Reserve
	ifmm->appendTLV(1, 1, myDIUC);              // Requested Downlink Burst Profile
	ifmm->appendTLV(2, 6, address()->buf());    // MS MAC Address

	if (RngLastStatus == Continue)              // Continue
		ifmm->appendTLV(3, 1, 3);               // Ranging Anomalies

	PushPDUintoConnection(initRangingCID, ifmm);

	// generate a burst
	burst = new upBurst(&savedALie, UCDProfile[savedALie.ie_14.uiuc_trans].fec);

	// compute total length
	pduInfo.clear();
	len = initRangingConnection->GetInformation(&pduInfo);
	thisLen = burst->toByte(savedALie.ie_14.duration);
	if (len > thisLen)
	{
		fprintf(stderr, "MS(%d) CDMA allocation slots is not enough to send RNG-REQ. (allocate:%d need:%d)\n", get_nid(), thisLen, len);
		exit(1);
	}

	// encapsulate all
	buf             = new char [len];
	memset(buf, 0, len);
	burst->payload  = buf;
	burst->length   = initRangingConnection->EncapsulateAll(buf, len);

	BurstCollection = new vector<upBurst *>;
	BurstCollection->push_back(burst);

	// send to PHY
	burstInfo               = new BurstInfo;
	burstInfo->Collection   = (vector<WiMaxBurst *> *) BurstCollection;
	burstInfo->flag         = PF_SEND;

	epkt            = createEvent();
	epkt->DataInfo_ = burstInfo;
	put(epkt, sendtarget_);

	RngLastStatus = REQWaitRSP;    // Waiting For RNGRSP
	resetTimerT(3u);

	return 1;
}

int mac802_16e_PMPMS::SendREGREQ()
{
	ifmgmt *ifmm = new ifmgmt(MG_REGREQ, 0);

	//printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS:%s()\n", GetCurrentTime(), get_nid(), __func__);

	// For PMP operation, the REG-REQ shall contain following three TLVs:
	ifmm->appendTLV( 2, 1, 0U);         // MS management support 11.7.2
	ifmm->appendTLV( 3, 1, 0U);         // IP management mode 11.7.3
	ifmm->appendTLV( 6, 2, 1);          // # of UL Transport CIDs supported
	ifmm->appendTLV( 7, 2, 0x09);       // Classification and SDU support
	ifmm->appendTLV(10, 1, AttrARQ);    // ARQ Support
	ifmm->appendTLV(15, 2, 1);          // # of DL Transport CIDs supported
	ifmm->appendTLV(31, 1, 0x01);       // Mobility supported

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = PriCID;

	PushPDUintoConnection(PriCID, ifmm);
	resetTimerT(6u); // wait for REG-RSP

	return 1;
}

int mac802_16e_PMPMS::SendSBCREQ()
{
	ifmgmt *ifmm = new ifmgmt(MG_SBCREQ, 0);

	//printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);

	ifmm->appendTLV(1, 1, 0U);          // Bandwidth Allocation Support 11.8.1
	ifmm->appendTLV(2, 2, 0x2020);      // Subscriber transition gaps 11.8.3.1  (TTG=RTG=90(PS)=32(us))
	ifmm->appendTLV(3, 4, 0xFFFFFFFF);  // Maximum transmit power 11.8.3.2
	ifmm->appendTLV(147, 1, 0xFF);      // Current transmit power 11.8.3.3

	// OFDMA specific parameters 11.8.3.7
	ifmm->appendTLV(150, 1, 0x10);      // FFT-1024
	ifmm->appendTLV(151, 2, 0x0001);    // 64-QAM
	ifmm->appendTLV(152, 1, 0x01);      // 64-QAM
	ifmm->appendTLV(154, 1, 0U);        // Permutation support

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = BasicCID;

	PushPDUintoConnection(BasicCID, ifmm);
	resetTimerT(18u);    // wait for SBC-RSP

	return 1;
}

/*
 * MS requests a scanning interval for scanning BSs
 */
int mac802_16e_PMPMS::SendMOB_SCNREQ()
{
	ifmgmt *ifmm            = NULL;
	int ifmmLen             = 0;
	uint8_t scanDuration    = 1;  // in frames
	uint8_t interleaveInt   = 10; // in frames
	uint8_t scanIteration   = 1;
	uint8_t N_RecBS_index   = NeighborBSList->nbrBSs_Index.size();
	uint8_t N_RecBS_Full    = NeighborBSList->nbrBSs_Full.size();
	uint8_t padding         = 0;
	int tmpBits             = 0;

	//printf("\nTime:%llu:MS(%d): mac802_16e_PMPMS::%s()\n", GetCurrentTime(), get_nid(), __func__);
	/*
	 * Compute all field bits
	 */
	tmpBits += 32;

	if (N_RecBS_index != 0)
		tmpBits += 8;

	for (int i = 0;i < N_RecBS_index;i++)
		tmpBits += 11;

	tmpBits += 8;

	for (int i = 0;i < N_RecBS_Full;i++)
		tmpBits += 51;

	if (tmpBits % 8 != 0)
		ifmmLen = tmpBits / 8 + 1;
	else
		ifmmLen = tmpBits / 8;

	/*
	 * Generate MOB_SCN-REQ Message
	 */

	// max = 5
	scanDuration = NeighborBSList->nbrBSs_Index.size() + NeighborBSList->nbrBSs_Full.size();
	if (scanDuration > 5)
	{
		scanDuration = 5;
	}
	else if (scanDuration == 0) // no BSs need to scan
	{
		return 1;
	}

	ifmm = new ifmgmt(MG_MOB_SCNREQ, ifmmLen);

	ifmm->appendBitField( 8, scanDuration);
	ifmm->appendBitField( 8, interleaveInt);
	ifmm->appendBitField( 8, scanIteration);
	ifmm->appendBitField( 8, N_RecBS_index);

	if (N_RecBS_index != 0)
		ifmm->appendBitField( 8, NeighborBSList->NBRADV_CfgCount);

	for (int i = 0;i < N_RecBS_index;i++)
	{
		ifmm->appendBitField( 8, i);
		ifmm->appendBitField( 3, NeighborBSList->nbrBSs_Index[i]->ScanType);
	}

	ifmm->appendBitField( 8, N_RecBS_Full);

	for (int i = 0;i < N_RecBS_Full;i++)
	{
		ifmm->appendBitField(48, NeighborBSList->nbrBSs_Full[i]->addr->buf());
		ifmm->appendBitField( 3, NeighborBSList->nbrBSs_Full[i]->ScanType);
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
int mac802_16e_PMPMS::SendMOB_SCNREP()
{
	ifmgmt *ifmm              = NULL;
	int ifmmLen               = 0;
	uint8_t repMode           = 0;
	uint8_t Comp_NBR_BSID_IND = 0;
	uint8_t N_curr_BSs        = 0;
	uint8_t rsv               = 0;
	uint8_t repMetric         = 0;
	uint8_t N_BS_Index        = 0;
	uint8_t N_BS_Full         = 0;
	int tmpBits               = 0;
	uint8_t tempBSID          = 0; // soft handover is not supported
	uint8_t BS_CINR           = 0;
	uint8_t BS_RSSI           = 0;
	uint8_t RelativeDelay     = 0;
	uint8_t BS_RTD            = 0;
	uint8_t CfgChangeCnt      = 0;
	uint8_t neighborBS_Index  = 0;
	uint8_t neighborBS_ID[6]  = "";

	/*
	 * Compute all field bits
	 */

	tmpBits += (1 + 1 + 3 + 3 + 8);

	for (int i = 0;i < N_curr_BSs;i++)
	{
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

	for (int i = 0;i < N_BS_Index;i++)
	{
		tmpBits += 8;

		if ((repMetric & 0x1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x2) >> 1) == 1)
			tmpBits += 8;

		if (((repMetric & 0x4) >> 2) == 1)
			tmpBits += 8;
	}

	tmpBits += 8;

	for (int i = 0;i < N_BS_Full;i++)
	{
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

	ifmm->appendBitField( 1, repMode);
	ifmm->appendBitField( 1, Comp_NBR_BSID_IND);
	ifmm->appendBitField( 3, N_curr_BSs);
	ifmm->appendBitField( 3, rsv);
	ifmm->appendBitField( 8, repMetric);

	for (int i = 0;i < N_curr_BSs;i++)
	{
		ifmm->appendBitField( 4, tempBSID);
		ifmm->appendBitField( 4, rsv);

		if ((repMetric & 0x1) == 1)
		{
			ifmm->appendBitField( 8, BS_CINR);
		}

		if (((repMetric & 0x2) >> 1) == 1)
		{
			ifmm->appendBitField( 8, BS_RSSI);
		}

		if (((repMetric & 0x4) >> 2) == 1)
		{
			ifmm->appendBitField( 8, RelativeDelay);
		}

		if (((repMetric & 0x8) >> 3) == 1)
		{
			ifmm->appendBitField( 8, BS_RTD);
		}
	}

	ifmm->appendBitField( 8, N_BS_Index);

	if (N_BS_Index != 0)
	{
		ifmm->appendBitField( 8, CfgChangeCnt);
	}

	for (int i = 0;i < N_BS_Index;i++)
	{
		ifmm->appendBitField( 8, neighborBS_Index);

		if ((repMetric & 0x1) == 1)
		{
			ifmm->appendBitField( 8, BS_CINR);
		}

		if (((repMetric & 0x2) >> 1) == 1)
		{
			ifmm->appendBitField( 8, BS_RSSI);
		}

		if (((repMetric & 0x4) >> 2) == 1)
		{
			ifmm->appendBitField( 8, RelativeDelay);
		}
	}

	ifmm->appendBitField( 8, N_BS_Full);

	for (int i = 0;i < N_BS_Full;i++)
	{
		ifmm->appendBitField(48, neighborBS_ID);

		if ((repMetric & 0x1) == 1)
		{
			ifmm->appendBitField( 8, BS_CINR);
		}

		if (((repMetric & 0x2) >> 1) == 1)
		{
			ifmm->appendBitField( 8, BS_RSSI);
		}

		if (((repMetric & 0x4) >> 2) == 1)
		{
			ifmm->appendBitField( 8, RelativeDelay);
		}
	}

	/*
	 * TLV encoded information
	 */

	PushPDUintoConnection(PriCID, ifmm);

	return 1;
}

/*
 * If MS wants to initial handover procedure, MS shall send MOB_MSHO-REQ
 */
int mac802_16e_PMPMS::SendMOB_MSHOREQ()
{
	ifmgmt *ifmm          = NULL;
	int ifmmLen           = 0;
	uint8_t repMetric     = 0x01; // CINR (SNR)
	uint8_t N_BS_Index    = 0;
	uint8_t N_BS_Full     = 0;
	uint8_t N_curr_BSs    = 1; // soft handover is not supported
	uint8_t ArrTimeInd    = 0;
	uint8_t padding       = 0;
	uint64_t tick         = 0;
	int tmpBits           = 0;
	uint8_t tempBSID      = 0; // soft handover is not supported
	uint8_t BS_CINR       = 0;
	uint8_t BS_RSSI       = 0;
	uint8_t RelativeDelay = 0;
	uint8_t BS_RTD        = 0;
	vector<NbrBS *>::iterator iter;
	uint8_t ServiceLevelPredict = 3; // No service level prediction available
	uint8_t ArrivalTimeDiff     = 0;

	//printf("\n\e[1;36mTime:%llu MS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	/*
	 * Compute handover arget BSs
	 */
	for (iter = NeighborBSList->nbrBSs_Index.begin();iter != NeighborBSList->nbrBSs_Index.end();iter++)
	{
		if ((*iter)->targetHO == true)
			N_BS_Index++;
	}

	for (iter = NeighborBSList->nbrBSs_Full.begin();iter != NeighborBSList->nbrBSs_Full.end();iter++)
	{
		if ((*iter)->targetHO == true)
			N_BS_Full++;
	}

	/*
	 * Compute all field bits
	 */
	tmpBits += 16;

	if (N_BS_Index != 0)
		tmpBits += 8;

	for (int i = 0;i < N_BS_Index;i++)
	{
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
	for (int i = 0;i < N_BS_Full;i++)
	{
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
	for (int i = 0;i < N_curr_BSs;i++)
	{
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

	ifmm->appendBitField( 8, repMetric);    // Report Metric
	ifmm->appendBitField( 8, N_BS_Index);   // N_New_BS_Index

	if (N_BS_Index != 0)
	{
		ifmm->appendBitField( 8, NeighborBSList->NBRADV_CfgCount);  // Configuration change count for MOB_NBR-ADV
	}

	for (int i = 0;i < N_BS_Index;i++)
	{
		ifmm->appendBitField( 8, i);
		ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Index[i]->PreambleIndex);

		if ((repMetric & 0x01) == 1)
		{
			ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Index[i]->CINR); // contain
		}

		if (((repMetric & 0x02) >> 1) == 1)
		{
			ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Index[i]->RSSI); // not implement
		}

		if (((repMetric & 0x04) >> 2) == 1)
		{
			ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Index[i]->RelativeDelay); // not implement
		}

		ifmm->appendBitField( 3, ServiceLevelPredict);
		ifmm->appendBitField( 1, ArrTimeInd);

		if (ArrTimeInd == 1)
		{
			ifmm->appendBitField( 4, ArrivalTimeDiff);
		}
	}

	ifmm->appendBitField( 8, N_BS_Full);

	for (int i = 0;i < N_BS_Full;i++)
	{
		ifmm->appendBitField(48, NeighborBSList->nbrBSs_Full[i]->addr->buf());
		ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Full[i]->PreambleIndex);

		if ((repMetric & 0x01) == 1)
		{
			ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Full[i]->CINR);
		}

		if (((repMetric & 0x02) >> 1) == 1)
		{
			ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Full[i]->RSSI);
		}

		if (((repMetric & 0x04) >> 2) == 1)
		{
			ifmm->appendBitField( 8, NeighborBSList->nbrBSs_Full[i]->RelativeDelay);
		}

		ifmm->appendBitField( 3, ServiceLevelPredict);
		ifmm->appendBitField( 1, ArrTimeInd);

		if (ArrTimeInd == 1)
		{
			ifmm->appendBitField( 4, ArrivalTimeDiff);
		}
	}

	ifmm->appendBitField( 3, N_curr_BSs);
	ifmm->appendBitField( 1, padding);

	// FIXME
	for (int i = 0;i < N_curr_BSs;i++)
	{
		ifmm->appendBitField( 4, tempBSID);

		if ((repMetric & 0x01) == 1)
		{
			ifmm->appendBitField( 8, BS_CINR);
		}

		if (((repMetric & 0x02) >> 1) == 1)
		{
			ifmm->appendBitField( 8, BS_RSSI);
		}

		if (((repMetric & 0x04) >> 2) == 1)
		{
			ifmm->appendBitField( 8, RelativeDelay);
		}

		if (((repMetric & 0x08) >> 3) == 1)
		{
			ifmm->appendBitField( 8, BS_RTD);
		}
	}

	if (tmpBits % 8 != 0)
	{
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
	MILLI_TO_TICK(tick, HORetransmitInterval * frameDuration[frameDuCode()]);
	timerHORetransmit->start(tick, 0);

	// Set HO-REQ retries available
	HOReqRetryAvailable = 3; // FIXME

	return 1;
}

int mac802_16e_PMPMS::SendMOB_HOIND(uint8_t HO_operation_mode)
{
	ifmgmt *ifmm    = NULL;
	int ifmmLen     = 0;
	int tmpBits     = 0;
	uint8_t rsv     = 0;
	uint8_t Mode    = 0;
	uint8_t padding = 0;
	NbrBS *targetBS = NeighborBSList->getTargetBS();
	uint8_t HO_IND_type          = 0;
	uint8_t Ranging_Params_valid = 0;
	uint8_t PreambleIndex        = 0;

	//printf("\n\e[1;36mTime:%llu MS(%d) %s()\e[0m\n", GetCurrentTime(), get_nid(), __func__);

	if (HO_operation_mode == 1) // Mandatory HO response
	{
		HO_IND_type = 0; // ==> Re-enter network at target BS
		//printf("\e[1;35m## MS(%d) starts re-enter network at target BS ... ##\e[0m\n", get_nid());
	}
	else // Recommended HO response
	{
		if (0) // HO cancel yes
		{
			HO_IND_type = 1; // ==> Done
			//printf("\e[1;35m## MS(%d) cancelled the handover response ... ##\e[0m\n", get_nid());
		}
		else // HO cancel no
		{
			HO_IND_type = 2; // ==> MS HO holding down
			resetTimerT(42u);
			//printf("\e[1;35m## MS(%d) perform handover holding down ... ##\e[0m\n", get_nid());
		}
	}

	/*
	 * Compute all field bits
	 */
	tmpBits += 8;

	if (Mode == 0x0)
	{
		tmpBits += 8;

		if (HO_IND_type == 0)
			tmpBits += 48;
	}

	if (Mode == 0x1)
	{
		;
	}

	if (Mode == 0x2)
	{
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
	ifmm->appendBitField( 6, rsv);
	ifmm->appendBitField( 2, Mode);

	if (Mode == 0x0) // HO
	{
		ifmm->appendBitField( 2, HO_IND_type);
		ifmm->appendBitField( 2, Ranging_Params_valid);
		ifmm->appendBitField( 4, rsv);

		if (HO_IND_type == 0)
		{
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

	if (tmpBits % 8 != 0)
	{
		ifmm->appendBitField(8 - tmpBits % 8, padding);
	}

	/*
	 * TLV encoding Information
	 */
	uint8_t hmac[21] = "";
	ifmm->appendTLV(149, 21, hmac);     // HMAC/CMAC Tuple

	ifmm->copyTo(saved_msg);

	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());

	lastCID = BasicCID;

	PushPDUintoConnection(BasicCID, ifmm);

	/*
	 * start HO timer
	 */
	if (HO_IND_type == 0)
	{
		uint64_t tick = 0;

		MICRO_TO_TICK(tick, TTG() * PS() + ULsymbols * Ts() + RTG() * PS());

		timerStartHO->start(tick, 0);
	}

	return 1;
}
