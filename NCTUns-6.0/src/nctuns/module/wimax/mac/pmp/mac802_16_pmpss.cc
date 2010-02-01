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
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <math.h>

#include <ethernet.h>
#include <nctuns_api.h>
#include <packet.h>
#include <tcp.h>

#include "mac802_16_pmpss.h"
#include "scheduler.h"
#include "../common.h"
#include "../library.h"
#include "../mac_address.h"
#include "../timer_mngr.h"
#include "../../phy/ofdm_80216.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"

MODULE_GENERATOR(mac802_16_PMPSS);

mac802_16_PMPSS::mac802_16_PMPSS(
        uint32_t type, uint32_t id, plist* pl, const char* name)
: mac802_16SSbase(type, id, pl, name)
{
	int i;
	BASE_OBJTYPE(obj);

	vBind("LinkMode", &LinkMode);

	_maxqlen = 10000;
	LinkMode = NULL;

	LostULinterval = LostDLinterval = 600;	// Max: 600 ms, Table 340
	RngLastStatus = 3;
	TimingAdjust = 0;
	DownChID = UpChID = 1;
	ifsavedmm = NULL;

	pScheduler = new SSScheduler(this);

    BASE_OBJTYPE(mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16_PMPSS, T1);
	timer_mngr()->set_func_t(1u, this, mem_func);
	mem_func = POINTER_TO_MEMBER(mac802_16_PMPSS, T3);
	timer_mngr()->set_func_t(3u, this, mem_func);
	mem_func = POINTER_TO_MEMBER(mac802_16_PMPSS, T6);
	timer_mngr()->set_func_t(6u, this, mem_func);
	mem_func = POINTER_TO_MEMBER(mac802_16_PMPSS, T8);
	timer_mngr()->set_func_t(8u, this, mem_func);
	mem_func = POINTER_TO_MEMBER(mac802_16_PMPSS, T18);
	timer_mngr()->set_func_t(18u, this, mem_func);

	timerLostDL = new timerObj;
	timerLostUL = new timerObj;
	timerSendRnging = new timerObj;
	timerSendUL = new timerObj;

	obj = POINTER_TO_MEMBER(mac802_16_PMPSS, LostDLMAP);
	timerLostDL->setCallOutObj(this, obj);
	obj = POINTER_TO_MEMBER(mac802_16_PMPSS, LostULMAP);
	timerLostUL->setCallOutObj(this, obj);
	obj = POINTER_TO_MEMBER(mac802_16_PMPSS, SendRNGREQ);
	timerSendRnging->setCallOutObj(this, obj);
	obj = POINTER_TO_MEMBER(mac802_16_PMPSS, PacketScheduling);
	timerSendUL->setCallOutObj(this, obj);

	fecSNR[0] = 0.0;
	fecSNR[BPSK_1_2] = 6.4;
	fecSNR[QPSK_1_2] = 9.4;
	fecSNR[QPSK_3_4] = 11.2;
	fecSNR[QAM16_1_2] = 16.4;
	fecSNR[QAM16_3_4] = 18.2;
	fecSNR[QAM64_2_3] = 22.7;
	fecSNR[QAM64_3_4] = 24.4;

	UCDCfgCount = DCDCfgCount = 0;
	for (i = 0; i < 16; i++) {
		DCDProfile[i].used = 0;
		UCDProfile[i].used = 0;
	}
	UCDProfile[1].fec = 0;	// Initial Ranging use BPSK 1/2

	/* register variable */
	//REG_VAR("MAC", MacAddr);
	REG_VAR("MAC", address()->buf());
	REG_VAR("DCDProfile", &DCDProfile);
	REG_VAR("UCDProfile", &UCDProfile);

}

mac802_16_PMPSS::~mac802_16_PMPSS()
{
	printf("mac802_16_PMPSS::~mac802_16_PMPSS()\n");

	delete timerSendRnging;
	delete timerSendUL;
	delete timerLostDL;
	delete timerLostUL;
	delete ifsavedmm;
}

int mac802_16_PMPSS::init(void)
{
	State = ssInit;
	AttrARQ = 0;
	AttrCRC = 1;

	initRangingConnection = new ManagementConnection(initRangingCID);

	uint64_t tick_interval;
	MILLI_TO_TICK(tick_interval, 500);
	timer_mngr()->set_interval_t(18u, tick_interval);

	return mac802_16::init();
}

int mac802_16_PMPSS::recv(Event* ep)
{
	Packet *recvBurst, *UpPkt;
	struct hdr_generic *hg;
	struct mgmt_msg *mm;
	u_int64_t tick;

	int cid, len, plen, BurstLen;
	char *AdaptedBurst, *ptr;

	unsigned int crc;
	vector < ConnectionReceiver * >::iterator iter;

	assert(ep && (recvBurst = (Packet*)ep->DataInfo_));
	ep->DataInfo_ = NULL;
	freePacket(ep);

	memcpy(&LastSignalInfo, recvBurst->pkt_getinfo("phyInfo"), sizeof(LastSignalInfo));
	AdaptedBurst = (char *) recvBurst->pkt_sget();
	BurstLen = recvBurst->pkt_getlen();
//      printf("(%d) mac802_16_PMPSS::recv() (BurstLen = %d) @%lld\n", get_nid(), BurstLen, GetCurrentTime());

	for (ptr = AdaptedBurst;
	     ptr + sizeof(struct hdr_generic) < AdaptedBurst + BurstLen;
	     ptr += plen) {
		hg = (struct hdr_generic *) ptr;
		GHDR_GET_CID(hg, cid);
		GHDR_GET_LEN(hg, plen);
		if (hcs(ptr, sizeof(struct hdr_generic)) || plen == 0) {
			DEBUG("%s#%d: HCS not match, drop this burst\n",
			       __FILE__, __LINE__);
			break;
		}

		len = plen;

		DEBUG("%s#%d: Extract PDU: CID=%d LEN=%d\n", __FILE__,
		       __LINE__, cid, plen);

		if (hg->ci) {
			len -= 4;
			crc = crc32(0, ptr, len);
			if (memcmp(&crc, ptr + len, 4) != 0) {
//for(int z=0;z<len+4;z++)
//  printf("%02x ", ptr[z]&0xff);
				printf("CRC Error (%08x)\n", crc);
				continue;
			}
		}

		if (cid == broadcastCID) {
			mm = (struct mgmt_msg *) (ptr +
						  sizeof(struct
							 hdr_generic));

			if (mm->type == MG_DLMAP) {
				procDLMAP(mm, cid,
					  len -
					  sizeof(struct hdr_generic));
				resetTimerT(1);
				MILLI_TO_TICK(tick, LostDLinterval);
				timerLostDL->start(tick, 0);
				if ((State & ssDLParam) == 0) {
					State |= ssDLParam;
					INFO("->%d Enter DLParam\n",
					      get_nid());
				}
			}
			if ((State & ssDLParam) == 0)
				continue;

			if (mm->type == MG_DCD)
				procDCD(mm, cid,
					len - sizeof(struct hdr_generic));

			if (mm->type == MG_UCD) {
				procUCD(mm, cid,
					len - sizeof(struct hdr_generic));

				if ((State & ssULParam) == 0) {
					MILLI_TO_TICK(tick,
						      LostULinterval);
					timerLostUL->start(tick, 0);
					INFO("->%d Enter ULParam\n",
					      get_nid());
					State |= ssULParam;

					RngTime = RngStart - 1;
					RngLastStatus = 4;	// Rerange
					WaitForInitRangingOpportuniy();
				}
			}
			if (mm->type == MG_ULMAP)
				procULMAP(mm, cid,
					  len -
					  sizeof(struct hdr_generic));

		} else if (cid == initRangingCID) {
			mm = (struct mgmt_msg *) (ptr +
						  sizeof(struct
							 hdr_generic));
			if (mm->type == MG_RNGRSP) {
				if (procRNGRSP(mm, cid, len - sizeof(struct hdr_generic)))	// Ranging Ok
				{
					State |= ssRanging;
					if ((State & ssNegcap) == 0) {
						SendSBCREQ();
					}
				}
			}
		} else if (cid == BasicCID)	// MAC PDU for this Subscriber Station(SS).
		{
			mm = (struct mgmt_msg *) (ptr +
						  sizeof(struct
							 hdr_generic));
			if (mm->type == MG_SBCRSP)	// Fig.66
			{
				timer_mngr()->cancel_t(18);
				delete ifsavedmm;
				ifsavedmm = NULL;
				if (procSBCRSP(mm, cid, len - sizeof(struct hdr_generic)))	// Response OK
				{
					/*
					 * Here, we skip procedure SS authorization and key exchange (Sec 6.3.9.8).
					 */
					SendREGREQ();
					State |= ssNegcap;
				}
			}
		} else if (cid == PriCID)	// MAC PDU for this SS
		{
			mm = (struct mgmt_msg *) (ptr +
						  sizeof(struct
							 hdr_generic));
			if (mm->type == MG_REGRSP)	// Fig.69
			{
				timer_mngr()->cancel_t(6);
				delete ifsavedmm;
				ifsavedmm = NULL;
				if (1)	// Response OK
				{
					State |= ssRegister;
					if (0)	// Managed SS?
					{
						// Establish Secondary Management Connection
					}
				}
			}

			if (mm->type == MG_DSAREQ && (State & ssRegister)) {
				procDSAREQ(mm, cid,
					   len -
					   sizeof(struct hdr_generic));
			}
			if (mm->type == MG_DSAACK) {
				timer_mngr()->cancel_t(8);
				delete ifsavedmm;
				ifsavedmm = NULL;
				procDSAACK(mm, cid,
						len -
						sizeof(struct hdr_generic));
				if ((State & ssProv) == 0) {
					State |= ssProv;
				}
			}
		} else {
			for (iter = ReceiverList.begin();
			     iter != ReceiverList.end(); iter++) {
				if ((*iter)->getCid() == cid) {
					char *ptr2;
					(*iter)->insert(hg, len);
					while ((ptr2 = (*iter)->getPacket(len)) != NULL) {
						UpPkt = asPacket(ptr2, len);
						UpPkt->pkt_setflow(PF_RECV);
						Event* e(createEvent());
						e->DataInfo_ = UpPkt;
						put(e, recvtarget_);
					}
					break;
				}
			}
		}
	}
	delete recvBurst;
	return 1;
}

int mac802_16_PMPSS::send(Event * event)
{
	vector < DataConnection * >::iterator iter;
	Packet *Pkt;
	int len;

	GET_PKT(Pkt, event);
	event->DataInfo_ = NULL;

	len = Pkt->pkt_getlen();
//      printf("%d mac802_16_PMPSS::send() len = %d\n", get_nid(), len);

	for (iter = ConnectionList.begin(); iter != ConnectionList.end();
	     iter++) {
		if ((*iter)->nf_pending_packet() > static_cast<size_t>(_maxqlen))	// Only one data connection for SS
		{
			delete Pkt;
			break;
		}

		(*iter)->Insert(Pkt);
		break;
	}
	freePacket(event);

	return 1;
}

Connection *mac802_16_PMPSS::getConnection(u_int16_t cid)
{
	vector < ManagementConnection * >::iterator iter1;
	vector < DataConnection * >::iterator iter2;
	Connection *c;

	for (iter1 = MnConnections.begin(); iter1 != MnConnections.end();
	     iter1++) {
		c = *iter1;
		if (c->DataCid == cid)
			return c;
	}
	for (iter2 = ConnectionList.begin(); iter2 != ConnectionList.end();
	     iter2++) {
		c = *iter2;
		if (c->DataCid == cid)
			return c;
	}
	return NULL;
}


void mac802_16_PMPSS::PushPDUintoConnection(u_int16_t cid, ifmgmt * pdu)
{
	vector < ManagementConnection * >::iterator iter;
	ManagementConnection *c;

	if (cid == initRangingCID) {
		initRangingConnection->Insert(pdu);
		return;
	}
	printf("SS#%d %s(%d,)\n", get_nid(), __FUNCTION__, cid);
	for (iter = MnConnections.begin(); iter != MnConnections.end();
	     iter++) {
		c = (ManagementConnection *) * iter;
		if (c->DataCid == cid) {
			c->Insert(pdu);
			return;
		}
	}
}

u_int64_t mac802_16_PMPSS::computeSendTime(const OFDM_ULMAP_IE & ie,
					   int fastime)
{
	u_int64_t firstTick = 0;
	int64_t tadj;
	int nPSs, adjus;

	nPSs = fastime + ie.stime * 80;
	MICRO_TO_TICK(firstTick, PSsToMicro(nPSs));
	adjus = (int64_t) (TimingAdjust * Tb() / 256.0);
	tadj = (int64_t) (adjus * 1000.0 / TICK);	// TICK_TO_MICRO
	firstTick -= (GetCurrentTime() - LastSignalInfo.frameStartTime);

	if ((int64_t) (firstTick) > -tadj)
		return firstTick + tadj;
	return 0;
}

/*
 * Periodically transmit uplink burst.
 */
void mac802_16_PMPSS::PacketScheduling()
{
	Event *ep;
	BurstInfo *burstInfo = new BurstInfo;

	burstInfo->Collection = pScheduler->Scheduling();
	burstInfo->flag = PF_SEND;

	if (burstInfo->Collection) {
		ep = createEvent();
		ep->DataInfo_ = burstInfo;
		put(ep, sendtarget_);
	} else {
//              printf("No Burst for transmission\n");
		delete burstInfo;
	}
	return;
}

void mac802_16_PMPSS::T1()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);
}

void mac802_16_PMPSS::T3()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);
	RngLastStatus = 4;	// Rerange
	WaitForInitRangingOpportuniy();
}

void mac802_16_PMPSS::T6()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm =
	    new ifmgmt(saved_msg, ifsavedmm->getLen(),
		       ifsavedmm->getFLen());

	resetTimerT(6);
}

void mac802_16_PMPSS::T8()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);

	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm =
	    new ifmgmt(saved_msg, ifsavedmm->getLen(),
		       ifsavedmm->getFLen());

	resetTimerT(8);
}

void mac802_16_PMPSS::T18()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);
	PushPDUintoConnection(lastCID, ifsavedmm);
	ifsavedmm =
	    new ifmgmt(saved_msg, ifsavedmm->getLen(),
		       ifsavedmm->getFLen());

	resetTimerT(18);
}


void mac802_16_PMPSS::LostDLMAP()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);
}

void mac802_16_PMPSS::LostULMAP()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);
}

/*
 *	Processing DCD (Fig.XX)
 */
int mac802_16_PMPSS::procDCD(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	ifTLV *tmptlv;
	u_char t, profilebuffer[128], diuc, value;
	u_char fdlChID, fCfgCount;
	int tlen, frameNumber;

	ifmm = new ifmgmt((u_char *) recvmm, len, 2);
	ifmm->getField(0, 1, &fdlChID);
	ifmm->getField(1, 1, &fCfgCount);

	if (fCfgCount == (DCDCfgCount + 1) % 256) {
		DCDCfgCount = fCfgCount;
		for (diuc = 0; diuc < 16; diuc++) {
			DCDProfile[diuc].used = 0;
		}
		while ((t = ifmm->getNextType()) != 0) {
			if (t == 1) {
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(profilebuffer);
				diuc = profilebuffer[0];
				printf("DIUC = %d  ", diuc);
				tmptlv =
				    new ifTLV(profilebuffer + 1, tlen - 1);
				DCDProfile[diuc].used = 1;
				while ((t = tmptlv->getNextType()) != 0) {
					tmptlv->getNextValue(&value);
					switch (t) {
					case 150:
						printf("fec = %d\t",
						       value);
						DCDProfile[diuc].fec =
						    value;
						break;
					case 151:
						printf
						    ("exitThreshold = %d\t",
						     value);
						DCDProfile[diuc].
						    exitThreshold = value;
						break;
					case 152:
						printf
						    ("entryThreshold = %d\t",
						     value);
						DCDProfile[diuc].
						    entryThreshold = value;
						break;
					case 153:
						printf("tcs = %d\t",
						       value);
						DCDProfile[diuc].tcs =
						    value;
						break;
					}
				}
				printf("\n");
				delete tmptlv;
				continue;
			}
			switch (t) {
			case 14:
				ifmm->getNextValue(&value);
				printf("Frame Duration Code = %d\n",
				       value);
				break;
			case 15:
				frameNumber = 0;
				ifmm->getNextValue(&frameNumber);
				printf("This frame Number is %d\n",
				       frameNumber);
				break;
			}
		}
	}
	delete ifmm;

	return 1;
}

/*
 *  Wait for Initial Ranging opportunity (Fig.60)
 */
void mac802_16_PMPSS::WaitForInitRangingOpportuniy()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);
	if (++RngTime > RngEnd) {
		printf("Rng Exhausted\n");
		return;
	}
	RngCount = random() % (int) powl(2, RngTime);
	printf("RngCount = %d %d\n", RngCount, RngTime);
}

/*
 *	Processing UCD (Fig.XX)
 */
int mac802_16_PMPSS::procUCD(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	ifTLV *tmptlv;
	u_char t, profilebuffer[128], uiuc, value;
	u_char fCfgCount, fRngStart, fRngEnd, fReqStart, fReqEnd;
	u_int16_t tmp;

	int tlen;

	ifmm = new ifmgmt((u_char *) recvmm, len, 5);
	ifmm->getField(0, 1, &fCfgCount);
	ifmm->getField(1, 1, &fRngStart);
	ifmm->getField(2, 1, &fRngEnd);
	ifmm->getField(3, 1, &fReqStart);
	ifmm->getField(4, 1, &fReqEnd);
	RngStart = fRngStart;
	RngEnd = fRngEnd;
	ReqStart = fReqStart;
	ReqEnd = fReqEnd;

	if (fCfgCount == (UCDCfgCount + 1) % 256) {
		UCDCfgCount = fCfgCount;
		for (uiuc = 0; uiuc < 16; uiuc++) {
			UCDProfile[uiuc].used = 0;
		}
		while ((t = ifmm->getNextType()) != 0) {
			if (t == 1) {
				tlen = ifmm->getNextLen();
				ifmm->getNextValue(profilebuffer);
				uiuc = profilebuffer[0];
				printf("UIUC = %d  ", uiuc);
				tmptlv =
				    new ifTLV(profilebuffer + 1, tlen - 1);
				UCDProfile[uiuc].used = 1;
				while ((t = tmptlv->getNextType()) != 0) {
					tmptlv->getNextValue(&value);
					switch (t) {
					case 150:
						printf("fec = %d\t",
						       value);
						UCDProfile[uiuc].fec =
						    value;
						break;
					case 151:
						printf
						    ("power boost = %d\t",
						     value);
						UCDProfile[uiuc].
						    powerBoost = value;
						break;
					case 152:
						printf("tcs = %d\t",
						       value);
						UCDProfile[uiuc].tcs =
						    value;
						break;
					}
				}
				printf("\n");
				delete tmptlv;
				continue;
			}
			ifmm->getNextValue(&tmp);
			switch (t) {
			case 3:
				UCDBWOpp = tmp;
				printf("UCDBWOpp = %d\n", tmp);
				break;
			case 4:
				UCDRngOpp = tmp;
				printf("UCDRngOpp = %d\n", tmp);
				break;
			}
		}
	}
	delete ifmm;

	return 1;
}

/*
 *	Processing DL-MAP (Fig.XX)
 */
int mac802_16_PMPSS::procDLMAP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	char flag = 0;
	int i, pos;
	struct OFDM_DLMAP_IE ie;
	ntfyDLMAP *cmd = NULL;
	dlmapinfo info;
	int byteOffset, last, fec;
	int codedBlockSize[] = { 24, 48, 48, 96, 96, 144, 144 };
	char fdcdcount, fbsid[6];

	ifmm = new ifmgmt((u_char *) recvmm, len, len - 1);
	ifmm->getField(0, 1, &fdcdcount);
	ifmm->getField(1, 6, fbsid);
//      printf("  Get DL-MAP: DCD Count=%d,  BS ID=%02x:%02x:%02x:%02x:%02x:%02x\n", fdcdcount, fbsid[0], fbsid[1], fbsid[2], fbsid[3], fbsid[4], fbsid[5]);

	if (State && ssDLParam) {
		i = 1, pos = 7;
		byteOffset = 0, last = -1;
		do {
			ifmm->getField(pos, sizeof(ie), &ie);
			pos += sizeof(ie);
//                      printf("\t\tburst#%2d  CID=%5d, DIUC=%d, PreamblePresent=%2d, StartTime=%d\n", i, ie.cid, ie.diuc, ie.preamble, ie.stime);

			if (last != -1) {
				byteOffset +=
				    (ie.stime -
				     last) * codedBlockSize[fec];
			}

			if (ie.diuc == Extended) {

			}
			if (flag) {
				info.end = ie.stime;
				fec =
				    DCDProfile[static_cast <
					       int >(info.usage)].fec;
				flag = 0;
				if (!cmd)
					cmd = new ntfyDLMAP;
				cmd->info.push_back(info);
			}
			if (ie.cid == broadcastCID || getConnection(ie.cid))	// If broadcast (or my) cid
			{
				info.start = ie.stime;
				info.usage = ie.diuc;
				info.byteOffset = byteOffset;
				flag = 1;
			}
			last = ie.stime;
			fec = DCDProfile[static_cast < int >(ie.diuc)].fec;
			i++;
		} while (ie.diuc != EndOfMap);

		if (cmd)
			SendNTFYtoPHY(cmd);
	}

	delete ifmm;

	return 1;
}

/*
	Processing UL-MAP (Fig.XX)
 */
int mac802_16_PMPSS::procULMAP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	int i, pos, flag = 0;
	struct OFDM_ULMAP_IE ie;
//	Connection *c;

	char fupchid, fucdcount, fbsid[6];
	int fastime;		// Allocation Start Time
	u_int64_t firstTick = 0;


	ifmm = new ifmgmt((u_char *) recvmm, len, len - 1);
	ifmm->getField(0, 1, &fupchid);
	ifmm->getField(1, 1, &fucdcount);
	ifmm->getField(2, 4, &fastime);	// Allocation Start Time (in PS)
	ifmm->getField(1, 6, fbsid);

//      printf("  Get UL-MAP: UpChID=%d, UCD Count=%d, A-Start Time=%d\n", fupchid, fucdcount, fastime);
	i = 1, pos = 6;
	do {
		ifmm->getField(pos, sizeof(ie), &ie);
//              printf("\t\tburst#%2d  CID=%5d, S-Time=%d, SubchID=%d, UIUC=%d, Duration=%d, Midamble=%2d\n", i, ie.cid, ie.stime, ie.chidx, ie.uiuc, ie.duration, ie.midamble);
		pos += sizeof(ie);
		if (ie.uiuc == Extended) {

		} else if (ie.uiuc == Initial_Ranging) {
			if (RngLastStatus == 3 || RngLastStatus == 5) {
				// Skip
			} else if (ie.cid == BasicCID) {
				printf("Get Invited Initial Ranging\n");
				firstTick = computeSendTime(ie, fastime);
				timerSendRnging->start(firstTick, 0);
				savedRNGie = ie;
				flag = 1;
			} else if (ie.cid == broadcastCID && flag == 0) {
				printf("Rng Count Down = %d - %d\n",
				       RngCount, ie.duration / 5);
				if (RngCount >= ie.duration / 5) {
					RngCount -= ie.duration / 5;
				} else {
					firstTick =
					    computeSendTime(ie, fastime);
					timerSendRnging->start(firstTick,
							       0);
					savedRNGie = ie;
				}
			}
		} else if (ie.cid == BasicCID)	//  Grant For This Subscribe Station
		{
			// Schedule Transmission Here
			savedULie = ie;

			firstTick = computeSendTime(ie, fastime);
			timerSendUL->start(firstTick, 0);
		}

		i++;
	} while (ie.uiuc != EndOfMap);

	delete ifmm;

	return 1;
}


/*
	Processing RNG-RSP (Fig.61)
 */
int mac802_16_PMPSS::procRNGRSP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	int type;

	char fstatus = 1, fmac[6];
	int ftimingadj = 0;
	u_int16_t fbasicid = 0, fpricid = 0;

	ifmm = new ifmgmt((u_char *) recvmm, len, 1);

	while ((type = ifmm->getNextType()) != 0) {
		printf("Type = %d\t", type);
		switch (type) {
		case 1:
			ifmm->getNextValue(&ftimingadj);
			printf("Timing Adjust = %d\n", ftimingadj);
			break;
		case 2:
			break;
		case 4:
			ifmm->getNextValue(&fstatus);
			printf("Status = %d\n", fstatus);
			break;
		case 8:
			ifmm->getNextValue(fmac);
			printf("mac=%x %x %x %x %x %x\n", fmac[0], fmac[1],
			       fmac[2], fmac[3], fmac[4], fmac[5]);
			if (memcmp(fmac, address()->buf(), 6))	// Discard RNG-RSP not to this SS
				return 0;
			break;
		case 9:
			ifmm->getNextValue(&fbasicid);
			printf("Basic CID = %d\n", fbasicid);
			break;
		case 10:
			ifmm->getNextValue(&fpricid);
			printf("Primary CID = %d\n", fpricid);
			break;
		case 11:
			break;
		case 12:
			break;
		case 13:
			break;
		}
	}
	delete ifmm;


	TimingAdjust += ftimingadj;
	RngLastStatus = fstatus;
	timerSendRnging->cancel();
	timer_mngr()->cancel_t(3);
	if (RngLastStatus == 2)	// Abort
	{
		return 0;
	}

	BasicCID = fbasicid;
	PriCID = fpricid;
	if (RngLastStatus == 1)	// Continue
	{
		// Do something
		RngTime = RngStart - 1;
		WaitForInitRangingOpportuniy();

		return 0;
	} else			// Success
	{
		MnConnections.
		    push_back(new ManagementConnection(BasicCID));
		MnConnections.push_back(new ManagementConnection(PriCID));
	}

	return 1;
}

int mac802_16_PMPSS::procSBCRSP(struct mgmt_msg *recvmm, int cid, int len)
{

	return 1;
}

int mac802_16_PMPSS::procDSAREQ(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	ifTLV *tmptlv;
	int type1, type2;

	u_char fbuf[256], fhmac[21], fcsspec;
	u_int16_t ftransid, fdatacid;

	ifmm = new ifmgmt((u_char *) recvmm, len, 2);
	ifmm->getField(0, 2, &ftransid);
	printf("Transaction ID = %d\n", ftransid);

	while ((type1 = ifmm->getNextType()) != 0) {
		printf("Type = %d\t", type1);
		switch (type1) {
		case 145:
			ifmm->getNextValue(fbuf);
			{
				printf("Uplink Service Flow = %d\n",
				       ifmm->getNextLen());
				tmptlv =
				    new ifTLV(fbuf, ifmm->getNextLen());
				while ((type2 =
					tmptlv->getNextType()) != 0) {
					printf("  Type = %d\t", type2);
					switch (type2) {
					case 2:
						tmptlv->
						    getNextValue
						    (&fdatacid);
						printf("  Data CID = %d\n",
						       fdatacid);
						break;
					case 28:
						tmptlv->
						    getNextValue(&fcsspec);
						_CSType = fcsspec;
						printf
						    ("  CS Specification = %d\n",
						     fcsspec);
					}
				}
				delete tmptlv;
			}
			break;
		case 149:
			ifmm->getNextValue(fhmac);
			printf("HMAC = ...\n");
			break;
		}

	}
	delete ifmm;

	if (_CSType == 3 || _CSType == 5)
		ConnectionList.
		    push_back(new DataConnectionEthernet(fdatacid));
	else
		ConnectionList.push_back(new DataConnection(fdatacid));

	ReceiverList.push_back(new ConnectionReceiver(fdatacid));

	ifmm = new ifmgmt(MG_DSARSP, 3);	// Sec 11.13

	ifmm->appendField(2, ftransid);	// Transcation ID
	ifmm->appendField(1, 0);	// CC: OK, Table 382

	ifmm->appendTLV(149, 21, fhmac);	// HMAC

	ifmm->copyTo(saved_msg);
	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), 3);
	lastCID = cid;

	PushPDUintoConnection(cid, ifmm);
	resetTimerT(8);
	return 1;
}

int mac802_16_PMPSS::procDSAACK(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	ifTLV *tmptlv;
	int type1, type2;

	u_char fbuf[256], fhmac[21], fcsspec;
	u_int16_t ftransid, fdatacid;

	ifmm = new ifmgmt((u_char *) recvmm, len, 2);
	ifmm->getField(0, 2, &ftransid);
	printf("Transaction ID = %d\n", ftransid);

	while ((type1 = ifmm->getNextType()) != 0) {
		printf("Type = %d\t", type1);
		switch (type1) {
		case 145:
			ifmm->getNextValue(fbuf);
			{
				printf("Uplink Service Flow = %d\n",
				       ifmm->getNextLen());
				tmptlv =
				    new ifTLV(fbuf, ifmm->getNextLen());
				while ((type2 =
					tmptlv->getNextType()) != 0) {
					printf("  Type = %d\t", type2);
					switch (type2) {
					case 2:
						tmptlv->
						    getNextValue
						    (&fdatacid);
						printf("  Data CID = %d\n",
						       fdatacid);
						break;
					case 28:
						tmptlv->
						    getNextValue(&fcsspec);
						printf
						    ("  CS Specification = %d\n",
						     fcsspec);
					}
				}
				delete tmptlv;
			}
			break;
		case 149:
			ifmm->getNextValue(fhmac);
			printf("HMAC = ...\n");
			break;
		}

	}
	delete ifmm;
	printf
	    ("\n================%d==IF=PROGRAM=RUN=TO=HERE=THAT'S=NETWORK=ENTRY=OK=======================\n",
	     get_nid());
	printf
	    ("\n================%d==IF=PROGRAM=RUN=TO=HERE=THAT'S=NETWORK=ENTRY=OK=======================\n\n",
	     get_nid());

	return 1;
}

u_char mac802_16_PMPSS::selectDIUC(double recvSNR)
{
	int i, fec;

	if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0) {
		for (i = 11; i >= 1; i--) {
			fec = DCDProfile[i].fec;
			if (DCDProfile[i].used && recvSNR >= fecSNR[fec]) {
//                              printf("SS#%d Receive SNR = %lf dB and use profile %d, fec = %d\n", get_nid(), LastSignalInfo.SNR, i, DCDProfile[i].fec);
				return i;
			}
		}
		return RobustDIUC;
	}

	if (strcasecmp(LinkMode, "BPSK1/2") == 0)
		fec = BPSK_1_2;
	else if (strcasecmp(LinkMode, "QPSK1/2") == 0)
		fec = QPSK_1_2;
	else if (strcasecmp(LinkMode, "QPSK3/4") == 0)
		fec = QPSK_3_4;
	else if (strcasecmp(LinkMode, "16QAM1/2") == 0)
		fec = QAM16_1_2;
	else if (strcasecmp(LinkMode, "16QAM3/4") == 0)
		fec = QAM16_3_4;
	else if (strcasecmp(LinkMode, "64QAM2/3") == 0)
		fec = QAM64_2_3;
	else if (strcasecmp(LinkMode, "64QAM3/4") == 0)
		fec = QAM64_3_4;
	else
		printf("%s#%d: Warning Configure String LinkMode:%s\n",
		       __FILE__, __LINE__, LinkMode);


	for (i = 1; i <= 11; i++)
		if (DCDProfile[i].used && DCDProfile[i].fec == fec)
			return i;
	return 255;
}

int mac802_16_PMPSS::SendRNGREQ()
{
	int i;
	ifmgmt *ifmm;
	char *buf;
	vector < upBurst * >*BurstCollection = NULL;
	upBurst *burst = NULL;
	BurstInfo *burstInfo;
	Event *ep;
	vector < int >PduInfo;

	printf("%d mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);

	myDIUC = selectDIUC(LastSignalInfo.SNR);
	printf("SS#%d Receive SNR = %lf dB and use profile %d, fec = %d\n",
	       get_nid(), LastSignalInfo.SNR, myDIUC,
	       DCDProfile[myDIUC].fec);

	ifmm = new ifmgmt(MG_RNGREQ, 1);
	ifmm->appendField(1, DownChID);	// Downlink Channel ID   ö�  ifmm->appendTLV(1, 1, myDIUC);          // Requested Downlink Burst Profile
	ifmm->appendTLV(2, 6, address()->buf());	// SS MAC Address
	if (RngLastStatus == 1)	// Continue
		ifmm->appendTLV(3, 1, 3);	// Ranging Anomalies

	PushPDUintoConnection(initRangingCID, ifmm);

	if (1)			// Using Initial Ranging Interval for Ranging
	{
		burst =
		    new upBurst(initRangingCID, Initial_Ranging,
				savedRNGie.stime, savedRNGie.duration,
				UCDProfile[Initial_Ranging].fec);

		PduInfo.clear();
		i = initRangingConnection->GetInformation(&PduInfo);

		buf = new char[i];
		burst->payload = buf;
		burst->length =
		    initRangingConnection->EncapsulateAll(buf, i);

		BurstCollection = new vector < upBurst * >;
		BurstCollection->push_back(burst);

		burstInfo = new BurstInfo;

		burstInfo->Collection =
		    (vector < WiMaxBurst * >*)BurstCollection;
		burstInfo->flag = PF_SEND;

		ep = createEvent();
		ep->DataInfo_ = burstInfo;
		put(ep, sendtarget_);

		RngLastStatus = 5;	// Waiting For RNGRSP
		resetTimerT(3);
		printf("->%d Enter ssRanging\n", get_nid());
	}

	return 1;
}

int mac802_16_PMPSS::SendREGREQ()
{
	ifmgmt *ifmm = new ifmgmt(MG_REGREQ, 0);

	printf("%d Send REG-REQ\n", get_nid());
	ifmm->appendTLV(149, 21, 0U);	// HMAC tuple

	// For PMP operation, the REG-REQ shall contain following three TLVs:
	ifmm->appendTLV(2, 1, 0U);	// SS management support 11.7.2
	ifmm->appendTLV(3, 1, 0U);	// IP management mode 11.7.3
	ifmm->appendTLV(6, 2, 3);	// Number of uplink CID supported
	ifmm->appendTLV(10, 1, AttrARQ);	// ARQ Support
	ifmm->appendTLV(12, 1, AttrCRC);	// MAC CRC support

	ifmm->copyTo(saved_msg);
	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());
	lastCID = PriCID;

	PushPDUintoConnection(PriCID, ifmm);
	resetTimerT(6);
	return 1;
}

int mac802_16_PMPSS::SendSBCREQ()
{
	ifmgmt *ifmm;

	// Send SBC-REQ
	printf("%d Send SBC-REQ\n", get_nid());
	ifmm = new ifmgmt(MG_SBCREQ, 0);

	ifmm->appendTLV(1, 1, 0U);	// Bandwidth Allocation Support 11.8.1

	ifmm->appendTLV(2, 2, 10);	// Subscriber transition gaps 11.8.3.1
	ifmm->appendTLV(3, 4, 0xffffffff);	// Maximum transmit power 11.8.3.2
	ifmm->appendTLV(147, 1, 0xff);	// Current transmit power 11.8.3.3

	// OFDM specfic parameters 11.8.3.6
	ifmm->appendTLV(150, 1, 1);	// FFT-256
	ifmm->appendTLV(151, 1, 1);	// 64-QAM
	ifmm->appendTLV(152, 1, 1);	// 64-QAM
	ifmm->appendTLV(153, 1, 0U);
	ifmm->appendTLV(154, 1, 0U);

	ifmm->copyTo(saved_msg);
	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());
	ifsavedmm->getLen();
	lastCID = BasicCID;

	PushPDUintoConnection(BasicCID, ifmm);
	resetTimerT(18);

	return 1;
}
