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

#include "scheduler.h"
#include "math.h"

#define DEBUG   0 

using namespace std;
using namespace MR_MacAddress_NT;
using namespace MR_ServiceFlow_NT;
using namespace MR_Burst_NT;

static bool compare(const void *a, const void *b)
{
    downBurst *oa = (downBurst *) a;
    downBurst *ob = (downBurst *) b;
    return (oa->ie->_diuc) < (ob->ie->_diuc);
}


/* ------------------------------------------------------------------------------------ */
/*                                                                                      */
/*                         Scheduler_NT common functions                                */
/*                                                                                      */
/* ------------------------------------------------------------------------------------ */
downBurst *Scheduler_NT::GetCorrespondBurst(vector<downBurst *> *burstCollection, int diuc)
{
    vector<downBurst *>::iterator   itd;
    downBurst                       *pBurst;

    for (itd = burstCollection->begin(); itd != burstCollection->end(); itd++)
    {
        if ((*itd)->ie->_diuc == diuc)
        {
            //printf("Time:%llu Find downBurst (diuc = %d)\n",GetCurrentTime(),diuc);
            return *itd;
        }
    }

    //printf("Time:%llu Create new downBurst !!! (burst diuc = %d)\n",GetCurrentTime(),diuc);
    pBurst = new downBurst(diuc, DCDProfile[diuc].fec);

    if (diuc == Extended_DIUC_2)
        pBurst->dlmap_ie.ie_14.diuc = Extended_DIUC_2;
    else if (diuc == Extended_DIUC)
        pBurst->dlmap_ie.ie_15.diuc = Extended_DIUC;
    else
        pBurst->dlmap_ie.ie_other.diuc = diuc;

    burstCollection->push_back(pBurst);
    //printf("Time:%llu push new downBurst (diuc = %d)\n",GetCurrentTime(),diuc);
    return pBurst;
}

void Scheduler_NT::mapComputeSymCh(int slots, uint8_t *nSym, uint8_t *nCh)
{

	logRidvan(TRACE,"-->Scheduler_NT::mapComputeSymCh ");
    int totalSymbols = slots * 2;

    if (totalSymbols == 0)
        return;

    if (totalSymbols % 20 != 0)
    {
        fprintf(stderr, "\e[1;31mCompute Slot Allocation Error\e[0m\n");
        exit(0);
    }

    *nSym = totalSymbols / 10;
    *nCh = 10;
}

int Scheduler_NT::computeNearestSlots(int realSlots)
{

	logRidvan(TRACE,"-->Scheduler_NT::computeNearestSlots ");
    int usingCh = 10; // FIXME: Assume each burst will occupy 10 subchannels
    int factor = 0;

    while (usingCh * factor < realSlots)
    {
        factor++;
    }

    return usingCh * factor;
}

void Scheduler_NT::Clear_UL_BurstCollection(vector <upBurst *>* burstCollection)
{
    while(!burstCollection->empty())
    {
        // delete pointer
        delete *(burstCollection->begin());
        burstCollection->erase(burstCollection->begin());
    }
}

void Scheduler_NT::Clear_DL_BurstCollection(vector <downBurst *>* burstCollection)
{
    while(!burstCollection->empty())
    {
        // delete pointer
        delete *(burstCollection->begin());
        burstCollection->erase(burstCollection->begin());
    }
}


/* ------------------------------------------------------------------------------------ */
/*                                                                                      */
/*                                  BS_16j_NT_Scheduler                                 */
/*                                                                                      */
/* ------------------------------------------------------------------------------------ */
vector<WiMaxBurst *> *BS_16j_NT_Scheduler::Scheduling()
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::Scheduling ");
    uint32_t requestLen     = 0;
    uint32_t len            = 0;
    downBurst *burst        = NULL;
    Connection *pConn       = NULL;
    int DLsubchannels       = Station->DLsubchannels();               // (30)
    int ULsubchannels       = Station->ULsubchannels();               // (35)
    int symbolsPerFrame     = Station->symbolsPerFrame();             // (48)
    int ULAvailableSymbols  = (int) (symbolsPerFrame * MAX_UL_RATIO); // (24)
    int DLAvailableSymbols  = symbolsPerFrame - ULAvailableSymbols;   // (24)
    int DLAvailableSlots    = 0;
    //int symbolOffset        = 0;

    // 16j add
    int ULRelayAvailableSymbols   = (int) (ULAvailableSymbols * MAX_UL_RELAY_RATIO); // (12)
    int ULAccessAvailableSymbols  = 0; 
    int DLRelayAvailableSymbols   = 0; 
    int DLAccessAvailableSymbols  = 0;

    int ULRelayAvailableSlots    = (ULRelayAvailableSymbols / UL_PUSC) * (ULsubchannels - 6); // (116)
    int ULAccessAvailableSlots   = 0;
    //int DLRelayAvailableSlots    = 0;
    //int DLAccessAvailableSlots   = 0;

    int ULoccupiedSymbols        = 0;
    int ULAccessoccupiedSymbols  = 0;
    int ULRelayoccupiedSymbols   = 0;
    int DLAccessoccupiedSymbols  = 0;
    int DLRelayoccupiedSymbols   = 0;

    vector<int> PduInfo;

    vector<downBurst *> *DLAccessBurstCollection = new vector<downBurst *>;

    DLRelayBurstCollection = new vector<downBurst *>;

    vector<downBurst *>::iterator itd;
    list<AllocInfo *>::iterator ita;

    /* Clear upBurstCollection */
    Clear_UL_BurstCollection(upBurstCollection);
    Clear_UL_BurstCollection(upRelayBurstCollection);

    // clear ulmap_ie_queue
    while(!ulmap_ie_queue->empty())
    {
        delete *(ulmap_ie_queue->begin());
        ulmap_ie_queue->erase(ulmap_ie_queue->begin());
    }

    for(int i=0 ; i<16 ; i++)
        relay_diuc[i] = false;

    /*----------------------------------------------*/
    /*               Uplink allocation              */
    /*----------------------------------------------*/

    /* UL Relay zone Scheduling */
    ULRelayoccupiedSymbols = ULRelayScheduling(upRelayBurstCollection,ULRelayAvailableSlots);

    // compute ULAccessAvailableSlots to schedule UL access zone 
    ULAccessAvailableSymbols = ULAvailableSymbols - ULRelayoccupiedSymbols;
    ULAccessAvailableSlots = (ULAccessAvailableSymbols / UL_PUSC)*(ULsubchannels - 6);

    /* UL Access zone Scheduling */
    ULAccessoccupiedSymbols = ULAccessScheduling(upBurstCollection,ULAccessAvailableSlots);

    ULoccupiedSymbols = ULAccessoccupiedSymbols + ULRelayoccupiedSymbols;

    /*----------------------------------------------*/
    /*              Downlink allocation             */
    /*----------------------------------------------*/
    DLAvailableSymbols = symbolsPerFrame - ULoccupiedSymbols;
    DLAvailableSlots = (int)((DLAvailableSymbols - PREAMBLE - DL_PUSC) / DL_PUSC) * DLsubchannels;

    ULallocStartTime = Station->symbolsToPSs(DLAvailableSymbols) + Station->TTG();
    generate_UL_MAP(upBurstCollection, ULallocStartTime, ULAccessoccupiedSymbols);

    RelayULallocStartTime = Station->symbolsToPSs(DLAvailableSymbols + ULAccessoccupiedSymbols) + Station->TTG();
    generate_UL_MAP_relay(upRelayBurstCollection, RelayULallocStartTime, ULRelayoccupiedSymbols);

    /* compute the DL Relay zone available symbols and slots */
    DLRelayAvailableSymbols = DLAvailableSymbols * MAX_DL_RELAY_RATIO;

    /* Scheduling DL Relay traffic */
    DLRelayoccupiedSymbols = DLRelay_Scheduling(DLRelayBurstCollection, DLRelayAvailableSymbols);

    /* compute the DL Access zone available symbols and slots */
    DLAccessAvailableSymbols = DLAvailableSymbols - DLRelayoccupiedSymbols;

    /* Scheduling DL Access traffic */
    DLAccessoccupiedSymbols = DLAccess_Scheduling(DLAccessBurstCollection, DLAccessAvailableSymbols);

    RelayDLallocStartTime = Station->symbolsToPSs(DLAccessAvailableSymbols);
    DLsymbols = DLAccessAvailableSymbols;

    /* Generate R-MAP and save to PHY */
    sort(DLRelayBurstCollection->begin(), DLRelayBurstCollection->end(), compare);
    generate_R_MAP(DLRelayBurstCollection,DLRelayoccupiedSymbols);

    /* Encapsulate all Burst */
    for (itd = DLRelayBurstCollection->begin();itd != DLRelayBurstCollection->end(); itd++)
    {
        char *buf       = NULL;
        burst           = *itd;
        buf             = new char [burst->length];
        memset(buf, 0, burst->length);
        burst->payload  = buf;
        for (ita = burst->ConnectionCollection.begin();ita != burst->ConnectionCollection.end(); ita++)
        {
            pConn       = (Connection *) (*ita)->conn;
            requestLen  = (*ita)->len;
            len         = pConn->EncapsulateAll(buf, requestLen);
            buf         += len;
        }
    }

    /* Generate DL-MAP and save to PHY */
    sort(DLAccessBurstCollection->begin(), DLAccessBurstCollection->end(), compare);
    generate_DL_MAP(DLAccessBurstCollection,DLAccessAvailableSymbols);

    /* Encapsulate all Burst */
    for (itd = DLAccessBurstCollection->begin();itd != DLAccessBurstCollection->end(); itd++)
    {
        char *buf       = NULL;
        burst           = *itd;
        buf             = new char [burst->length];
        memset(buf, 0, burst->length);
        burst->payload  = buf;

        for (ita = burst->ConnectionCollection.begin();ita != burst->ConnectionCollection.end(); ita++)
        {
            pConn       = (Connection *) (*ita)->conn;
            requestLen  = (*ita)->len;
            len         = pConn->EncapsulateAll(buf, requestLen);
            buf         += len;
        }
    }

    return (vector<WiMaxBurst *> *)DLAccessBurstCollection;
}

/* ----------- UL Relay allocation ----------- */
int BS_16j_NT_Scheduler::ULRelayScheduling(vector <upBurst *>* burstCollection,int AvailableSlots)
{
    /* Scheduling UL Relay UGS traffic */
    int ULsubchannels       = Station->ULsubchannels(); //35
    int allocSlots = 0;
    int allocSymbols = 0;
    int ULRelay_allocSlots = 0;
    int ULRelayoccupiedSymbols = 0; 

    // check RS_RangingCodeList
    allocSlots = ULRelay_CDMAallocScheduling(burstCollection, AvailableSlots);
    ULRelay_allocSlots += allocSlots;
    AvailableSlots -= allocSlots;

    // check rslist
    allocSlots = ULRelay_UGSScheduling(burstCollection, AvailableSlots);
    ULRelay_allocSlots += allocSlots;
    AvailableSlots -= allocSlots;

    /* Scheduling UL Relay BE traffic */
    allocSlots = ULRelay_BEScheduling(burstCollection, AvailableSlots);
    ULRelay_allocSlots += allocSlots;
    AvailableSlots -= allocSlots;

    /* At least one slot at UL Relay subframe */
    if(ULRelay_allocSlots == 0)
        ULRelay_allocSlots++;

    if (ULRelay_allocSlots % (ULsubchannels - 6) == 0)
        ULRelayoccupiedSymbols = (ULRelay_allocSlots / (ULsubchannels - 6)) * UL_PUSC;
    else
        ULRelayoccupiedSymbols = (int)(ULRelay_allocSlots / (ULsubchannels - 6) + 1) * UL_PUSC;

    allocSymbols = ULRelay_ContentionScheduling(burstCollection, ULRelayoccupiedSymbols);

    return ULRelayoccupiedSymbols;
}

int BS_16j_NT_Scheduler::ULAccessScheduling(vector <upBurst *>*burstCollection,int ULAccessAvailableSlots)
{
    int ULsubchannels       = Station->ULsubchannels(); //35
    int allocSlots = 0;
    int allocSymbols = 0;
    int ULAccess_allocSlots = 0;
    int ULAccessoccupiedSymbols = 0; 

    /* Scheduling Access MS and Relay MS CDMA_Alloc_IE */
    allocSlots = CDMAallocScheduling(burstCollection, ULAccessAvailableSlots);
    ULAccess_allocSlots += allocSlots;
    ULAccessAvailableSlots -= allocSlots;

    /* Scheduling UL UGS traffic */
    allocSlots = UGSScheduling(burstCollection, ULAccessAvailableSlots);
    ULAccess_allocSlots += allocSlots;
    ULAccessAvailableSlots -= allocSlots;

    /* Scheduling BE traffic */
    allocSlots = BEScheduling(burstCollection, ULAccessAvailableSlots);
    ULAccess_allocSlots += allocSlots;
    ULAccessAvailableSlots -= allocSlots;

    /* At least one slot at UL Access subframe */
    if (ULAccess_allocSlots == 0)
        ULAccess_allocSlots++;

    /* Scheduling contention period */
    if (ULAccess_allocSlots % (ULsubchannels - 6) == 0)
        ULAccessoccupiedSymbols = (ULAccess_allocSlots / (ULsubchannels - 6)) * UL_PUSC;
    else
        ULAccessoccupiedSymbols = (int)(ULAccess_allocSlots / (ULsubchannels - 6) + 1) * UL_PUSC;

    allocSymbols = ContentionScheduling(upBurstCollection, ULAccessoccupiedSymbols);

    return ULAccessoccupiedSymbols;
}


/*
 * Append downlink allocations to burstCollection.
 */
int BS_16j_NT_Scheduler::DLAccess_Scheduling(vector<downBurst *> *burstCollection, int availableSymbols)
{
    int DLsubchannels       = Station->DLsubchannels();               // (30)

    bool isDone         = false;

    Connection *pConn   = NULL;
    downBurst *burst    = NULL;
    AllocInfo *aInfo    = NULL;

    uint32_t thisLen    = 0;
    uint32_t allocLen   = 0;
    int needSlots       = 0;
    int allocSlots      = 0;
    int broadcastLen    = 0;
    int symbolOffset    = 0;
    int DLAccessoccupiedSymbols = 0;
    int availableSlots  = (int)((availableSymbols - PREAMBLE - DL_PUSC)/DL_PUSC)*DLsubchannels;

    vector<int> pduInfo;
    list<DataConnection *>::iterator    itdc;
    list<AllocInfo *>::iterator         ita;
    list<msObject_NT *>::iterator       its;
    vector<downBurst *>::iterator       itd;

    mac802_16j_NT_PMPRS *pRS    = NULL;
    msObject_NT *pMS    = NULL;

    //printf("Time:%llu BS_16j_NT_Scheduler::%s\n",GetCurrentTime(),__func__);

    /* Compute total bytes of broadcast connections */
    pConn = Station->broadcastConnection;
    pduInfo.clear();
    broadcastLen = pConn->GetInformation(&pduInfo); // ignore pduInfo here

    if (broadcastLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->length += broadcastLen;
        //printf("Time:%llu broadcastLen=%d burst->length=%d\n",GetCurrentTime(),broadcastLen,burst->length);
        burst->duration = computeNearestSlots(burst->toSlots(burst->length));
        allocSlots += burst->duration;
        //printf("BS::%s() allocSlots=%d availableSlots=%d\n",__func__,allocSlots,availableSlots);
    }

    /* Examine Initial Ranging connection and use QPSK 1/2 */
    pConn = Station->initRangingConnection;
    pduInfo.clear();
    allocLen = pConn->GetInformation(&pduInfo);

    if (allocLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));

        burst->length += allocLen;
        burst->duration = computeNearestSlots(burst->toSlots(burst->length));
        allocSlots += burst->duration;
        //printf("BS::%s() allocSlots=%d availableSlots=%d\n",__func__,allocLen,availableSlots);
    }

    /* Examine all MS's Management and Data Connection */
    for (int i = 0; i < MAXMS; i++)
    {
        if (Station->mslist[i] == NULL)
            continue;

        if (Station->mslist[i]->ScanFlag == true)
            continue;

        if (Station->mslist[i]->ResourceStatus != Serving)
            continue;

        pMS = Station->mslist[i];
        if((mac802_16j_NT_PMPBS *)pMS->accessStation != Station)
            continue;

        /* Find the burst using the same FEC profile from burstCollection */
        if(pMS->accessStation != Station)
        {

            pRS = (mac802_16j_NT_PMPRS *)pMS->accessStation; 
            //printf("MS's accessStation is RS[%d] (diuc=%d)\n",pRS->get_nid(),pRS->getDIUC());

            burst = GetCorrespondBurst(burstCollection, pMS->diuc);
            //printf("pMS->diuc=%d\n pRS->getDIUC()=%d\n",pMS->diuc,pRS->getDIUC());
            //relay_diuc[pRS->getDIUC()] = true; 
            //continue;
        }
        else
        {
            burst = GetCorrespondBurst(burstCollection, pMS->diuc);
        }
        //printf("[%d]Time:%llu mslist[%d] -> MS diuc = %d\n",__LINE__,GetCurrentTime(),i,pMS->diuc);

        /* Management Connection */
        for (int j = 0; j < 2; j++)
        {
            if (pMS->MnConnections[j] == NULL)
                continue;

            pConn = pMS->MnConnections[j];
            pduInfo.clear();
            allocLen = pConn->GetInformation(&pduInfo);

            if (allocLen > 0)
            {
                burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));
                //printf("pConn->getCID() = %d\n",pConn->getCID());
                burst->length += allocLen;
                burst->duration = computeNearestSlots(burst->toSlots(burst->length));
                allocSlots += burst->duration;
                if(allocSlots > availableSlots)
                {
                    allocSlots -= burst->duration;
                    printf("\e[1;35mBS::%s() Access MS Management Connection allocSlots=%d availableSlots=%d\e[0m\n",__func__,allocLen,availableSlots);
                }
            }
        }

        /* Data Connection */
        for(itdc = pMS->DtConnections.begin();itdc != pMS->DtConnections.end();itdc++)
        {
            pConn = *itdc;
            pduInfo.clear();
            thisLen = pConn->GetInformation(&pduInfo);

            if(thisLen > 0)
            {
                burst->ConnectionCollection.push_back(new AllocInfo(pConn, 0, pduInfo)); // need check why len = 0?
            }
        }
    }
    /* Select pdus from each Connection of each burst at one time */
    isDone = false;

    while (isDone == false)
    {
        isDone = true;

        /* for each Burst */
        for (itd = burstCollection->begin();itd != burstCollection->end();itd++)
        {
            burst = *itd;
            allocSlots -= burst->duration;

            /* for each Connection */
            for (ita = burst->ConnectionCollection.begin();ita != burst->ConnectionCollection.end();ita++)
            {
                aInfo = *ita;
                pConn = (Connection *) aInfo->conn;

                if (aInfo->position >= aInfo->pduInfo.size()) // this AllocInfo has no more pdu
                {
                    continue;
                }

                if (pConn->getCID() > 2*MAXMS + 2*MAXRS)
                {
                /* check enough slots to allocate */
                needSlots = computeNearestSlots(burst->toSlots(burst->length + aInfo->getNext()));
                if (allocSlots + needSlots > availableSlots) // not enough slots for this pdu
                {
                    isDone = true;

                    aInfo->len      += burst->toByte(availableSlots - allocSlots) - burst->length;
                    burst->length   = burst->toByte(availableSlots - allocSlots);
                    break;
                }
                else // enough slots for this pdu
                {
                    isDone = false;
                    //			printf("Time:%llu burst->length=%d \n",GetCurrentTime(),burst->length);

                    burst->length += aInfo->getNext();
                    burst->addCID(pConn->getCID());
                    //printf("Time:%llu BS_16j_NT_Scheduler::%s() burst->addCID(%d) length=%d\n",GetCurrentTime(),__func__,pConn->getCID(),burst->length);
                    aInfo->len += aInfo->getNext();
                    aInfo->position++;
                }
                }
            }
            
            burst->duration = computeNearestSlots(burst->toSlots(burst->length));
            allocSlots += burst->duration;
            
            //burst->Dump_downBurst();
        }
    }

    /* Examine Broadcast connection */
    pConn = Station->broadcastConnection;
    pduInfo.clear();
    allocLen = pConn->GetInformation(&pduInfo);

    if (allocLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->ConnectionCollection.push_front(new AllocInfo(pConn, allocLen, pduInfo));
        burst->addCID(broadcastCID);
    }

    if(allocSlots % DLsubchannels == 0)
        DLAccessoccupiedSymbols = (int)(allocSlots / DLsubchannels) * DL_PUSC;
    else
        DLAccessoccupiedSymbols = (int)(allocSlots / DLsubchannels +1) * DL_PUSC;

    DLAccessoccupiedSymbols = DLAccessoccupiedSymbols + PREAMBLE + DL_PUSC;
    //printf("===Time:%llu ==END=OF=DL=SCHEDULING===SIZE=\e[35m%d\e[0m===\n",GetCurrentTime(),burstCollection->size());

    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        uint8_t nSyms = 0;
        uint8_t nChs = 0;

        burst = *itd;

        burst->dlmap_ie.ie_other.numCid     = burst->nCid;
        burst->dlmap_ie.ie_other.cid        = new uint16_t [burst->nCid];
        memset(burst->dlmap_ie.ie_other.cid, 0, burst->nCid);
        for (int i = 0;i < burst->nCid;i++)
            burst->dlmap_ie.ie_other.cid[i] = burst->cid[i];
        burst->dlmap_ie.ie_other.symOff     = symbolOffset;
        burst->dlmap_ie.ie_other.chOff      = 0;
        burst->dlmap_ie.ie_other.boosting   = 0;
        mapComputeSymCh(burst->duration, &nSyms, &nChs);
        burst->dlmap_ie.ie_other.numSym     = nSyms;
        burst->dlmap_ie.ie_other.numCh      = nChs;
        burst->dlmap_ie.ie_other.repeCode   = 0;
        burst->encapsulateAllField();
        symbolOffset += nSyms;
    }

    /* Erase unused burst */
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        burst = *itd;

        if (burst->duration == 0)
        {
            delete burst;
            burstCollection->erase(itd);
            itd--;
        }
    }

    return DLAccessoccupiedSymbols;
}

/*
 * Append downlink allocations to burstCollection.
 */
int BS_16j_NT_Scheduler::DLRelay_Scheduling(vector<downBurst *> *burstCollection, int availableSymbols)
{
    int DLsubchannels   = Station->DLsubchannels();               // (30)
    int availableSlots  = (int)((availableSymbols - DL_PUSC)/DL_PUSC)*DLsubchannels;

    list<DataConnection *>::iterator    itdc;
    list<AllocInfo *>::iterator         ita;
    list<msObject_NT *>::iterator       its;
    vector<downBurst *>::iterator       itd;

    Connection *pConn   = NULL;
    downBurst *burst    = NULL;
    AllocInfo *aInfo    = NULL;
    msObject_NT *pMS       = NULL;
    rsObject_NT *pRS       = NULL;
    uint32_t thisLen    = 0;
    uint32_t allocLen   = 0;
    int needSlots       = 0;
    int allocSlots      = 0;
    int broadcastLen    = 0;
    bool isDone         = false;
    int symbolOffset    = 0;

    vector<int> pduInfo;
    int DLRelayoccupiedSymbols = 0;

    /* Compute total bytes of broadcast connections */
    pConn = Station->broadcastConnection_relay;
    pduInfo.clear();
    broadcastLen = pConn->GetInformation(&pduInfo); // ignore pduInfo here

    if (broadcastLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->length += broadcastLen;
        burst->duration = computeNearestSlots(burst->toSlots(burst->length));
        allocSlots += burst->duration;
    //    printf("BS::%s() allocSlots= %d availableSlots=%d\n",__func__,allocSlots,availableSlots);
    }

    /* Examine Initial Ranging connection and use QPSK 1/2 */
    pConn = Station->initRangingConnection_relay;
    pduInfo.clear();
    allocLen = pConn->GetInformation(&pduInfo);

    if (allocLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));

        burst->length += allocLen;
        burst->duration = computeNearestSlots(burst->toSlots(burst->length));
        allocSlots += burst->duration;

        if(allocSlots > availableSlots)
        {
            allocSlots -= burst->duration;
            //printf("\e[35mBS::%s() RS[%d] Initial Ranging Connection allocSlots= %d availableSlots=%d\e[0m\n",__func__,pRS->myRS->get_nid(),allocSlots,availableSlots);
        }

        //   printf("BS::%s() allocSlots= %d availableSlots=%d\n",__func__,allocSlots,availableSlots);
    }

    /* Examine all RS's Management and Data Connection */
    for (int i = 0; i < MAXRS; i++)
    {
        if (Station->rslist[i] == NULL)
            continue;

        if (Station->rslist[i]->ScanFlag == true)
            continue;

        if (Station->rslist[i]->ResourceStatus != Serving)
            continue;

        pRS = Station->rslist[i];
        //pRS->Dump();

        /* Find the burst using the same FEC profile from burstCollection */
        burst = GetCorrespondBurst(burstCollection, pRS->diuc);

        if(pRS->myRS->NE_success == true)
        {
            generate_DCD_relay(pRS);
            generate_UCD_relay(pRS);
            //generate_RCD(pRS);
            generate_RS_AccessMAP(pRS);
        }

        /* Management Connection */
        for (int j = 0; j < 2; j++)
        {
            if (pRS->MnConnections[j] == NULL)
                continue;

            pConn = pRS->MnConnections[j];

            pduInfo.clear();
            allocLen = pConn->GetInformation(&pduInfo);

            if (allocLen > 0)
            {
                burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));

                burst->length += allocLen;
                burst->duration = computeNearestSlots(burst->toSlots(burst->length));
                allocSlots += burst->duration;

                if(allocSlots > availableSlots)
                {
                    allocSlots -= burst->duration;
                    //printf("\e[35mBS::%s() RS[%d] Management Connection allocSlots= %d availableSlots=%d\e[0m\n",__func__,pRS->myRS->get_nid(),allocSlots,availableSlots);
                }
            }
        }

        /* Data Connection */
    }


    /* Examine all relay MS's Management and Data Connection */
    for (int i = 0; i < MAXMS; i++)
    {
        if (Station->mslist[i] == NULL)
            continue;

        if (Station->mslist[i]->ScanFlag == true)
            continue;

        if (Station->mslist[i]->ResourceStatus != Serving)
            continue;

        pMS = Station->mslist[i];

        if((mac802_16j_NT_PMPBS *)pMS->accessStation == Station)
            continue;

        /* Find the burst using the same FEC profile from burstCollection */
        burst = GetCorrespondBurst(burstCollection, pMS->diuc);

        /* Management Connection */
        for (int j = 0; j < 2; j++)
        {
            if (pMS->MnConnections[j] == NULL)
                continue;

            pConn = pMS->MnConnections[j];
            pduInfo.clear();
            allocLen = pConn->GetInformation(&pduInfo);

            if (allocLen > 0)
            {
                //printf("BS allocate MS[%d] %d bytes\n",pMS->myMS->get_nid(),allocLen);
                burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));

                burst->length += allocLen;
                burst->duration = computeNearestSlots(burst->toSlots(burst->length));
                allocSlots += burst->duration;

                if(allocSlots > availableSlots)
                {
                    allocSlots -= burst->duration;
                    //printf("\e[35mBS::%s() relay MS[%d] Management Connection allocSlots= %d availableSlots=%d\e[0m\n",__func__,pMS->myMS->get_nid(),allocSlots,availableSlots);
                }
            }
        }

        //printf("allocSlots= %d availableSlots=%d\n",allocSlots,availableSlots);
        /* Data Connection */
        for(itdc = pMS->DtConnections.begin();itdc != pMS->DtConnections.end();itdc++)
        {
            pConn = *itdc;
            pduInfo.clear();
            thisLen = pConn->GetInformation(&pduInfo);

            if(thisLen > 0)
            {
                burst->ConnectionCollection.push_back(new AllocInfo(pConn, 0, pduInfo)); // need check why len = 0?
            }
        }
    }

    /* Select pdus from each Connection of each burst at one time */
    isDone = false;

    while (isDone == false)
    {
        isDone = true;

        /* for each Burst */
        for (itd = burstCollection->begin();itd != burstCollection->end();itd++)
        {
            burst = *itd;
            allocSlots -= burst->duration;

            /* for each Connection */
            for (ita = burst->ConnectionCollection.begin();ita != burst->ConnectionCollection.end();ita++)
            {
                aInfo = *ita;
                pConn = (Connection *) aInfo->conn;

                if (aInfo->position >= aInfo->pduInfo.size()) // this AllocInfo has no more pdu
                {
                    continue;
                }
#ifdef aaa 
                if (pConn->getCID() > 2*MAXMS + 2*MAXRS)  /* data connection*/
                {
                    /* check enough slots to allocate */
                    needSlots = computeNearestSlots(burst->toSlots(burst->length + aInfo->getNext()));
                    //FIXME
                    if ((allocSlots + needSlots) > availableSlots*2/3) // not enough slots for this pdu
                    {
                        isDone = true;

                        aInfo->len      += burst->toByte(availableSlots - allocSlots) - burst->length;
                        burst->length   = burst->toByte(availableSlots - allocSlots);
                        break;
                    }
                    else // enough slots for this pdu
                    {
                        isDone = false;

                        burst->length += aInfo->getNext();
                        burst->addCID(pConn->getCID());
                        aInfo->len += aInfo->getNext();
                        aInfo->position++;
                    }
                }
#endif
                //else
                //{
                    /* check enough slots to allocate */
                    needSlots = computeNearestSlots(burst->toSlots(burst->length + aInfo->getNext()));
                    if (allocSlots + needSlots > availableSlots) // not enough slots for this pdu
                    {
                        isDone = true;

                        aInfo->len      += burst->toByte(availableSlots - allocSlots) - burst->length;
                        burst->length   = burst->toByte(availableSlots - allocSlots);
                        break;
                    }
                    else // enough slots for this pdu
                    {
                        isDone = false;

                        burst->length += aInfo->getNext();
                        burst->addCID(pConn->getCID());
                        aInfo->len += aInfo->getNext();
                        aInfo->position++;
                    }
                //}
            }
            burst->duration = computeNearestSlots(burst->toSlots(burst->length));
            allocSlots += burst->duration;
        }
    }

    /* Examine Broadcast connection */
    pConn = Station->broadcastConnection_relay;
    pduInfo.clear();
    allocLen = pConn->GetInformation(&pduInfo);

    if (allocLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->ConnectionCollection.push_front(new AllocInfo(pConn, allocLen, pduInfo));
        burst->addCID(broadcastCID);
    }

    if(allocSlots % DLsubchannels == 0)
    {
        DLRelayoccupiedSymbols = (int)(allocSlots / DLsubchannels) * DL_PUSC;
    }
    else
    {
        DLRelayoccupiedSymbols = (int)(allocSlots / DLsubchannels +1) * DL_PUSC;
    }

    DLRelayoccupiedSymbols += DL_PUSC;

    symbolOffset = availableSymbols - DLRelayoccupiedSymbols;
    for(itd = burstCollection->begin();itd!=burstCollection->end();itd++)
    {
        uint8_t nSyms = 0;
        uint8_t nChs = 0;

        burst = *itd;

        burst->dlmap_ie.ie_other.numCid     = burst->nCid;
        burst->dlmap_ie.ie_other.cid        = new uint16_t [burst->nCid];
        memset(burst->dlmap_ie.ie_other.cid, 0, burst->nCid);
        for (int i = 0;i < burst->nCid;i++)
            burst->dlmap_ie.ie_other.cid[i] = burst->cid[i];
        burst->dlmap_ie.ie_other.symOff     = symbolOffset;
        burst->dlmap_ie.ie_other.chOff      = 0;
        burst->dlmap_ie.ie_other.boosting   = 0;
        mapComputeSymCh(burst->duration, &nSyms, &nChs);
        burst->dlmap_ie.ie_other.numSym     = nSyms;
        burst->dlmap_ie.ie_other.numCh      = nChs;
        burst->dlmap_ie.ie_other.repeCode   = 0;
        burst->encapsulateAllField();
        symbolOffset += nSyms;
    }
    /* Erase unused burst */
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        burst = *itd;

        if (burst->duration == 0)
        {
            delete burst;
            burstCollection->erase(itd);
            itd--;
        }
    }

    return DLRelayoccupiedSymbols;
}

/*
 * Append contention allocations to burstCollection at Ranging Subchannel
 */
int BS_16j_NT_Scheduler::ContentionScheduling(vector<upBurst *> *burstCollection, int ULoccupiedSymbols)
{
    int fec             = 0;
    int allocSymbols    = 0;
    int rangMethod      = 0;
    int rangOverSymbol  = 0;
    int rangOpportunity = 0;
    upBurst *uburst     = NULL;


    /**************************************************
     * --- CDMA Initial Ranging / Handover Ranging ---
     **************************************************/
    fec = Station->UCDProfile[CDMA_BWreq_Ranging].fec; // BPSK
    rangMethod      = 0;
    rangOverSymbol  = 2;
    rangOpportunity = ULoccupiedSymbols / 3;

    uburst = new upBurst(broadcastCID, CDMA_BWreq_Ranging, fec);
    uburst->ulmap_ie.ie_12.cid              = broadcastCID;
    uburst->ulmap_ie.ie_12.uiuc             = CDMA_BWreq_Ranging;
    uburst->ulmap_ie.ie_12.symOff           = allocSymbols;
    uburst->ulmap_ie.ie_12.chOff            = 0;
    uburst->ulmap_ie.ie_12.numSym           = rangOpportunity * rangOverSymbol;
    uburst->ulmap_ie.ie_12.numCh            = 6;
    uburst->ulmap_ie.ie_12.rangMethod       = rangMethod;
    uburst->ulmap_ie.ie_12.rangIndicator    = 1;
    uburst->encapsulateAllField();

    burstCollection->push_back(uburst);
    allocSymbols += rangOpportunity * rangOverSymbol;

    /****************************************
     * --- BW Request / Periodic Ranging ---
     ****************************************/
    fec = Station->UCDProfile[CDMA_BWreq_Ranging].fec; // BPSK
    rangMethod      = 2;
    rangOverSymbol  = 1;
    rangOpportunity = ULoccupiedSymbols / 3;

    uburst = new upBurst(broadcastCID, CDMA_BWreq_Ranging, fec);
    uburst->ulmap_ie.ie_12.cid              = broadcastCID;
    uburst->ulmap_ie.ie_12.uiuc             = CDMA_BWreq_Ranging;
    uburst->ulmap_ie.ie_12.symOff           = allocSymbols;
    uburst->ulmap_ie.ie_12.chOff            = 0;
    uburst->ulmap_ie.ie_12.numSym           = rangOpportunity * rangOverSymbol;
    uburst->ulmap_ie.ie_12.numCh            = 6;
    uburst->ulmap_ie.ie_12.rangMethod       = rangMethod;
    uburst->ulmap_ie.ie_12.rangIndicator    = 1;
    uburst->encapsulateAllField();

    burstCollection->push_back(uburst);
    allocSymbols += rangOpportunity * rangOverSymbol;

    return allocSymbols;
}

/*
 * Append CDMA allocation IE at Uplink allocation
 */
int BS_16j_NT_Scheduler::CDMAallocScheduling(vector<upBurst *> *burstCollection, int availableSlots)
{
    int fec         = 0;
    int allocSlots  = 0;
    upBurst *uburst = NULL;
    list<RangingObject *>::iterator its1;

    //	printf("Time:%llu:BS_16j_NT_Scheduler(%d)::%s\n", GetCurrentTime(), Station->get_nid(), __func__);

    /* Polled List of Ranging Code MSs */
    for (its1 = Station->RangingCodeList.begin();its1 != Station->RangingCodeList.end();its1++)
    {
        if ((*its1)->collision == true)
        {
            printf("\e[1;33mTime:%llu BS(%d) detect Ranging Code Collision\e[0m\n", GetCurrentTime(), Station->get_nid());
            continue;
        }
        else if ((*its1)->allocated == false)
        {
            continue;
        }
        else
        {
            /*---------------------------*/
            /*     CDMA allocation IE    */
            /*---------------------------*/
            fec = Station->UCDProfile[CDMA_Alloc_IE].fec;

            uburst = new upBurst(broadcastCID, CDMA_Alloc_IE, fec);
            uburst->ulmap_ie.ie_14.cid          = broadcastCID;
            uburst->ulmap_ie.ie_14.uiuc         = CDMA_Alloc_IE;
            uburst->ulmap_ie.ie_14.duration     = 5;    // FIXME: enough??
            uburst->ulmap_ie.ie_14.uiuc_trans   = RobustUIUC;
            uburst->ulmap_ie.ie_14.repeCode     = 0;
            uburst->ulmap_ie.ie_14.frameIndex   = (*its1)->rangingFrameNumber & 0x0F;
            uburst->ulmap_ie.ie_14.rangCode     = (*its1)->rangingCodeIndex & 0xFF;
            uburst->ulmap_ie.ie_14.rangSym      = (*its1)->rangingSymbol & 0xFF;
            uburst->ulmap_ie.ie_14.rangCh       = (*its1)->rangingSubchannel & 0x7F;
            uburst->ulmap_ie.ie_14.bwReq        = 0;
            uburst->encapsulateAllField();

            burstCollection->push_back(uburst);
            allocSlots += 5;
        }
    }

    //FIXME 
    if(num_relay_ir) 
    {
        //printf("Time:%llu BS generate CDMA_Alloc_IE with field zero out\n", GetCurrentTime());

        for(int i=0 ; i < num_relay_ir ; i++)
        {
            /*****************************/
            /* --- CDMA allocation IE ---*/
            /*****************************/
            fec = Station->UCDProfile[CDMA_Alloc_IE].fec;

            uburst = new upBurst(broadcastCID, CDMA_Alloc_IE, fec);
            uburst->ulmap_ie.ie_14.cid          = broadcastCID;
            uburst->ulmap_ie.ie_14.uiuc         = CDMA_Alloc_IE;
            uburst->ulmap_ie.ie_14.duration     = 6;    // FIXME: enough?? => for carring ms nid and pid
            uburst->ulmap_ie.ie_14.uiuc_trans   = RobustUIUC;
            uburst->ulmap_ie.ie_14.repeCode     = 0;
            uburst->ulmap_ie.ie_14.frameIndex   = 0; 
            uburst->ulmap_ie.ie_14.rangCode     = 0;
            uburst->ulmap_ie.ie_14.rangSym      = 0;
            uburst->ulmap_ie.ie_14.rangCh       = 0;
            uburst->ulmap_ie.ie_14.bwReq        = 0;
            uburst->encapsulateAllField();

            ulmap_ie_queue->push_back(uburst);
            allocSlots += 6;

        }
    }

    return allocSlots;
}

/*
 * Append uplink UGS allocations to burstCollection.
 */
int BS_16j_NT_Scheduler::UGSScheduling(vector<upBurst *> *burstCollection, int availableSlots)
{
    upBurst *uburst         = NULL;
    upBurst *uburst_relay   = NULL;
    ServiceFlow *sflow      = NULL;
    ServiceClass *sclass    = NULL;
    Mac_address *mac        = NULL;
    int cid                 = 0;
    int bitsPerFrame        = 0;
    int slots               = 0;
    int allocSlots          = 0;
    int uiuc                = 0;
    int fec                 = 0;
    //int tmpSlots            = 0;
    double rate             = 1;
    char msg[128];


    // The following code is replaced by run-time messages.
#if 0
    do{
        tmpSlots = 0;
        for (int i = 0; i < MAXMS; i++)
        {
            if (Station->mslist[i] == NULL)
                continue;

            if (Station->mslist[i]->ScanFlag == true)
                continue;

            if (Station->mslist[i]->ResourceStatus != Serving)
                continue;

            // Get RS's information
            cid     = Station->mslist[i]->getBasicCID();
            if((Station->mslist[i])->accessStation != Station)
            {
                //printf("MS(%d)'s accessStation is not BS !!!\n",(Station->mslist[i])->myMS->get_nid());
                uiuc    = Station->mslist[i]->myMS->getRUIUC();
            }
            else
            {
                uiuc    = Station->mslist[i]->uiuc;
            }
            fec     = Station->UCDProfile[uiuc].fec;
            mac     = Station->mslist[i]->address();
            sflow   = Station->GetProvisionedFlow(mac->buf());

            // No this MS Qos configure, need check $NAME.sim/$NAME.mobilewimax_cfg
            if (sflow == NULL)
            {
                fprintf(stderr, "\e[1;31mBS[%d]:%s() Can't find QoS configure for %s, check XXX.sim/XXX.mobilewimax_cfg\e[0m\n", Station->get_nid(),__func__, mac->str());
                exit(1);
            }

            sclass = Station->GetServiceClass(sflow->GetQosIndex());
            bitsPerFrame = (int) ((sclass->MaxSustainedRate*rate)/1000.0*frameDuration_NT[Station->FrameDuCode()]);   //FIXME
            slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);
            if (slots <= 0)
            {
                fprintf(stderr, "\e[1;31mWe cannot give this MS any allocation!\e[0m\n");
                exit(1);
            }
            tmpSlots += slots;
        }
        if(tmpSlots <= availableSlots)
            break;
        else
            rate -= 0.01;


    }while(tmpSlots > availableSlots);

    if (rate == 0)
        rate = 1;
#endif	// #if 0
    // Data Grant Burst Type IEs
    for (int i = 0; i < MAXMS; i++)
    {
        if (Station->mslist[i] == NULL)
            continue;

        if (Station->mslist[i]->ScanFlag == true)
            continue;

        if (Station->mslist[i]->ResourceStatus != Serving)
            continue;

        // Get MS's information
        cid     = Station->mslist[i]->getBasicCID();
        if(Station->mslist[i]->accessStation != Station)
        {
            uiuc = (Station->mslist[i]->myMS)->getRUIUC();
        }
        else
        {
            uiuc    = Station->mslist[i]->uiuc;
        }
        fec     = Station->UCDProfile[uiuc].fec;
        mac     = Station->mslist[i]->address();
        sflow   = Station->GetProvisionedFlow(mac->buf());

        // No this MS Qos configure, need check $NAME.sim/$NAME.mobilewimax_cfg
        if (sflow == NULL)
        {
            fprintf(stderr, "\e[1;31mBS[%d] Can't find QoS configure for %s, check XXX.sim/XXX.mobilewimax_cfg\e[0m\n", Station->get_nid(), mac->str());

            memset(msg, 128, 0);
            sprintf(msg, "BS[%d] Can't find QoS configure for %s, check XXX.sim/XXX.mr_wimax_cfg", Station->get_nid(), mac->str());
            sendRuntimeMsg(RTMSG_FATAL_ERROR, Station->get_nid(), "BS_16j_NT_Scheduler", msg);

            //exit(1);
        }

        // MS->MaxSustainedRate may be too high
        sclass = Station->GetServiceClass(sflow->GetQosIndex());
        bitsPerFrame = (int) ((sclass->MaxSustainedRate * rate) / 1000.0 * frameDuration_NT[Station->FrameDuCode()]);
        slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);
        //printf("%s() MS bits=%d slots=%d\n",__func__,bitsPerFrame,slots);

        if (slots <= 0)
        {
            fprintf(stderr, "\e[1;31mWe cannot give this MS any allocation!\e[0m\n");
            memset(msg, 128, 0);
            sprintf(msg, "We cannot give MS[%d] any allocation!", Station->mslist[i]->myMS->get_nid());
            sendRuntimeMsg(RTMSG_FATAL_ERROR, Station->get_nid(), "BS_16j_NT_Scheduler", msg);
            //exit(1);
        }

        if(Station->mslist[i]->accessStation != Station)
        {
            uburst_relay = new upBurst(cid, uiuc, fec);
            uburst_relay->ulmap_ie.ie_other.cid = cid;
            uburst_relay->ulmap_ie.ie_other.uiuc = uiuc;
            uburst_relay->ulmap_ie.ie_other.duration = slots;
            uburst_relay->ulmap_ie.ie_other.repeCode = 0;
            uburst_relay->encapsulateAllField();

            //printf("new upBurst cid=%d uiuc=%d fec=%d bits=%d\n",cid,uiuc,fec,uburst_relay->ie->getBits());

            ulmap_ie_queue->push_back(uburst_relay);
        }
        else
        {
            uburst = new upBurst(cid, uiuc, fec);
            uburst->ulmap_ie.ie_other.cid = cid;
            uburst->ulmap_ie.ie_other.uiuc = uiuc;
            uburst->ulmap_ie.ie_other.duration = slots;
            uburst->ulmap_ie.ie_other.repeCode = 0;
            uburst->encapsulateAllField();

            burstCollection->push_back(uburst);
        }
        allocSlots += slots;

        if (allocSlots > availableSlots)
        {
            fprintf(stderr, "\e[1;31mTime:%llu:%s() UGS allocation exhausting! (alloc : %d , available : %d slots) Maybe we need some sort of admission control.\e[0m\n",GetCurrentTime(),__func__,allocSlots,availableSlots);

            memset(msg, 128, 0);
            sprintf(msg,"BS[%d]::%s() UGS allocation exhausting! (allocSlots=%d, availableSlots=%d)",Station->get_nid(),__func__,allocSlots,availableSlots);
            delete uburst;
            //break;
            sendRuntimeMsg(RTMSG_FATAL_ERROR ,Station->get_nid(),"BS_16j_NT_Scheduler",msg);
            //exit(1);
        }
    }

    return allocSlots;
}

/*
 * Append uplink BE allocations to burstCollection.
 */
int BS_16j_NT_Scheduler::BEScheduling(vector < upBurst * >*burstCollection, int availableSymbols)
{
    return 0;
}


/*
 * Append contention allocations to burstCollection at Ranging Subchannel
 */
int BS_16j_NT_Scheduler::ULRelay_ContentionScheduling(vector<upBurst *> *burstCollection, int ULRelayoccupiedSymbols)
{
    int fec             = 0;
    int allocSymbols    = 0;
    int rangMethod      = 0;
    int rangOverSymbol  = 0;
    int rangOpportunity = 0;
    upBurst *uburst     = NULL;


    /**************************************************
     * --- CDMA Initial Ranging / Handover Ranging ---
     **************************************************/
    fec = Station->UCDProfile[CDMA_BWreq_Ranging].fec; // BPSK
    rangMethod      = 0;
    rangOverSymbol  = 2;
    rangOpportunity = ULRelayoccupiedSymbols / 3;

    uburst = new upBurst(broadcastCID, CDMA_BWreq_Ranging, fec);
    uburst->ulmap_ie.ie_12.cid              = broadcastCID;
    uburst->ulmap_ie.ie_12.uiuc             = CDMA_BWreq_Ranging;
    uburst->ulmap_ie.ie_12.symOff           = allocSymbols;
    uburst->ulmap_ie.ie_12.chOff            = 0;
    uburst->ulmap_ie.ie_12.numSym           = rangOpportunity * rangOverSymbol;
    uburst->ulmap_ie.ie_12.numCh            = 6;
    uburst->ulmap_ie.ie_12.rangMethod       = rangMethod;
    uburst->ulmap_ie.ie_12.rangIndicator    = 1;
    uburst->encapsulateAllField();

    burstCollection->push_back(uburst);
    allocSymbols += rangOpportunity * rangOverSymbol;

    /****************************************
     * --- BW Request / Periodic Ranging ---
     ****************************************/
    fec = Station->UCDProfile[CDMA_BWreq_Ranging].fec; // BPSK
    rangMethod      = 2;
    rangOverSymbol  = 1;
    rangOpportunity = ULRelayoccupiedSymbols / 3;

    uburst = new upBurst(broadcastCID, CDMA_BWreq_Ranging, fec);
    uburst->ulmap_ie.ie_12.cid              = broadcastCID;
    uburst->ulmap_ie.ie_12.uiuc             = CDMA_BWreq_Ranging;
    uburst->ulmap_ie.ie_12.symOff           = allocSymbols;
    uburst->ulmap_ie.ie_12.chOff            = 0;
    uburst->ulmap_ie.ie_12.numSym           = rangOpportunity * rangOverSymbol;
    uburst->ulmap_ie.ie_12.numCh            = 6;
    uburst->ulmap_ie.ie_12.rangMethod       = rangMethod;
    uburst->ulmap_ie.ie_12.rangIndicator    = 1;
    uburst->encapsulateAllField();

    burstCollection->push_back(uburst);
    allocSymbols += rangOpportunity * rangOverSymbol;

    return allocSymbols;
}

/*
 * Append CDMA allocation IE at Uplink allocation
 */
int BS_16j_NT_Scheduler::ULRelay_CDMAallocScheduling(vector<upBurst *> *burstCollection, int availableSlots)
{
    int fec         = 0;
    int allocSlots  = 0;
    upBurst *uburst = NULL;
    list<RangingObject *>::iterator its1;

    /* Polled List of Ranging Code MSs */
    for (its1 = Station->RS_RangingCodeList.begin();its1 != Station->RS_RangingCodeList.end();its1++)
    {
        if ((*its1)->collision == true)
        {
            printf("\e[1;33mTime:%llu BS(%d) detect Ranging Code Collision\e[0m\n", GetCurrentTime(), Station->get_nid());
            continue;
        }
        else if ((*its1)->allocated == false)
        {
            continue;
        }
        else
        {
            //	printf("Time:%llu:BS_16j_NT_Scheduler(%d)::%s\n", GetCurrentTime(), Station->get_nid(), __func__);

            /*****************************
             * --- CDMA allocation IE ---   Table 379.
             *****************************/
            fec = Station->UCDProfile[CDMA_Alloc_IE].fec;

            uburst = new upBurst(broadcastCID, CDMA_Alloc_IE, fec);
            uburst->ulmap_ie.ie_14.cid          = broadcastCID;
            uburst->ulmap_ie.ie_14.uiuc         = CDMA_Alloc_IE;
            uburst->ulmap_ie.ie_14.duration     = 5;    // FIXME: enough??
            uburst->ulmap_ie.ie_14.uiuc_trans   = RobustUIUC;
            uburst->ulmap_ie.ie_14.repeCode     = 0;
            uburst->ulmap_ie.ie_14.frameIndex   = (*its1)->rangingFrameNumber & 0x0F;   // 4 bits
            uburst->ulmap_ie.ie_14.rangCode     = (*its1)->rangingCodeIndex & 0xFF;     // 8 bits
            uburst->ulmap_ie.ie_14.rangSym      = (*its1)->rangingSymbol & 0xFF;        // 8 bits
            uburst->ulmap_ie.ie_14.rangCh       = (*its1)->rangingSubchannel & 0x7F;    // 7 bits
            uburst->ulmap_ie.ie_14.bwReq        = 0;
            //uburst->ulmap_ie.ie_14.relayRScid   = (*its1)->RS_cid;
            //printf("Relay RS cid = %d\n",(*its1)->RS_cid);
            uburst->encapsulateAllField();

            burstCollection->push_back(uburst);
            allocSlots += 5;
        }
    }

    return allocSlots;
}

/*
 * Append UL Relay  UGS allocations to burstCollection.
 */
int BS_16j_NT_Scheduler::ULRelay_UGSScheduling(vector<upBurst *> *burstCollection, int availableSlots)
{ 
    upBurst *uburst         = NULL;
    ServiceFlow *sflow      = NULL;
    ServiceClass *sclass    = NULL;
    Mac_address *mac        = NULL;
    int cid                 = 0;
    int bitsPerFrame        = 0;
    int slots               = 0;
    int allocSlots          = 0;
    int uiuc                = 0;
    int fec                 = 0;
    //int tmpSlots            = 0;
    int rate                = 1;
    int msnid		= 0;
    int rsnid		= 0;
    char msg[128];
    // The following code is replaced by run-time messages.
#if 0
    do{
        tmpSlots = 0;
        for (int i = 0; i < MAXRS; i++)
        {
            if (Station->rslist[i] == NULL)
                continue;

            if (Station->rslist[i]->ScanFlag == true)
                continue;

            if (Station->rslist[i]->ResourceStatus != Serving)
                continue;

            // Get RS's information
            cid     = Station->rslist[i]->getBasicCID();
            uiuc    = Station->rslist[i]->uiuc;
            fec     = Station->UCDProfile[uiuc].fec;

            // FIXME unsolicited uplink bandwidth allocation for each RS (100 kbs)
            bitsPerFrame = (int) (100*1024 / 1000.0 * frameDuration_NT[Station->FrameDuCode()]);   
            slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);
            int nid = (Station->rslist[i])->myRS->get_nid();
            if (slots <= 0)
            {
                fprintf(stderr, "\e[1;31mWe cannot give RS(%d) any allocation!\e[0m\n",nid);
                exit(1);
            }
            //printf("[%d]Time:%llu allocation for RS(%d) [%d] slots\n",__LINE__,GetCurrentTime(),nid,slots);
            tmpSlots += slots;

            /* Compute relay capacity */
            for (int j = 0; j < MAXMS; j++)
            {
                if (Station->mslist[j] == NULL)
                    continue;

                if (Station->mslist[j]->ScanFlag == true)
                    continue;

                if (Station->mslist[j]->ResourceStatus != Serving)
                    continue;

                if (Station->mslist[j]->accessStation != Station && ((mac802_16j_NT_PMPRS *)Station->mslist[j]->accessStation == (Station->rslist[i])->myRS))
                {
                    // Get MS's information
                    mac     = Station->mslist[j]->address();
                    sflow   = Station->GetProvisionedFlow(mac->buf());
                    if (sflow == NULL)
                    {
                        fprintf(stderr, "\e[1;31mBS[%d]:%s Can't find QoS configure for %s, check XXX.sim/XXX.mobilewimax_cfg\e[0m\n", Station->get_nid(),__func__, mac->str());
                        exit(1);
                    }
                    //mac->Dump();
                    // MS->MaxSustainedRate may be too high
                    sclass = Station->GetServiceClass(sflow->GetQosIndex());
                    bitsPerFrame = (int) ((sclass->MaxSustainedRate * rate) / 1000.0 * frameDuration_NT[Station->FrameDuCode()]);

                    //    printf("MAC:%s MaxSustainedRate=%d\n",mac->str(),sclass->MaxSustainedRate);    
                    /* Here , we use RS MCS to calculate MS relay link capacity */
                    slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);

                    tmpSlots += slots;
                }
            }
        }
        if(tmpSlots <= availableSlots)
            break;
        else
            rate -= 0.01;

    }while(tmpSlots > availableSlots);

    if (rate == 0)
        rate = 1;
#endif
    // Data Grant Burst Type IEs
    for (int i = 0; i < MAXRS; i++)
    {
        if (Station->rslist[i] == NULL)
            continue;

        if (Station->rslist[i]->ScanFlag == true)
            continue;

        if (Station->rslist[i]->ResourceStatus != Serving)
            continue;

        // Get RS's information
        cid     = Station->rslist[i]->getBasicCID();
        uiuc    = Station->rslist[i]->uiuc;
        fec     = Station->UCDProfile[uiuc].fec;
        rsnid	= ((Station->rslist[i])->myRS)->get_nid();
        bitsPerFrame = (int) (100*1024 / 1000.0 * frameDuration_NT[Station->FrameDuCode()]);   //FIXME
        slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);
        if (slots <= 0)
        {
            fprintf(stderr, "\e[1;31mWe cannot give RS(%d) any allocation!\e[0m\n",rsnid);
            memset(msg, 128, 0);
            sprintf(msg, "We cannot give RS(%d) any allocation!",(Station->rslist[i])->myRS->get_nid());
            sendRuntimeMsg(RTMSG_FATAL_ERROR, Station->get_nid(), "BS_16j_NT_Scheduler" ,msg);
            //exit(1);
        }

        allocSlots += slots;

        uburst = new upBurst(cid, uiuc, fec);
        uburst->ulmap_ie.ie_other.cid = cid;
        uburst->ulmap_ie.ie_other.uiuc = uiuc;
        uburst->ulmap_ie.ie_other.duration = slots;
        uburst->ulmap_ie.ie_other.repeCode = 0;

        /* Compute relay capacity */
        for (int j = 0; j < MAXMS; j++)
        {
            if (Station->mslist[j] == NULL)
                continue;

            if (Station->mslist[j]->ScanFlag == true)
                continue;

            if (Station->mslist[j]->ResourceStatus != Serving)
                continue;

            if (Station->mslist[j]->accessStation != Station && 
                    ((rsObject_NT*)Station->mslist[j]->accessStation != Station->rslist[i]))
            {    

                // Get MS's information
                mac     = Station->mslist[j]->address();
                sflow   = Station->GetProvisionedFlow(mac->buf());
                msnid	= (Station->mslist[j])->myMS->get_nid();
                // No this MS Qos configure, need check $NAME.sim/$NAME.mobilewimax_cfg
                if (sflow == NULL)
                {
                    fprintf(stderr, "\e[1;31mBS[%d]:%s Can't find QoS configure for %s, check XXX.sim/XXX.mr_wimax_cfg\e[0m\n", Station->get_nid(),__func__, mac->str());
                    memset(msg, 128, 0);
                    sprintf(msg, "BS[%d]:%s Can't find QoS configure for %s, check XXX.sim/XXX.mr_wimax_cfg", Station->get_nid(),__func__, mac->str());
                    sendRuntimeMsg(RTMSG_FATAL_ERROR, Station->get_nid(), "BS_16j_NT_Scheduler" ,msg);
                    //exit(1);
                }

                // MS->MaxSustainedRate may be too high
                sclass = Station->GetServiceClass(sflow->GetQosIndex());
                bitsPerFrame = (int) (sclass->MaxSustainedRate*rate / 1000.0 * frameDuration_NT[Station->FrameDuCode()]);
                /* Here , we use RS MCS to calculate MS relay link capacity */
                slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);
                //printf("%s() relay MS bits=%d slots=%d\n",__func__,bitsPerFrame,slots);
                if (slots <= 0)
                {
                    fprintf(stderr, "\e[1;31mWe cannot give this MS any allocation!\e[0m\n");
                    memset(msg, 128, 0);
                    sprintf(msg, "We cannot give relay MS[%d] any allocation!",msnid);
                    sendRuntimeMsg(RTMSG_FATAL_ERROR, Station->get_nid(), "BS_16j_NT_Scheduler" ,msg);
                    //exit(1);
                }

                uburst->ulmap_ie.ie_other.duration += slots;
                allocSlots += slots;

            }
        }

        if (allocSlots > availableSlots)
        {
            fprintf(stderr, "\e[1;31mTime:%llu BS::%s() Too many UGS connections! Maybe we need some sort of admission control. (allocSlots=%d availableSlots=%d)\e[0m\n",GetCurrentTime(),__func__,allocSlots,availableSlots);
            allocSlots -= uburst->ulmap_ie.ie_other.duration;

            memset(msg,128,0);
            sprintf(msg, "BS[%d]::%s() UGS allocation exhausting! (allocSlots=%d availableSlots=%d)",Station->get_nid(),__func__,allocSlots,availableSlots);
            sendRuntimeMsg(RTMSG_FATAL_ERROR, Station->get_nid(), "BS_16j_NT_Scheduler" ,msg);
            //exit(1);
            //    delete uburst;
            //    break;
        }

        uburst->encapsulateAllField();

        burstCollection->push_back(uburst);

    }

    return allocSlots;
}

/*
 * Append uplink BE allocations to burstCollection.
 */
int BS_16j_NT_Scheduler::ULRelay_BEScheduling(vector < upBurst * >*burstCollection, int availableSymbols)
{
    return 0;
}

/*
 * Generate a MAC PDU containing UCD.
 */
void BS_16j_NT_Scheduler::generate_UCD()
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::generate_UCD ");
    ifmgmt *ifmm        = NULL;
    ifTLV *tmptlv       = NULL;
    ifmgmt *ifmm_relay        = NULL;
    ifTLV *tmptlv_relay       = NULL;
    uint8_t rangStart   = 2;
    uint8_t rangEnd     = 10;
    uint8_t reqStart    = 0;
    uint8_t reqEnd      = 10;
    uint8_t burstProfileBuffer[128] = "";
    struct UCDBurstProfile *pf      = NULL;

    ifmm = new ifmgmt(MG_UCD, 5);
    ifmm->appendBitField( 8, Station->UCDCfgCount);    // Configuration change count
    ifmm->appendBitField( 8, rangStart); // Ranging backoff start
    ifmm->appendBitField( 8, rangEnd);   // Ranging backoff end
    ifmm->appendBitField( 8, reqStart);  // Request backoff start
    ifmm->appendBitField( 8, reqEnd);    // Request backoff end

    ifmm_relay= new ifmgmt(MG_UCD, 5);
    ifmm_relay->appendBitField( 8, Station->UCDCfgCount);    // Configuration change count
    ifmm_relay->appendBitField( 8, rangStart); // Ranging backoff start
    ifmm_relay->appendBitField( 8, rangEnd);   // Ranging backoff end
    ifmm_relay->appendBitField( 8, reqStart);  // Request backoff start
    ifmm_relay->appendBitField( 8, reqEnd);    // Request backoff end

    /*
     * TLV encoded information (Spec 11.3.1)
     */

    /*
     * Setting uplink burst profile (Spec 8.3.5.5 and
     */
    for (int i = 0; i < 16; i++)
    {
        if (!Station->UCDProfile[i].used)
            continue;

        pf = Station->UCDProfile + i;
        burstProfileBuffer[0] = i;    // UIUC
        tmptlv = new ifTLV(burstProfileBuffer + 1, 0);
        tmptlv->Add(150, 1, pf->fec);
        tmptlv->Add(151, 1, pf->rangRatio);
        ifmm->appendTLV(1, tmptlv->getLen() + 1, burstProfileBuffer);

        tmptlv_relay = new ifTLV(burstProfileBuffer + 1, 0);
        tmptlv_relay->Add(150, 1, pf->fec);
        tmptlv_relay->Add(151, 1, pf->rangRatio);
        ifmm_relay->appendTLV(1, tmptlv_relay->getLen() + 1, burstProfileBuffer);

        delete tmptlv;
        delete tmptlv_relay;
    }

    Station->broadcastConnection->Insert(ifmm);
    Station->broadcastConnection_relay->Insert(ifmm_relay);
}

/*
 * Generate a MAC PDU containing UCD for RS.
 */
void BS_16j_NT_Scheduler::generate_UCD_relay(rsObject_NT *pRS)
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::generate_UCD_relay ");
    ifmgmt *ifmm        = NULL;
    ifTLV *tmptlv       = NULL;
    uint8_t rangStart   = 2;
    uint8_t rangEnd     = 10;
    uint8_t reqStart    = 0;
    uint8_t reqEnd      = 10;
    uint8_t burstProfileBuffer[128] = "";
    struct UCDBurstProfile *pf      = NULL;

    ifmm = new ifmgmt(MG_UCD, 5);
    ifmm->appendBitField( 8, Station->UCDCfgCount);    // Configuration change count
    ifmm->appendBitField( 8, rangStart); // Ranging backoff start
    ifmm->appendBitField( 8, rangEnd);   // Ranging backoff end
    ifmm->appendBitField( 8, reqStart);  // Request backoff start
    ifmm->appendBitField( 8, reqEnd);    // Request backoff end

    /*
     * TLV encoded information (Spec 11.3.1)
     */

    /*
     * Setting uplink burst profile (Spec 8.3.5.5 and
     */
    for (int i = 0; i < 16; i++)
    {
        if (!Station->UCDProfile[i].used)
            continue;

        pf = Station->UCDProfile + i;
        burstProfileBuffer[0] = i;    // UIUC
        tmptlv = new ifTLV(burstProfileBuffer + 1, 0);
        tmptlv->Add(150, 1, pf->fec);
        tmptlv->Add(151, 1, pf->rangRatio);
        ifmm->appendTLV(1, tmptlv->getLen() + 1, burstProfileBuffer);
        //printf("getLen=%d %d\n",tmptlv->getLen() + 1,*burstProfileBuffer);
        delete tmptlv;
    }

    //pRS->myRS->PushPDUintoConnection(pRS->PriCID,ifmm); 
    (pRS->MnConnections[1])->Insert(ifmm);
}

/*
 * Generate a MAC PDU containing DCD.
 */
void BS_16j_NT_Scheduler::generate_DCD()
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::generate_DCD ");
    ifmgmt *ifmm    = NULL;
    ifTLV *tmptlv   = NULL;
    ifmgmt *ifmm_relay    = NULL;
    ifTLV *tmptlv_relay   = NULL;
    uint8_t rsv     = 0;
    uint8_t burstProfileBuffer[128]  = "";

    ifmm = new ifmgmt(MG_DCD, 2);
    ifmm->appendBitField( 8, rsv);
    ifmm->appendBitField( 8, Station->DCDCfgCount);    // Configuration change count

    ifmm_relay = new ifmgmt(MG_DCD, 2);
    ifmm_relay->appendBitField( 8, rsv);
    ifmm_relay->appendBitField( 8, Station->DCDCfgCount);    // Configuration change count

    /*
     * TLV encoded information (Spec 11.4.1) (16j 11.4 Table 575)
     */
    ifmm->appendTLV(50, 1, Station->_HO_support);   // Support HO
    ifmm_relay->appendTLV(50, 1, Station->_HO_support);   // Support HO
    ifmm_relay->appendTLV(64, 1, 0x02);   // end-to-end metric ; hop count 

    /*
     * Setting downlink burst profile (Spec 8.4.5.5 and 11.4.2)
     */
    for (int i = 0; i < 16; i++)
    {
        if (!Station->DCDProfile[i].used)
            continue;

        burstProfileBuffer[0] = i;    // DIUC
        tmptlv = new ifTLV(burstProfileBuffer + 1, 0);
        tmptlv->Add(150, 1, Station->DCDProfile[i].fec);
        ifmm->appendTLV(1, tmptlv->getLen() + 1, burstProfileBuffer);

        tmptlv_relay = new ifTLV(burstProfileBuffer + 1, 0);
        tmptlv_relay->Add(150, 1, Station->DCDProfile[i].fec);
        ifmm_relay->appendTLV(1, tmptlv_relay->getLen() + 1, burstProfileBuffer);

        delete tmptlv;
        delete tmptlv_relay;
    }
    Station->broadcastConnection->Insert(ifmm);
    Station->broadcastConnection_relay->Insert(ifmm_relay);
}

/*
 * Generate a MAC PDU containing DCD. (for relay broadcast messages with RS PriCID)
 */
void BS_16j_NT_Scheduler::generate_DCD_relay(rsObject_NT *pRS)
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::generate_DCD_relay ");
    ifmgmt *ifmm    = NULL;
    ifTLV *tmptlv   = NULL;
    uint8_t rsv     = 0;
    uint8_t burstProfileBuffer[128]  = "";

    ifmm = new ifmgmt(MG_DCD, 2);
    ifmm->appendBitField( 8, rsv);
    ifmm->appendBitField( 8, Station->DCDCfgCount);    // Configuration change count

    /*
     * TLV encoded information (Spec 11.4.1)
     */
    ifmm->appendTLV(50, 1, Station->_HO_support);   // Support HO

    /*
     * Setting downlink burst profile (Spec 8.4.5.5 and 11.4.2)
     */
    for (int i = 0; i < 16; i++)
    {
        if (!Station->DCDProfile[i].used)
            continue;

        burstProfileBuffer[0] = i;    // DIUC
        tmptlv = new ifTLV(burstProfileBuffer + 1, 0);
        tmptlv->Add(150, 1, Station->DCDProfile[i].fec);
        ifmm->appendTLV(1, tmptlv->getLen() + 1, burstProfileBuffer);

        delete tmptlv;
    }
    //pRS->myRS->PushPDUintoConnection(pRS->PriCID,ifmm); 
    (pRS->MnConnections[1])->Insert(ifmm);

}

void BS_16j_NT_Scheduler::generate_RCD(rsObject_NT *pRS)
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::generate_RCD ");
    ;
}

void BS_16j_NT_Scheduler::procBR_header(BR_Connection *br)
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::procBR_header ");
    //printf("--- Time:%llu BS_16j_NT_Scheduler::%s() size=%d ---\n",GetCurrentTime(),__func__,br->pduQ.size());
    uint16_t cid;
    num_relay_ir = 0;
    num_relay_ho = 0;
    num_relay_br = 0;

    rs_br_hdr_tid->clear();

    while(!br->pduQ.empty()) 
    {
        BR_HDR *_br = NULL; 
        _br = (BR_HDR *)(br->pduQ.front());

        if(_br->type == type1)
        {
            ;   // not implement
        }
        else if(_br->type == type2_0)
        {
            ;   // not implement
        }
        else if(_br->type == type2_1)    
        { // Extended relay MAC Signaling Header Type II

            //struct BR_header_type2_1 br_hdr_2;
            //br_hdr_2 = _br->br_hdr.br_type2_1;
            if(_br->br_hdr.br_type2_1.type_ext == RS_BR_hdr)
            {
                br_header_type2_ext[RS_BR_hdr]++; 

                const RS_BR_header *hdr = NULL; 
                hdr = (RS_BR_header *)(_br->br_hdr.br_type2_1.hdr);
                uint8_t tid = (hdr->tid_msb << 2) + (hdr->tid_lsb);
                cid = hdr->cid_lsb + (hdr->cid_msb << 8);
                //printf("Time:%llu\tBS(%d) Receive RS_BR_header (tid=%d)\n",GetCurrentTime(),Station->get_nid(),tid);

                rs_br_hdr_tid->push_back(tid);
                delete hdr;
            }
            else if(_br->br_hdr.br_type2_1.type_ext == MR_CodeREP_hdr)
            {
                br_header_type2_ext[MR_CodeREP_hdr]++; 

                MR_CodeREP_header *hdr = NULL;
                hdr = ( MR_CodeREP_header *)(_br->br_hdr.br_type2_1.hdr);

                num_relay_ir += hdr->num_ir; 
                num_relay_ho += hdr->num_hr_lsb + (hdr->num_hr_msb << 2); 
                num_relay_br += hdr->num_br; 
                cid = hdr->cid_lsb + (hdr->cid_msb << 8);
                //printf("Time:%llu\tBS(%d) Receive MR_CodeREP_header (cid=%d , num_ir=%d , num_ho=%d , num_br=%d)\n",GetCurrentTime(),Station->get_nid(),cid,recv_rng->num_ir,recv_rng->num_ho,recv_rng->num_br);

                delete hdr;
            }
            else
            {
                //not implement !!!
            }
        }
        else
        {
            ;
        }

        delete _br;
        br->pduqlen--;
        br->pduQ.pop_front();
    }
}

void BS_16j_NT_Scheduler::generate_RS_AccessMAP(rsObject_NT *pRS)
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::generate_RS_AccessMAP ");
    vector<downBurst *>::iterator itd;
    vector<upBurst *>::iterator itu;

    ifmgmt *ifmm                = NULL;
    OFDMA_ULMAP_IE  *ulmap_ie   = NULL; 
    unsigned int len            = 0;
    unsigned int nBits          = 0;
    unsigned int nBytes         = 0;
    bool padding                = false;
    int indicator               = 0;
    uint8_t indicator_lsb       = 0;
    uint8_t indicator_msb       = 0;
    BR_Connection *pBRConn      = pRS->myRS->brConnection;

    /* check BR Connection */
    int allocLen = pBRConn->GetInformation(); 
    if (allocLen > 0)
    {
        //printf("Time:%llu brConnection size = %d\n",GetCurrentTime(),allocLen);
        for(int i=0 ; i < 8 ; i++)
        {
            br_header_type2_ext[i] = 0;
        }
        procBR_header(pBRConn);
    }

    if(!ulmap_ie_queue->empty())
    {
        // include UL_MAP_ie => set bit 3 on
        indicator |= 0x08;
        for(itu = ulmap_ie_queue->begin();itu != ulmap_ie_queue->end();itu++)
        {
            nBits += (*itu)->ie->getBits();
        }
        //printf("nBits = %d\n",nBits);
        nBytes += 2;    // UCD count : 8 , UL_MAP_ie count : 8
    }

    if(br_header_type2_ext[RS_BR_hdr] > 0)
    {
        //printf("Time:%llu\tReceive %d RS_BR_header\n",GetCurrentTime(),br_header_type2_ext[RS_BR_hdr]);
        // include RS_BW_Alloc_IE => set bit 6 on
        indicator |= 0x40;
        uint8_t num_ie = br_header_type2_ext[RS_BR_hdr];
        //nBits += 4;             // FIXME num_ie : 4 bits ; for alignment,we let it 8 bits 
        //nBits += num_ie*14;     // spec 16j Table 166aa,RS_BW_Alloc_IE - type:0 (2+4+8 = 14 bits)
        nBytes += (1 + num_ie*2);
    }


    if (nBits % 8 == 0)
    {
        len = 2 + nBytes + nBits / 8;
        padding = false;
    }
    else
    {
        len = 2 + nBytes + nBits / 8 + 1;
        padding = true;
    }

    indicator_lsb       = indicator % 256;
    indicator_msb       = (indicator >> 8);

    //len = 2 + nBytes;
    ifmm = new ifmgmt(MG_RS_AccessMAP,len);
    ifmm->appendBitField(8,indicator_lsb );
    ifmm->appendBitField(4,indicator_msb );
    ifmm->appendBitField(4,(Station->frameNumber & 0xF));   // 4 LSB

    if ( br_header_type2_ext[RS_BR_hdr] > 0 )
    {
        uint8_t num_ie = br_header_type2_ext[RS_BR_hdr];
        uint8_t type           = 0x00; 
        uint8_t dlmap_idx      = 0x00; 
        uint8_t zero = 0x00;

        ifmm->appendBitField(8,num_ie);

        for(int i=0 ; i<num_ie ; i++)
        {   // RS_BW_Alloc_IE format, spec 16j Table 166aa
            ifmm->appendBitField(2,type);   // type:0 , response for _S_BR_header
            uint8_t tid = *(rs_br_hdr_tid->begin());
            ifmm->appendBitField(4,tid);
            ifmm->appendBitField(2,zero);
            ifmm->appendBitField(8,dlmap_idx);
            rs_br_hdr_tid->erase(rs_br_hdr_tid->begin());

            br_header_type2_ext[RS_BR_hdr]--;
        }

    }

    if(!ulmap_ie_queue->empty())
    {
        uint8_t num_ulmap_ie = ulmap_ie_queue->size();

        ifmm->appendBitField(8,Station->UCDCfgCount);
        ifmm->appendBitField(8,num_ulmap_ie);
        for(itu = ulmap_ie_queue->begin();itu != ulmap_ie_queue->end();itu++)
        {
            ulmap_ie = (*itu)->ie;
            ifmm->appendBitField(ulmap_ie->getBits(), ulmap_ie->getData());
        }

        if(padding == true)
        {
            uint8_t zero[4] = "";
            ifmm->appendBitField(4,zero);
        }

    }

    //pRS->myRS->PushPDUintoConnection(pRS->BasicCID,ifmm); 
    (pRS->MnConnections[0])->Insert(ifmm);
}

/*
 * Generate a MAC PDU containing MOB_NBR-ADV.
 */
void BS_16j_NT_Scheduler::generate_MOB_NBRADV()
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::generate_MOB_NBRADV ");
    ifmgmt *ifmm          = NULL;
    ifmgmt *ifmm_relay          = NULL;
    int tmpBits           = 0;
    uint8_t N_neighbors   = 2;
    uint8_t skipOptField  = 0xFC; // 1111 1100
    uint8_t fragmentIndex = 0;
    uint8_t totalFragment = 1;
    uint8_t *loopLen      = NULL;

    loopLen = new uint8_t [N_neighbors];
    memset(loopLen, 0, N_neighbors);

    /*
     * Compute all field bits
     */
    tmpBits += 8;
    if ((skipOptField & 0x01) == 0)
        tmpBits += 24;

    tmpBits += 24;

    N_neighbors = Station->NeighborMRBSList->nbrBSs_Index.size();

    for (int i = 0;i < N_neighbors;i++)
    {
        int len = 0;
        uint8_t phyProfileID = Station->NeighborMRBSList->nbrBSs_Index[i]->PHY_ProfileID;

        tmpBits += 16;
        if (((phyProfileID & 0x40) >> 6) == 1)
        {
            tmpBits += 8;
            len += 1;
        }

        if (((phyProfileID & 0x10) >> 4) == 1)
        {
            tmpBits += 8;
            len += 1;
        }

        if ((skipOptField & 0x02) == 0)
        {
            tmpBits += 24;
            len += 3;
        }

        tmpBits += 8;

        if ((skipOptField & 0x04) == 0)
        {
            tmpBits += 8;
            len += 1;
        }

        if ((skipOptField & 0x08) == 0)
        {
            tmpBits += 8;
            len += 1;
        }

        tmpBits += 8;
        len += 1;

        /*
         * TLV Encoded Neighbor Information
         */

        loopLen[i] = len;
    }

    /*
     * Generate MOB_NBR-ADV Message
     */
    ifmm = new ifmgmt(MG_MOB_NBRADV, tmpBits / 8);
    ifmm->appendBitField( 8, skipOptField);

    ifmm_relay = new ifmgmt(MG_MOB_NBRADV, tmpBits / 8);
    ifmm_relay->appendBitField( 8, skipOptField);

    if ((skipOptField & 0x01) == 0)  // skipOptField[0]
    {
        ifmm->appendBitField(24, Station->address()->buf() + 3);  // Operator ID
        ifmm_relay->appendBitField(24, Station->address()->buf() + 3);  // Operator ID
    }
    ifmm->appendBitField( 8, Station->NeighborMRBSList->NBRADV_CfgCount);
    ifmm->appendBitField( 4, fragmentIndex);
    ifmm->appendBitField( 4, totalFragment);
    ifmm->appendBitField( 8, N_neighbors);

    ifmm_relay->appendBitField( 8, Station->NeighborMRBSList->NBRADV_CfgCount);
    ifmm_relay->appendBitField( 4, fragmentIndex);
    ifmm_relay->appendBitField( 4, totalFragment);
    ifmm_relay->appendBitField( 8, N_neighbors);

    for (int i = 0;i < N_neighbors;i++)
    {
        uint8_t FA_Index     = 0;
        uint8_t BS_EIRP      = 0;
        uint8_t phyProfileID = Station->NeighborMRBSList->nbrBSs_Index[i]->PHY_ProfileID;

        ifmm->appendBitField( 8, loopLen[i]);   // Length of this iteration in bytes
        ifmm->appendBitField( 8, phyProfileID); // Spec 16e. Table 109g

        ifmm_relay->appendBitField( 8, loopLen[i]);   // Length of this iteration in bytes
        ifmm_relay->appendBitField( 8, phyProfileID); // Spec 16e. Table 109g

        if (((phyProfileID & 0x40) >> 6) == 1) // FA Index Indicator
        {
            ifmm->appendBitField( 8, FA_Index);
            ifmm_relay->appendBitField( 8, FA_Index);
        }

        if (((phyProfileID & 0x10) >> 4) == 1) // BS EIRP Indicator
        {
            ifmm->appendBitField( 8, BS_EIRP);
            ifmm_relay->appendBitField( 8, BS_EIRP);
        }

        if ((skipOptField & 0x02) == 0) // skipOptField[1]
        {
            ifmm->appendBitField(24, Station->NeighborMRBSList->nbrBSs_Index[i]->addr->buf() + 3); // Neighbor BSID
            ifmm_relay->appendBitField(24, Station->NeighborMRBSList->nbrBSs_Index[i]->addr->buf() + 3); // Neighbor BSID
        }

        ifmm->appendBitField( 8, Station->NeighborMRBSList->nbrBSs_Index[i]->PreambleIndex);  // Preamble Index
        ifmm_relay->appendBitField( 8, Station->NeighborMRBSList->nbrBSs_Index[i]->PreambleIndex);  // Preamble Index

        if ((skipOptField & 0x04) == 0) // skipOptField[2]
        {
            ifmm->appendBitField( 8, Station->NeighborMRBSList->nbrBSs_Index[i]->HO_Optimize); // HO Process Optimization
            ifmm_relay->appendBitField( 8, Station->NeighborMRBSList->nbrBSs_Index[i]->HO_Optimize); // HO Process Optimization
        }

        if ((skipOptField & 0x08) == 0) // skipOptField[3]
        {
            ifmm->appendBitField( 8, Station->NeighborMRBSList->nbrBSs_Index[i]->SchServSupport); // Scheduling Service Supported
            ifmm_relay->appendBitField( 8, Station->NeighborMRBSList->nbrBSs_Index[i]->SchServSupport); // Scheduling Service Supported
        }

        ifmm->appendBitField( 4, Station->NeighborMRBSList->nbrBSs_Index[i]->DCDCfgCount & 0x0F);  // DCD Configuration Change Count
        ifmm->appendBitField( 4, Station->NeighborMRBSList->nbrBSs_Index[i]->UCDCfgCount & 0x0F);  // UCD Configuration Change Count

        ifmm_relay->appendBitField( 4, Station->NeighborMRBSList->nbrBSs_Index[i]->DCDCfgCount & 0x0F);  // DCD Configuration Change Count
        ifmm_relay->appendBitField( 4, Station->NeighborMRBSList->nbrBSs_Index[i]->UCDCfgCount & 0x0F);  // UCD Configuration Change Count

        /*
         * x
         * TLV Encoded Neighbor Information
         */
    }

    delete [] loopLen;

    Station->broadcastConnection->Insert(ifmm);
    Station->broadcastConnection_relay->Insert(ifmm_relay);
}

/*
 * Generate a MAC PDU containing DL-MAP.
 */

void BS_16j_NT_Scheduler::generate_DL_MAP(vector<downBurst *> *burstCollection,int DLsymbols)
{
    unsigned int len            = 0;
    unsigned int nBits          = 0;
    ifmgmt *ifmm                = NULL;
    OFDMA_DLMAP_IE *dlmap_ie    = NULL;
    bool padding                = false;
    BroadcastConnection *pConn  = NULL;
    int totalBytes              = 0;
    char *dstbuf                = NULL;
    vector<downBurst *>::iterator itd;
    vector<int> pduInfo;
    int size = 0;

    //printf("burstCollection->size = %d\n",burstCollection->size());
    // Compute total length of DL-MAP
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {   
        nBits += (*itd)->ie->getBits();
        //printf("dlmap_ie[%d] = %d bits\n",size,nBits);
        size++;
    }

    if (nBits % 8 == 0)
    {
        len = 12 + nBits / 8;
        padding = false;
    }
    else
    {
        len = 12 + nBits / 8 + 1;
        padding = true;
    }

    /*
     * Generate DL-MAP Management Message (Spec 16e. Table 16)
     */
    //printf("%s() len=%d\n",__func__,len);
    ifmm = new ifmgmt(MG_DLMAP, len);
    ifmm->appendBitField( 8, Station->FrameDuCode());       // Frame Duration Code
    ifmm->appendBitField(24, Station->frameNumber);         // Frame Number
    ifmm->appendBitField( 8, Station->DCDCfgCount);         // Configuration change count
    ifmm->appendBitField(48, Station->address()->buf());    // Base Station ID
    ifmm->appendBitField( 8, DLsymbols);                    // OFDMA DL symbols

    Station->saveDLsymbols(DLsymbols);
    Station->saveDLAccessSymbols(DLsymbols);

    /*
     * DL-MAP IE (Spec 16e. 8.4.5.3)
     */
    //int nbytes = 0;
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        dlmap_ie = (*itd)->ie;
        ifmm->appendBitField(dlmap_ie->getBits(), dlmap_ie->getData());

        /*   if(relay_diuc[dlmap_ie->getDIUC()] == true)
             {
             if(dlmap_ie->getBits() % 8 == 0)
             nbytes = dlmap_ie->getBits() / 8;
             else
             nbytes = dlmap_ie->getBits() / 8 + 1;
             }
             */
    }

    if (padding == true)
    {
        uint8_t zero[4] = "";
        ifmm->appendBitField( 4, zero);
    }

    // In order to encapsulate all fields, we temporarily create a BroadcastConnection here.
    pConn = new BroadcastConnection(broadcastCID);
    pConn->Insert(ifmm);
    pduInfo.clear();
    totalBytes = pConn->GetInformation(&pduInfo);
    dstbuf = new char [totalBytes];
    memset(dstbuf, 0, totalBytes);
    pConn->EncapsulateAll(dstbuf, totalBytes);

    // DLFP need DLMAP information
    Station->saveDLMAP(dstbuf, totalBytes);
    delete pConn;
}

/*
 * Generate a MAC PDU containing UL-MAP.
 */
void BS_16j_NT_Scheduler::generate_UL_MAP(vector<upBurst *> *burstCollection, int ULallocStartTime, int ULsymbols)
{
    unsigned int len            = 0;
    unsigned int nBits          = 0;
    uint8_t rsv = 0;
    ifmgmt *ifmm                = NULL;
    OFDMA_ULMAP_IE *ulmap_ie    = NULL;
    bool padding                = false;
    vector<upBurst *>::iterator itu;

    //printf("Time:%llu BS[%d]::%s() ULallocStartTime = %d (PSs) ULsymbols=%d\n",GetCurrentTime(),Station->get_nid(),__func__,ULallocStartTime,ULsymbols);

    // compute total length of ul-map
    for(itu = burstCollection->begin();itu != burstCollection->end();itu++)
    {
        nBits += (*itu)->ie->getBits();
    }

    // check if need padding
    if(nBits % 8 == 0)
    {
        len = 7 + nBits / 8;
        padding = false;
    }
    else
    {
        len = 7 + nBits / 8 + 1;
        padding = true;
    }

    /*
     * generate ul-map management message (spec 16e. table 18)
     */
    ifmm = new ifmgmt(MG_ULMAP, len);
    ifmm->appendBitField( 8, rsv);                  // reserved
    ifmm->appendBitField( 8, Station->UCDCfgCount); // configuration change count
    ifmm->appendBitField(32, ULallocStartTime);     // Uplink Alloation start time
    ifmm->appendBitField( 8, ULsymbols);            // OFDMA UL symbols

    Station->saveULAccessSymbols(ULsymbols);

    /*
     * UL-MAP IE (Spec 16e. 8.4.5.4)
     */
    for (itu = burstCollection->begin();itu != burstCollection->end();itu++)
    {
        ulmap_ie = (*itu)->ie;
        ifmm->appendBitField(ulmap_ie->getBits(), ulmap_ie->getData());
    }

    if (padding == true)
    {
        uint8_t zero[4] = "";
        ifmm->appendBitField( 4, zero);
    }

    Station->broadcastConnection->Insert(ifmm);
}

void BS_16j_NT_Scheduler::generate_UL_MAP_relay(vector<upBurst *> *burstCollection, int ULRelayallocStartTime, int ULRelaySymbols)
{
    unsigned int len            = 0;
    unsigned int nBits          = 0;
    uint8_t rsv = 0;
    ifmgmt *ifmm                = NULL;
    OFDMA_ULMAP_IE *ulmap_ie    = NULL;
    bool padding                = false;
    vector<upBurst *>::iterator itu;

    // Compute total length of UL-MAP
    for(itu = burstCollection->begin();itu != burstCollection->end();itu++)
    {
        nBits += (*itu)->ie->getBits();
    }

    // Check if need padding
    if(nBits % 8 == 0)
    {
        len = 7 + nBits / 8;
        padding = false;
    }
    else
    {
        len = 7 + nBits / 8 + 1;
        padding = true;
    }

    /*
     * Generate UL-MAP Management Message (Spec 16e. Table 18)
     */
    ifmm = new ifmgmt(MG_ULMAP, len);
    ifmm->appendBitField( 8, rsv);                  // Reserved
    ifmm->appendBitField( 8, Station->UCDCfgCount); // Configuration change count
    ifmm->appendBitField(32, ULRelayallocStartTime);     // Uplink Alloation start time
    ifmm->appendBitField( 8, ULRelaySymbols);            // OFDMA UL symbols
    Station->saveULRelaySymbols(ULRelaySymbols);

    /*
     * UL-MAP IE (Spec 16e. 8.4.5.4)
     */
    for (itu = burstCollection->begin();itu != burstCollection->end();itu++)
    {
        ulmap_ie = (*itu)->ie;
        ifmm->appendBitField(ulmap_ie->getBits(), ulmap_ie->getData());
    }

    if (padding == true)
    {
        uint8_t zero[4] = "";
        ifmm->appendBitField( 4, zero);
    }

    Station->broadcastConnection_relay->Insert(ifmm);
}

/*
 *  Generate a MAC PDU containing R-MAP. 
 */

//void BS_16j_NT_Scheduler::generate_R_MAP(vector<downBurst *> *DLRelayburstCollection, vector<upBurst *> *ULRelayburstCollection,int DLsymbols,int ULsymbols,int RelayULallocStartTime)
void BS_16j_NT_Scheduler::generate_R_MAP(vector<downBurst *> *burstCollection, int DLsymbols)
{
    unsigned int len            = 0;
    unsigned int nBits          = 0;
    ifmgmt *ifmm                = NULL;
    OFDMA_DLMAP_IE *dlmap_ie    = NULL;
    bool padding                = false;
    BroadcastConnection *pBConn  = NULL;
    int totalBytes              = 0;
    char *dstbuf                = NULL;
    vector<downBurst *>::iterator itd;
    list<AllocInfo *>::iterator ita;
    vector<int> pduInfo;

    /* 16j added */
    OFDMA_DLMAP_IE  *rie = NULL;
    DLMAP_IE_u  r_dlmap_ie;

    if(burstCollection->empty())
    {
        fprintf(stderr,"Time:%llu BS_16j_NT_Scheduler::%s DLRelayBurstCollection is empty!!!\n",GetCurrentTime(),__func__);
        exit(-1);
    }

    // encoding DL relay burst
    itd = burstCollection->begin();
    memset(&r_dlmap_ie,0x00,sizeof(DLMAP_IE_u));

    r_dlmap_ie.ie_13.diuc     = Gap_PAPR_reduction;
    r_dlmap_ie.ie_13.symOff   = (*itd)->dlmap_ie.ie_other.symOff;
    r_dlmap_ie.ie_13.chOff    = (*itd)->dlmap_ie.ie_other.chOff;
    r_dlmap_ie.ie_13.numSym   = (*itd)->dlmap_ie.ie_other.numSym;
    r_dlmap_ie.ie_13.numCh          = 0x00;
    r_dlmap_ie.ie_13.papr_or_safety = 0x00;
    r_dlmap_ie.ie_13.sounding       = 0x00;
    r_dlmap_ie.ie_13.relay_zone_ind = 0x01;

    rie = new OFDMA_DLMAP_IE(Gap_PAPR_reduction);   //diuc = 13
    rie->mallocIE(0);
    rie->appendBitField(4, r_dlmap_ie.ie_13.diuc);
    rie->appendBitField(8, r_dlmap_ie.ie_13.symOff);
    rie->appendBitField(7, r_dlmap_ie.ie_13.chOff);
    rie->appendBitField(7, r_dlmap_ie.ie_13.numSym);
    rie->appendBitField(7, r_dlmap_ie.ie_13.numCh);
    rie->appendBitField(1, r_dlmap_ie.ie_13.papr_or_safety);
    rie->appendBitField(1, r_dlmap_ie.ie_13.sounding);
    rie->appendBitField(1, r_dlmap_ie.ie_13.relay_zone_ind);   // 1:relay zone indicator

    nBits += rie->getBits();

    // Compute total length of R-MAP
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        nBits += (*itd)->ie->getBits();
    }

    int DLMAP_filed = 12;
    if (nBits % 8 == 0)
    {
        len = DLMAP_filed + nBits / 8;
        padding = false;
    }
    else
    {
        len = DLMAP_filed + nBits / 8 + 1;
        padding = true;
    }

    /*
     * Generate DL-MAP Management Message (Spec 16e. Table 16)
     */
    ifmm = new ifmgmt(MG_RMAP, len);
    ifmm->appendBitField( 8, Station->FrameDuCode());       // Frame Duration Code
    ifmm->appendBitField(24, Station->frameNumber);         // Frame Number
    ifmm->appendBitField( 8, Station->DCDCfgCount);         // Configuration change count
    ifmm->appendBitField(48, Station->address()->buf());    // Base Station ID
    ifmm->appendBitField( 8, DLsymbols);                    // OFDMA DL Relay symbols

    // set _DLRelaySymbols on phy
    Station->saveDLRelaySymbols(DLsymbols);

    /*
     * DL-MAP IE (Spec 16e. 8.4.5.3)
     */
    ifmm->appendBitField(rie->getBits(), rie->getData());

    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        dlmap_ie = (*itd)->ie;
        ifmm->appendBitField(dlmap_ie->getBits(), dlmap_ie->getData());
    }

    if (padding == true)
    {
        uint8_t zero[4] = "";
        ifmm->appendBitField( 4, zero);
    }

    // In order to encapsulate all fields, we temporarily create a BroadcastConnection here.
    pBConn = new BroadcastConnection(broadcastCID);
    pBConn->Insert(ifmm);
    pduInfo.clear();
    totalBytes = pBConn->GetInformation(&pduInfo);
    dstbuf = new char [totalBytes];
    memset(dstbuf, 0, totalBytes);
    pBConn->EncapsulateAll(dstbuf, totalBytes);

    // RzonePrefix need R-MAP information
    Station->saveRMAP(dstbuf, totalBytes);

    delete pBConn;
    if(rie)
        delete rie;
}

/*
 *  Search corresponding UL-MAP IE and return its related interval in that frame
 */
int BS_16j_NT_Scheduler::SearchULTime(int symOffset)
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::SearchULTime ");
    return ULallocStartTime + Station->symbolsToPSs(symOffset); // in PS
}


int BS_16j_NT_Scheduler::SearchULRelayTime(int symOffset)
{

	logRidvan(TRACE,"-->BS_16j_NT_Scheduler::SearchULRelayTime ");
    return RelayULallocStartTime + Station->symbolsToPSs(symOffset); // in PS
}


/* ------------------------------------------------------------------------------------ */
/*                                                                                      */
/*                                      MS Scheduler                                    */
/*                                                                                      */
/* ------------------------------------------------------------------------------------ */

vector<WiMaxBurst *> *MS_16j_NT_Scheduler::Scheduling()
{

	logRidvan(TRACE,"-->MS_16j_NT_Scheduler::Scheduling ");
    char *buf               = NULL;
    int fec                 = 0;
    unsigned int thisLen    = 0;
    unsigned int allocLen   = 0;
    unsigned int len        = 0;
    upBurst *uburst         = NULL;
    Connection *pConn       = NULL;
    vector<upBurst *> *burstCollection = new vector<upBurst *>;
    list<AllocInfo *>::iterator ita;
    vector<ManagementConnection *>::iterator itm;
    vector<DataConnection *>::iterator itd;
    vector<int> pduInfo;

    fec     = Station->UCDProfile[Station->savedULie.ie_other.uiuc].fec;
    uburst  = new upBurst(&(Station->savedULie), fec);

    // Examine management connections
    for (itm = Station->MnConnections.begin();itm != Station->MnConnections.end();itm++)
    {
        pConn = *itm;
        pduInfo.clear();
        thisLen = pConn->GetInformation(&pduInfo);

        if (thisLen > 0)
        {
            uburst->ConnectionCollection.push_back(new AllocInfo(pConn, thisLen, pduInfo));
            allocLen += thisLen;
        }
    }

    // Examine data connections
    for (itd = Station->DtConnections.begin();itd != Station->DtConnections.end();itd++)
    {
        pConn = *itd;
        pduInfo.clear();
        thisLen = pConn->GetInformation(&pduInfo);

        if (thisLen > 0)
        {
            uburst->ConnectionCollection.push_back(new AllocInfo(pConn, 0, pduInfo));
            allocLen += thisLen;
        }
    }

    // Nothing to send
    if (uburst->ConnectionCollection.empty())
    {
        delete uburst;
        delete burstCollection;
        return NULL;
    }

    burstCollection->push_back(uburst);

    // Compute burst capacity
    thisLen = uburst->toByte(Station->savedULie.ie_other.duration);
    if (allocLen > thisLen)
    {
        allocLen = thisLen;
    }

    uburst->length = allocLen;
    buf = new char [uburst->length];
    memset(buf, 0, uburst->length);
    uburst->payload = buf;

    for (ita = uburst->ConnectionCollection.begin();ita != uburst->ConnectionCollection.end(); ita++)
    {
        pConn       = (Connection *) (*ita)->conn;
        len         = pConn->EncapsulateAll(buf, allocLen);
        buf         += len;
        allocLen    -= len;
    }

    return (vector<WiMaxBurst *> *)burstCollection;
}


/* ------------------------------------------------------------------------------------ */
/*                                                                                      */
/*                                      RS Scheduler                                    */
/*                                                                                      */
/* ------------------------------------------------------------------------------------ */
vector<WiMaxBurst *> *RS_16j_NT_Scheduler::UL_Scheduling()
{

	logRidvan(TRACE,"-->RS_16j_NT_Scheduler::UL_Scheduling ");
    char *buf               = NULL;
    int fec                 = 0;
    unsigned int thisLen    = 0;
    unsigned int allocLen   = 0;
    unsigned int len        = 0;
    upBurst *uburst         = NULL;
    Connection *pConn       = NULL;

    list<AllocInfo *>::iterator ita;
    vector<ManagementConnection *>::iterator itm;
    vector<DataConnection *>::iterator itd;
    vector<int> pduInfo;

    vector<upBurst *> *burstCollection = new vector<upBurst *>;

    fec     = Station->UCDProfile[Station->savedULie.ie_other.uiuc].fec;
    uburst  = new upBurst(&(Station->savedULie), fec);

    // Examine RS management connections
    for (itm = Station->MnConnections.begin();itm != Station->MnConnections.end();itm++)
    {
        pConn = *itm;
        pduInfo.clear();
        thisLen = pConn->GetInformation(&pduInfo);

        if (thisLen > 0)
        {
            uburst->ConnectionCollection.push_back(new AllocInfo(pConn, thisLen, pduInfo));
            allocLen += thisLen;
        }
    }

    // Examine relay MS management connections
    for (itm = Station->ULRelayMnConnections.begin();itm != Station->ULRelayMnConnections.end();itm++)
    {
        pConn = *itm;
        pduInfo.clear();
        thisLen = pConn->GetInformation(&pduInfo);

        if (thisLen > 0)
        {
            //printf("[%d] pConn->getCID() = %d\n",__LINE__,pConn->getCID());
            uburst->ConnectionCollection.push_back(new AllocInfo(pConn, thisLen, pduInfo));
            allocLen += thisLen;
        }
    }

    // Examine relay MS data connections
    for (itd = Station->ULRelayDtConnections.begin();itd != Station->ULRelayDtConnections.end();itd++)
    {
        pConn = *itd;
        pduInfo.clear();
        thisLen = pConn->GetInformation(&pduInfo);

        if (thisLen > 0)
        {
            //printf("pConn->getCID() = %d\n",pConn->getCID());
            uburst->ConnectionCollection.push_back(new AllocInfo(pConn, 0, pduInfo));
            allocLen += thisLen;
        }
    }

    // Nothing to send
    if (uburst->ConnectionCollection.empty())
    {
        delete uburst;
        delete burstCollection;
        return NULL;
    }

    burstCollection->push_back(uburst);

    // Compute burst capacity
    thisLen = uburst->toByte(Station->savedULie.ie_other.duration);
    if (allocLen > thisLen)
    {
        allocLen = thisLen;
    }

    uburst->length = allocLen;
    buf = new char [uburst->length];
    memset(buf, 0, uburst->length);
    uburst->payload = buf;

    for (ita = uburst->ConnectionCollection.begin();ita != uburst->ConnectionCollection.end(); ita++)
    {
        pConn       = (Connection *) (*ita)->conn;
        len         = pConn->EncapsulateAll(buf, allocLen);
        buf         += len;
        allocLen    -= len;
    }

    return (vector<WiMaxBurst *> *)burstCollection;
}

void RS_16j_NT_Scheduler::DLAccess_Scheduling(vector <downBurst *>*burstCollection,int availableSlots)
{
    int DLsubchannels       = Station->DLsubchannels();               // (30)
    Connection *pConn   = NULL;
    downBurst *burst    = NULL;
    AllocInfo *aInfo    = NULL;
    uint32_t allocLen   = 0;
    int thisLen         = 0;
    int broadcastLen    = 0;
    int symbolOffset    = 0;
    int allocSlots      = 0;
    int needSlots       = 0; 
    vector<int> pduInfo;
    int DLAccessoccupiedSymbols = 0;
    vector<downBurst *>::iterator       itd;
    list<AllocInfo *>::iterator         ita;
    vector <ManagementConnection *>::iterator itm;
    vector <DataConnection *>::iterator itdc;
    vector <msInfo>::iterator           itms;
    bool    isDone  = false;
    msObject_NT *pMS = NULL;

    /* Compute total bytes of broadcast connections */
    pConn = Station->broadcastConnection;
    pduInfo.clear();
    broadcastLen = pConn->GetInformation(&pduInfo); // ignore pduInfo here
    //printf("Time:%llu RS_16j_NT_Scheduler::%s broadcastLen=%d\n",GetCurrentTime(),__func__,broadcastLen);
    if (broadcastLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->length += broadcastLen;
        burst->duration = computeNearestSlots(burst->toSlots(burst->length));
        allocSlots += burst->duration;
        //printf("RS %s() allocSlots=%d availableSlots=%d\n",__func__,allocSlots,availableSlots);
    }

    /* Examine Initial Ranging connection and use QPSK 1/2 */
    pConn = Station->initRangingConnection;
    pduInfo.clear();
    allocLen = pConn->GetInformation(&pduInfo);

    if (allocLen > 0)
    {
        //printf("Time:%llu RS[%d]::%s() initRangingConnection allocLen=%d\n",GetCurrentTime(),Station->get_nid(),__func__,allocLen);
        
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));

        burst->length += allocLen;
        burst->duration = computeNearestSlots(burst->toSlots(burst->length));
        allocSlots += burst->duration;
        //printf("RS %s() allocSlots=%d availableSlots=%d\n",__func__,allocSlots,availableSlots);
    }

    // Examine relay MS management connections
    for (itm = Station->DLRelayMnConnections.begin();itm != Station->DLRelayMnConnections.end();itm++)
    {
        pConn = *itm;
        pMS = ((mac802_16j_NT_PMPBS *)(Station->access_station))->getMSbycid(pConn->getCID());
        if(!pMS)
        {
            printf("RS::%s() get MS(bcid=%d) object failed\n",__func__,pConn->getCID());
            exit(1);
        }

        burst = GetCorrespondBurst(burstCollection,(pMS->myMS)->getDIUC());
        pduInfo.clear();
        thisLen = pConn->GetInformation(&pduInfo);

        if (thisLen > 0)
        {
            burst->ConnectionCollection.push_back(new AllocInfo(pConn, thisLen, pduInfo));
            //printf("[%d] pConn->getCID() = %d\n",__LINE__,pConn->getCID());
            burst->length += allocLen;
            burst->duration = computeNearestSlots(burst->toSlots(burst->length));
            allocSlots += burst->duration;

            if(allocSlots > availableSlots)
            {
                allocSlots -= burst->duration;
                printf("\e[1;35mRS %s() relay MS[%d] management connections allocSlots=%d availableSlots=%d\e[0m\n",__func__,pMS->myMS->get_nid(),allocSlots,availableSlots);
            }
        }
    }

    // Examine relay MS data connections
    for (itdc = Station->DLRelayDtConnections.begin();itdc != Station->DLRelayDtConnections.end();itdc++)
    {
        pConn = *itdc;
        pMS = ((mac802_16j_NT_PMPBS *)(Station->access_station))->getMSbyDtcid(pConn->getCID());
        if(!pMS)
        {
            printf("RS::%s() get MS(dcid=%d) object failed\n",__func__,pConn->getCID());
            exit(1);
        }
        //printf("RS getMSbyDtcid (nid=%d cid=%d)\n",pMS->myMS->get_nid(),pConn->getCID());

        burst = GetCorrespondBurst(burstCollection,(pMS->myMS)->getDIUC());
        pduInfo.clear();
        thisLen = pConn->GetInformation(&pduInfo);

        if (thisLen > 0)
        {
            //printf("[%d] pConn->getCID() = %d\n",__LINE__,pConn->getCID());
            //burst->length += allocLen;
           // burst->duration = computeNearestSlots(burst->toSlots(burst->length));
            burst->ConnectionCollection.push_back(new AllocInfo(pConn, 0, pduInfo));
            //allocLen += thisLen;
            //allocLen += burst->duration;
        }
    }

    /* Select pdus from each Connection of each burst at one time */
    isDone = false;

    while (isDone == false)
    {
        isDone = true;

        /* for each Burst */
        for (itd = burstCollection->begin();itd != burstCollection->end();itd++)
        {
            burst = *itd;
            allocSlots -= burst->duration;

            /* for each Connection */
            for (ita = burst->ConnectionCollection.begin();ita != burst->ConnectionCollection.end();ita++)
            {
                aInfo = *ita;
                pConn = (Connection *) aInfo->conn;

                if (aInfo->position >= aInfo->pduInfo.size()) // this AllocInfo has no more pdu
                {
                    continue;
                }

                //if(pConn->getCID() > 2*MAXMS + 2*MAXRS)
                //{
                /* check enough slots to allocate */
                needSlots = computeNearestSlots(burst->toSlots(burst->length + aInfo->getNext()));
                if (allocSlots + needSlots > availableSlots) // not enough slots for this pdu
                {
                    isDone = true;

                    aInfo->len      += burst->toByte(availableSlots - allocSlots) - burst->length;
                    burst->length   = burst->toByte(availableSlots - allocSlots);
                    break;
                }
                else // enough slots for this pdu
                {
                    isDone = false;
                    //			printf("Time:%llu burst->length=%d \n",GetCurrentTime(),burst->length);

                    burst->length += aInfo->getNext();
                    burst->addCID(pConn->getCID());
                    //printf("Time:%llu RS_16j_NT_Scheduler::%s() burst->addCID(%d) length=%d\n",GetCurrentTime(),__func__,pConn->getCID(),burst->length);
                    aInfo->len += aInfo->getNext();
                    aInfo->position++;
                }
                //}
            }
            burst->duration = computeNearestSlots(burst->toSlots(burst->length));
            allocSlots += burst->duration;

            //burst->Dump_downBurst();
        }
    }

    /* Examine Broadcast connection */
    pConn = Station->broadcastConnection;
    pduInfo.clear();
    allocLen = pConn->GetInformation(&pduInfo);

    if (allocLen > 0)
    {
        burst = GetCorrespondBurst(burstCollection, RobustDIUC);
        burst->ConnectionCollection.push_front(new AllocInfo(pConn, allocLen, pduInfo));
        burst->addCID(broadcastCID);
    }

    if(allocSlots % DLsubchannels == 0)
        DLAccessoccupiedSymbols = (int)(allocSlots / DLsubchannels) * DL_PUSC;
    else
        DLAccessoccupiedSymbols = (int)(allocSlots / DLsubchannels +1) * DL_PUSC;

    DLAccessoccupiedSymbols += (PREAMBLE + DL_PUSC);
    //printf("===Time:%llu ==END=OF=DL=SCHEDULING===SIZE=\e[35m%d\e[0m===\n",GetCurrentTime(),burstCollection->size());
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        uint8_t nSyms = 0;
        uint8_t nChs = 0;

        burst = *itd;

        burst->dlmap_ie.ie_other.numCid     = burst->nCid;
        burst->dlmap_ie.ie_other.cid        = new uint16_t [burst->nCid];
        memset(burst->dlmap_ie.ie_other.cid, 0, burst->nCid);
        for (int i = 0;i < burst->nCid;i++)
            burst->dlmap_ie.ie_other.cid[i] = burst->cid[i];
        burst->dlmap_ie.ie_other.symOff     = symbolOffset;
        burst->dlmap_ie.ie_other.chOff      = 0;
        burst->dlmap_ie.ie_other.boosting   = 0;
        mapComputeSymCh(burst->duration, &nSyms, &nChs);
        burst->dlmap_ie.ie_other.numSym     = nSyms;
        burst->dlmap_ie.ie_other.numCh      = nChs;
        burst->dlmap_ie.ie_other.repeCode   = 0;
        burst->encapsulateAllField();
        symbolOffset += nSyms;
    }

    /* Erase unused burst */
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        burst = *itd;

        if (burst->duration == 0)
        {
            delete burst;
            burstCollection->erase(itd);
            itd--;
        }
    }
}

vector<WiMaxBurst *> *RS_16j_NT_Scheduler::DL_Scheduling()
{

	logRidvan(TRACE,"-->RS_16j_NT_Scheduler::DL_Scheduling ");
    int DLsubchannels       = Station->DLsubchannels();               // (30)
    int ULsubchannels       = Station->DLsubchannels();               // (35)
    Connection *pConn   = NULL;
    downBurst *burst    = NULL;
    uint32_t requestLen     = 0;
    uint32_t len            = 0;
    list<AllocInfo *>::iterator         ita;

    vector<int> PduInfo;
    vector<downBurst *> *RS_DLAccessBurstCollection = new vector<downBurst *>;

    list<DataConnection *>::iterator    itdc;
    list<msObject_NT *>::iterator       its;
    vector<downBurst *>::iterator       itd;

    int DLAccessAvailableSymbols = Station->_DLAccessSymbols;
    int ULAccessAvailableSymbols = Station->_ULAccessSymbols;
    int ULAccessAvailableSlots = (int)(ULAccessAvailableSymbols/UL_PUSC)*ULsubchannels;
    int DLAccessAvailableSlots = (int)((DLAccessAvailableSymbols - PREAMBLE - DL_PUSC)/DL_PUSC)*DLsubchannels;

    DLsymbols = Station->_DLAccessSymbols + Station->_DLRelaySymbols;
    ULallocStartTime = Station->symbolsToPSs(DLsymbols) + Station->TTG(); 
    RelayDLallocStartTime = Station->symbolsToPSs(DLAccessAvailableSymbols);
    RelayULallocStartTime = ULallocStartTime + Station->symbolsToPSs(Station->_ULAccessSymbols);

    //printf("[%d]Time:%llu %s() ULallocStartTime = %d (PSs)\n",__LINE__,GetCurrentTime(),__func__,ULallocStartTime);
    /*
       printf("Time:%llu BS_16j_NT_Scheduler::%s\n",GetCurrentTime(),__func__);
       printf("--- DLAccessSymbols = %d\n",Station->_DLAccessSymbols);
       printf("--- DLRelaySymbols = %d\n",Station->_DLRelaySymbols);
       printf("--- ULAccessSymbols = %d\n",Station->_ULAccessSymbols);
       printf("--- ULRelaySymbols = %d\n",Station->_ULRelaySymbols);
       */   


    // nothing to send
    if(!Station->broadcastConnection)
    {
        printf("Time:%llu RS_16j_NT_Scheduler::%s() no broadcast messages\n",GetCurrentTime(),__func__);
        delete RS_DLAccessBurstCollection;
        return NULL;
    }

    /* Clear BurstCollection */
    while(!RS_ULAccessBurstCollection->empty())
    {
        upBurst *burst = RS_ULAccessBurstCollection->back();
        delete burst;
        RS_ULAccessBurstCollection->pop_back();
    }

    // Schedule UL Access zone 
    ContentionScheduling(RS_ULAccessBurstCollection,ULAccessAvailableSymbols);

    CDMAallocScheduling(RS_ULAccessBurstCollection,ULAccessAvailableSlots);
    UGSScheduling(RS_ULAccessBurstCollection);

    generate_UL_MAP(RS_ULAccessBurstCollection,ULallocStartTime,Station->_ULAccessSymbols);

    // Schedule DL Access zone to send to MSs
    DLAccess_Scheduling(RS_DLAccessBurstCollection,DLAccessAvailableSlots);    

    sort(RS_DLAccessBurstCollection->begin(), RS_DLAccessBurstCollection->end(), compare);
    RS_16j_NT_Scheduler::generate_DL_MAP(RS_DLAccessBurstCollection,DLAccessAvailableSymbols);

    /* Encapsulate all Burst */
    for (itd = RS_DLAccessBurstCollection->begin();itd != RS_DLAccessBurstCollection->end(); itd++)
    {
        char *buf       = NULL;
        burst           = *itd;
        buf             = new char [burst->length];
        memset(buf, 0, burst->length);
        burst->payload  = buf;

        for (ita = burst->ConnectionCollection.begin();ita != burst->ConnectionCollection.end(); ita++)
        {
            pConn       = (Connection *) (*ita)->conn;
            requestLen  = (*ita)->len;
            len         = pConn->EncapsulateAll(buf, requestLen);
            buf         += len;
        }
    }

    return (vector <WiMaxBurst *>*)RS_DLAccessBurstCollection;
}

/*
 * Generate a MAC PDU containing DL-MAP.
 */
void RS_16j_NT_Scheduler::generate_DL_MAP(vector<downBurst *> *burstCollection, int DLsymbols)
{
    unsigned int len            = 0;
    unsigned int nBits          = 0;
    ifmgmt *ifmm                = NULL;
    OFDMA_DLMAP_IE *dlmap_ie    = NULL;
    bool padding                = false;
    BroadcastConnection *pConn  = NULL;
    int totalBytes              = 0;
    char *dstbuf                = NULL;
    vector<downBurst *>::iterator itd;
    vector<int> pduInfo;

    //printf("Time:%llu RS_16j_NT_Scheduler::%s\n",GetCurrentTime(),__func__);
    // Compute total length of DL-MAP
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        nBits += (*itd)->ie->getBits();
    }

    if (nBits % 8 == 0)
    {
        len = 12 + nBits / 8;
        padding = false;
    }
    else
    {
        len = 12 + nBits / 8 + 1;
        padding = true;
    }

    /*
     * Generate DL-MAP Management Message (Spec 16e. Table 16)
     */
    ifmm = new ifmgmt(MG_DLMAP, len);
    ifmm->appendBitField( 8, Station->frameDuCode());       // Frame Duration Code
    ifmm->appendBitField(24, Station->frameNumber);         // Frame Number
    ifmm->appendBitField( 8, Station->DCDCfgCount);         // Configuration change count
    ifmm->appendBitField(48, Station->address()->buf());    // Base Station ID
    ifmm->appendBitField( 8, DLsymbols);                    // OFDMA DL symbols

    //Station->saveDLAccessSymbols(DLsymbols);

    /*
     * DL-MAP IE (Spec 16e. 8.4.5.3)
     */
    for (itd = burstCollection->begin();itd != burstCollection->end(); itd++)
    {
        dlmap_ie = (*itd)->ie;
        ifmm->appendBitField(dlmap_ie->getBits(), dlmap_ie->getData());
    }

    if (padding == true)
    {
        uint8_t zero[4] = "";
        ifmm->appendBitField( 4, zero);
    }

    // In order to encapsulate all fields, we temporarily create a BroadcastConnection here.
    pConn = new BroadcastConnection(broadcastCID);
    pConn->Insert(ifmm);
    pduInfo.clear();
    totalBytes = pConn->GetInformation(&pduInfo);
    dstbuf = new char [totalBytes];
    memset(dstbuf, 0, totalBytes);
    pConn->EncapsulateAll(dstbuf, totalBytes);

    // DLFP need DLMAP information
    Station->saveDLMAP(dstbuf, totalBytes);
    delete pConn;
}

/*
 * Generate a MAC PDU containing UL-MAP.
 */
void RS_16j_NT_Scheduler::generate_UL_MAP(vector<upBurst *> *burstCollection, int _ULallocStartTime, int ULsymbols)
{
    unsigned int len            = 0;
    unsigned int nBits          = 0;
    uint8_t rsv = 0;
    ifmgmt *ifmm                = NULL;
    OFDMA_ULMAP_IE *ulmap_ie    = NULL;
    bool padding                = false;
    vector<upBurst *>::iterator itu;

    //printf("Time:%llu RS[%d]::%s() ULallocStartTime=%d ULsymbols=%d\n",GetCurrentTime(),Station->get_nid(),__func__,_ULallocStartTime,ULsymbols);

    // Compute total length of UL-MAP
    for(itu = burstCollection->begin();itu != burstCollection->end();itu++)
    {
        nBits += (*itu)->ie->getBits();
    }

    // Check if need padding
    if(nBits % 8 == 0)
    {
        len = 7 + nBits / 8;
        padding = false;
    }
    else
    {
        len = 7 + nBits / 8 + 1;
        padding = true;
    }

    /*
     * Generate UL-MAP Management Message (Spec 16e. Table 18)
     */
    ifmm = new ifmgmt(MG_ULMAP, len);
    ifmm->appendBitField( 8, rsv);                  // Reserved
    ifmm->appendBitField( 8, Station->UCDCfgCount); // Configuration change count
    ifmm->appendBitField(32, _ULallocStartTime);     // Uplink Alloation start time
    ifmm->appendBitField( 8, ULsymbols);            // OFDMA UL symbols
    //Station->saveULsymbols(ULsymbols);

    /*
     * UL-MAP IE (Spec 16e. 8.4.5.4)
     */
    for (itu = burstCollection->begin();itu != burstCollection->end();itu++)
    {
        ulmap_ie = (*itu)->ie;
        ifmm->appendBitField(ulmap_ie->getBits(), ulmap_ie->getData());
    }

    if (padding == true)
    {
        uint8_t zero[4] = "";
        ifmm->appendBitField( 4, zero);
    }

    Station->broadcastConnection->Insert(ifmm);
}

/*
 * Append contention allocations to burstCollection at Ranging Subchannel
 */
int RS_16j_NT_Scheduler::ContentionScheduling(vector<upBurst *> *burstCollection, int ULoccupiedSymbols)
{
    int fec             = 0;
    int allocSymbols    = 0;
    int rangMethod      = 0;
    int rangOverSymbol  = 0;
    int rangOpportunity = 0;
    upBurst *uburst     = NULL;


    /**************************************************
     * --- CDMA Initial Ranging / Handover Ranging ---
     **************************************************/
    fec = Station->UCDProfile[CDMA_BWreq_Ranging].fec; // BPSK
    rangMethod      = 0;
    rangOverSymbol  = 2;
    rangOpportunity = ULoccupiedSymbols / 3;

    uburst = new upBurst(broadcastCID, CDMA_BWreq_Ranging, fec);
    uburst->ulmap_ie.ie_12.cid              = broadcastCID;
    uburst->ulmap_ie.ie_12.uiuc             = CDMA_BWreq_Ranging;
    uburst->ulmap_ie.ie_12.symOff           = allocSymbols;
    uburst->ulmap_ie.ie_12.chOff            = 0;
    uburst->ulmap_ie.ie_12.numSym           = rangOpportunity * rangOverSymbol;
    uburst->ulmap_ie.ie_12.numCh            = 6;
    uburst->ulmap_ie.ie_12.rangMethod       = rangMethod;
    uburst->ulmap_ie.ie_12.rangIndicator    = 1;
    uburst->encapsulateAllField();

    burstCollection->push_back(uburst);
    allocSymbols += rangOpportunity * rangOverSymbol;

    /****************************************
     * --- BW Request / Periodic Ranging ---
     ****************************************/
    fec = Station->UCDProfile[CDMA_BWreq_Ranging].fec; // BPSK
    rangMethod      = 2;
    rangOverSymbol  = 1;
    rangOpportunity = ULoccupiedSymbols / 3;

    uburst = new upBurst(broadcastCID, CDMA_BWreq_Ranging, fec);
    uburst->ulmap_ie.ie_12.cid              = broadcastCID;
    uburst->ulmap_ie.ie_12.uiuc             = CDMA_BWreq_Ranging;
    uburst->ulmap_ie.ie_12.symOff           = allocSymbols;
    uburst->ulmap_ie.ie_12.chOff            = 0;
    uburst->ulmap_ie.ie_12.numSym           = rangOpportunity * rangOverSymbol;
    uburst->ulmap_ie.ie_12.numCh            = 6;
    uburst->ulmap_ie.ie_12.rangMethod       = rangMethod;
    uburst->ulmap_ie.ie_12.rangIndicator    = 1;
    uburst->encapsulateAllField();

    burstCollection->push_back(uburst);
    allocSymbols += rangOpportunity * rangOverSymbol;

    return allocSymbols;
}

/*
 * Append CDMA allocation IE at Uplink allocation
 */
void RS_16j_NT_Scheduler::CDMAallocScheduling(vector<upBurst *> *burstCollection, int availableSlots)
{
    int fec = 0;
    upBurst *uburst = NULL;
    if(saved_ir_code.empty() || (Station->relay_AL.empty()))
        return;

    vector <ULMAP_IE_u>::iterator itu;
    vector <rngcode>::iterator itr;


    for(itu=Station->relay_AL.begin() ; itu != Station->relay_AL.end() ; itu++)
    {
        if(itr == saved_ir_code.end())
            break;
        
        itr = saved_ir_code.begin();

        fec = Station->UCDProfile[CDMA_Alloc_IE].fec;
        uburst = new upBurst(broadcastCID, CDMA_Alloc_IE, fec);

        (*itu).ie_14.frameIndex   = (*itr).frameIndex;
        (*itu).ie_14.rangCode     = (*itr).rangCode; 
        (*itu).ie_14.rangSym      = (*itr).rangSym;
        (*itu).ie_14.rangCh       = (*itr).rangCh;

        uburst->ulmap_ie.ie_14 = (*itu).ie_14;

        uburst->encapsulateAllField();
        burstCollection->push_back(uburst);

        // save to RS for relaying
        //itr++;
        saved_ir_code.erase(itr);
    }
    //saved_ir_code.clear();
}

void RS_16j_NT_Scheduler::UGSScheduling(vector <upBurst *> *burstCollection)
{
    upBurst *uburst = NULL;
    vector <ULMAP_IE_u>::iterator itu; 
    //printf("Station->relay_ULie_others.size() = %d\n",Station->relay_ULie_others.size());
    for(itu=Station->relay_ULie_others.begin() ; itu != Station->relay_ULie_others.end() ; itu++)
    {
        ULMAP_IE_u ULie;
        memset(&ULie, 0, sizeof(ULMAP_IE_u));

        ULie = *itu;

        uburst = new upBurst(&ULie,Station->UCDProfile[ULie.ie_other.uiuc].fec);

        burstCollection->push_back(uburst);
    }
    Station->relay_ULie_others.clear();
}

int RS_16j_NT_Scheduler::SearchULTime(int symOffset)
{

	logRidvan(TRACE,"-->RS_16j_NT_Scheduler::SearchULTime ");
    return ULallocStartTime + Station->symbolsToPSs(symOffset); // in PS
}

