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

#include <ethernet.h>
#include <nctuns_api.h>
#include <packet.h>

#include "mac802_16_pmpbs.h"
#include "scheduler.h"
#include "../library.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../timer_mngr.h"
#include "../../phy/ofdm_80216.h"
//#include "../connection.h"

#define VERBOSE_LEVEL   MSG_WARNING
#include "../verbose.h"

MODULE_GENERATOR(mac802_16_PMPBS);

mac802_16_PMPBS::mac802_16_PMPBS(uint32_t type, uint32_t id, plist* pl, const char* name)
: mac802_16(type, id, pl, name)
{
	int i, fec;
	BASE_OBJTYPE(obj);

	vBind("BSCfgFile", &BSCfgFile);
	vBind("CSTYPE", &CSTYPE);
	vBind("LinkMode", &LinkMode);

	_maxqlen = 10000;
	LinkMode = NULL;

	// Register variable
	REG_VAR("DCDProfile", &DCDProfile);
	REG_VAR("UCDProfile", &UCDProfile);
//      REG_VAR("MAC", mac);
	//REG_VAR("Frame_Number", &frame_number);

	// Initialize variables

	DCDinterval = UCDinterval = 2;	// Max: 10 second, Table 340

	for (i = 0; i < MAXSS; i++) {
		sslist[i] = NULL;
	}
	for (i = 0; i < MAXCONN; i++)
		conlist[i] = NULL;
	globalConnection[0] = broadcastConnection =
	    new BroadcastConnection(broadcastCID);
	globalConnection[1] = initRangingConnection =
	    new ManagementConnection(initRangingCID);
	globalConnection[2] = NULL;
	dlmapflag = 1;

	SfTable = new ProvisionedSfTable();
	ScTable = new ServiceClassTable();
	CrTable = new ClassifierRuleTable();

	/*
	 * FIXME: Why different?
	 */
	uint64_t tick_interval;
	SEC_TO_TICK(tick_interval, 3);
	timer_mngr()->set_interval_t(10u, tick_interval);

	timerDCD = new timerObj;
	timerUCD = new timerObj;
	downlinkTimer = new timerObj;
	assert(timerDCD && timerUCD && downlinkTimer);

	obj = POINTER_TO_MEMBER(mac802_16_PMPBS, generate_DCD);
	timerDCD->setCallOutObj(this, obj);

	obj = POINTER_TO_MEMBER(mac802_16_PMPBS, generate_UCD);
	timerUCD->setCallOutObj(this, obj);

	obj = POINTER_TO_MEMBER(mac802_16_PMPBS, PacketScheduling);
	downlinkTimer->setCallOutObj(this, obj);

	fecSNR[BPSK_1_2] = 6.4;
	fecSNR[QPSK_1_2] = 9.4;
	fecSNR[QPSK_3_4] = 11.2;
	fecSNR[QAM16_1_2] = 16.4;
	fecSNR[QAM16_3_4] = 18.2;
	fecSNR[QAM64_2_3] = 22.7;
	fecSNR[QAM64_3_4] = 24.4;

	frameNumber = 0;
	UCDCfgCount = DCDCfgCount = 1;

	for (i = 0; i < 16; i++) {
		DCDProfile[i].used = 0;
		UCDProfile[i].used = 0;
	}
	for (i = 1, fec = BPSK_1_2; i <= 11 && fec <= QAM64_3_4; i++, fec++)	// Table 235
	{
		// Table 360
		DCDProfile[i].used = 1;
		DCDProfile[i].frequency = 0;
		DCDProfile[i].fec = fec;
		DCDProfile[i].exitThreshold = 0xff;
		DCDProfile[i].entryThreshold = 0xff;
		DCDProfile[i].tcs = 0x00;
	}

	UCDProfile[Initial_Ranging].fec = BPSK_1_2;
	UCDProfile[REQ_Full].fec = BPSK_1_2;
	for (i = 5, fec = BPSK_1_2; i <= 12 && fec <= QAM64_3_4; i++, fec++)	// Table 244
	{
		// Table 354
		UCDProfile[i].used = 1;
		UCDProfile[i].fec = fec;
		UCDProfile[i].powerBoost = 0xff;
		UCDProfile[i].tcs = 0x00;
	}
}

mac802_16_PMPBS::~mac802_16_PMPBS()
{
	int i;
	INFO("mac802_16_PMPBS::~mac802_16_PMPBS()\n");

	delete globalConnection[0];
	delete globalConnection[1];
	delete globalConnection[2];
	delete downlinkTimer;
	for (i = 0; i < MAXSS; i++) {
		if (sslist[i]) {
			delete sslist[i];
			sslist[i] = NULL;
		}
	}
	for (i = 0; i < MAXCONN; i++) {
		if (conlist[i]) {
			delete conlist[i];
			conlist[i] = NULL;
		}
	}
}

int mac802_16_PMPBS::init(void)
{
	u_int64_t timeInTick, firstTick = 0;
	FILE *fp;
	char fn[256];
	SfTableReader *r1;
	SfClassTableReader *r2;
	ClassifierRuleTableReader *r3;
	INFO("mac802_16_PMPBS::init()\n");

	// Read configurations
	snprintf(fn, sizeof(fn), "%s%s", GetConfigFileDir(), BSCfgFile);
	if ((fp = fopen(fn, "r")) != NULL) {
		r1 = new SfTableReader(fp, SfTable);
		r2 = new SfClassTableReader(fp, ScTable);
		r3 = new ClassifierRuleTableReader(fp, CrTable);

//              SfTable->Dump();
//              ScTable->Dump();
//              CrTable->Dump();
		fclose(fp);
	} else
		WARN("No config file:%s exist. Skip it....\n", BSCfgFile);

	if (strcmp(CSTYPE, "IPv4") == 0)
		_CSType = csIPv4;
	else if (strcmp(CSTYPE, "Ethernet") == 0)
		_CSType = csEthernet;
	else {
		printf("CS-TYPE not assigned\n");
		exit(1);
	}

	// Set relevant timers
	SEC_TO_TICK(timeInTick, UCDinterval);
	timerUCD->start(firstTick, timeInTick);
	SEC_TO_TICK(timeInTick, DCDinterval);
	timerDCD->start(firstTick, timeInTick);
	MILLI_TO_TICK(timeInTick, 10);
	downlinkTimer->start(firstTick, timeInTick);

	pScheduler = new BSScheduler(this);

	return mac802_16::init();
}

int mac802_16_PMPBS::recv(Event * ep)
{
	Packet *recvBurst;
	struct hdr_generic *hg;
	struct mgmt_msg *recvmm;
	int cid = -1, len, plen, BurstLen;
	char *AdaptedBurst, *ptr;
	unsigned int crc;
	ssObject *targetSS;

	assert(ep && (recvBurst = (Packet *) ep->DataInfo_));
	ep->DataInfo_ = NULL;
	freePacket(ep);

	LastSignalInfo = (PHYInfo*)recvBurst->pkt_getinfo("phyInfo");

	AdaptedBurst = (char *) recvBurst->pkt_sget();
	BurstLen = recvBurst->pkt_getlen();
	//printf("%d mac802_16_PMPBS::recv() BurstLen = %d\n", get_nid(), BurstLen);

	for (ptr = AdaptedBurst;
	     ptr + sizeof(struct hdr_generic) < AdaptedBurst + BurstLen;
	     ptr += plen) {
		hg = (struct hdr_generic *) ptr;
		GHDR_GET_CID(hg, cid);
		GHDR_GET_LEN(hg, plen);
		if (hcs(ptr, sizeof(struct hdr_generic)) || plen == 0)
			break;

		len = plen;

		DEBUG("%s#%d: Extract PDU: CID=%d LEN=%d\n",
				__FILE__, __LINE__, cid, plen);
		if (hg->ci) {
			len -= 4;
			crc = crc32(0, ptr, len);
			if (memcmp(&crc, ptr + len, 4) != 0) {
				WARN("CRC Error (%08x)\n", crc);
				continue;;
			}
		}

		recvmm =
		    (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

		if (cid == initRangingCID || cid == broadcastCID) {
			DEBUG("cid = %d recvmm->type = %d %p %p\n",
					cid, recvmm->type, hg, recvmm);
			if (recvmm->type == MG_RNGREQ) {
				DEBUG("RNGREQ len = %d\n",
						len - sizeof(struct hdr_generic));
				procRNGREQ(recvmm, cid,
					   len - sizeof(struct hdr_generic));
			}
		} else if ((targetSS = getSSbycid(cid))) {
			targetSS->handle(recvmm, cid,
					 len - sizeof(struct hdr_generic));
		} else {
			vector < ConnectionReceiver * >::iterator iter;
			char *ptr2;

			for (iter = ReceiverList.begin();
			     iter != ReceiverList.end(); iter++) {
				if ((*iter)->getCid() == cid) {
					(*iter)->insert(hg, len);
					while ((ptr2 = (*iter)->getPacket(len))) {
						Packet *pkt;

						pkt = asPacket(ptr2, len);
//                                              if( DownlinkClassifier(pkt)==false )
						{
							/* pass to backhaul network on the other side */
							pkt->pkt_setflow(PF_RECV);
							Event* e(createEvent());
							e->DataInfo_ = pkt;
							put(e, recvtarget_);
						}
					}
					break;
				}
			}
			if (iter == ReceiverList.end()) {
				printf
				    ("BS can't receive this packet of CID(%d)\n",
				     cid);
			}
		}
	}
	delete recvBurst;

	return 1;
}

int mac802_16_PMPBS::send(Event * ep)
{
	Packet *pkt;

	GET_PKT(pkt, ep);
	INFO("(current time = %lld) mac802_16_PMPBS::send() (len = %d)\n",
	      GetCurrentTime(), pkt->pkt_getlen());

	if (DownlinkClassifier(pkt))
		ep->DataInfo_ = NULL;

	freePacket(ep);
	return 1;
}

Connection *mac802_16_PMPBS::CreateDataConnection(ssObject * pSS)
{

	int i;
	for (i = 0; i < MAXCONN; i++) {
		if (conlist[i])
			continue;
		if (_CSType == csIPv4)
			conlist[i] = new DataConnection(i + 2 * MAXSS + 1);
		else
			conlist[i] =
			    new DataConnectionEthernet(i + 2 * MAXSS + 1);

		pSS->DataConnections.push_back(conlist[i]);
		ReceiverList.
		    push_back(new ConnectionReceiver(i + 2 * MAXSS + 1));
		return conlist[i];
	}
	return NULL;
}

ServiceFlow *mac802_16_PMPBS::GetProvisionedFlow(u_char mac[6])
{
	return SfTable->GetServiceFlow(mac);
}

ServiceFlow *mac802_16_PMPBS::GetProvisionedFlow(u_int32_t flowId)
{
	return SfTable->GetServiceFlow(flowId);
}

ServiceClass *mac802_16_PMPBS::GetServiceClass(u_int32_t qosIndex)
{
	return ScTable->GetServiceClass(qosIndex);
}

/*  Implementation of Private Member Functions    */

ssObject *mac802_16_PMPBS::CreateSS(u_char * mac)
{
	int i;

	for (i = 0; i < MAXSS; i++) {
		if (sslist[i])
			continue;
		sslist[i] =
		    new ssObject(mac, i + 1, MAXSS + i + 1, 0, this);
		return sslist[i];
	}
	return NULL;
}

/*
 * Map each MAC SDU onto a particular connection (queue) for later transmission.
 */
bool mac802_16_PMPBS::DownlinkClassifier(Packet * p)
{
	struct ether_header *eh = (struct ether_header *) p->pkt_get();
	struct ip *ih;
	ClassifierRule *MatchRule = NULL;
	ServiceFlow *Sf = NULL;
	int i;

	DEBUG("%d mac802_16_PMPBS::%s\n", get_nid(), __FUNCTION__);

	if (_CSType == csEthernet
	    && ntohs(eh->ether_type) == ETHERTYPE_ARP) {
		printf
		    ("ARP Packet, but we should run Know In Advance Mode\n");
		exit(1);
	}

	ih = (struct ip *) (p->pkt_sget());
//      MatchRule = CrTable->Find(eh, ih);
	MatchRule = CrTable->Find(p->rt_gateway());
	if (MatchRule && (Sf = SfTable->GetServiceFlow(MatchRule->Sfid))) {
//              MatchRule->Dump();
//              Sf->Dump();
		for (i = 0; i < MAXCONN; i++) {
			if (conlist[i] && conlist[i]->DataCid == Sf->Cid) {
				if (conlist[i]->nf_pending_packet() > static_cast<size_t>(_maxqlen)) {
					printf
					    ("Connection(%d) is full (%d, %d)\n",
					     conlist[i]->DataCid,
					     conlist[i]->nf_pending_packet(), static_cast<size_t>(_maxqlen));
					return false;
				}
				conlist[i]->Insert(p);
				return true;
			}
		}
	}
	printf("Classifiy fail!! Drop (dst=%08lx)\n", ih->ip_dst);
	return false;
}

/*
 * Periodically transmit downlink subframe.
 */
void mac802_16_PMPBS::PacketScheduling()
{
	Event *ep;
	vector < downBurst * >::iterator it_p;


	BurstInfo *burstInfo = new BurstInfo;

	// Setting frame infomation per frame
	frameStartTime = GetCurrentTime();
	frameNumber++;

	fflush(stdout);
	burstInfo->Collection = pScheduler->Scheduling();
	burstInfo->flag = PF_SEND;

	if (burstInfo->Collection) {

		std::vector < downBurst * >*db_collection =
		    (std::vector < downBurst * >*)burstInfo->Collection;
		downBurst *burst = NULL;
		for (it_p = db_collection->begin();
		     it_p != db_collection->end(); it_p++) {

			burst = *it_p;
			NSLOBJ_DEBUG("Dump burst\n");
			DEBUG_FUNC(burst->Dump());

		}

		ep = createEvent();
		ep->DataInfo_ = burstInfo;
		put(ep, sendtarget_);
	} else {
		delete burstInfo;
	}

	return;
}

/*
 * Generate a MAC PDU containing UCD.
 */
void mac802_16_PMPBS::generate_UCD()
{
	pScheduler->generate_UCD();
	return;
}

/*
 * Generate a MAC PDU containing DCD.
 */
void mac802_16_PMPBS::generate_DCD()
{
	pScheduler->generate_DCD();
	generate_DL_MAP();
	return;
}

/*
 * Generate a MAC PDU containing DL-MAP.
 */
void mac802_16_PMPBS::generate_DL_MAP()
{
	DEBUG("%s#%d: Set DLMAP FLAG = true\n", __FILE__, __LINE__);
	dlmapflag = 1;
}

ssObject *mac802_16_PMPBS::getSSbycid(int cid)
{
	if (cid >= 1 && cid <= MAXSS)
		return sslist[cid - 1];
	else if (cid >= MAXSS + 1 && cid <= 2 * MAXSS)	// Primary CID of SS
		return sslist[cid - 1 - MAXSS];
	else if (cid >= 2 * MAXSS + 1 && cid <= 0xFEFE)	// Transport CID
		return NULL;
	return NULL;
}

/*
 * Figure 62
 */
void mac802_16_PMPBS::procRNGREQ(struct mgmt_msg *recvmm, int cid, int len)
{

	ifmgmt *ifmm;
	ssObject *pCurrentSS;
	int i, t = 0;

	u_char *ptr;
	u_char fprofile = 1, fmac[6];

	DEBUG("mac802_16_PMPBS::procRNGREQ(): mm=%p, cid=%d len=%d\n",
	       recvmm, cid, len);
	ptr = recvmm->msg;
	ifmm = new ifmgmt((u_char *) recvmm, len, 1);

	while ((t = ifmm->getNextType()) != 0) {

		DEBUG("Type = %d\t", t);
		switch (t) {

		case 1:
			ifmm->getNextValue(&fprofile);
			DEBUG("profile(diuc) = %d\n", fprofile);
			break;

		case 2:
			ifmm->getNextValue(fmac);
			DEBUG("mac=%x %x %x %x %x %x\n", fmac[0],
				 fmac[1], fmac[2], fmac[3], fmac[4],
				 fmac[5]);
			break;

		case 3:
			break;

		case 4:
			break;

		default:
			break;
		}
	}
	delete ifmm;

	for (i = 0; i < MAXSS; i++) {

		if (sslist[i] && memcmp(sslist[i]->address()->buf(), fmac, 6) == 0) {

			pCurrentSS = sslist[i];
			break;

		}
	}

	if (i == MAXSS)		// New Station
	{

		printf("[%u]%s: fprofile = %u .\n", get_nid(),
		       __FUNCTION__, fprofile);
		pCurrentSS = CreateSS(fmac);
		pCurrentSS->diuc = fprofile;

		// Add to Polled list
		PolledList.push_back(pCurrentSS);

		if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0) {

			pCurrentSS->uiuc = 5;	// Default: BPSK_1_2

			for (i = 12; i >= 5; i--) {

				t = UCDProfile[i].fec;

				if (UCDProfile[i].used
				    && LastSignalInfo->SNR >= fecSNR[t]) {

					printf
					    ("Receive SNR = %lf dB and use profile %d, fec = %d\n",
					     LastSignalInfo->SNR, i,
					     UCDProfile[i].fec);

					pCurrentSS->uiuc = i;
					break;
				}
			}
		} else if (strcasecmp(LinkMode, "Manual") == 0) {

			t = DCDProfile[fprofile].fec;

			for (i = 5; i <= 12; i++) {

				if (UCDProfile[i].used
				    && UCDProfile[i].fec == t) {

					pCurrentSS->uiuc = i;
					printf
					    ("Manual Mode: uiuc = %d fec=%d\n",
					     i, t);
					break;

				}
			}
		} else {
			printf("Warning Configure String LinkMode:%s\n", LinkMode);
		}

		printf("LinkMode=%s, Result DIUC=%d UIUC=%d\n", LinkMode,
		       pCurrentSS->diuc, pCurrentSS->uiuc);
	}

	int nPSs = pScheduler->SearchULIE(LastSignalInfo->ie);
	u_int64_t ticks;
	double diff;
	int timingAdjust;

	MICRO_TO_TICK(ticks, PSsToMicro(nPSs));
	ticks += frameStartTime;
	printf("ticks=%lld, frameStartTime=%lld\n", ticks, frameStartTime);

	TICK_TO_MICRO(diff, (int) (ticks - GetCurrentTime()));
	timingAdjust = (int) (diff * 256.0 / Tb());
	printf("currTime=%lld, diff=%lf, timingAdjust=%d\n",
	       GetCurrentTime(), diff, timingAdjust);

	ifmm = new ifmgmt(MG_RNGRSP, 1);	// Table 365
	ifmm->appendField(1, 1);	// Uplink channel ID
	if (timingAdjust > 20 || timingAdjust < -20) {
		ifmm->appendTLV(4, 1, 1);	// Ranging Status / 8bit / 1=continue, 2=abort, 3=Success, 4=rerange
		ifmm->appendTLV(1, 4, timingAdjust);	// Timing Adjust. Unit: 1/Fsample
	} else {
		PolledList.remove(pCurrentSS);

		ifmm->appendTLV(4, 1, 3);
	}
	ifmm->appendTLV(8, 6, pCurrentSS->address()->buf());	// SS MAC Address / 48bit /
	ifmm->appendTLV(9, 2, pCurrentSS->BasicCID);	// Basic CID / 16bit /
	ifmm->appendTLV(10, 2, pCurrentSS->PriCID);	// Primary CID / 16bit /

	initRangingConnection->Insert(ifmm);
}
