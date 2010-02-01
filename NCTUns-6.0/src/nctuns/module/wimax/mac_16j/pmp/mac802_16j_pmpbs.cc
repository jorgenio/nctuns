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
#include "mac802_16j_pmpbs.h"

using namespace mobileRelayMac80216j;
using namespace mobileRelayCommon;
using namespace mobileRelayMacAddress;
using namespace mobileRelayServiceFlow;
using namespace mobileRelayTimer;


MODULE_GENERATOR(mac802_16j_PMPBS);

	mac802_16j_PMPBS::mac802_16j_PMPBS(uint32_t type, uint32_t id, plist *pl, const char *name)
: mac802_16j(type, id, pl, name)
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

	for (int i = 0; i < MAXRS; i++)
        {
                rslist[i] = NULL;
        }

	for (int i = 0; i < MAXCONN; i++)
	{
		conlist[i] = NULL;
	}

	broadcastConnection     = new BroadcastConnection(broadcastCID);
	initRangingConnection   = new ManagementConnection(initRangingCID);

	assert(broadcastConnection && initRangingConnection);

	PsTable = new ProvisionedSfTable();
	ScTable = new ServiceClassTable();
	CrTable = new ClassifierRuleTable();
	NeighborBSList = new NeighborBSs_MR();

	assert(PsTable && ScTable && CrTable && NeighborBSList);

	uint64_t tick_interval;
	SEC_TO_TICK(tick_interval, 3); // 3 sec
	timer_mngr()->set_interval_t(10u, tick_interval); // wait for transaction end timeout

	timerDCD         = new timerObj;
	timerUCD         = new timerObj;
	downlinkTimer    = new timerObj;
	timerMOB_NBR_ADV = new timerObj;
	timerSetldle     = new timerObj;
	timerSelectPath = new timerObj;

	assert(timerDCD && timerUCD && downlinkTimer && timerMOB_NBR_ADV && timerSetldle && timerSelectPath);

	BASE_OBJTYPE(mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_PMPBS, generate_DCD);
	timerDCD->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_PMPBS, generate_UCD);
	timerUCD->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_PMPBS, PacketScheduling);
	downlinkTimer->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_PMPBS, generate_MOB_NBRADV);
	timerMOB_NBR_ADV->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_PMPBS, changeStateToIdle);
	timerSetldle->setCallOutObj(this, mem_func);

	mem_func = POINTER_TO_MEMBER(mac802_16j_PMPBS, SelectMSsPath);
        timerSelectPath->setCallOutObj(this, mem_func);

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
	EndtoEndMetric = 0;
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
		switch(fec)
		{
			case QPSK_1_2:
				DCDProfile[i].weight = 1;
				break;
			case QPSK_3_4:
				DCDProfile[i].weight = 0.67;
				break;
			case QAM16_1_2:
				DCDProfile[i].weight = 0.5;
				break;
        		case QAM16_3_4:
				DCDProfile[i].weight = 0.34;
                                break;
       	 	        case QAM64_1_2:
				DCDProfile[i].weight = 0.34;
                                break;
        		case QAM64_2_3:
				DCDProfile[i].weight = 0.25;
                                break;
        		case QAM64_3_4:
				DCDProfile[i].weight = 0.22;
                                break;
			default:
				DCDProfile[i].weight = 2;
		}
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
		switch(fec)
                {
                        case QPSK_1_2:
                                UCDProfile[i].weight = 1;
                                break;
                        case QPSK_3_4:
                                UCDProfile[i].weight = 0.67;
                                break;
                        case QAM16_1_2:
                                UCDProfile[i].weight = 0.5;
                                break;
                        case QAM16_3_4:
                                UCDProfile[i].weight = 0.34;
                                break;
                        case QAM64_1_2:
                                UCDProfile[i].weight = 0.34;
                                break;
                        case QAM64_2_3:
                                UCDProfile[i].weight = 0.25;
                                break;
                        case QAM64_3_4:
                                UCDProfile[i].weight = 0.22;
                                break;
                        default:
                                UCDProfile[i].weight = 2;
                }
	}
}

mac802_16j_PMPBS::~mac802_16j_PMPBS()
{
	for (int i = 0; i < MAXMS; i++)
	{
		if (mslist[i] != NULL)
		{
			delete mslist[i];
			mslist[i] = NULL;
		}
	}

	for (int i = 0; i < MAXRS; i++)
        {
                if (rslist[i] != NULL)
                {
                        delete rslist[i];
                        rslist[i] = NULL;
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

	while (!ms_PolledList.empty())
	{
		delete *(ms_PolledList.begin());
		ms_PolledList.erase(ms_PolledList.begin());
	}

	while (!RangingCodeList.empty())
	{
		delete *(RangingCodeList.begin());
		RangingCodeList.erase(RangingCodeList.begin());
	}
	
	while (!MS_pendingRangingCodeList.empty())
        {
                delete *(MS_pendingRangingCodeList.begin());
                MS_pendingRangingCodeList.erase(MS_pendingRangingCodeList.begin());
        }

	while (!ReceiverList.empty())
	{
		delete *(ReceiverList.begin());
		ReceiverList.erase(ReceiverList.begin());
	}

	delete broadcastConnection;
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
	delete timerSelectPath;
}

int mac802_16j_PMPBS::init(void)
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
	SEC_TO_TICK(timeInTick, UCDinterval);			//2sec
	timerUCD->start(firstTick, timeInTick);

	SEC_TO_TICK(timeInTick, DCDinterval);			//2sec
	timerDCD->start(firstTick, timeInTick);

	MILLI_TO_TICK(timeInTick, MOB_NBR_ADVinterval);		//300ms
	timerMOB_NBR_ADV->start(firstTick, timeInTick);
	
	_frameDuCode = GET_REG_VAR(get_port(), "frameDuCode", int *);
	
	// Spec 802.16j_D5 10.1 Table 550
	MR_RNGREP_interval = 6 *frameDuration[*_frameDuCode];  //ms     , 6 frames
	//printf("MR_RNGREP_interval =%d\n",MR_RNGREP_interval);

	MILLI_TO_TICK(timeInTick, frameDuration[*_frameDuCode]); // frameDuration: 5 ms
	downlinkTimer->start(firstTick, timeInTick); // period timer

	pScheduler = new BS_16j_Scheduler(this);

	assert(pScheduler);

	return mac802_16j::init();
}


/* 
 * Parse the neighbor BS's configuration file
 */
void mac802_16j_PMPBS::parseCfgFile()
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
			/* storage Neighbor BS node ID && channel ID into NBRList */
			if (nid != get_nid()) // not myself
			{
				// save to list
				printf("\e[1;33m[NID] %d, [Channel ID] %d, [MAC] %x:%x:%x:%x:%x:%x\e[0m\n", nid, chID,
						real_mac[0], real_mac[1], real_mac[2], real_mac[3], real_mac[4], real_mac[5]);
				NbrBS_MR *nbrBS = new NbrBS_MR(nid, real_mac);
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

int mac802_16j_PMPBS::recv(ePacket_ *epkt)
{
	Packet *recvPacket      = NULL;
	struct hdr_generic *hg  = NULL;
	struct mgmt_msg *recvmm = NULL;
	char *adaptedBurst      = NULL;
	char *ptr               = NULL;
	msObject_mr *targetMS   = NULL;
	rsObject    *targetRS	= NULL;
	int cid                 = -1;
	int len                 = 0;
	int plen                = 0;
	int burstLen            = 0;
	int uiuc                = 0;
	unsigned int crc        = 0;
	uint64_t	timeTick = 0;
	int fec = 0;
	int i = 0;

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
		//printf("symboloffset = %d, ULAccessoccupiedSymbols =%d\n", LastSignalInfo->symOffset ,ULAccessoccupiedSymbols);
		if (LastSignalInfo->symOffset < (ULAccessoccupiedSymbols / 3) * 2)  // Initial or Handover Ranging Region
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
		else if (codeIndex > 172 && codeIndex < 215 )	//RS initial ranging code region
		{
			// Add RS to ranging list
			RangingCodeList.push_back(new RangingObject(codeIndex, frameNumber & 0xFF, LastSignalInfo->symOffset, LastSignalInfo->chOffset, usage, GetCurrentTime(), LastSignalInfo->SNR, pScheduler->ULallocStartTime, 0, 0,RobustUIUC));
		}
		else	//MS initial ranging/period ranging need delay 6 * frameduration for access station selection
		{
			if (LastSignalInfo->MSBcid > 510 && LastSignalInfo->MSBcid < 765)	// RS period ranging code
			{
				RangingCodeList.push_back(new RangingObject(codeIndex, frameNumber & 0xFF, LastSignalInfo->symOffset, LastSignalInfo->chOffset, usage, GetCurrentTime(), LastSignalInfo->SNR, pScheduler->ULallocStartTime, 0, LastSignalInfo->MSBcid, RobustUIUC));

				targetRS = getRSbycid(LastSignalInfo->MSBcid);
                                if (targetRS == NULL)
                                {
                                        printf("***%d::%s:Getting RS(mcid=%d) object failed!***\n", __LINE__, __func__, LastSignalInfo->MSBcid);
                                        exit(1); 
                                }
				/* Adptivly set UL UIUC & DL DIUC from BS to RS */
				targetRS->diuc = LastSignalInfo->d_diuc;

				if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
					targetRS->uiuc = selectUIUC(LastSignalInfo->SNR);
				else		// if LinkMode=Manual, DL and UL use the same profile
				{
					fec = DCDProfile[targetRS->diuc].fec;
					for (i = 1; i <= 10; i++)
					{
						if (UCDProfile[i].used && UCDProfile[i].fec == fec)
						{
							targetRS->uiuc = i;
							break;
						}
					}
				}
			}
			else			// MS initial ranging && period ranging && handover ranging
			{
				MS_pendingRangingCodeList.push_back(new RangingObject(codeIndex, frameNumber & 0xFF, LastSignalInfo->symOffset, LastSignalInfo->chOffset, usage, GetCurrentTime(), LastSignalInfo->SNR, pScheduler->ULallocStartTime, 0, LastSignalInfo->MSBcid, 0));	// filled relay uiuc = 0

				if (usage == PERIOD_RANGING)
				{
					
					targetMS = getMSbyManacid(LastSignalInfo->MSBcid);
					if (targetMS == NULL)
					{
						printf("***(%d):%s:Getting MS(mcid=%d) object failed!***\n", __LINE__, __func__, LastSignalInfo->MSBcid);
						exit(1); 
					}
					/* Adaptivly set UL UIUC & DL DIUC from BS to MS */
					targetMS->diuc = LastSignalInfo->d_diuc;

					if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
						targetMS->uiuc = selectUIUC(LastSignalInfo->SNR);
					else
					{
						fec = DCDProfile[targetMS->diuc].fec;
						for (i = 1; i <= 10; i++)
						{
							if (UCDProfile[i].used && UCDProfile[i].fec == fec)
							{
								targetMS->uiuc = i;
								break;
							}
						}
					}
				}
				/* Start MS Path Selection timer iff MR-BS receive MS ranging code */
				if(timerSelectPath->busy_ == 0)
				{
					MILLI_TO_TICK(timeTick, MR_RNGREP_interval);
					timerSelectPath->start(timeTick, 0);
				}
			}
	
		}
	}
	else
	{
		for (ptr = adaptedBurst;ptr + sizeof(struct hdr_generic) < adaptedBurst + burstLen;ptr += plen)
		{
			// Get generic mac header
			hg      = (struct hdr_generic *) ptr;
			cid     = GHDR_GET_CID(hg);
			plen    = GHDR_GET_LEN(hg);	//len including header & padding

			// None
			if (hcs_16j(ptr, sizeof(struct hdr_generic)) != 0)
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
				crc = crc32_16j(0, ptr, len);
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
			else if (recvmm->type == MG_MR_RNGREP)
			{
				procMR_RNGREP(recvmm, cid, len - sizeof(struct hdr_generic));	
			}
			else if ((targetRS = getRSbycid(cid)) != NULL) // Basic CID or Primary CID
                        {
                                targetRS->handle(recvmm, cid, len - sizeof(struct hdr_generic),frameNumber);
                        }
			else if ((targetMS = getMSbyManacid(cid)) != NULL) // Basic CID or Primary CID
			{
				/* process management message from MS directly */
				if (targetMS->servingBS == targetMS->accessStation)
				{
					if (LastSignalInfo->relay_flag == false)
						targetMS->handle(recvmm, cid, len - sizeof(struct hdr_generic));
				}
				else 
				{
					if (LastSignalInfo->relay_flag == true)
						targetMS->handle(recvmm, cid, len - sizeof(struct hdr_generic));
				}
			}
			else // Secondary CID or Transport CID
			{
				vector<ConnectionReceiver *>::iterator iter;
				char *ptr2 = NULL;
				
				if ((targetMS = getMSbyDtcid(cid)) != NULL)
				{
					if (targetMS->servingBS == targetMS->accessStation)
					{
						if (LastSignalInfo->relay_flag == false)
						{
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
					else
					{
						if (LastSignalInfo->relay_flag == true)
						{
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
				else
				{
					printf("Time:%llu BS(%d) can't find specify MS for this(cid = %d) data connection\n", GetCurrentTime(), get_nid(), cid);
				}				


			}
		}
	}

	delete recvPacket;

	return 1;
}

int mac802_16j_PMPBS::send(ePacket_ *epkt)
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

Connection *mac802_16j_PMPBS::CreateDataConnection(msObject_mr *pMS)
{
	//DataFlow CID : 1021~
	for (int i = 0; i < MAXCONN; i++)
	{
		if (conlist[i] != NULL)
		{
			continue;
		}

		if (_CSType == csIPv4)
		{
			conlist[i] = new DataConnection(i + 2 * MAXMS + 2* MAXRS + 1);
		}
		else
		{
			conlist[i] = new DataConnectionEthernet(i + 2 * MAXMS + 2* MAXRS + 1);
		}

		pMS->DtConnections.push_back(conlist[i]);
		ReceiverList.push_back(new ConnectionReceiver(i + 2 * MAXMS + 2* MAXRS + 1));
		return conlist[i];
	}
	return NULL;
}

/*
 * When MS performed handover, BS shall remove the previous resourse for that MS
 */
void mac802_16j_PMPBS::RemoveMS(msObject_mr *pMS)
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


/*
 * When RS performed handover, MR-BS shall remove the previous resourse for that RS
 */
/*void mac802_16j_PMPBS::RemoveRS(rsObject *pRS)
{
	;
}*/
ServiceFlow *mac802_16j_PMPBS::GetProvisionedFlow(uint8_t mac[6])
{
	return PsTable->GetServiceFlow(mac);
}

ServiceFlow *mac802_16j_PMPBS::GetProvisionedFlow(uint32_t flowId)
{
	return PsTable->GetServiceFlow(flowId);
}

ServiceClass *mac802_16j_PMPBS::GetServiceClass(uint32_t qosIndex)
{
	return ScTable->GetServiceClass(qosIndex);
}

msObject_mr *mac802_16j_PMPBS::CreateMS(uint8_t *mac)
{
	mac802_16j_PMPMS *pMS = NULL;

        pMS = (mac802_16j_PMPMS *)InstanceLookup(LastSignalInfo->nid, LastSignalInfo->pid, "MAC802_16J_PMPMS");
        if (pMS == NULL)
                printf("***BS(%d)::%s:Get MS(nid = %d , pid = %d)) module failed!***\n", get_nid(), __func__, LastSignalInfo->nid, LastSignalInfo->pid);

	//MS BCID : 1~255 ,PCID : 256~510
	for (int i = 0; i < MAXMS; i++)
	{
		if (mslist[i] == NULL)
		{
			/* bcid : 1~255 , pcid : 256 ~ 510 */
			mslist[i] = new msObject_mr(mac, i + 1, MAXMS + i + 1, 0, this, pMS);	
			return mslist[i];
		}
	}
	return NULL;
}

rsObject *mac802_16j_PMPBS::CreateRS(uint8_t *mac)
{
	mac802_16j_PMPRS *pRS = NULL;

	pRS = (mac802_16j_PMPRS *)InstanceLookup(LastSignalInfo->nid, LastSignalInfo->pid, "MAC802_16J_PMPRS");
	if (pRS == NULL)
		printf("***BS(%d)::%s:Get RS(nid = %d , pid = %d)) module failed!***\n", get_nid(), __func__, LastSignalInfo->nid, LastSignalInfo->pid);

	//RS BCID : 511~765 ,PCID : 766~1020
        for (int i = 0; i < MAXRS; i++)
        {
                if (rslist[i] == NULL)
                {
                        rslist[i] = new rsObject(mac, i + 1 + 2*MAXMS, MAXRS + i + 1+ 2*MAXMS, 0, this, pRS); 
                        return rslist[i];
                }
        }
        return NULL;
}

/*
 * Map each MAC SDU onto a particular connection (queue) for later transmission.
 */
bool mac802_16j_PMPBS::DownlinkClassifier(Packet *pkt)
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

void mac802_16j_PMPBS::handleRangingCode()
{
	ifmgmt *ifmm            = NULL;
	uint32_t rangAttribute  = 0;
	int timingAdjust        = 0;
	int nPSs                = 0;
	uint64_t ticks          = 0;
	uint64_t offset_ticks          = 0;
	double diff             = 0.0;
	uint8_t rsv             = 0;
	//uint8_t	DLrelay		= 0;
	list<RangingObject *>::iterator its1, its2;


	//printf("mac802_16j_PMPBS::handleRangingCode()\n");

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
	//Encoding RNG-RSP for CDMA ranging code to initialRangingConnection , clear vector  af every Packet Scheduling
	for (its1 = RangingCodeList.begin();its1 != RangingCodeList.end();its1++)
	{
		if ((*its1)->collision == true)
			continue;
		if((*its1)->rangingUsage == INITIAL_RANGING || (*its1)->rangingUsage == HANDOVER_RANGING || (*its1)->rangingUsage == RS_INITIAL_RANGING)
		{
			//printf("Time:%llu=**BS(%d)**== rangingUsage=%d == rangingCodeIndex =%d == RS cid = %d =====\n", GetCurrentTime(), get_nid(), (*its1)->rangingUsage, (*its1)->rangingCodeIndex, (*its1)->RS_cid);

			ifmm = new ifmgmt(MG_RNGRSP, 1);
			ifmm->appendField(1, rsv);

			// TLV infomation
			rangAttribute = ((*its1)->rangingSymbol      & 0x3FF) << 22 |
				((*its1)->rangingSubchannel  & 0x3F ) << 16 |
				((*its1)->rangingCodeIndex   & 0xFF ) << 8  |
				((*its1)->rangingFrameNumber & 0xFF );

			// Compute time adjustment
			nPSs = pScheduler->SearchULTime((*its1)->ULallocStartTime,(*its1)->rangingSymbol) + symbolsToPSs(2);
			MICRO_TO_TICK(ticks, PSsToMicro(nPSs));

			/* for overflow offset check since we only have LSB 8 bits of frameNo.*/
                        if ( (frameNumber & 0XFF) < (*its1)->rangingFrameNumber)
                                MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) + 256 - (*its1)->rangingFrameNumber) * frameDuration[*_frameDuCode]);
                        else
                                MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) - (*its1)->rangingFrameNumber) * frameDuration[*_frameDuCode]);
			/*recalculate pending MSs ranging code time adjustment*/
			ticks += (frameStartTime - offset_ticks);

			TICK_TO_MICRO(diff, (int) (ticks - (*its1)->recv_time));
			timingAdjust = (int) (diff * 1024.0 / Tb());	//Nttf = 1024

			if (timingAdjust > 10 || timingAdjust < -10) 
			{
				printf("\e[1;36mTime:%llu BS(%d)Rangingusage = %d , timingAdjust = %d\e[0m\n", GetCurrentTime(), get_nid(), (*its1)->rangingUsage, timingAdjust);
				ifmm->appendTLV(1, 4, timingAdjust); // unit: 1/Fs
				ifmm->appendTLV(4, 1, 1);   // Ranging Status: 1=continue
				ifmm->appendTLV(99, 2, (*its1)->RS_cid);
			}
			else
			{
				ifmm->appendTLV(4, 1, 3);   // Ranging Status: 3=success
                                ifmm->appendTLV(99, 2, (*its1)->RS_cid);
				(*its1)->allocated = true;  
				
			}
			ifmm->appendTLV(150, 4, rangAttribute);     // Ranging Code Attributes

			initRangingConnection->Insert(ifmm);    // using Initial Ranging Connection
		}
		else if ((*its1)->rangingUsage == PERIOD_RANGING)
		{
			//printf("Time:%llu=**BS(%d)**== rangingUsage=%d == rangingCodeIndex =%d == RS cid = %d =====\n", GetCurrentTime(), get_nid(), (*its1)->rangingUsage, (*its1)->rangingCodeIndex, (*its1)->RS_cid);

			ifmm = new ifmgmt(MG_RNGRSP, 1);
			ifmm->appendField(1, rsv);

			// TLV infomation
			rangAttribute = ((*its1)->rangingSymbol      & 0x3FF) << 22 |
				((*its1)->rangingSubchannel  & 0x3F ) << 16 |
				((*its1)->rangingCodeIndex   & 0xFF ) << 8  |
				((*its1)->rangingFrameNumber & 0xFF );

			// Compute time adjustment
			nPSs = pScheduler->SearchULTime((*its1)->ULallocStartTime,(*its1)->rangingSymbol) + symbolsToPSs(1);
			MICRO_TO_TICK(ticks, PSsToMicro(nPSs));

			/* for overflow offset check since we only have LSB 8 bits of frameNo.*/
			if ( (frameNumber & 0XFF) < (*its1)->rangingFrameNumber)
				MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) + 256 - (*its1)->rangingFrameNumber) * frameDuration[*_frameDuCode]); 
			else
				MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) - (*its1)->rangingFrameNumber) * frameDuration[*_frameDuCode]);
                        /*recalculate pending MSs ranging code time adjustment*/
                        ticks += (frameStartTime - offset_ticks);

			TICK_TO_MICRO(diff, (int) (ticks - (*its1)->recv_time));
			timingAdjust = (int) (diff * 1024.0 / Tb());

			if (timingAdjust > 10 || timingAdjust < -10) 
			{
				printf("\e[1;36mTime:%llu BS(%d)Rangingusage = %d , timingAdjust = %d\e[0m\n", GetCurrentTime(), get_nid(), (*its1)->rangingUsage, timingAdjust);
				ifmm->appendTLV(1, 4, timingAdjust); // unit: 1/Fs
				ifmm->appendTLV(4, 1, 1);   // Ranging Status: 1=continue
                                ifmm->appendTLV(99, 2, (*its1)->RS_cid);
					
			}
			else
			{
				ifmm->appendTLV(4, 1, 3);   // Ranging Status: 3=success
                                ifmm->appendTLV(99, 2, (*its1)->RS_cid);
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
 * Periodically transmit downlink subframe in DL access zone
 */
void mac802_16j_PMPBS::PacketScheduling()
{
	ePacket_ *epkt          = NULL;
	BurstInfo *burstInfo    = NULL;
	uint32_t nid = get_nid();
	uint32_t pid = get_port();

	//printf("Time:%llu ==================BS(%d):: %s==================\n", GetCurrentTime(), get_nid(), __func__);

	// Before scheduling, we need to check the ranging code
	handleRangingCode();

	/* Update MS status (scanning or handover) */ // FIXME: only consider that iteration times = 1
	for (int i = 0;i < MAXMS;i++)
	{
		msObject_mr *pMS = mslist[i];

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

	/* Update RS status (scanning or handover) */ // FIXME: not implement NRS/MRS yet
	for (int i = 0;i < MAXRS;i++)
	{
		rsObject *pRS = rslist[i];

		if (pRS == NULL)
			continue;

		if (pRS->ResourceStatus == NeedClear) // MS resource need to remove
		{
			printf("\e[1;36m======= RS status: NeedClear ... BS(%d) remove this RS now ========\e[0m\n", get_nid());
			delete rslist[i];
			rslist[i] = NULL;
			continue;
		}

		if (pRS->ScanFlag == true)
		{
			if (pRS->ScanTimes != pRS->ScanDuration) // continue scanning
			{
				pRS->ScanTimes++;
			}
			else // end scanning
			{
				pRS->ScanTimes = 0;
				pRS->ScanFlag = false;
			}
		}
		else
		{
			if (pRS->ChangeToScanCnt >= 0)
			{
				pRS->ChangeToScanCnt--;
			}
			else if (pRS->ChangeToScanCnt == -1) // change to scanning
			{
				pRS->ScanFlag = true;
				pRS->ScanTimes++;
				pRS->ChangeToScanCnt = -2; // don't care
			}
			else // continue normal operation
			{
				; // normal operation
			}
		}
	}

	//Burst info with burst mapping infor within per frame
	burstInfo = new BurstInfo;

	// Set per frame infomation , including frame start time & frame number
	frameStartTime = GetCurrentTime();
	frameNumber++;

	// Scheduling , burstInfo->Collection is a pointer point to a container that contains burst pointer
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

	//Scheduling Transparent zone burst timer 
	uint64_t ticks = 0;
	
	// Set the state of PHY to Busy to sending packets
	((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setStateBusy();

	// Start a timer to change state ,correct the shift value when calculate between PS and symbol,16j-FIXME  
	ticks = 0;
	if ((pScheduler->DLsymbols * Ts() + PSsToMicro(TTG())) < PSsToMicro(pScheduler->ULallocStartTime))
		MICRO_TO_TICK(ticks, pScheduler->DLsymbols * Ts() + PSsToMicro(TTG()));
	else	
		MICRO_TO_TICK(ticks, PSsToMicro(pScheduler->ULallocStartTime));
	timerSetldle->start(ticks, 0);
}

/*
 * Generate a MAC PDU containing UCD.
 */
void mac802_16j_PMPBS::generate_UCD()
{
	pScheduler->generate_UCD();
	return;
}

/*
 * Generate a MAC PDU containing DCD.
 */
void mac802_16j_PMPBS::generate_DCD()
{
	pScheduler->generate_DCD();
	return;
}

/*
 * Generate a MAC PDU containing MOB_NBR-ADV
 */
void mac802_16j_PMPBS::generate_MOB_NBRADV()
{
	pScheduler->generate_MOB_NBRADV();
	return;
}

msObject_mr *mac802_16j_PMPBS::getMSbyManacid(int cid)
{

	if (cid >= 1 && cid <= MAXMS)   // Basic CID of MS
	{
		return mslist[cid - 1];
	}
	else if (cid >= MAXMS + 1 && cid <= 2 * MAXMS)    // Primary CID of MS
	{
		return mslist[cid - 1 - MAXMS];
	}
	else
	{
		return NULL;
	}
}


msObject_mr *mac802_16j_PMPBS::getMSbyDtcid(int cid)
{
	int i = 0;

	list<DataConnection *>::iterator ited;
	for( i = 0; i < MAXMS; i++)
	{
		if (mslist[i] != NULL)
		{
			for( ited = mslist[i]->DtConnections.begin(); ited != mslist[i]->DtConnections.end(); ited++)
			{
				if (cid == (*ited)->getCID())
					return mslist[i];
			}
		}

	}
	return NULL;
}

rsObject *mac802_16j_PMPBS::getRSbycid(int cid)
{
        if (cid >= (1 + 2 * MAXMS) && cid <= (2 * MAXMS + MAXRS))   // Basic CID of RS
        {
                return rslist[cid - 1 - 2 * MAXMS];
        }
        else if (cid >= (2 * MAXMS + 1 + MAXRS) && cid <= (2 * MAXMS + 2 * MAXMS))    // Primary CID of MS
        {
                return rslist[cid - 1 - MAXRS - 2 * MAXMS];
        }
        else
        {
                return NULL;
        }
}

void mac802_16j_PMPBS::SelectMSsPath()
{
	
	bool list_add_flag = false;
	msObject_mr *pMS = NULL;
	rsObject *pRS = NULL;
	uint16_t prsbcid = 0;
	uint8_t uiuc_mstobs = 0;
	uint8_t uiuc_mstors = 0;
	uint8_t uiuc_rstobs = 0;
	double  Ws = 0;
	double  Wp = 0;
	double  Wr = 0;
	uint8_t diuc_bstoms = 0;
	uint8_t diuc_bstors = 0;
	uint8_t diuc_rstoms = 0;
	bool flag = false;
	RangingObject *Rptr = NULL;
	int fec = 0;
	int i = 0;
	
	list <uint16_t> cidlist;	
        list<RangingObject *>::iterator its1, its2,  best;
	list<DataConnection *>:: iterator itd;

	//printf("\nTime:%llu:mac802_16j_PMPBS::%s()\n", GetCurrentTime(), __func__);

	// check if collision	
	for (its1 = MS_pendingRangingCodeList.begin();its1 != MS_pendingRangingCodeList.end();its1++)
	{
		for (its2 = MS_pendingRangingCodeList.begin();its2 != MS_pendingRangingCodeList.end();its2++)
		{
			if (its1 == its2) // skip
				continue;

			if (((*its1)->rangingCodeIndex      == (*its2)->rangingCodeIndex)   &&
					((*its1)->rangingSymbol         == (*its2)->rangingSymbol)      &&
					((*its1)->rangingSubchannel     == (*its2)->rangingSubchannel)  &&
					((*its1)->rangingFrameNumber    == (*its2)->rangingFrameNumber) &&
					((*its1)->MSBcid    != (*its2)->MSBcid))
			{
				(*its1)->collision = true;
				(*its2)->collision = true;
			}
		}
	}
	
	/* Examine MS initial ranging code and select best access station depend on SNR*/	
	while (!MS_pendingRangingCodeList.empty())
	{
		flag = false;
		list_add_flag = false;
		pMS = NULL;
		pRS = NULL;
		Rptr = NULL;

		for(its1 = MS_pendingRangingCodeList.begin(); its1 != MS_pendingRangingCodeList.end(); its1++)
		{
			if ((*its1)->RS_cid == 0 && (*its1)->ruiuc ==0)
			{
				best = its1;
				if ((*its1)->rangingUsage == PERIOD_RANGING)
				{
					pMS = getMSbyManacid((*its1)->MSBcid);
					if (pMS == NULL)
					{
						printf("***(%d)::%s:Getting MS(mcid=%d) object failed!***\n", __LINE__, __func__, (*its1)->MSBcid);
						exit(1);
					}
					if(LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
						uiuc_mstobs = selectUIUC((*its1)->SNR);
					else
						uiuc_mstobs = pMS->uiuc;
				}
				else
					uiuc_mstobs = selectUIUC((*its1)->SNR);
				flag = true;
				break;
			}
			else
				continue;
		}
		
		/* If we can not get any ranging code information that sent by MS to BS directly ,
 		 * we will ignore the MS pending ranging code list at this round and clear pending list.
 		 */ 	
		if (!flag)
		{
			while (!MS_pendingRangingCodeList.empty())
			{
				delete *(MS_pendingRangingCodeList.begin());
				MS_pendingRangingCodeList.erase(MS_pendingRangingCodeList.begin());
			}
			break;
		}

		for (its1 = MS_pendingRangingCodeList.begin(); its1 != MS_pendingRangingCodeList.end();)
		{
			if(its1 == best)
				++its1;
			else if ((*its1)->collision == true)
			{
				delete *(its1);
				its1 = MS_pendingRangingCodeList.erase(its1);
			}	
			else	
			{
				if (((*its1)->rangingCodeIndex      == (*best)->rangingCodeIndex)   &&
                                        ((*its1)->rangingSymbol         == (*best)->rangingSymbol)      &&
                                        ((*its1)->rangingSubchannel     == (*best)->rangingSubchannel)  &&
                                        ((*its1)->rangingFrameNumber    == (*best)->rangingFrameNumber) &&
                                        ((*its1)->MSBcid    == (*best)->MSBcid))
				{
					if((pRS = getRSbycid((*its1)->RS_cid)) == NULL)
					{
						printf("(%d)%s , Getting RS(%d) Obecjt failed!\n", __LINE__, __func__, (*its1)->RS_cid);
						++its1;
						continue;
					}

					if ((*its1)->rangingUsage == PERIOD_RANGING)
					{
						pMS = getMSbyManacid((*its1)->MSBcid);
						if (pMS == NULL)
						{
							printf("***(%d)::%s:Getting MS(mcid=%d) object failed!***\n", __LINE__, __func__, (*its1)->MSBcid);
							exit(1);
						}
						if(LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
							uiuc_mstors = (*its1)->ruiuc;
						else
						{
							fec = DCDProfile[pMS->diuc].fec;
							for (i = 1; i <= 10; i++)
							{
								if (UCDProfile[i].used && UCDProfile[i].fec == fec)
								{
									uiuc_mstors = i;
									break;
								}
							}
						}
					}
					else					
						uiuc_mstors = (*its1)->ruiuc;

					uiuc_rstobs = pRS->getUIUC();		
										
					if((*best)->rangingUsage == PERIOD_RANGING)
					{
						diuc_bstors = pRS->getDIUC();
						/*pMS = getMSbyManacid((*best)->MSBcid);
						if (pMS == NULL )
						{
							printf("***%d::%s:Getting MS(mcid=%d) object failed!***\n", __LINE__, __func__, (*its1)->MSBcid);
							exit(1);
						}*/
						diuc_bstoms = pMS->diuc;
						if(LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
							diuc_rstoms = pMS->rdiuc;
						else
							diuc_rstoms = pMS->diuc;
					}
					if ((*best)->rangingUsage == PERIOD_RANGING)
						printf("\e[1;35m***MS(%d) Ranging code relay from RS(%d)>>>(u_mstors = %d , u_rstobs = %d , u_mstobs = %d)<<< , >>>(d_bstors = %d , drstoms = %d , d_bstoms = %d)<<<***\e[0m\n", pMS->myMS->get_nid(), pRS->myRS->get_nid(), uiuc_mstors, uiuc_rstobs , uiuc_mstobs , diuc_bstors, diuc_rstoms , diuc_bstoms);
					else
						printf("\e[1;35m***IR/HR relay from RS(%d), >>>(u_mstors = %d , u_rstobs = %d , u_mstobs = %d)<<<***\e[0m\n", pRS->myRS->get_nid(), uiuc_mstors, uiuc_rstobs , uiuc_mstobs );

					/* Path selection algorithm : 
 					 * Reference paper:"An Interference-Aware Analytical Model for Performance Analysis
 					 * of Transparent Mode 802.16j Systems"
 					 * If (Weight(uiuc_mstors) + Weight(uiuc_rstobs) < Weight(uiuc_mstobs), 
 					 * We will select the path pass through RS be UL relay path
 					 */ 
					Ws = getWeightbyuiuc(uiuc_mstobs);
					Wr = getWeightbyuiuc(uiuc_mstors);
					Wp = getWeightbyuiuc(uiuc_rstobs);

					if(Wr + Wp <  Ws)
					{
						if (pMS != NULL)	// PR	
						{
							printf("\e[1;31mTime:%llu:***MS(%d)==>RS(%d) , Relay Weight = %f < Direct Weight = %f\e[0m\n",GetCurrentTime(),pMS->myMS->get_nid(), pRS->myRS->get_nid(), Wr + Wp, Ws);
								
							if (Wr + Wp < (*best)->relayWeight)
							{
								(*best)->relayWeight = Wr + Wp;
								(*best)->RS_cid = (*its1)->RS_cid;
								(*best)->ruiuc = (*its1)->ruiuc;
							}
							else if ((pMS->relayRSBcid == (*its1)->RS_cid) && (Wr + Wp == (*best)->relayWeight))
							{
								(*best)->RS_cid = (*its1)->RS_cid;
                                                                (*best)->ruiuc = (*its1)->ruiuc;
							}
						}
						else	// IR/HR
						{
							(*best)->relayWeight = Wr + Wp;
                                                        (*best)->RS_cid = (*its1)->RS_cid;
                                                        (*best)->ruiuc = (*its1)->ruiuc;
						}
					}
					else
					{
						if (pMS != NULL)
						{
							if ((*best)->RS_cid == 0)
								printf("\e[1;34mTime:%llu***MS(%d)==>BS(%d) , Relay Weight = %f > Direct Weight = %f\e[0m\n",GetCurrentTime(), pMS->myMS->get_nid(), get_nid(), Wr + Wp, Ws);
							else
								printf("\e[1;34mTime:%llu***MS(%d)==>RS(%d) , Relay Weight = %f > Direct Weight = %f\e[0m\n",GetCurrentTime(), pMS->myMS->get_nid(), pRS->myRS->get_nid(), Wr + Wp, Ws);
						}
					}

					delete *(its1);
					its1 = MS_pendingRangingCodeList.erase(its1);
				}
				else
					++its1;
			}
		}
	
		//printf("***Time:%llu:BS(%d)::%s:best->RScid= %d, best->MScid = %d****\n", GetCurrentTime(), get_nid(), __func__, (*best)->RS_cid, (*best)->MSBcid);
		
		/* Updating Member List if needed */
		if ((*best)->rangingUsage == PERIOD_RANGING && (*best)->MSBcid != 0)
		{
			pMS = getMSbyManacid((*best)->MSBcid);
			if (pMS == NULL )
			{
				printf("***%d::%s:Getting MS(mcid=%d) object failed!***\n", __LINE__, __func__, (*best)->MSBcid);
				exit(1);
			}
			prsbcid = pMS->relayRSBcid;	// record previous RS basic cid
			if (prsbcid != (*best)->RS_cid)
			{
				/* Examine iff new access station be BS or RS */
				if((pRS = getRSbycid((*best)->RS_cid)) != NULL)
				{
					if (pMS->accessStation != pRS)
					{

						pMS->accessStation = pRS;
						if(LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
						{
							pMS->rdiuc = RobustDIUC;    // reset rdiuc to Robust DIUC
							pMS->myMS->SetRDIUC(RobustDIUC);

							pMS->ruiuc = (*best)->ruiuc;	// uiuc from ms to rs
                                                	pMS->myMS->SetRUIUC((*best)->ruiuc);
						}
						else
						{
							pMS->rdiuc = pMS->diuc;    // reset rdiuc to Robust DIUC
                                                        pMS->myMS->SetRDIUC(pMS->diuc);

							fec = DCDProfile[pMS->diuc].fec;
                                                        for (i = 1; i <= 10; i++)
                                                        {
                                                                if (UCDProfile[i].used && UCDProfile[i].fec == fec)
                                                                {
                                                                        pMS->ruiuc = i;
									pMS->myMS->SetRUIUC(i);
                                                                        break;
                                                                }
                                                        }
						}
						pMS->relayRSBcid = (*best)->RS_cid;
						printf("\e[1;33m>>>Time::%llu:MS(%d) selects RS(%d) to be new access station, RS_DIUC = %d, RS_UIUC = %d <<<\e[0m\n", GetCurrentTime(), pMS->myMS->get_nid(), pRS->myRS->get_nid(), pRS->diuc, pRS->uiuc);

						list_add_flag = true;
						cidlist.push_back(pMS->BasicCID);
						cidlist.push_back(pMS->PriCID);
						if ( pMS->SecCID != 0)
							cidlist.push_back(pMS->SecCID);
						for (itd = pMS->DtConnections.begin(); itd != pMS->DtConnections.end(); itd++)
							cidlist.push_back((*itd)->getCID());
						SendRS_MemberListUpdate(cidlist, list_add_flag, (*best)->RS_cid);
						SendMS_InfoDel((*best)->MSBcid, prsbcid);
					}
				}
				else 
				{
					printf("\e[1;33m>>>Time::%llu:MS(%d) selects BS(%d) to be new access station, DIUC = %d , UIUC = %d<<<\e[0m\n", GetCurrentTime(), pMS->myMS->get_nid(), get_nid(), pMS->diuc, pMS->uiuc);
					pMS->accessStation = this;
					pMS->ruiuc = RobustUIUC;
					pMS->myMS->SetRUIUC(RobustUIUC);
					pMS->rdiuc = RobustDIUC;    // reset rdiuc to Robust DIUC
					pMS->myMS->SetRDIUC(RobustDIUC);
					pMS->relayRSBcid = 0;
					SendMS_InfoDel((*best)->MSBcid, prsbcid);

				}
			}
			else	/* Same RS is selected to be access station */
			{
				/* Updating ruiuc information  , rdiuc will be updated while procREPRSP from MS*/
			
				if(LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)	
				{
					pMS->ruiuc = (*best)->ruiuc;	// uiuc from ms to rs
					pMS->myMS->SetRUIUC((*best)->ruiuc);
				}
				else
				{
					fec = DCDProfile[pMS->diuc].fec;
					for (i = 1; i <= 10; i++)
					{
						if (UCDProfile[i].used && UCDProfile[i].fec == fec)
						{
							pMS->ruiuc = i;
							pMS->myMS->SetRUIUC(i);
							break;
						}
					}
				}
			}
		}
	
		/* Insert the selected ranging code into original Ranging List for BS responsing RNGRSP */	
		Rptr = new RangingObject((*best)->rangingCodeIndex, (*best)->rangingFrameNumber, (*best)->rangingSymbol, (*best)->rangingSubchannel, (*best)->rangingUsage,  (*best)->recv_time, (*best)->SNR, (*best)->ULallocStartTime, (*best)->RS_cid, (*best)->MSBcid, (*best)->ruiuc);

		RangingCodeList.push_back(Rptr);
		MS_pendingRangingCodeList.erase(best);
		while(!cidlist.empty())
		{
			*(cidlist.begin());
			cidlist.erase(cidlist.begin());
		}
	}
}


void mac802_16j_PMPBS::saveDLMAP(char *dstbuf, int len)
{
	uint32_t nid = get_nid();
	uint32_t pid = get_port();

	((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->saveDLMAP(dstbuf, len);
}

void mac802_16j_PMPBS::changeStateToIdle()
{
	uint32_t nid = get_nid();
        uint32_t pid = get_port();

	((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setStateIdle();
}

void mac802_16j_PMPBS::saveDLsymbols(int Nsymbols)
{
	uint32_t nid = get_nid();
        uint32_t pid = get_port();

	((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setDLsymbols(Nsymbols);
}

void mac802_16j_PMPBS::saveULsymbols(int Nsymbols)
{
	uint32_t nid = get_nid();
        uint32_t pid = get_port();

	ULoccupiedSymbols = Nsymbols;
	((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setULsymbols(Nsymbols);
}

void mac802_16j_PMPBS::saveDLAccessSymbols(int Nsymbols)
{
	uint32_t nid = get_nid();
        uint32_t pid = get_port();

        DLAccessoccupiedSymbols = Nsymbols;
        ((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setDLAccessSymbols(Nsymbols);
}

void mac802_16j_PMPBS::saveDLTransparentSymbols(int Nsymbols)
{
	uint32_t nid = get_nid();
        uint32_t pid = get_port();
	
        DLTransparentoccupiedSymbols = Nsymbols;
        ((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setDLTransparentSymbols(Nsymbols);
}

void mac802_16j_PMPBS::saveULAccessSymbols(int Nsymbols)
{
	uint32_t nid = get_nid();
        uint32_t pid = get_port();

	ULAccessoccupiedSymbols = Nsymbols;
	((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setULAccessSymbols(Nsymbols);
}

void mac802_16j_PMPBS::saveULRelaySymbols(int Nsymbols)
{
	uint32_t nid = get_nid();
        uint32_t pid = get_port();

        ULRelayoccupiedSymbols = Nsymbols;
        ((OFDMA_PMPBS_MR *)InstanceLookup(nid,pid,"OFDMA_PMPBS_MR"))->setULRelaySymbols(Nsymbols);
}

/*
 * Figure 62
 */
void mac802_16j_PMPBS::procRNGREQ(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm_req        = NULL;
	ifmgmt *ifmm_rsp        = NULL;
	rsObject    *pCurrentRS    = NULL;
	rsObject    *pRelayRS    = NULL;
	msObject_mr *pCurrentMS    = NULL;
	int i                   = 0;
	int type                = 0;
	int fec                 = 0;
	uint8_t fprofile        = 1;	//RobustUIUC
	uint8_t fmac[6]         = "";
	uint8_t rsv             = 0;
	uint8_t fanomalies      = 0;
	int nPSs                = 0;
	uint64_t ticks          = 0;
	double diff             = 0.0;
	int timingAdjust        = 0;
	uint16_t frelayrsbcid   = 0;
	bool list_add_flag = false;
	list <uint16_t>	cidlist;

	/* Extract fields*/
	ifmm_req = new ifmgmt((uint8_t *) recvmm, len, 1);
	ifmm_req->extractBitField( 8, &rsv);

	while ((type = ifmm_req->getNextType()) != 0)
	{
		switch (type) {
			case 1:
				ifmm_req->getNextValue(&fprofile);	// DIUC
				break;

			case 2:
				ifmm_req->getNextValue(fmac);
				break;

			case 3:
				ifmm_req->getNextValue(&fanomalies);
			
			case 99:
				ifmm_req->getNextValue(&frelayrsbcid);
				//printf("***%s::[MAC] %x:%x:%x:%x:%x:%x, RelayRSBcid = %d***\n", __func__, fmac[0], fmac[1], fmac[2], fmac[3], fmac[4], fmac[5], frelayrsbcid);

			default:
				break;
		}
	}
	delete ifmm_req;

	//printf("***Time:%llu::%s:: [MAC] %x:%x:%x:%x:%x:%x, RelayRSBcid = %d***\n",GetCurrentTime(), __func__, fmac[0], fmac[1], fmac[2], fmac[3], fmac[4], fmac[5], frelayrsbcid);
	
	/* Process RNG_REQ from RS */	
	if (LastSignalInfo->RS_RNGREQ_flag == true)
	{
		
		/* Get RS or a new RS */
		for (i = 0; i < MAXRS; i++)
		{
			if (rslist[i] && memcmp(rslist[i]->address()->buf(), fmac, 6) == 0)
			{
				pCurrentRS = rslist[i];
				break;
			}
		}

		if (i == MAXRS)        // New Station
		{
			pCurrentRS = CreateRS(fmac);
			if (pCurrentRS == NULL)
			{
				printf("CreateRS error: pCurrentRS is NULL\n");
				return;
			}
			pCurrentRS->diuc = fprofile;

			// Add to Polled list
			rs_PolledList.push_back(pCurrentRS);

			// Decide Current RS's uiuc depend on LinkMode
			if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
			{
				pCurrentRS->uiuc = 1;    // Default: QPSK_1_2

				//Examine the best modulation type
				for (i = 10; i >= 1; i--)
				{
					fec = UCDProfile[i].fec;
					if (UCDProfile[i].used && LastSignalInfo->SNR >= fecSNR[fec])
					{
						pCurrentRS->uiuc = i;
						break;
					}
				}
				//printf("BS(%d) Receive SNR=%lf dB and use uiuc=%d (fec=%d)\n", get_nid(), LastSignalInfo->SNR, pCurrentRS->uiuc, UCDProfile[i].fec);
			}
			else if (strcasecmp(LinkMode, "Manual") == 0)  // if LinkMode=Manual, DL and UL use the same profile
			{
				fec = DCDProfile[fprofile].fec;
				for (i = 1; i <= 10; i++)
				{
					if (UCDProfile[i].used && UCDProfile[i].fec == fec)
					{
						pCurrentRS->uiuc = i;
						break;
					}
				}
				//printf("BS(%d) Manual Mode: uiuc=%d(fec=%d)\n", get_nid(), pCurrentRS->uiuc, fec);
			}
			else
			{
				printf("Warning Configure String LinkMode:%s\n", LinkMode);
			}
			printf("\e[33mTime:%llu BS(%d):Accept New RS(%d):LinkMode=%s (SNR=%lf dB), DIUC=%d UIUC=%d\e[0m\n", GetCurrentTime(), get_nid(), pCurrentRS->myRS->get_nid(), LinkMode, LastSignalInfo->SNR, pCurrentRS->diuc, pCurrentRS->uiuc);
		}
	}
	else
	{
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
			ms_PolledList.push_back(pCurrentMS);
			
			/* If MS is relayed via RS */
			if ((pRelayRS = getRSbycid(frelayrsbcid)) != NULL)
			{
				pCurrentMS->accessStation = getRSbycid(frelayrsbcid);
				pCurrentMS->relayRSBcid = frelayrsbcid;
				/* We temporarily set rdiuc/ruiuc to robust DIUC/UIUC while initial ranging
 				 * , this value will be set during path selection period
 				 */ 
				pCurrentMS->myMS->SetRUIUC(RobustUIUC);
				pCurrentMS->myMS->SetRDIUC(RobustDIUC);
			}
			/* Decide Current MS's uiuc depend on LinkMode .
 			 * For special purpose , user can configure speficied modulation rate of DL.
 			 * It can be set via modify the LinkMode parameter in MAC module in *.tcl file.
 			 * The suppoting MCSs are QPSK1/2 , QPSK3/4, 16QAM1/2, 16QAM3/4, 64QAM1/2, 64QAM2/3, 64QAM3/4
 			 * Defulat MCS method is setting to be Auto. MCS of MS will adaptive cahnge due to power attenuation.
 			 */
			if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
			{
				pCurrentMS->uiuc = 1;    // Default: QPSK_1_2
				//Examine the best modulation type
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

			if (pRelayRS == NULL)
			{
				printf("\e[32mTime:%llu BS(%d) Accept New MS:LinkMode=%s (SNR=%lf dB), DIUC=%d UIUC=%d\e[0m\n", GetCurrentTime(), get_nid(), LinkMode, LastSignalInfo->SNR, pCurrentMS->diuc, pCurrentMS->uiuc);

				printf("\e[1;32mTime:%llu MS [MAC] %x:%x:%x:%x:%x:%x selects BS(%d) be access station ==>DIUC = %d , UIUC = %d\e[0m\n", GetCurrentTime(), fmac[0], fmac[1], fmac[2], fmac[3], fmac[4], fmac[5], get_nid(), pCurrentMS->diuc, pCurrentMS->uiuc);
			}
			else
			{
				printf("\e[32mTime:%llu BS(%d) Accept New MS:LinkMode=%s (SNR=%lf dB), DIUC=%d UIUC=%d\e[0m\n", GetCurrentTime(), get_nid(), LinkMode, LastSignalInfo->SNR, pCurrentMS->diuc, pCurrentMS->uiuc);

				printf("\e[1;32mTime:%llu MS [MAC] %x:%x:%x:%x:%x:%x selects RS(%d) be relay access station ==>RS_DIUC = %d , RS_UIUC = %d, Relay DIUC = %d ,Relay UIUC = %d\e[0m\n", GetCurrentTime(), fmac[0], fmac[1], fmac[2], fmac[3], fmac[4], fmac[5], pRelayRS->myRS->get_nid(), pRelayRS->diuc, pRelayRS->uiuc, pCurrentMS->rdiuc, pCurrentMS->ruiuc);

			}
	}
}

	/* Compute time adjustment */
	if (LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC > ULAccessoccupiedSymbols)
	{
		nPSs = pScheduler->SearchULTime(pScheduler->ULallocStartTime, 0) + symbolsToPSs(ULAccessoccupiedSymbols);
	}
	else
	{
		nPSs = pScheduler->SearchULTime(pScheduler->ULallocStartTime, 0) + symbolsToPSs(LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC);
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
	if (timingAdjust > 10 || timingAdjust < -10) 
	{
		printf("\e[1;36mTime:%llu BS(%d) timingAdjust = %d\e[0m\n", GetCurrentTime(), get_nid(), timingAdjust);
		ifmm_rsp->appendTLV(4, 1, 1);            // Ranging Status / 8bit / 1=continue, 2=abort, 3=Success
		ifmm_rsp->appendTLV(1, 4, timingAdjust); // Timing Adjust. Unit: 1/Fsample
		
	}
	else
	{
		if(LastSignalInfo->RS_RNGREQ_flag == true)
		{
			rs_PolledList.remove(pCurrentRS);
                        ifmm_rsp->appendTLV(4, 1, 3);  // Success
		}
		else
		{
			ms_PolledList.remove(pCurrentMS);
			ifmm_rsp->appendTLV(4, 1, 3);  // Success	//RngSuccess
		}
	}
	if(LastSignalInfo->RS_RNGREQ_flag == true)
	{
		ifmm_rsp->appendTLV( 8, 6, pCurrentRS->address()->buf());      // RS MAC Address
                ifmm_rsp->appendTLV( 9, 2, pCurrentRS->BasicCID);              // Basic CID
                ifmm_rsp->appendTLV(10, 2, pCurrentRS->PriCID);                // Primary CID
                ifmm_rsp->appendTLV(17, 1, pCurrentRS->ServerLevelPredict);    // Server Level
                ifmm_rsp->appendTLV(26, 2, pCurrentRS->PeriodRangingInterval); // Next Periodic Ranging in frames , 200
	}
	else
	{
		ifmm_rsp->appendTLV( 8, 6, pCurrentMS->address()->buf());      // MS MAC Address
		ifmm_rsp->appendTLV( 9, 2, pCurrentMS->BasicCID);              // Basic CID
		ifmm_rsp->appendTLV(10, 2, pCurrentMS->PriCID);                // Primary CID
		ifmm_rsp->appendTLV(17, 1, pCurrentMS->ServerLevelPredict);    // Server Level
		ifmm_rsp->appendTLV(26, 2, pCurrentMS->PeriodRangingInterval); // Next Periodic Ranging in frames
	}

	initRangingConnection->Insert(ifmm_rsp);
	
	/* Encoding RS_Member_List_Update message for specify RS updating connection list */
	if (pRelayRS != NULL)	// MS relay via RS
	{
		list_add_flag = true;
		cidlist.push_back(pCurrentMS->BasicCID);
		cidlist.push_back(pCurrentMS->PriCID);

		SendRS_MemberListUpdate(cidlist, list_add_flag, frelayrsbcid);
	}
}

void mac802_16j_PMPBS::procMR_RNGREP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm    = NULL;
        uint8_t fmode   = 0;
        uint8_t flen    = 0;
        uint8_t fframeno = 0;
        uint8_t fcodeind = 0;
        uint8_t fsymoff = 0;
        uint8_t fchoff = 0;
	uint8_t fruiuc = 0;
        uint8_t fta = 0;
        uint8_t fpla = 0;
        uint8_t fofa = 0;
	uint16_t fmsbcid = 0;       
	int rscid = cid;
	int usage = -1;
	uint64_t timeTick = 0;
	RangingObject *R_ptr = NULL;
	
	//printf("\nTime:%llu:mac802_16j_PMPBS::%s(), cid = %d\n", GetCurrentTime(), __func__, cid);

        ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);
        ifmm->extractBitField(1, &fmode);
        ifmm->extractBitField(7, &flen);
        ifmm->extractBitField(8, &fframeno);
        ifmm->extractBitField(8, &fcodeind);
        ifmm->extractBitField(8, &fsymoff);
        ifmm->extractBitField(7, &fchoff);
	ifmm->extractBitField(6, &fruiuc);
        ifmm->extractBitField(1, &fta);
        ifmm->extractBitField(1, &fpla);
        ifmm->extractBitField(1, &fofa);
	ifmm->extractBitField(16, (uint8_t*)&fmsbcid);	
	if ( fcodeind < 43 )
		usage = INITIAL_RANGING;
	else if ( fcodeind >= 43 && fcodeind < 86)
		usage = PERIOD_RANGING;
	else if ( fcodeind >= 86  && fcodeind < 129)
		usage = BW_REQUEST;
	else if ( fcodeind >= 129  && fcodeind < 172)
		usage = HANDOVER_RANGING;
	
	/* Insert the CDMA ranging code relay from RS into pending MS code list */
	R_ptr = new RangingObject(fcodeind, fframeno, fsymoff, fchoff ,usage, GetCurrentTime(),LastSignalInfo->SNR, pScheduler->ULallocStartTime, rscid, fmsbcid, fruiuc);

	MS_pendingRangingCodeList.push_back(R_ptr);

        delete ifmm;

	if(timerSelectPath->busy_ == 0)
	{
		MILLI_TO_TICK(timeTick, MR_RNGREP_interval);
		timerSelectPath->start(timeTick, 0);
	}

}

void mac802_16j_PMPBS::SendRS_MemberListUpdate(list<uint16_t> cidlist, bool flag, uint16_t rsbcid)
{
	ifmgmt *ifmm_memlist;
	rsObject    *pRelayRS    = NULL;	
	
	list <uint16_t>::iterator itec;
	uint8_t  config_para_type = 1;	/* force RS reply Genereic ACK after receive */

	pRelayRS = getRSbycid(rsbcid);

	//printf("\nTime:%llu:mac802_16j_PMPBS::%s(), cid = %d\n", GetCurrentTime(), __func__, cid);	
	
	/* Here , we use reserve b2-bit to indicate add/remove following cid list , Add(1) , Remove(0)
	 *  b0 = 1 :RS sends acknowledge if the message is received
	 */
	if (flag)
		config_para_type |= 0x04;	/* list add */

	ifmm_memlist = new ifmgmt(MG_RS_Member_List_Update, 3);
	ifmm_memlist->appendBitField(4, frameNumber & 0x0F);
	ifmm_memlist->appendBitField(4, config_para_type & 0x0F);
	ifmm_memlist->appendField(2, 1234);	//Transaction ID

	/* Append cid TLV list */	
	for (itec = cidlist.begin(); itec != cidlist.end(); itec++)
	{
		ifmm_memlist->appendTLV(45, 2 ,(*itec));
	}

	pRelayRS->MnConnections[0]->Insert(ifmm_memlist);
}

void mac802_16j_PMPBS::SendMS_InfoDel(uint16_t MSBcid , uint16_t RSBcid)
{
	
	msObject_mr *pMS = NULL;
	bool list_add_flag = false;
	list<uint16_t> cidlist;	
	list<DataConnection *>:: iterator itd;
	
	pMS = getMSbyManacid(MSBcid);
	//printf("\nTime:%llu:mac802_16j_PMPBS::%s(), cid = %d\n", GetCurrentTime(), __func__, cid);
	
	list_add_flag = false;		// remove cid from cidlist
	cidlist.push_back(pMS->BasicCID);
	cidlist.push_back(pMS->PriCID);
	if (pMS->SecCID != 0)
		cidlist.push_back(pMS->SecCID);
	for (itd = pMS->DtConnections.begin(); itd != pMS->DtConnections.end(); itd++)
		cidlist.push_back((*itd)->getCID());
		
	/* We will not send RS_MemberListUpdate iff new access station is BS */
	if (RSBcid != 0)
		SendRS_MemberListUpdate(cidlist, list_add_flag, RSBcid);
}

uint8_t mac802_16j_PMPBS::selectUIUC(double recvSNR)
{
        int fec = 0;

	for (int i = 10; i >= 1; i--)
	{
		fec = UCDProfile[i].fec;
		if (UCDProfile[i].used && recvSNR >= fecSNR[fec])
		{
			return i;
		}
	}
	return RobustUIUC;
}

/*
 * 	Table:MCSs/Weights
 * 	====================================================
 * 	Burst profile	Bit/Symbol	Weight(Symbol/Bit)
 * 	QPSK1/2		1		1
 * 	QPSK3/4		3/2		2/3
 * 	16QAM1/2	2		1/2
 * 	16QAM3/4	3		1/3
 * 	64QAM1/2	3		1/3
 * 	64QAM2/3	4		1/4
 * 	64QAM3/4	9/2		2/9
 * 
 */


double  mac802_16j_PMPBS::getWeightbyuiuc(uint8_t uiuc)
{
        switch(uiuc)
                {
                        case 1:
                                return 1;
                        case 2:
                                return 0.67;
                        case 3:
                                return 0.5;
                        case 4:
                                return 0.34;
                        case 5:
                                return 0.34;
                        case 6:
                                return 0.25;
                        case 7:
                                return 0.22;
                        default:
                                return 2;
                }
}


double  mac802_16j_PMPBS::getWeightbydiuc(uint8_t diuc)
{
        switch(diuc)
                {
                        case 0:
                                return 1;
                        case 1:
                                return 0.67;
                        case 2:
                                return 0.5;
                        case 3:
                                return 0.34;
                        case 4:
                                return 0.34;
                        case 5:
                                return 0.25;
                        case 6:
                                return 0.22;
                        default:
                                return 2;
                }
}
