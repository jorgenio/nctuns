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
#include "mac802_16j_NT_pmpbs.h"

using namespace MR_Mac80216j_NT;
using namespace MR_Common_NT;
using namespace MR_MacAddress_NT;
using namespace MR_ServiceFlow_NT;
using namespace MR_Timer_NT;

MODULE_GENERATOR(mac802_16j_NT_PMPBS);

    mac802_16j_NT_PMPBS::mac802_16j_NT_PMPBS(uint32_t type, uint32_t id, plist *pl, const char *name)
: mac802_16j_NT(type, id, pl, name)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::mac802_16j_NT_PMPBS ",get_nid());


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

    DLRelayBurstCollection = NULL;

    broadcastConnection     = new BroadcastConnection(broadcastCID);
    initRangingConnection   = new ManagementConnection(initRangingCID);

    broadcastConnection_relay     = new BroadcastConnection(broadcastCID);
    initRangingConnection_relay   = new ManagementConnection(initRangingCID);

    assert(broadcastConnection && initRangingConnection && broadcastConnection_relay && initRangingConnection_relay);

    PsTable = new ProvisionedSfTable();
    ScTable = new ServiceClassTable();
    CrTable = new ClassifierRuleTable();
    NeighborMRBSList = new NeighborMRBSs_NT();

    assert(PsTable && ScTable && CrTable && NeighborMRBSList);

    uint64_t tick_interval;
    SEC_TO_TICK(tick_interval, 3); // 3 sec
    timer_mngr()->set_interval_t(10u, tick_interval); // wait for transaction end timeout

    timerDCD         = new timerObj;
    timerUCD         = new timerObj;
    downlinkTimer    = new timerObj;
    timerMOB_NBR_ADV = new timerObj;
    timerSetldle     = new timerObj;
    timerSelectPath  = new timerObj;
    timerSendDLRelayBurst = new timerObj;

    assert(timerDCD && timerUCD && downlinkTimer && timerMOB_NBR_ADV && timerSetldle);

    BASE_OBJTYPE(mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPBS, generate_DCD);
    timerDCD->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPBS, generate_UCD);
    timerUCD->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPBS, PacketScheduling);
    downlinkTimer->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPBS, generate_MOB_NBRADV);
    timerMOB_NBR_ADV->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPBS, changeStateToIdle);
    timerSetldle->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPBS, SelectMSsPath);
    timerSelectPath->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16j_NT_PMPBS, DLRelayPacketScheduling);
    timerSendDLRelayBurst->setCallOutObj(this, mem_func);

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
    ULAccessoccupiedSymbols = 0;
    ULRelayoccupiedSymbols = 0;

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

mac802_16j_NT_PMPBS::~mac802_16j_NT_PMPBS()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::~mac802_16j_NT_PMPBS ",get_nid());


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

    ClearRNGCodeList(&RangingCodeList);
    ClearRNGCodeList(&RS_RangingCodeList);

    while (!ReceiverList.empty())
    {
        delete *(ReceiverList.begin());
        ReceiverList.erase(ReceiverList.begin());
    }

    delete broadcastConnection;
    delete broadcastConnection_relay;
    delete initRangingConnection;
    delete initRangingConnection_relay;
    delete PsTable;
    delete ScTable;
    delete CrTable;
    delete NeighborMRBSList;
    delete timerDCD;
    delete timerUCD;
    delete downlinkTimer;
    delete timerMOB_NBR_ADV;
    delete timerSetldle;
    delete pScheduler;
    delete timerSelectPath;
    delete timerSendDLRelayBurst;
}

int mac802_16j_NT_PMPBS::init(void)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::init ",get_nid());


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

    RS_BR_header_interval = 6*frameDuration_NT[*_frameDuCode];  // 6 frames
    MILLI_TO_TICK(timeInTick, RS_BR_header_interval);
    timerSelectPath->start(firstTick, timeInTick);

    MILLI_TO_TICK(timeInTick, frameDuration_NT[*_frameDuCode]); // frameDuration_NT: 5 ms
    downlinkTimer->start(firstTick, timeInTick); // period timer

    pScheduler = new BS_16j_NT_Scheduler(this);

    assert(pScheduler);

    return mac802_16j_NT::init();
}


/*
 * Parse the neighbor BS's configuration file
 */
void mac802_16j_NT_PMPBS::parseCfgFile()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::parseCfgFile ",get_nid());


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
                NbrMRBS_NT *nbrBS = new NbrMRBS_NT(nid, real_mac);
                nbrBS->PreambleIndex = chID;
                NeighborMRBSList->nbrBSs_Index.push_back(nbrBS);
            }
        }

        fclose(fp);
    }
    else
    {
        fprintf(stderr, "No Neighbor BS list\n");
    }

}

int mac802_16j_NT_PMPBS::recv(ePacket_ *epkt)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::recv ",get_nid());


    Packet *recvPacket      = NULL;
    struct hdr_generic *hg  = NULL;
    struct mgmt_msg *recvmm = NULL;
    char *adaptedBurst      = NULL;
    char *ptr               = NULL;
    msObject_NT *targetMS   = NULL;
    rsObject_NT *targetRS   = NULL;
    int cid                 = -1;
    int len                 = 0;
    int plen                = 0;
    int burstLen            = 0;
    int uiuc                = 0;
    unsigned int crc        = 0;
    //printf("Time:%llu BS(%d) receive this packet of CID(%d)\n", GetCurrentTime(), get_nid(), cid);
    int nid                 = 0;
    int pid                 = 0;

    recvPacket = (Packet *) epkt->DataInfo_;
    assert(epkt && recvPacket);

    epkt->DataInfo_ = NULL;
    freePacket(epkt);

    LastSignalInfo  = (PHYInfo *) recvPacket->pkt_getinfo("phyInfo");
    adaptedBurst    = (char *) recvPacket->pkt_sget();
    burstLen        = recvPacket->pkt_getlen();

    assert(LastSignalInfo);

    uiuc = LastSignalInfo->uiuc;
    nid = LastSignalInfo->nid;
    pid = LastSignalInfo->pid;
   // logRidvan(TRACE,"pid %d\tnid %d",pid,nid);
    mac802_16j_NT_PMPMS *pMS = NULL;

    if (uiuc == CDMA_BWreq_Ranging)
    {
        uint8_t codeIndex   = 0;
        uint8_t code[18];
        uint8_t usage       = 0;
        //printf("\e[1;35mTime:%llu BS[%d] process Node[%d] CDMA_BWreq_Ranging\e[0m\n",GetCurrentTime(),get_nid(),LastSignalInfo->nid);

        if(LastSignalInfo->relay_flag)
            ULoccupiedSymbols = ULRelayoccupiedSymbols;
        else
            ULoccupiedSymbols = ULAccessoccupiedSymbols;

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
        //printf("usage=%d | symOffset = %d ,ULoccupiedSymbols =%d(%d)\n",usage,LastSignalInfo->symOffset,ULoccupiedSymbols,(ULoccupiedSymbols / 3) * 2);

        if (codeIndex == 255)
        {
            ; // ranging code not found
        }
        else if( usage == RS_INITIAL_RANGING )
        {   // RS initial ranging code
            //    printf("Time:%llu BS::recv() add RS(%d)to RS_RangingCodeList\n",GetCurrentTime(),LastSignalInfo->nid);
            RS_RangingCodeList.push_back(new RangingObject(codeIndex, frameNumber & 0xFF, LastSignalInfo->symOffset,  LastSignalInfo->chOffset, usage, GetCurrentTime()));

        }
        else    // MS initial ranging & MS period ranging
        {
            //printf("Time:%llu BS receive MS(%d) RangingCode (relay_flag=%d)\n",GetCurrentTime(),LastSignalInfo->nid,LastSignalInfo->relay_flag);
            // MS initial ranging & MS period ranging

            pMS = getMS(nid,pid);
            if(!pMS->AS)
            {
                pMS->AS = this;
            }

            if(pMS->AS != this)
            {
                //delete  recvPacket;
                //return 1;
            }
            else
            {
            RangingCodeList.push_back(new RangingObject(codeIndex, frameNumber & 0xFF, LastSignalInfo->symOffset,     LastSignalInfo->chOffset, usage, GetCurrentTime()));
            }
        }

    }//CDMA_BWreq_Ranging
    else
    {
        for (ptr = adaptedBurst;ptr + sizeof(struct hdr_generic) < adaptedBurst + burstLen;ptr += plen)
        {
            // Get generic mac header
            hg      = (struct hdr_generic *) ptr;
            cid     = GHDR_GET_CID(hg);
            plen    = GHDR_GET_LEN(hg);

            // None
            if (hcs_16j_NT(ptr, sizeof(struct hdr_generic)) != 0)//CRC check
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
                crc = crc32_16j_NT(0, ptr, len);
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
                    if(LastSignalInfo->relay_flag) /* RS RNG-REQ*/
                    {
                        procRNGREQ(recvmm, cid, len - sizeof(struct hdr_generic));
                    }
                    else
                    {
                        pMS = getMS(nid,pid);
                        if(pMS->AS == this)
                        {
                            procRNGREQ(recvmm, cid, len - sizeof(struct hdr_generic));
                        }
                        else
                        {
                            //printf("procRNGREQ MS[%d]'s Access Station is not BS\n",pMS->get_nid());
                        }
                    }
                }
            }
            else if ((targetRS = getRSbycid(cid)) != NULL) // Basic CID or Primary CID
            {
                if (recvmm->type == MG_RNGREQ) // recv RNG-REQ
                {
                    // relay MS RNG-REQ
                    procRNGREQ(recvmm, cid, len - sizeof(struct hdr_generic));
                }
                else
                {
                    targetRS->handle(recvmm, cid, len - sizeof(struct hdr_generic),frameNumber);
                }
            }
            else if ((targetMS = getMSbycid(cid)) != NULL) // Basic CID or Primary CID
            {

                if((targetMS->accessStation != this && LastSignalInfo->relay_flag)
                        || ((targetMS->accessStation == this && !LastSignalInfo->relay_flag)))
                {
                	logRidvan(WARN,"msobject handle nid:%d cid:%d",get_nid(),cid);
                    targetMS->handle(recvmm, cid, len - sizeof(struct hdr_generic));
                }
            }
            else // Secondary CID or Transport CID
            {
            	struct ip *ip = (struct ip *)(ptr+sizeof(hdr_generic));
                vector<ConnectionReceiver *>::iterator iter;
                char *ptr2 = NULL;
                logRidvan(WARN,"BS recv ip_src:%s ip_dst:%s",ipToStr(ip->ip_src),ipToStr(ip->ip_dst));
                targetMS = getMSbyDtcid(cid);
                assert(targetMS);

                if((targetMS->accessStation != this && LastSignalInfo->relay_flag)
                        || ((targetMS->accessStation == this && !LastSignalInfo->relay_flag)))
                {
                    //printf("\e[1;35mTime:%llu BS[%d]::%s() MS(%d)'s data cid = %d\e[0m\n",GetCurrentTime(),get_nid(),__func__,targetMS->myMS->get_nid(),cid);
                    for (iter = ReceiverList.begin();iter != ReceiverList.end(); iter++)
                    {
                        if ((*iter)->getCid() == cid)
                        {
                            (*iter)->insert(hg, len);
                            while ((ptr2 = (*iter)->getPacket(len)) != NULL)
                            {
                                //printf("MS(%d) data cid=%d\n",targetMS->myMS->get_nid(),cid);
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
    }

    delete recvPacket;
    return 1;
}

int mac802_16j_NT_PMPBS::send(ePacket_ *epkt)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::send ",get_nid());


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

Connection *mac802_16j_NT_PMPBS::CreateDataConnection(msObject_NT *pMS)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::CreateDataConnection ",get_nid());



    for (int i = 0; i < MAXCONN; i++)
    {
        if (conlist[i] != NULL)
        {
            continue;
        }

        if (_CSType == csIPv4)
        {
            conlist[i] = new DataConnection(i + 2 * MAXMS + 2 * MAXRS + 1);
        }
        else
        {
            conlist[i] = new DataConnectionEthernet(i + 2 * MAXMS + 2 * MAXRS + 1);
        }

        pMS->DtConnections.push_back(conlist[i]);
        ReceiverList.push_back(new ConnectionReceiver(i + 2 * MAXMS + 2 * MAXRS + 1));
        logRidvan(TRACE,"bs data connecton %d created ms %s",i + 2 * MAXMS + 2 * MAXRS + 1, pMS->address()->str());
        return conlist[i];
    }
    return NULL;
}

/*
 * When MS performed handover, BS shall remove the previous resourse for that MS
 */
void mac802_16j_NT_PMPBS::RemoveMS(msObject_NT *pMS)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::RemoveMS ",get_nid());


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

ServiceFlow *mac802_16j_NT_PMPBS::GetProvisionedFlow(uint8_t mac[6])
{
    return PsTable->GetServiceFlow(mac);
}

ServiceFlow *mac802_16j_NT_PMPBS::GetProvisionedFlow(uint32_t flowId)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::GetProvisionedFlow ",get_nid());


    return PsTable->GetServiceFlow(flowId);
}

ServiceClass *mac802_16j_NT_PMPBS::GetServiceClass(uint32_t qosIndex)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::GetServiceClass ",get_nid());


    return ScTable->GetServiceClass(qosIndex);
}

msObject_NT *mac802_16j_NT_PMPBS::CreateMS(uint8_t *mac,uint8_t nid,uint8_t pid)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::CreateMS ",get_nid());


    mac802_16j_NT_PMPMS *pMS = NULL;
    pMS = (mac802_16j_NT_PMPMS *)InstanceLookup(nid, pid, "MAC802_16J_NT_PMPMS");
    if(pMS == NULL){
        printf("\e[1;34m***BS(%d)::%s:Get MS(nid = %d , pid = %d)) module failed!***\e[0m\n", get_nid(), __func__,LastSignalInfo->nid, LastSignalInfo->pid);
        exit(1);
    }
    else{
        //printf("\e[1;34m***BS(%d)::%s:Get MS(nid = %d , pid = %d)) module success!***\e[0m\n", get_nid(), __func__,LastSignalInfo->nid, LastSignalInfo->pid);
    }

    //MS BCID : 1~255 ,PCID : 256~510
    for (int i = 0; i < MAXMS; i++)
    {
        if (mslist[i] == NULL)
        {
            int BasicCID = i + 1 ;
            int PriCID = MAXMS + i + 1;
            int SecCID = 0;
            printf("\e[1;34mTime:%llu BS::CreateMS (BasicCID=%d,PriCID=%d)\e[0m\n",GetCurrentTime(),BasicCID,PriCID);
            mslist[i] = new msObject_NT(mac, BasicCID, PriCID, SecCID, this,pMS);
            return mslist[i];
        }
    }

    // mslist is full
    return NULL;
}

rsObject_NT *mac802_16j_NT_PMPBS::CreateRS(uint8_t *mac)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::CreateRS ",get_nid());


    mac802_16j_NT_PMPRS *pRS = NULL;
    pRS = (mac802_16j_NT_PMPRS *)InstanceLookup(LastSignalInfo->nid, LastSignalInfo->pid, "MAC802_16J_NT_PMPRS");
    if(pRS == NULL){
        printf("\e[1;34m***BS(%d)::%s:Get RS(nid = %d , pid = %d)) module failed!***\e[0m\n", get_nid(), __func__,LastSignalInfo->nid, LastSignalInfo->pid);
    }

    //RS BCID : 1~255 ,PCID : 256~510
    for (int i = 0; i < MAXRS; i++)
    {
        if (rslist[i] == NULL)
        {
            int BasicCID = i + 1 + 2*MAXMS;
            int PriCID = MAXRS + BasicCID;
            int SecCID = 0;
            //     printf("\e[1;34mTime:%llu BS::CreateRS (BasicCID=%d,PriCID=%d)\e[0m\n",GetCurrentTime(),BasicCID,PriCID);
            rslist[i] = new rsObject_NT(mac, BasicCID, PriCID, SecCID, this, pRS);
            return rslist[i];
        }
    }

    // RSlist is full
    return NULL;
}

/*
 * Map each MAC SDU onto a particular connection (queue) for later transmission.
 */
bool mac802_16j_NT_PMPBS::DownlinkClassifier(Packet *pkt)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::DownlinkClassifier ",get_nid());


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
                        //printf("BS(%d) recv pkt (num=%d) (cid=%d)\n", get_nid(), conlist[i]->nf_pending_packet(),conlist[i]->cid);
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

void mac802_16j_NT_PMPBS::handleRangingCode()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::handleRangingCode ",get_nid());


    ifmgmt *ifmm            = NULL;
    uint32_t rangAttribute  = 0;
    int timingAdjust        = 0;
    int nPSs                = 0;
    uint64_t ticks          = 0;
    double diff             = 0.0;
    uint8_t rsv             = 0;
    list<RangingObject *>::iterator its1, its2;

    //printf("Time:%llu BS[%d]::%s()\n",GetCurrentTime(),get_nid(),__func__);

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

        if((*its1)->rangingUsage == INITIAL_RANGING || (*its1)->rangingUsage == HANDOVER_RANGING )
        {
            //printf("Time:%llu=**BS(%d)::%s**==\n",GetCurrentTime(),get_nid(),__func__);
            //            printf("Time:%llu=**BS(%d)**== rangingUsage=%d == rangingCodeIndex =%d\n",GetCurrentTime(), get_nid(), (*its1)->rangingUsage, (*its1)->rangingCodeIndex);
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
            //printf("\n---Time:%llu BS::%s()---\n",GetCurrentTime(),__func__);
            //printf("nPSs=%d ticks=%llu\n",nPSs,ticks);
            //printf("frameStartTime = %llu recv_time=%llu\n",frameStartTime,(*its1)->recv_time);

            /*
                        uint64_t offset_ticks = 0;
                        if ( (frameNumber & 0XFF) < (*its1)->rangingFrameNumber)
                        MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) + 256 - (*its1)->rangingFrameNumber) * frameDuration_NT[*_frameDuCode]);
                        else
                        MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) - (*its1)->rangingFrameNumber) *
                        frameDuration_NT[*_frameDuCode]);
                        ticks += (frameStartTime - offset_ticks);
                        */
            ticks += frameStartTime;
            TICK_TO_MICRO(diff, (int) (ticks - (*its1)->recv_time));
            timingAdjust = (int) (diff * 1024.0 / Tb());    // Tb/Nfft : sampling time

            if (timingAdjust > 10 || timingAdjust < -10) // FIXME: Need to check the value -10 ~ 10
            {
                printf("\e[1;36mTime:%llu BS(%d)::%s timingAdjust = %d (Initial)\e[0m\n", GetCurrentTime(), get_nid(),__func__, timingAdjust);
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
        else if ((*its1)->rangingUsage == PERIOD_RANGING)   // MS or RS period ranging
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
            /*
                uint64_t offset_ticks = 0;
                if ( (frameNumber & 0XFF) < (*its1)->rangingFrameNumber)
                MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) + 256 - (*its1)->rangingFrameNumber) * frameDuration_NT[*_frameDuCode]);
                else
                MILLI_TO_TICK(offset_ticks , ((frameNumber & 0XFF) - (*its1)->rangingFrameNumber) *
                frameDuration_NT[*_frameDuCode]);
                ticks += (frameStartTime - offset_ticks);
                */
            ticks += frameStartTime;

            TICK_TO_MICRO(diff, (int) (ticks - (*its1)->recv_time));
            timingAdjust = (int) (diff * 1024.0 / Tb());

            if (timingAdjust > 10 || timingAdjust < -10) // FIXME: Need to check the value -10 ~ 10
            {
                printf("\e[1;36mTime:%llu BS(%d) timingAdjust = %d(Period)\e[0m\n", GetCurrentTime(), get_nid(), timingAdjust);
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


void mac802_16j_NT_PMPBS::handleRSRangingCode()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::handleRSRangingCode ",get_nid());


    ifmgmt *ifmm            = NULL;
    uint32_t rangAttribute  = 0;
    int timingAdjust        = 0;
    int nPSs                = 0;
    uint64_t ticks          = 0;
    double diff             = 0.0;
    uint8_t rsv             = 0;
    list<RangingObject *>::iterator its1, its2;

    uint64_t frame_delay_ticks = 0;
    uint64_t tick_diff = 0;
    MILLI_TO_TICK(frame_delay_ticks, frameDuration_NT[*_frameDuCode]);  // delay one frame : 5ms

    // Check if collision
    for (its1 = RS_RangingCodeList.begin();its1 != RS_RangingCodeList.end();its1++)
    {
        for (its2 = RS_RangingCodeList.begin();its2 != RS_RangingCodeList.end();its2++)
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

    for (its1 = RS_RangingCodeList.begin();its1 != RS_RangingCodeList.end();its1++)
    {
        if ((*its1)->collision == true)
            continue;

        if((*its1)->rangingUsage == RS_INITIAL_RANGING)
        {
            //            printf("Time:%llu=**BS(%d)::%s**==\n",GetCurrentTime(),get_nid(),__func__);
            //printf("frameNumber=%d , rangingFrameNumber =%d\n",frameNumber&0xFF,(*its1)->rangingFrameNumber&0xFF);

            ifmm = new ifmgmt(MG_RNGRSP, 1);
            ifmm->appendField(1, rsv);

            // TLV infomation
            rangAttribute = ((*its1)->rangingSymbol      & 0x3FF) << 22 |
                ((*its1)->rangingSubchannel  & 0x3F ) << 16 |
                ((*its1)->rangingCodeIndex   & 0xFF ) << 8  |
                ((*its1)->rangingFrameNumber & 0xFF );

            // Compute time adjustment
            nPSs = pScheduler->SearchULRelayTime((*its1)->rangingSymbol) + symbolsToPSs(2); // over two symbols
            MICRO_TO_TICK(ticks, PSsToMicro(nPSs));
            ticks += frameStartTime;

            //printf("ticks=%llu recv_time=%llu\n",ticks,(*its1)->recv_time);

            if((*its1)->recv_time < ticks)
            {
                tick_diff = ticks - (*its1)->recv_time - frame_delay_ticks;
            }
            else
            {
                tick_diff = ticks - (*its1)->recv_time;
            }

            TICK_TO_MICRO(diff, (int)tick_diff);
            timingAdjust = (int) (diff * 1024.0 / Tb());

            if (timingAdjust > 10 || timingAdjust < -10) // FIXME: Need to check the value -10 ~ 10
            {
                printf("\e[1;36mTime:%llu BS(%d)::%s timingAdjust = %d (Initial)\e[0m\n", GetCurrentTime(), get_nid(),__func__, timingAdjust);
                ifmm->appendTLV(1, 4, timingAdjust); // unit: 1/Fs
                ifmm->appendTLV(4, 1, 1);   // Ranging Status: 1=continue
            }
            else
            {
                ifmm->appendTLV(4, 1, 3);   // Ranging Status: 3=success
                (*its1)->allocated = true;  // BS shall provide BW allocation using CDMA_Allocation_IE
            }
            ifmm->appendTLV(150, 4, rangAttribute);     // Ranging Code Attributes

            initRangingConnection_relay->Insert(ifmm);    // using Initial Ranging Connection
        }
        else if ((*its1)->rangingUsage == PERIOD_RANGING)   // RS period ranging
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
                printf("\e[1;36mTime:%llu BS(%d) timingAdjust = %d (period)\e[0m\n", GetCurrentTime(), get_nid(), timingAdjust);
                ifmm->appendTLV(1, 4, timingAdjust); // unit: 1/Fs
                ifmm->appendTLV(4, 1, 1);   // Ranging Status: 1=continue

            }
            else
            {
                ifmm->appendTLV(4, 1, 3);   // Ranging Status: 3=success
            }
            ifmm->appendTLV(150, 4, rangAttribute);     // Ranging Code Attributes

            initRangingConnection_relay->Insert(ifmm);    // using Initial Ranging Connection
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

void mac802_16j_NT_PMPBS::DLRelayPacketScheduling()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::DLRelayPacketScheduling ",get_nid());


    ePacket_ *epkt          = NULL;
    BurstInfo *burstInfo    = NULL;

    // Scheduling DL Relay Burst
    burstInfo = new BurstInfo;
    //printf("new BurstInfo (DL relay) %x\n",burstInfo);
    burstInfo->Collection = (vector <WiMaxBurst *> *)(pScheduler->DLRelayBurstCollection);

    burstInfo->flag = PF_SEND;
    burstInfo->type = DL_RELAY;

    if (burstInfo->Collection != NULL)
    {
        //printf("Time:%llu ================BS[%d]::%s() send DL relay burst(type = %d)================\n", GetCurrentTime(), get_nid(),__func__,burstInfo->type);
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
 * Periodically transmit downlink subframe.
 */
void mac802_16j_NT_PMPBS::PacketScheduling()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::PacketScheduling ",get_nid());


    ePacket_ *epkt          = NULL;
    BurstInfo *burstInfo    = NULL;
    uint64_t ticks = 0;

    //printf("Time:%llu ==================BS(%d) PacketScheduling==================\n", GetCurrentTime(), get_nid());

    // Before scheduling, we need to check the ranging code
    handleRangingCode();
    handleRSRangingCode();

    /* Update MS status (scanning or handover) */ // FIXME: only consider that iteration times = 1
    for (int i = 0;i < MAXMS;i++)
    {
        msObject_NT *pMS = mslist[i];

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
    //printf("new BurstInfo (DL access) %x\n",burstInfo);

    // Set per frame infomation
    frameStartTime = GetCurrentTime();
    frameNumber++;
    //printf("%s(%d) frameStartTime = %llu\n",__func__,__LINE__,frameStartTime);

    // Scheduling DL Access Burst
    burstInfo->Collection = pScheduler->Scheduling();
    burstInfo->flag = PF_SEND;
    burstInfo->type = DL_ACCESS;

    if (burstInfo->Collection != NULL)
    {
        epkt = createEvent();
        epkt->DataInfo_ = burstInfo;
        put(epkt, sendtarget_);

        //	printf("Time:%llu ================BS(%d) send DL access burst(type=%d)================\n", GetCurrentTime(), get_nid(),burstInfo->type);
    }
    else
    {
        delete burstInfo;
    }

    // Erase RangingCodeList
    ClearRNGCodeList(&RangingCodeList);
    ClearRNGCodeList(&RS_RangingCodeList);

    // Set the state of PHY to Busy to sending packets
    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setStateBusy_access();
    // we will change the state to Idle in phy (after DLAccessSymbols)

    // After pScheduler->Scheduling() , the DLRelayBurstCollection is ready.
    MICRO_TO_TICK(ticks,PSsToMicro(pScheduler->RelayDLallocStartTime));
    timerSendDLRelayBurst->start(ticks,0);

    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setStateBusy_relay();
    // we will change the state to Idle in phy (after DLRelaySymbols)

}

/*
 * Generate a MAC PDU containing UCD.
 */
void mac802_16j_NT_PMPBS::generate_UCD()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::generate_UCD ",get_nid());


    //printf("Time:%llu == %s ==\n",GetCurrentTime(),__func__);
    pScheduler->generate_UCD();
    return;
}

/*
 * Generate a MAC PDU containing DCD.
 */
void mac802_16j_NT_PMPBS::generate_DCD()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::generate_DCD ",get_nid());


    //printf("Time:%llu == %s ==\n",GetCurrentTime(),__func__);

    pScheduler->generate_DCD();
    return;
}

/*
 * Generate a MAC PDU containing DCD.
 */
void mac802_16j_NT_PMPBS::generate_RCD()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::generate_RCD ",get_nid());


    //pScheduler->generate_RCD();
    return;
}

/*
 * Generate a MAC PDU containing RS_AccessMAP.
 */

/*
 * Generate a MAC PDU containing MOB_NBR-ADV
 */
void mac802_16j_NT_PMPBS::generate_MOB_NBRADV()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::generate_MOB_NBRADV ",get_nid());



    //printf("Time:%llu == %s ==\n",GetCurrentTime(),__func__);
    pScheduler->generate_MOB_NBRADV();
    return;
}

msObject_NT *mac802_16j_NT_PMPBS::getMSbyDtcid(uint16_t cid)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::getMSbyDtcid ",get_nid());


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

msObject_NT *mac802_16j_NT_PMPBS::getMSbycid(int cid)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::getMSbycid ",get_nid());


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

rsObject_NT *mac802_16j_NT_PMPBS::getRSbycid(int cid)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::getRSbycid ",get_nid());


    int RS_start = 2 * MAXMS;
    if (cid >= (RS_start+1) && cid <= (RS_start + MAXRS))   // Basic CID of RS
    {
        return rslist[cid - 1 - RS_start];
    }
    else if (cid >= RS_start + MAXRS + 1 && cid <= (RS_start + 2*MAXRS))    // Primary CID of RS
    {
        return rslist[cid - 1 - MAXRS - RS_start];
    }
    else
    {
        return NULL;
    }
}

void mac802_16j_NT_PMPBS::saveDLMAP(char *dstbuf, int len)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveDLMAP ",get_nid());


    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->saveDLMAP(dstbuf, len);
}

void mac802_16j_NT_PMPBS::saveRMAP(char *dstbuf, int len)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveRMAP ",get_nid());


    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->saveRMAP(dstbuf, len);
}

void mac802_16j_NT_PMPBS::changeStateToIdle()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::changeStateToIdle ",get_nid());


    //printf("Time:%llu == %s ==\n",GetCurrentTime(),__func__);
    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setStateIdle();
}

void mac802_16j_NT_PMPBS::saveDLsymbols(int Nsymbols)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveDLsymbols ",get_nid());


    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setDLsymbols(Nsymbols);
}

void mac802_16j_NT_PMPBS::saveDLAccessSymbols(int Nsymbols)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveDLAccessSymbols ",get_nid());


    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setDLAccessSymbols(Nsymbols);
}

void mac802_16j_NT_PMPBS::saveDLRelaySymbols(int Nsymbols)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveDLRelaySymbols ",get_nid());


    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setDLRelaySymbols(Nsymbols);
}

void mac802_16j_NT_PMPBS::saveULsymbols(int Nsymbols)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveULsymbols ",get_nid());


    ULoccupiedSymbols = Nsymbols;
    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setULsymbols(Nsymbols);
}

void mac802_16j_NT_PMPBS::saveULAccessSymbols(int Nsymbols)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveULAccessSymbols ",get_nid());


    ULAccessoccupiedSymbols = Nsymbols;
    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setULAccessSymbols(Nsymbols);
}

void mac802_16j_NT_PMPBS::saveULRelaySymbols(int Nsymbols)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::saveULRelaySymbols ",get_nid());


    ULRelayoccupiedSymbols = Nsymbols;
    ((OFDMA_PMPBS_NT *)sendtarget_->bindModule())->setULRelaySymbols(Nsymbols);
}

/*
 * Figure 62
 */
void mac802_16j_NT_PMPBS::procRNGREQ(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::procRNGREQ ",get_nid());


    ifmgmt *ifmm_req        = NULL;
    ifmgmt *ifmm_rsp        = NULL;
    ifmgmt *ifmm_rsp_relay        = NULL;
    ifmgmt *ifmm_rsp_relay_ms     = NULL;
    msObject_NT *pCurrentMS = NULL;
    rsObject_NT *pCurrentRS = NULL;
    rsObject_NT *pRelayRS   = NULL;
    int i                   = 0;
    int type                = 0;
    int fec                 = 0;
    uint8_t fprofile        = 1;
    uint8_t fmac[6]         = "";
    uint8_t rsv             = 0;
    uint8_t fanomalies      = 0;
    uint16_t frelayrsbcid   = 0;
    int nPSs                = 0;
    uint64_t ticks          = 0;
    double diff             = 0.0;
    int timingAdjust        = 0;
    uint8_t fmsnid          = 0;
    uint8_t fmspid          = 0;
    uint8_t nid             = 0;
    uint8_t pid             = 0;
    uint8_t fmsdiuc         = 0;
    uint8_t fmsduration     = 0;
    bool    relay           = false;

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
                break;

            case 99:
                ifmm_req->getNextValue(&frelayrsbcid);
                //printf("relay RS Bcid = %d\n",frelayrsbcid);
                //printf("LastSignalInfo->RS_RNGREQ_flag = %d\n",LastSignalInfo->RS_RNGREQ_flag);
                break;

            case 100:
                ifmm_req->getNextValue(&fmsdiuc);
                //printf("MS diuc = %d\n",fmsdiuc);
                relay = true;
                break;

            case 101:
                ifmm_req->getNextValue(&fmsnid);
                //printf("relay MS nid = %d\n",fmsnid);
                break;

            case 102:
                ifmm_req->getNextValue(&fmspid);
                //printf("relay MS pid = %d\n",fmspid);
                break;

            case 103:
                ifmm_req->getNextValue(&fmsduration);
                //printf("relay MS ie_14 duration = %d\n",fmsduration);
                break;


            default:
                break;
        }
    }
    delete ifmm_req;


    //printf("\e[36m***Time:%llu mac802_16j_NT_PMPBS::%s() [MAC] %x:%x:%x:%x:%x:%x, cid = %d***\e[0m\n", GetCurrentTime(),__func__, fmac[0], fmac[1],fmac[2], fmac[3], fmac[4], fmac[5], cid);

    // process RNG-REQ from RS

    if(LastSignalInfo->RS_RNGREQ_flag == true)
    {
        //printf("Time:%llu Receive RNG-REQ from RS(%d) \n",GetCurrentTime(),LastSignalInfo->nid);
        //printf("timingAdjust | (%d,%d)\n",LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC,ULoccupiedSymbols);
        /* Get RS or a new MS */
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

            pCurrentRS->myRS->access_station = (mac802_16j_NT *)this;
            pCurrentRS->diuc = fprofile;

            // Decide Current MS's uiuc depend on LinkMode
            if (LinkMode == NULL || strcasecmp(LinkMode, "Auto") == 0)
            {
                pCurrentRS->uiuc = 1;    // Default: QPSK_1_2

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
                        pCurrentMS->uiuc = i;
                        break;
                    }
                }
                //printf("BS(%d) Manual Mode: uiuc=%d(fec=%d)\n", get_nid(), pCurrentRS->uiuc, fec);
            }
            else
            {
                printf("Warning Configure String LinkMode:%s\n", LinkMode);
            }
            printf("\e[32mTime:%llu BS(%d) Accept New RS:LinkMode=%s (SNR=%lf dB), DIUC=%d UIUC=%d\e[0m\n", GetCurrentTime(), get_nid(), LinkMode, LastSignalInfo->SNR, pCurrentRS->diuc, pCurrentRS->uiuc);
        }
    }
    else
    {
        //printf("Time:%llu Receive RNG-REQ from MS(%d) \n",GetCurrentTime(),LastSignalInfo->nid);
        //printf("timingAdjust | (%d,%d)\n",LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC,ULoccupiedSymbols);

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
            if(relay)
            {
                nid = fmsnid;
                pid = fmspid;
            }
            else
            {
                nid = LastSignalInfo->nid;
                pid = LastSignalInfo->pid;
            }

            pCurrentMS = CreateMS(fmac,nid,pid);
            if (pCurrentMS == NULL)
            {
                printf("CreateMS error: pCurrentMS is NULL\n");
                return;
            }

            if(relay)
            {
                if(frelayrsbcid)
                {   int r = 0;
                    for (r = 0; r < MAXRS; r++)
                    {
                        if (rslist[r]->BasicCID == frelayrsbcid)
                        {
                            pRelayRS = rslist[r];
                            break;
                        }
                    }
                    if (r == MAXRS)        // Not find relay RS
                    {
                        printf("Relay RS not found !!!\n");
                        exit(1);
                    }
                }
                else
                {
                    printf("Relay RS not found !!!\n");
                    exit(1);
                }
                printf("\e[1;35m=== Time:%llu BS[%d] decide MS(%d)'s Access Station is RS(%d) ===\e[0m\n",GetCurrentTime(),get_nid(),nid,((mac802_16j_NT *)(pRelayRS->myRS))->get_nid());
                pCurrentMS->accessStation = (mac802_16j_NT *)(pRelayRS->myRS);
                pCurrentMS->diuc = fmsdiuc;
            }
            else
            {
                printf("\e[1;35m=== Time:%llu BS[%d] decide MS(%d)'s Access Station is BS(%d) ===\e[0m\n",GetCurrentTime(),get_nid(),nid,get_nid());
                pCurrentMS->accessStation = this;
                pCurrentMS->diuc = fprofile;
            }
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
                if(relay)
                    fec = DCDProfile[fmsdiuc].fec;
                else
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

            if(relay)
            {   // relay by RS
                printf("\e[32mTime:%llu BS(%d) Accept New MS:LinkMode=%s (RS SNR=%lf dB)\e[0m\n", GetCurrentTime(), get_nid(), LinkMode, LastSignalInfo->SNR);

            }
            else
            {
                printf("\e[32mTime:%llu BS(%d) Accept New MS:LinkMode=%s (SNR=%lf dB), DIUC=%d UIUC=%d\e[0m\n", GetCurrentTime(), get_nid(), LinkMode, LastSignalInfo->SNR, pCurrentMS->diuc, pCurrentMS->uiuc);
            }
        }

        //printf("AS Bcid = %d\n",((mac802_16j_NT_PMPRS *)((pCurrentMS->myMS)->AS))->BasicCID);
    }

    uint64_t tick1,tick2;
    MICRO_TO_TICK(tick1,PSsToMicro(symbolsToPSs(ULAccessoccupiedSymbols)));
    MICRO_TO_TICK(tick2,PSsToMicro(symbolsToPSs(ULRelayoccupiedSymbols)));

    //printf("\nULAccessoccupiedSymbols = %d (%llu ticks) , ULRelayoccupiedSymbols = %d (%llu ticks)\n",ULAccessoccupiedSymbols,tick1,ULRelayoccupiedSymbols,tick2);

    if(LastSignalInfo->RS_RNGREQ_flag && LastSignalInfo->relay_flag)
    {   // RS RNG-REQ
        if (LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC > ULRelayoccupiedSymbols)
        {
            nPSs = pScheduler->SearchULRelayTime(ULRelayoccupiedSymbols);
        }
        else
        {
            nPSs = pScheduler->SearchULRelayTime(LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC);
        }
        //printf("Time:%llu RS nPSs=%d\n",GetCurrentTime(),nPSs);
    }
    else if(!LastSignalInfo->RS_RNGREQ_flag && !LastSignalInfo->relay_flag)
    {   // MS RNG-REQ
        /* Compute time adjustment */
        if (LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC > ULAccessoccupiedSymbols)
        {
            nPSs = pScheduler->SearchULTime(ULAccessoccupiedSymbols);
        }
        else
        {
            nPSs = pScheduler->SearchULTime(LastSignalInfo->ulmap_ie.ie_14.duration * UL_PUSC);
        }
        //printf("Time:%llu MS nPSs=%d\n",GetCurrentTime(),nPSs);
    }
    else if(!LastSignalInfo->RS_RNGREQ_flag && relay)
    {   // Relay MS RNG-REQ
        if (fmsduration*UL_PUSC > ULRelayoccupiedSymbols)
        {
            nPSs = pScheduler->SearchULRelayTime(ULRelayoccupiedSymbols);
        }
        else
        {
            nPSs = pScheduler->SearchULRelayTime(fmsduration*UL_PUSC);
        }
    }

    MICRO_TO_TICK(ticks, PSsToMicro(nPSs));
    //printf("Time:%llu BS recv Node[%d] | frameStartTime=%llu ticks=%llu PS=%lf\n",GetCurrentTime(),LastSignalInfo->nid,frameStartTime,ticks,1024.0 / Tb());
    ticks += frameStartTime;

    //uint64_t tick1;
    //MICRO_TO_TICK(tick1,PSsToMicro(pScheduler->RelayDLallocStartTime));
    //printf("Time:%llu BS::%s off=%llu\n",GetCurrentTime(),__func__,ticks);

    timingAdjust = (int) (diff * 1024.0 / Tb());

    /* Generate RNG-RSP */
    ifmm_rsp = new ifmgmt(MG_RNGRSP, 1);    // Table 365
    ifmm_rsp->appendField(1, rsv);

    ifmm_rsp_relay = new ifmgmt(MG_RNGRSP, 1);    // Table 365
    ifmm_rsp_relay->appendField(1, rsv);

    ifmm_rsp_relay_ms = new ifmgmt(MG_RNGRSP, 1);    // Table 365
    ifmm_rsp_relay_ms->appendField(1, rsv);

    /*
     * TLV encoded information
     */
    if (timingAdjust > 10 || timingAdjust < -11) // FIXME: Need to check the value -10 ~ 10
    {
        printf("\e[1;36mTime:%llu BS(%d)::%s timingAdjust = %d from Node[%d]\e[0m\n", GetCurrentTime(), get_nid(),__func__, timingAdjust,LastSignalInfo->nid);
        ifmm_rsp->appendTLV(4, 1, 1);            // Ranging Status / 8bit / 1=continue, 2=abort, 3=Success
        ifmm_rsp->appendTLV(1, 4, timingAdjust); // Timing Adjust. Unit: 1/Fsample

        ifmm_rsp_relay->appendTLV(4, 1, 1);            // Ranging Status / 8bit / 1=continue, 2=abort, 3=Success
        ifmm_rsp_relay->appendTLV(1, 4, timingAdjust); // Timing Adjust. Unit: 1/Fsample

        ifmm_rsp_relay_ms->appendTLV(4, 1, 1);            // Ranging Status / 8bit / 1=continue, 2=abort, 3=Success
        ifmm_rsp_relay_ms->appendTLV(1, 4, timingAdjust); // Timing Adjust. Unit: 1/Fsample
    }
    else
    {
        //printf("\e[1;32mTime:%llu BS(%d)::%s timingAdjust = %d from Node[%d] success\e[0m\n", GetCurrentTime(), get_nid(),__func__, timingAdjust,LastSignalInfo->nid);
        ifmm_rsp->appendTLV(4, 1, 3);  // Success

        ifmm_rsp_relay->appendTLV(4, 1, 3);  // Success

        ifmm_rsp_relay_ms->appendTLV(4, 1, 3);  // Success
    }

    if(LastSignalInfo->RS_RNGREQ_flag && LastSignalInfo->relay_flag)
    {
        //printf("\e[36mpCurrentRS BasicCID=%d , PriCID=%d , ServerLevelPredict=%d , PeriodRangingInterval=%d\e[0m\n",pCurrentRS->BasicCID,pCurrentRS->PriCID,pCurrentRS->ServerLevelPredict,pCurrentRS->PeriodRangingInterval);
        ifmm_rsp_relay->appendTLV( 8, 6, pCurrentRS->address()->buf());      // RS MAC Address
        ifmm_rsp_relay->appendTLV( 9, 2, pCurrentRS->BasicCID);              // Basic CID
        ifmm_rsp_relay->appendTLV(10, 2, pCurrentRS->PriCID);                // Primary CID
        ifmm_rsp_relay->appendTLV(17, 1, pCurrentRS->ServerLevelPredict);    // Server Level
        ifmm_rsp_relay->appendTLV(26, 2, pCurrentRS->PeriodRangingInterval); // Next Periodic Ranging in frames

        pCurrentRS->myRS->brConnection = new BR_Connection(pCurrentRS->BasicCID);
    }
    else if(!LastSignalInfo->RS_RNGREQ_flag && !LastSignalInfo->relay_flag)
    {
        ifmm_rsp->appendTLV( 8, 6, pCurrentMS->address()->buf());      // MS MAC Address
        ifmm_rsp->appendTLV( 9, 2, pCurrentMS->BasicCID);              // Basic CID
        ifmm_rsp->appendTLV(10, 2, pCurrentMS->PriCID);                // Primary CID
        ifmm_rsp->appendTLV(17, 1, pCurrentMS->ServerLevelPredict);    // Server Level
        ifmm_rsp->appendTLV(26, 2, pCurrentMS->PeriodRangingInterval); // Next Periodic Ranging in frames
    }
    else if(!LastSignalInfo->RS_RNGREQ_flag && relay)
    {
        ifmm_rsp_relay_ms->appendTLV( 8, 6, pCurrentMS->address()->buf());      // Relay MS MAC Address
        ifmm_rsp_relay_ms->appendTLV( 9, 2, pCurrentMS->BasicCID);              // Basic CID
        ifmm_rsp_relay_ms->appendTLV(10, 2, pCurrentMS->PriCID);                // Primary CID
        ifmm_rsp_relay_ms->appendTLV(17, 1, pCurrentMS->ServerLevelPredict);    // Server Level
        ifmm_rsp_relay_ms->appendTLV(26, 2, pCurrentMS->PeriodRangingInterval); // Next Periodic Ranging in frames
        ifmm_rsp_relay_ms->appendTLV(100, 1, fmsnid); // Next Periodic Ranging in frames
        ifmm_rsp_relay_ms->appendTLV(101, 1, fmspid); // Next Periodic Ranging in frames

    }
    else
    {
        fprintf(stderr,"mac802_16j_NT_PMPBS:%s Error condition !\n",__func__);
        exit(1);
    }

    initRangingConnection->Insert(ifmm_rsp);
    initRangingConnection_relay->Insert(ifmm_rsp_relay);
    if(pRelayRS)
    {
        list <uint16_t> cidlist;
        cidlist.push_back(pCurrentMS->BasicCID);
        cidlist.push_back(pCurrentMS->PriCID);
        SendRS_MemberListUpdate(cidlist, true , frelayrsbcid);

        (pRelayRS->MnConnections[0])->Insert(ifmm_rsp_relay_ms);
    }
    else
    {
        delete ifmm_rsp_relay_ms;
    }
}

void mac802_16j_NT_PMPBS::SendRS_MemberListUpdate(list<uint16_t> cidlist, bool flag, uint16_t rsbcid)
{
    ifmgmt *ifmm_memlist;
    rsObject_NT    *pRelayRS    = NULL;

    list <uint16_t>::iterator itec;
    uint8_t  config_para_type = 1;  /* force RS reply Genereic ACK after receive */

    pRelayRS = getRSbycid(rsbcid);

    //printf("\nTime:%llu:mac802_16j_PMPBS::%s(), cid = %d\n", GetCurrentTime(), __func__, cid);

    /* Here , we use reserve b2-bit to indicate add/remove following cid list , Add(1) , Remove(0)
     * b0 = 1 :RS sends acknowledge if the message is received
     */
    if (flag)
            config_para_type |= 0x04;   /* list add */

    ifmm_memlist = new ifmgmt(MG_RS_Member_List_Update, 3);
    ifmm_memlist->appendBitField(4, frameNumber & 0x0F);
    ifmm_memlist->appendBitField(4, config_para_type & 0x0F);
    ifmm_memlist->appendField(2, 1234); //Transaction ID

    /* Append cid TLV list */
    for (itec = cidlist.begin(); itec != cidlist.end(); itec++)
    {
        ifmm_memlist->appendTLV(45, 2 ,(*itec));
    }

    pRelayRS->MnConnections[0]->Insert(ifmm_memlist);
}

mac802_16j_NT_PMPMS *mac802_16j_NT_PMPBS::getMS(uint8_t nid, uint8_t pid)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::getMS ",get_nid());


    mac802_16j_NT_PMPMS *pMS = NULL;

    pMS = (mac802_16j_NT_PMPMS *)InstanceLookup(nid, pid, "MAC802_16J_NT_PMPMS");
    if(!pMS){
        printf("\e[1;35m***BS(%d)::%s:Get MS(nid=%d , pid=%d)) module failed!***\e[0m\n", get_nid(), __func__,nid,pid);
        exit(1);
    }
    else
    {
        //printf("\e[1;33m***BS(%d)::%s:Get MS(nid=%d , pid=%d)) module success!***\e[0m\n", get_nid(), __func__,LastSignalInfo->nid,LastSignalInfo->pid);
        return pMS;
    }
}

uint8_t mac802_16j_NT_PMPBS::getMSindex(uint16_t bcid)
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::getMSindex ",get_nid());


    for(int i=0 ; i<MAXMS ; i++)
    {
        if(mslist[i]->BasicCID == bcid)
            return i;
    }
    return 0;
}

void mac802_16j_NT_PMPBS::SelectMSsPath()
{

	logRidvan(TRACE,"-->%d	mac802_16j_NT_PMPBS::SelectMSsPath ",get_nid());


    //printf("Time:%llu == %s ==\n",GetCurrentTime(),__func__);
}

/*
 *  Table:MCSs/Weights
 *  ====================================================
 *  Burst profile   Bits/Symbol Weight(Symbol/Bit)
 *  QPSK1/2             1           1
 *  QPSK3/4             3/2         2/3
 *  16QAM1/2            2           1/2
 *  16QAM3/4            3           1/3
 *  64QAM1/2            3           1/3
 *  64QAM2/3            4           1/4
 *  64QAM3/4            9/2         2/9
 */



