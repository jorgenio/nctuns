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

//#include <ethernet.h>
#include "mac802_16e_pmpbs.h"

using namespace mobileMac80216e;
using namespace mobileCommon;
using namespace mobileMacAddress;
using namespace mobileServiceFlow;
using namespace mobileTimer;

MODULE_GENERATOR(mac802_16e_PMPBS);

	mac802_16e_PMPBS::mac802_16e_PMPBS(uint32_t type, uint32_t id, plist *pl, const char *name)
: mac802_16e(type, id, pl, name)
{
	vBind("BSCfgFile", &BSCfgFile);
	vBind("NBRBSCfgFile", &NBRBSCfgFile);
	vBind("CSTYPE", &CSTYPE);
	vBind("LinkMode", &LinkMode);

	_maxqlen = 10000;
	LinkMode = NULL;
	_HO_support = 0x00;

	// Register variable
	REG_VAR("DCDProfile", &DCDProfile);
	REG_VAR("UCDProfile", &UCDProfile);

	// Initial variables
	// Max: 10 second, Spec 10.1 Table 342
	DCDinterval = 2; // sec
	UCDinterval = 2; // sec
	MOB_NBR_ADVinterval = 300; // ms

	for (int i = 0; i < MAXMS; i++)
	{
		mslist[i] = NULL;
	}

	for (int i = 0; i < MAXCONN; i++)
	{
		conlist[i] = NULL;
	}

	broadcastConnection     = new BroadcastConnection(broadcastCID);
	dlmapConnection         = new BroadcastConnection(broadcastCID);
	initRangingConnection   = new ManagementConnection(initRangingCID);

	assert(broadcastConnection && dlmapConnection && initRangingConnection);

	PsTable = new ProvisionedSfTable();
	ScTable = new ServiceClassTable();
	CrTable = new ClassifierRuleTable();
	NeighborBSList = new NeighborBSs();

	assert(PsTable && ScTable && CrTable && NeighborBSList);

	uint64_t tick_interval;
	SEC_TO_TICK(tick_interval, 3); // 3 sec
	timer_mngr()->set_interval_t(10u, tick_interval); // wait for transaction end timeout

	timerDCD         = new timerObj;
	timerUCD         = new timerObj;
	downlinkTimer    = new timerObj;
	timerMOB_NBR_ADV = new timerObj;
	timerSetldle     = new timerObj;

	assert(timerDCD && timerUCD && downlinkTimer && timerMOB_NBR_ADV && timerSetldle);

	BASE_OBJTYPE(mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPBS, generate_DCD);
	timerDCD->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPBS, generate_UCD);
	timerUCD->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPBS, PacketScheduling);
	downlinkTimer->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPBS, generate_MOB_NBRADV);
	timerMOB_NBR_ADV->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16e_PMPBS, changeStateToIdle);
	timerSetldle->setCallOutObj(this, mem_func);

	// Spec 16e. Table 338
	fecSNR[QPSK_1_2]    = 5;
	fecSNR[QPSK_3_4]    = 8;
	fecSNR[QAM16_1_2]   = 10.5;
	fecSNR[QAM16_3_4]   = 14;
	fecSNR[QAM64_1_2]   = 16;
	fecSNR[QAM64_2_3]   = 18;
	fecSNR[QAM64_3_4]   = 20;

	frameNumber = 0;
	DCDCfgCount = 1;
	UCDCfgCount = 1;
	ULoccupiedSymbols = 0;

	for (int i = 0; i < 16; i++)
	{
		DCDProfile[i].used = 0;
		UCDProfile[i].used = 0;
	}

	for (int i = 0, fec = QPSK_1_2; i <= 12 && fec <= QAM64_3_4; i++, fec++)    // Spec 8.4.5.3.1 Table 276
	{
		// Spec 11.4.2 Table 363
		DCDProfile[i].used  = 1;
		DCDProfile[i].fec   = fec;
	}

	UCDProfile[CDMA_BWreq_Ranging].used         = 1;
	UCDProfile[CDMA_BWreq_Ranging].fec          = BPSK_;    // 12
	UCDProfile[CDMA_BWreq_Ranging].rangRatio    = 0x00;
	UCDProfile[CDMA_Alloc_IE].used              = 1;
	UCDProfile[CDMA_Alloc_IE].fec               = QPSK_1_2; // 14
	UCDProfile[CDMA_Alloc_IE].rangRatio         = 0x00;

	for (int i = 1, fec = QPSK_1_2; i <= 10 && fec <= QAM64_3_4; i++, fec++)    // Spec 8.4.5.4.1 Table 288
	{
		// Spec 11.3.1.1 Table 357
		UCDProfile[i].used      = 1;
		UCDProfile[i].fec       = fec;
		UCDProfile[i].rangRatio = 0x00;
	}
}

mac802_16e_PMPBS::~mac802_16e_PMPBS()
{
	for (int i = 0; i < MAXMS; i++)
	{
		if (mslist[i] != NULL)
		{
			delete mslist[i];
			mslist[i] = NULL;
		}
	}

	for (int i = 0; i < MAXCONN; i++) 
	{
		if (conlist[i] != NULL)
		{
			delete conlist[i];
			conlist[i] = NULL;
		}
	}

	while (!PolledList.empty())
	{
		delete *(PolledList.begin());
		PolledList.erase(PolledList.begin());
	}

	while (!RangingCodeList.empty())
	{
		delete *(RangingCodeList.begin());
		RangingCodeList.erase(RangingCodeList.begin());
	}

	while (!ReceiverList.empty())
	{
		delete *(ReceiverList.begin());
		ReceiverList.erase(ReceiverList.begin());
	}

	delete broadcastConnection;
	delete dlmapConnection;
	delete initRangingConnection;
	delete PsTable;
	delete ScTable;
	delete CrTable;
	delete NeighborBSList;
	delete timerDCD;
	delete timerUCD;
	delete downlinkTimer;
	delete timerMOB_NBR_ADV;
	delete timerSetldle;
	delete pScheduler;
}

int mac802_16e_PMPBS::init(void)
{
	uint64_t timeInTick             = 0;
	uint64_t firstTick              = 0;
	FILE *fp                        = NULL;
	char fn[256]                    = "";
	SfTableReader *r1               = NULL;
	SfClassTableReader *r2          = NULL;
	ClassifierRuleTableReader *r3   = NULL;

	_HO_support = HHO;

	// Read configuration file
	snprintf(fn, sizeof(fn), "%s%s", GetConfigFileDir(), BSCfgFile);
	if ((fp = fopen(fn, "r")) != NULL)
	{
		r1 = new SfTableReader(fp, PsTable);
		r2 = new SfClassTableReader(fp, ScTable);
		r3 = new ClassifierRuleTableReader(fp, CrTable);

		delete r1;
		delete r2;
		delete r3;

		fclose(fp);
	}
	else
	{
		printf("No config file:%s exist. Skip it....\n", BSCfgFile);
	}

	// Parse Neighbor BS List
	parseCfgFile();

	// Check vBind
	if (strcmp(CSTYPE, "IPv4") == 0)
	{
		_CSType = csIPv4;
	}
	else if (strcmp(CSTYPE, "Ethernet") == 0)
	{
		_CSType = csEthernet;
	}
	else
	{
		printf("CS-TYPE not assigned\n");
		exit(1);
	}

	// Init relevant timers
	SEC_TO_TICK(timeInTick, UCDinterval);
	timerUCD->start(firstTick, timeInTick);

	SEC_TO_TICK(timeInTick, DCDinterval);
	timerDCD->start(firstTick, timeInTick);

	MILLI_TO_TICK(timeInTick, MOB_NBR_ADVinterval);
	timerMOB_NBR_ADV->start(firstTick, timeInTick);

	_frameDuCode = GET_REG_VAR(get_port(), "frameDuCode", int *);

	MILLI_TO_TICK(timeInTick, frameDuration[*_frameDuCode]); // frameDuration: 5 ms
	downlinkTimer->start(firstTick, timeInTick); // period timer

	pScheduler = new BS_16e_Scheduler(this);

	assert(pScheduler);

	return mac802_16e::init();
}


/* 
 * Parse the neighbor BS's configuration file
 */
void mac802_16e_PMPBS::parseCfgFile()
{
	char fn[255] = "";
	FILE *fp     = NULL;

	snprintf(fn, sizeof(fn), "%s%s", GetConfigFileDir(), NBRBSCfgFile);
	//printf("NBRBSCfgFile:%s\n", fn);

	if ((fp = fopen(fn, "r")) != NULL)
	{
		char buffer[128] = "";

		while (fgets(buffer, 128, fp) != NULL)
		{
			uint8_t nid     = 0;
			uint32_t chID   = 0;
			uint32_t mac[6] = {};
			char tmp[4]     = "";
			int tmp_i       = 0;
			int i           = 0;

			// Ignore comment
			if (buffer[0] == '#')
				continue;

			// NID
			for (;i < 128;i++)
			{
				if (buffer[i] == ' ' || buffer[i] == '\t')
				{
					nid = atoi(tmp);
					tmp_i = 0;
					i++;
					break;
				}
				else
				{
					tmp[tmp_i] = buffer[i];
					tmp_i++;
				}
			}

			// Channel ID
			for (;i < 128;i++)
			{
				if (buffer[i] == ' ' || buffer[i] == '\t')
				{
					chID = atoi(tmp);
					tmp_i = 0;
					i++;
					break;
				}
				else
				{
					tmp[tmp_i] = buffer[i];
					tmp_i++;
				}
			}

			// BS MAC Address
			char tmp_mac[18] = "";
			uint8_t real_mac[6] = "";

			while (!((buffer[i] == ' ') || (buffer[i] == '\n') || (buffer[i] == '\0')))
			{
				tmp_mac[tmp_i] = buffer[i];
				tmp_i++;
				i++;
			}

			sscanf(tmp_mac, "%x:%x:%x:%x:%x:%x", mac, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5);
			for (int j = 0;j < 6;j++)
			{
				real_mac[j] = mac[j];
			}

			if (nid != get_nid()) // not myself
			{
				// save to list
				printf("\e[1;33m[NID] %d, [Channel ID] %d, [MAC] %x:%x:%x:%x:%x:%x\e[0m\n", nid, chID,
						real_mac[0], real_mac[1], real_mac[2], real_mac[3], real_mac[4], real_mac[5]);
				NbrBS *nbrBS = new NbrBS(nid, real_mac);
				nbrBS->PreambleIndex = chID;
				NeighborBSList->nbrBSs_Index.push_back(nbrBS);
			}
		}

		fclose(fp);
	}
	else
	{
		fprintf(stderr, "No Neighbor BS list\n");
	}

}

int mac802_16e_PMPBS::recv(ePacket_ *epkt)
{
	Packet *recvPacket      = NULL;
	struct hdr_generic *hg  = NULL;
	struct mgmt_msg *recvmm = NULL;
	char *adaptedBurst      = NULL;
	char *ptr               = NULL;
	msObject *targetMS      = NULL;
	int cid                 = -1;
	int len                 = 0;
	int plen                = 0;
	int burstLen            = 0;
	int uiuc                = 0;
	unsigned int crc        = 0;

	recvPacket = (Packet *) epkt->DataInfo_;
	assert(epkt && recvPacket);

	epkt->DataInfo_ = NULL;
	freePacket(epkt);

	LastSignalInfo  = (PHYInfo *) recvPacket->pkt_getinfo("phyInfo");
	adaptedBurst    = (char *) recvPacket->pkt_sget();
	burstLen        = recvPacket->pkt_getlen();

	assert(LastSignalInfo);

	uiuc = LastSignalInfo->uiuc;

	if (uiuc == CDMA_BWreq_Ranging)
	{
		uint8_t codeIndex   = 0;
		uint8_t code[18]    = "";
		uint8_t usage       = 0;

		if (LastSignalInfo->symOffset < (ULoccupiedSymbols / 3) * 2)  // Initial or Handover Ranging Region
		{
			// two copy, random select one code
			if (random() % 2 == 0)
			{
				memcpy(code, adaptedBurst, 18);
			}
			else
			{
				memcpy(code, adaptedBurst + 18, 18);
			}
		}
		else    // Period Ranging or BW Request Region
		{
			memcpy(code, adaptedBurst, 18);
		}

		// search the index of the code
		codeIndex = getCodeIndex(code, &usage);

		if (codeIndex == 255)
		{
			; // ranging code not found
		}
		else
		{
			// Add to ranging MS list
			RangingCodeList.push_back(new RangingObject(codeIndex, frameNumber & 0xFF, LastSignalInfo->symOffset, LastSignalInfo->chOffset, usage, GetCurrentTime()));
		}
	}
	else
	{
		for (ptr = adaptedBurst;ptr + sizeof(struct hdr_generic) < adaptedBurst + burstLen;ptr += plen)
		{
			// Get generic mac header
			hg      = (struct hdr_generic *) ptr;
			cid     = GHDR_GET_CID(hg);
			plen    = GHDR_GET_LEN(hg);

			// None
			if (hcs_16e(ptr, sizeof(struct hdr_generic)) != 0)
			{
				break;
			}
			else if (plen == 0)
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
					printf("BS(%d) CRC Error (%08x)\n", get_nid(), crc);
					continue;;
				}
			}

			// map to management message
			recvmm = (struct mgmt_msg *) (ptr + sizeof(struct hdr_generic));

			if (cid == initRangingCID || cid == broadcastCID)
			{
				if (recvmm->type == MG_RNGREQ) // recv RNG-REQ
				{
					procRNGREQ(recvmm, cid, len - sizeof(struct hdr_generic));
				}
			}
			else if ((targetMS = getMSbycid(cid)) != NULL) // Basic CID or Primary CID
			{
				targetMS->handle(recvmm, cid, len - sizeof(struct hdr_generic));
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
				if (iter == ReceiverList.end())
				{
					printf("Time:%llu BS(%d) can't receive this packet of CID(%d)\n", GetCurrentTime(), get_nid(), cid);
				}
			}
		}
	}

	delete recvPacket;

	return 1;
}

int mac802_16e_PMPBS::send(ePacket_ *epkt)
{
	Packet *pkt = NULL;

	assert(epkt);
	pkt = (Packet *)epkt->DataInfo_;

	if (DownlinkClassifier(pkt) == false)
	{
		delete pkt;
	}

	epkt->DataInfo_ = NULL;
	freePacket(epkt);

	return 1;
}

Connection *mac802_16e_PMPBS::CreateDataConnection(msObject *pMS)
{
	for (int i = 0; i < MAXCONN; i++)
	{
		if (conlist[i] != NULL)
		{
			continue;
		}

		if (_CSType == csIPv4)
		{
			conlist[i] = new DataConnection(i + 2 * MAXMS + 1);
		}
		else
		{
			conlist[i] = new DataConnectionEthernet(i + 2 * MAXMS + 1);
		}

		pMS->DtConnections.push_back(conlist[i]);
		ReceiverList.push_back(new ConnectionReceiver(i + 2 * MAXMS + 1));
		return conlist[i];
	}
	return NULL;
}

/*
 * When MS performed handover, BS shall remove the previous resourse for that MS
 */
void mac802_16e_PMPBS::RemoveMS(msObject *pMS)
{
	list<DataConnection *>::iterator iterD;
	vector<ConnectionReceiver *>::iterator iterC;
	int cid = 0;

	for (int i = 0;i < MAXCONN;i++)
	{
		if (conlist[i] == NULL)
		{
			continue;
		}

		// Remove pMS->DtConnection <==> conlist
		for (iterD = pMS->DtConnections.begin();iterD != pMS->DtConnections.end(); iterD++)
		{
			if (conlist[i]->cid == (*iterD)->cid)
			{
				cid = conlist[i]->cid;

				delete conlist[i];
				conlist[i] = NULL;
				*iterD     = NULL;
				pMS->DtConnections.erase(iterD);

				break;
			}
		}

		// Remove ReceiverList of that cid
		for (iterC = ReceiverList.begin();iterC != ReceiverList.end(); iterC++)
		{
			if (cid == (*iterC)->getCid())
			{
				delete (*iterC);
				ReceiverList.erase(iterC);
				break;
			}
		}

		// Clear Input Queue of that cid

	}
}

ServiceFlow *mac802_16e_PMPBS::GetProvisionedFlow(uint8_t mac[6])
{
	return PsTable->GetServiceFlow(mac);
}

ServiceFlow *mac802_16e_PMPBS::GetProvisionedFlow(uint32_t flowId)
{
	return PsTable->GetServiceFlow(flowId);
}

ServiceClass *mac802_16e_PMPBS::GetServiceClass(uint32_t qosIndex)
{
	return ScTable->GetServiceClass(qosIndex);
}

msObject *mac802_16e_PMPBS::CreateMS(uint8_t *mac)
{
	for (int i = 0; i < MAXMS; i++)
	{
		if (mslist[i] == NULL)
		{
			mslist[i] = new msObject(mac, i + 1, MAXMS + i + 1, 0, this);
			return mslist[i];
		}
	}
	return NULL;
}

/*
 * Map each MAC SDU onto a particular connection (queue) for later transmission.
 */
bool mac802_16e_PMPBS::DownlinkClassifier(Packet *pkt)
{
	struct ether_header *eh     = (struct ether_header *)pkt->pkt_get();
	ClassifierRule *MatchRule   = NULL;
	ServiceFlow *Sf             = NULL;

	// Check Ether Header
	if (_CSType == csEthernet && ntohs(eh->ether_type) == ETHERTYPE_ARP)
	{
		printf("ARP Packet, but we should run Know In Advance Mode\n");
		exit(1);
	}

	// Find the matched rule (by gateway)
	MatchRule = CrTable->Find(pkt->rt_gateway());

	if (MatchRule != NULL)
	{
		Sf = PsTable->GetServiceFlow(MatchRule->Sfid);

		if (Sf != NULL)
		{
			for (int i = 0; i < MAXCONN; i++)
			{
				if (conlist[i] && conlist[i]->cid == Sf->cid)
				{
					if (conlist[i]->nf_pending_packet() > static_cast<size_t>(_maxqlen))
					{
						//printf("Connection(%d) is full (current=%d, max=%d)\n",
						//        conlist[i]->cid, conlist[i]->nf_pending_packet(), static_cast<size_t>(_maxqlen));
						return false;
					}
					else
					{
						//printf("BS(%d) recv pkt (num=%d)\n", get_nid(), conlist[i]->nf_pending_packet());
						conlist[i]->Insert(pkt);
						return true;
					}
				}
			}
		}
	}
	else // FIXME: if this packet is broadcast like MobileIP packets, how shall we do? (now insert to each conlist)
	{
		for (int i = 0; i < MAXCONN; i++)
		{
			if (conlist[i])
			{
				if (conlist[i]->nf_pending_packet() > static_cast<size_t>(_maxqlen))
				{
					//printf("Connection(%d) is full (current=%d, max=%d)\n",
					//        conlist[i]->cid, conlist[i]->nf_pending_packet(), static_cast<size_t>(_maxqlen));
					return false;
				}
				else
				{
					conlist[i]->Insert(pkt);
					return true;
				}
			}
		}
	}

	//printf("BS(%d) Classifiy fail!! Drop (dst=%08lx)\n", get_nid(), ((struct ip *)pkt->pkt_sget())->ip_dst);
	return false;
}

void mac802_16e_PMPBS::handleRangingCode()
{
	ifmgmt *ifmm            = NULL;
	uint32_t rangAttribute  = 0;
	int timingAdjust        = 0;
	int nPSs                = 0;
	uint64_t ticks          = 0;
	double diff             = 0.0;
	uint8_t rsv             = 0;
	list<RangingObject *>::iterator its1, its2;

	//printf("mac802_16e_PMPBS::handleRangingCode()\n");

	// Check if collision
	for (its1 = RangingCodeList.begin();its1 != RangingCodeList.end();its1++)
	{
		for (its2 = RangingCodeList.begin();its2 != RangingCodeList.end();its2++)
		{
			if (its1 == its2) // skip
				continue;

			if (((*its1)->rangingCodeIndex      == (*its2)->rangingCodeIndex)   &&
					((*its1)->rangingSymbol         == (*its2)->rangingSymbol)      &&
					((*its1)->rangingSubchannel     == (*its2)->rangingSubchannel)  &&
					((*its1)->rangingFrameNumber    == (*its2)->rangingFrameNumber) )
			{
				(*its1)->collision = true;
				(*its2)->collision = true;
			}
		}
	}

	for (its1 = RangingCodeList.begin();its1 != RangingCodeList.end();its1++)
	{
		if ((*its1)->collision == true)
			continue;

		if((*its1)->rangingUsage == INITIAL_RANGING || (*its1)->rangingUsage == HANDOVER_RANGING)
		{
			ifmm = new ifmgmt(MG_RNGRSP, 1);
			ifmm->appendField(1, rsv);

			// TLV infomation
			rangAttribute = ((*its1)->rangingSymbol      & 0x3FF) << 22 |
				((*its1)->rangingSubchannel  & 0x3F ) << 16 |
				((*its1)->rangingCodeIndex   & 0xFF ) << 8  |
				((*its1)->rangingFrameNumber & 0xFF );

			// Compute time adjustment
			nPSs = pScheduler->SearchULTime((*its1)->rangingSymbol) + symbolsToPSs(2); // over two symbols
			MICRO_TO_TICK(ticks, PSsToMicro(nPSs));
			ticks += frameStartTime;

			TICK_TO_MICRO(diff, (int) (ticks - (*its1)->recv_time));
			timingAdjust = (int) (diff * 1024.0 / Tb());

			if (timingAdjust > 10 || timingAdjust < -10) // FIXME: Need to check the value -10 ~ 10
			{
				printf("\e[1;36mTime:%llu BS(%d) timingAdjust = %d\e[0m\n", GetCurrentTime(), get_nid(), timingAdjust);
				ifmm->appendTLV(1, 4, timingAdjust); // unit: 1/Fs
				ifmm->appendTLV(4, 1, 1);   // Ranging Status: 1=continue
			}
			else
			{
				ifmm->appendTLV(4, 1, 3);   // Ranging Status: 3=success
				(*its1)->allocated = true;  // BS shall provide BW allocation using CDMA_Allocation_IE
			}
			ifmm->appendTLV(150, 4, rangAttribute);     // Ranging Code Attributes

			initRangingConnection->Insert(ifmm);    // using Initial Ranging Connection
		}
		else if ((*its1)->rangingUsage == PERIOD_RANGING)
		{
			ifmm = new ifmgmt(MG_RNGRSP, 1);
			ifmm->appendField(1, rsv);

			// TLV infomation
			rangAttribute = ((*its1)->rangingSymbol      & 0x3FF) << 22 |
				((*its1)->rangingSubchannel  & 0x3F ) << 16 |
				((*its1)->rangingCodeIndex   & 0xFF ) << 8  |
				((*its1)->rangingFrameNumber & 0xFF );

			// Compute time adjustment
			nPSs = pScheduler->SearchULTime((*its1)->rangingSymbol) + symbolsToPSs(1); // over one symbols
			MICRO_TO_TICK(ticks, PSsToMicro(nPSs));
			ticks += frameStartTime;

			TICK_TO_MICRO(diff, (int) (ticks - (*its1)->recv_time));
			timingAdjust = (int) (diff * 1024.0 / Tb());

			if (timingAdjust > 10 || timingAdjust < -10) // FIXME: Need to check the value -10 ~ 10
			{
				printf("\e[1;36mTime:%llu BS(%d) timingAdjust = %d\e[0m\n", GetCurrentTime(), get_nid(), timingAdjust);
				ifmm->appendTLV(1, 4, timingAdjust); // unit: 1/Fs
				ifmm->appendTLV(4, 1, 1);   // Ranging Status: 1=continue
			}
			else
			{
				ifmm->appendTLV(4, 1, 3);   // Ranging Status: 3=success
			}
			ifmm->appendTLV(150, 4, rangAttribute);     // Ranging Code Attributes

			initRangingConnection->Insert(ifmm);    // using Initial Ranging Connection
		}
		else if ((*its1)->rangingUsage == BW_REQUEST)
		{
			;
		}
		else
		{
			printf("Impossible Ranging Usage\n");
		}
	}
}

/*
 * Periodically transmit downlink subframe.
 */
void mac802_16e_PMPBS::PacketScheduling()
{
	ePacket_ *epkt          = NULL;
	BurstInfo *burstInfo    = NULL;

	//printf("Time:%llu ==================BS(%d) PacketScheduling==================\n", GetCurrentTime(), get_nid());

	// Before scheduling, we need to check the ranging code
	handleRangingCode();

	/* Update MS status (scanning or handover) */ // FIXME: only consider that iteration times = 1
	for (int i = 0;i < MAXMS;i++)
	{
		msObject *pMS = mslist[i];

		if (pMS == NULL)
			continue;

		if (pMS->ResourceStatus == NeedClear) // MS resource need to remove
		{
			printf("\e[1;35m======= MS status: NeedClear ... BS(%d) remove this MS now ========\e[0m\n", get_nid());
			RemoveMS(mslist[i]);

			delete mslist[i];
			mslist[i] = NULL;
			continue;
		}

		if (pMS->ScanFlag == true)
		{
			if (pMS->ScanTimes != pMS->ScanDuration) // continue scanning
			{
				pMS->ScanTimes++;
			}
			else // end scanning
			{
				pMS->ScanTimes = 0;
				pMS->ScanFlag = false;
			}
		}
		else
		{
			if (pMS->ChangeToScanCnt >= 0)
			{
				pMS->ChangeToScanCnt--;
			}
			else if (pMS->ChangeToScanCnt == -1) // change to scanning
			{
				pMS->ScanFlag = true;
				pMS->ScanTimes++;
				pMS->ChangeToScanCnt = -2; // don't care
			}
			else // continue normal operation
			{
				; // normal operation
			}
		}
	}

	burstInfo = new BurstInfo;

	// Set per frame infomation
	frameStartTime = GetCurrentTime();
	frameNumber++;

	// Scheduling
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

	// Erase RangingCodeList
	while (!RangingCodeList.empty())
	{
		delete *(RangingCodeList.begin());
		RangingCodeList.erase(RangingCodeList.begin());
	}

	// Set the state of PHY to Busy to sending packets
	((OFDMA_PMPBS *)sendtarget_->bindModule())->setStateBusy();

	// Start a timer to change state
	uint64_t ticks = 0;
	MICRO_TO_TICK(ticks, PSsToMicro(pScheduler->ULallocStartTime));
	timerSetldle->start(ticks, 0);
}

/*
 * Generate a MAC PDU containing UCD.
 */
void mac802_16e_PMPBS::generate_UCD()
{
	pScheduler->generate_UCD();
	return;
}

/*
 * Generate a MAC PDU containing DCD.
 */
void mac802_16e_PMPBS::generate_DCD()
{
	pScheduler->generate_DCD();
	return;
}

/*
 * Generate a MAC PDU containing MOB_NBR-ADV
 */
void mac802_16e_PMPBS::generate_MOB_NBRADV()
{
	pScheduler->generate_MOB_NBRADV();
	return;
}

msObject *mac802_16e_PMPBS::getMSbycid(int cid)
{
	if (cid >= 1 && cid <= MAXMS)   // Basic CID of MS
	{
		return mslist[cid - 1];
	}
	else if (cid >= MAXMS + 1 && cid <= 2 * MAXMS)    // Primary CID of MS
	{
		return mslist[cid - 1 - MAXMS];
	}
	else if (cid >= 2 * MAXMS + 1 && cid <= 0xFE9F)    // Transport CID
	{
		return NULL;
	}
	else
	{
		return NULL;
	}
}

void mac802_16e_PMPBS::saveDLMAP(char *dstbuf, int len)
{
	((OFDMA_PMPBS *)sendtarget_->bindModule())->saveDLMAP(dstbuf, len);
}

void mac802_16e_PMPBS::changeStateToIdle()
{
	((OFDMA_PMPBS *)sendtarget_->bindModule())->setStateIdle();
}

void mac802_16e_PMPBS::saveDLsymbols(int Nsymbols)
{
	((OFDMA_PMPBS *)sendtarget_->bindModule())->setDLsymbols(Nsymbols);
}

void mac802_16e_PMPBS::saveULsymbols(int Nsymbols)
{
	ULoccupiedSymbols = Nsymbols;
	((OFDMA_PMPBS *)sendtarget_->bindModule())->setULsymbols(Nsymbols);
}

/*
 * Figure 62
 */
void mac802_16e_PMPBS::procRNGREQ(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm_req        = NULL;
	ifmgmt *ifmm_rsp        = NULL;
	msObject *pCurrentMS    = NULL;
	int i                   = 0;
	int type                = 0;
	int fec                 = 0;
	uint8_t fprofile        = 1;
	uint8_t fmac[6]         = "";
	uint8_t rsv             = 0;
	uint8_t fanomalies      = 0;
	int nPSs                = 0;
	uint64_t ticks          = 0;
	double diff             = 0.0;
	int timingAdjust        = 0;

	/* Extract fields*/
	ifmm_req = new ifmgmt((uint8_t *) recvmm, len, 1);
	ifmm_req->extractBitField( 8, &rsv);

	while ((type = ifmm_req->getNextType()) != 0)
	{
		switch (type) {
			case 1:
				ifmm_req->getNextValue(&fprofile);
				break;

			case 2:
				ifmm_req->getNextValue(fmac);
				break;

			case 3:
				ifmm_req->getNextValue(&fanomalies);

			default:
				break;
		}
	}
	delete ifmm_req;

	/* Get MS or a new MS */
	for (i = 0; i < MAXMS; i++)
	{
		if (mslist[i] && memcmp(mslist[i]->address()->buf(), fmac, 6) == 0)
		{
			pCurrentMS = mslist[i];
			break;
		}
	}

	if (i == MAXMS)        // New Station
	{
		pCurrentMS = CreateMS(fmac);
		if (pCurrentMS == NULL)
		{
			printf("CreateMS error: pCurrentMS is NULL\n");
			return;
		}
		pCurrentMS->diuc = fprofile;

		// Add to Polled list
		PolledList.push_back(pCurrentMS);

		// Decide Current MS's uiuc depend on LinkMode
		if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
		{
			pCurrentMS->uiuc = 1;    // Default: QPSK_1_2

			for (i = 10; i >= 1; i--)
			{
				fec = UCDProfile[i].fec;
				if (UCDProfile[i].used && LastSignalInfo->SNR >= fecSNR[fec])
				{
					pCurrentMS->uiuc = i;
					break;
				}
			}
			//printf("BS(%d) Receive SNR=%lf dB and use uiuc=%d (fec=%d)\n", get_nid(), LastSignalInfo->SNR, pCurrentMS->uiuc, UCDProfile[i].fec);
		}
		else if (strcasecmp(LinkMode, "Manual") == 0)  // if LinkMode=Manual, DL and UL use the same profile
		{
			fec = DCDProfile[fprofile].fec;
			for (i = 1; i <= 10; i++)
			{
				if (UCDProfile[i].used && UCDProfile[i].fec == fec)
				{
					pCurrentMS->uiuc = i;
					break;
				}
			}
			//printf("BS(%d) Manual Mode: uiuc=%d(fec=%d)\n", get_nid(), pCurrentMS->uiuc, fec);
		}
		else
		{
			printf("Warning Configure String LinkMode:%s\n", LinkMode);
		}
		printf("\e[32mTime:%llu BS(%d) Accept New MS:LinkMode=%s (SNR=%lf dB), DIUC=%d UIUC=%d\e[0m\n", GetCurrentTime(), get_nid(), LinkMode, LastSignalInfo->SNR, pCurrentMS->diuc, pCurrentMS->uiuc);
	}

	/* Compute time adjustment */
	if (LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC > ULoccupiedSymbols)
	{
		nPSs = pScheduler->SearchULTime(0) + symbolsToPSs(ULoccupiedSymbols);
	}
	else
	{
		nPSs = pScheduler->SearchULTime(0) + symbolsToPSs(LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC);
	}
	MICRO_TO_TICK(ticks, PSsToMicro(nPSs));
	ticks += frameStartTime;

	TICK_TO_MICRO(diff, (int) (ticks - GetCurrentTime()));
	timingAdjust = (int) (diff * 1024.0 / Tb());

	/* Generate RNG-RSP */
	ifmm_rsp = new ifmgmt(MG_RNGRSP, 1);    // Table 365
	ifmm_rsp->appendField(1, rsv);

	/*
	 * TLV encoded information
	 */
	if (timingAdjust > 10 || timingAdjust < -10) // FIXME: Need to check the value -10 ~ 10
	{
		printf("\e[1;36mTime:%llu BS(%d) timingAdjust = %d\e[0m\n", GetCurrentTime(), get_nid(), timingAdjust);
		ifmm_rsp->appendTLV(4, 1, 1);            // Ranging Status / 8bit / 1=continue, 2=abort, 3=Success
		ifmm_rsp->appendTLV(1, 4, timingAdjust); // Timing Adjust. Unit: 1/Fsample
	}
	else
	{
		PolledList.remove(pCurrentMS);
		ifmm_rsp->appendTLV(4, 1, 3);  // Success
	}
	ifmm_rsp->appendTLV( 8, 6, pCurrentMS->address()->buf());      // MS MAC Address
	ifmm_rsp->appendTLV( 9, 2, pCurrentMS->BasicCID);              // Basic CID
	ifmm_rsp->appendTLV(10, 2, pCurrentMS->PriCID);                // Primary CID
	ifmm_rsp->appendTLV(17, 1, pCurrentMS->ServerLevelPredict);    // Server Level
	ifmm_rsp->appendTLV(26, 2, pCurrentMS->PeriodRangingInterval); // Next Periodic Ranging in frames

	initRangingConnection->Insert(ifmm_rsp);
}
