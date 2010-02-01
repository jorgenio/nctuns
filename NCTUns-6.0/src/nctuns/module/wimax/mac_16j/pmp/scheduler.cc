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
#include "msobject.h"

using namespace std;
using namespace mobileRelayMacAddress;
using namespace mobileRelayServiceFlow;
using namespace mobileRelayBurst;

static bool compare(const void *a, const void *b)
{
	downBurst *oa = (downBurst *) a;
	downBurst *ob = (downBurst *) b;
	return (oa->ie->_diuc) < (ob->ie->_diuc);
}

downBurst *BS_16j_Scheduler::GetCorrespondBurst(vector<downBurst *> *burstCollection, int diuc)
{
	vector<downBurst *>::iterator   itd;
	downBurst                       *pBurst;

	for (itd = burstCollection->begin(); itd != burstCollection->end(); itd++)
	{
		if ((*itd)->ie->_diuc == diuc)
			return *itd;
	}
	/*New diuc busrt*/
	pBurst = new downBurst(diuc, Station->DCDProfile[diuc].fec);

	if (diuc == Extended_DIUC_2)
		pBurst->dlmap_ie.ie_14.diuc = Extended_DIUC_2;
	else if (diuc == Extended_DIUC)
		pBurst->dlmap_ie.ie_15.diuc = Extended_DIUC;
	else
		pBurst->dlmap_ie.ie_other.diuc = diuc;

	burstCollection->push_back(pBurst);
	return pBurst;
}

/* ------------------------------------------------------------------------------------ */

vector<WiMaxBurst *> *BS_16j_Scheduler::Scheduling()
{
	uint32_t requestLen     = 0;
	uint32_t len            = 0;
	downBurst *burst        = NULL;
	Connection *pConn       = NULL;
	int DLsubchannels       = Station->DLsubchannels();               // (30)
	int ULsubchannels       = Station->ULsubchannels();               // (35)
	int symbolsPerFrame     = Station->symbolsPerFrame();             // (48)
	int ULAvailableSymbols  = (int) (symbolsPerFrame * MAX_UL_RATIO); // (24)
	int ULRelayAvailableSymbols = (int) (ULAvailableSymbols * MAX_UL_RELAY_ZONE_RATIO); //(12)
	int ULAccessAvailableSymbols = 0;
	int DLTransAvailableSymbols = 0;
	int DLAccessAvailableSymbols = 0;
	int DLAvailableSymbols  = symbolsPerFrame - ULAvailableSymbols;   // (24)
	int DLAvailableSlots    = 0;
	int DLAccessAvailableSlots = 0;
	int DLTransAvailableSlots    = 0;
	int ULRelayAvailableSlots = (ULRelayAvailableSymbols / UL_PUSC) * (ULsubchannels - 6); //(116)
	int ULAccessAvailableSlots = 0;
	int allocSymbols        = 0;
	int allocSlots_access   = 0;
        int allocSlots_relay    = 0;
	int allocSlots_trans	= 0;
	int thisSlots           = 0;
	int ULoccupiedSymbols   = 0;
	int ULRelayoccupiedSymbols   = 0;
	int ULAccessoccupiedSymbols   = 0;
	int DLTransoccupiedSymbols = 0;
	int symbolOffset        = 0;
	vector<int> PduInfo;
	vector<downBurst *> *downAccessBurstCollection = new vector<downBurst *>;
	vector<downBurst *>::iterator itd;
	list<AllocInfo *>::iterator ita;

	/* Clear upBurstCollection */
	while(!upBurstCollection->empty())
	{
		delete *(upBurstCollection->begin());
		upBurstCollection->erase(upBurstCollection->begin());
	}

	/* Clear pre-downTransparentBurstCollection */
	while(!downTransparentBurstCollection->empty())
        {
                delete *(downTransparentBurstCollection->begin());
                downTransparentBurstCollection->erase(downTransparentBurstCollection->begin());
        }
	
	/*Compute UL relay zone allocate symbols */	
	thisSlots = UGSScheduling_RSs(upBurstCollection, ULRelayAvailableSlots);
        allocSlots_relay += thisSlots;
        ULRelayAvailableSlots -= thisSlots;

	/* Scheduling UL relay zone BE traffic */
	thisSlots = BEScheduling_RSs(upBurstCollection, ULRelayAvailableSlots);
        allocSlots_relay += thisSlots;
        ULRelayAvailableSlots -= thisSlots;

	if (allocSlots_relay % (ULsubchannels - 6) == 0)
                ULRelayoccupiedSymbols = (allocSlots_relay / (ULsubchannels - 6)) * UL_PUSC;
        else
                ULRelayoccupiedSymbols = (allocSlots_relay / (ULsubchannels - 6) + 1) * UL_PUSC;
	
	ULAccessAvailableSymbols = ULAvailableSymbols - ULRelayoccupiedSymbols;
	ULAccessAvailableSlots = (ULAccessAvailableSymbols / UL_PUSC) * (ULsubchannels - 6);
	
	/* ----------- Uplink allocation ----------- */
	/* Scheduling CDMA_Allocation_IE */
	thisSlots = CDMAallocScheduling(upBurstCollection, ULAccessAvailableSlots);
	allocSlots_access += thisSlots;
	ULAccessAvailableSlots -= thisSlots;

	/* Scheduling UL UGS traffic */
	thisSlots = UGSScheduling_MSs(upBurstCollection, ULAccessAvailableSlots);
	allocSlots_access += thisSlots;
	ULAccessAvailableSlots -= thisSlots;

	/* Scheduling BE traffic */
	thisSlots = BEScheduling_MSs(upBurstCollection, ULAccessAvailableSlots);
	allocSlots_access += thisSlots;
	ULAccessAvailableSlots -= thisSlots;

	/* At least one slot at UL subframe */
	if (allocSlots_access == 0)
		allocSlots_access++;

        if (allocSlots_access % (ULsubchannels - 6) == 0)
                ULAccessoccupiedSymbols = (allocSlots_access / (ULsubchannels - 6)) * UL_PUSC;
        else
                ULAccessoccupiedSymbols = (int)(allocSlots_access / (ULsubchannels - 6) + 1) * UL_PUSC;
	
	ULoccupiedSymbols = ULAccessoccupiedSymbols + ULRelayoccupiedSymbols;

	/* save UL-subframe zone mapping information */
	Station->saveULAccessSymbols(ULAccessoccupiedSymbols);
	Station->saveULRelaySymbols(ULRelayoccupiedSymbols);	

	/* Scheduling contention period */
	allocSymbols = ContentionScheduling(upBurstCollection, ULAccessoccupiedSymbols);

	/* Recompute the DL occupied symbols and slots */
	DLAvailableSymbols = symbolsPerFrame - ULoccupiedSymbols;
	DLAvailableSlots = (int)((DLAvailableSymbols - PREAMBLE - DL_PUSC) / DL_PUSC) * DLsubchannels;
	/*save DL symbols to allocStartTime adjustment*/
	DLsymbols = DLAvailableSymbols;

	ULallocStartTime = Station->symbolsToPSs(DLsymbols) + Station->TTG();
	ULrelayallocStartTime = Station->symbolsToPSs(DLAvailableSymbols+ ULAccessoccupiedSymbols) + Station->TTG();
	
	/* Generate UL-MAP */
	generate_UL_MAP(upBurstCollection, ULallocStartTime, ULoccupiedSymbols);

	/* Scheduling DL Transparent zone traffics */
	DLTransAvailableSymbols = (int)((DLAvailableSymbols - PREAMBLE - DL_PUSC) * MAX_TRANSPARENT_ZONE_RATIO);
	DLTransAvailableSlots = (int)(DLTransAvailableSymbols / DL_PUSC) * DLsubchannels;
	
	allocSlots_trans = TransparentScheduling(downTransparentBurstCollection, DLTransAvailableSlots);
	//printf("\e[1;31m***DLTransAvailableSlots = %d, allocSlots_trans = %d ****\e[0m\n", DLTransAvailableSlots, allocSlots_trans);
	if (allocSlots_trans % DLsubchannels == 0)
                DLTransoccupiedSymbols = (allocSlots_trans / DLsubchannels) * DL_PUSC;
        else
                DLTransoccupiedSymbols = (int)(allocSlots_trans / DLsubchannels + 1) * DL_PUSC;
	
	DLAccessAvailableSymbols = DLAvailableSymbols - DLTransoccupiedSymbols;
	DLAccessAvailableSlots = (int)((DLAccessAvailableSymbols - PREAMBLE - DL_PUSC) / DL_PUSC) * DLsubchannels;

	/* Scheduling DL access zone traffics */
	allocSlots_access = DLAccessScheduling(downAccessBurstCollection, DLAccessAvailableSlots);

	//printf("\e[1;31m***DLAccessAvailableSlots = %d ,DLTransAvailableSlots = %d,  allocSlots_access = %d , allocSlots_trans = %d ****\e[0m\n",DLAccessAvailableSlots, DLTransAvailableSlots, allocSlots_access, allocSlots_trans);
	//printf("***ULAccessoccupiedSymbols = %d , ULRelayoccupiedSymbols = %d ****\n", ULAccessoccupiedSymbols, ULRelayoccupiedSymbols);
	
	/* record DL zone partition information*/
	Station->saveDLAccessSymbols(DLAccessAvailableSymbols);
	Station->saveDLTransparentSymbols(DLTransoccupiedSymbols);

	/* Compute burst slot allocation to set IE */
	for (itd = downAccessBurstCollection->begin();itd != downAccessBurstCollection->end(); itd++)
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

	symbolOffset = DLAccessAvailableSymbols;
	for (itd = downTransparentBurstCollection->begin();itd != downTransparentBurstCollection->end(); itd++)
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
	for (itd = downAccessBurstCollection->begin();itd != downAccessBurstCollection->end(); itd++)
	{
		burst = *itd;

		if (burst->duration == 0)
		{
			delete burst;
			downAccessBurstCollection->erase(itd);
			itd--;
		}
	}

	/* Erase unused burst */
	for (itd = downTransparentBurstCollection->begin();itd != downTransparentBurstCollection->end(); itd++)
        {
                burst = *itd;

                if (burst->duration == 0)
                {
                        delete burst;
                        downTransparentBurstCollection->erase(itd);
                        itd--;
                }
        }

	/* Sorting burst ordering with ascending and generate DL-MAP and save to PHY*/
	sort(downAccessBurstCollection->begin(), downAccessBurstCollection->end(), compare);
	sort(downTransparentBurstCollection->begin(), downTransparentBurstCollection->end(), compare);

	generate_DL_MAP(downAccessBurstCollection, downTransparentBurstCollection, DLAvailableSymbols);

	/* Encapsulate access zone Burst */
	for (itd = downAccessBurstCollection->begin();itd != downAccessBurstCollection->end(); itd++)
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

	/* Encapsulate transparent zone Burst , this Burst is used on Transparent zone*/
	for (itd = downTransparentBurstCollection->begin();itd != downTransparentBurstCollection->end(); itd++)
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
	

	return (vector<WiMaxBurst *> *)downAccessBurstCollection;
}
/* Scheduling DL Transparent zone */
int BS_16j_Scheduler::TransparentScheduling(vector<downBurst *> *burstCollection, int availableSlots)
{
	
	Connection *pConn   = NULL;
	downBurst *burst    = NULL;
	AllocInfo *aInfo    = NULL;
	uint32_t thisLen    = 0;
	uint32_t allocLen   = 0;
	int needSlots       = 0;
	int allocSlots      = 0;
	bool isDone         = false;
	vector<int> pduInfo;

	rsObject *pRS       = NULL;
	mac802_16j_PMPRS *pRSm = NULL;
	msObject_mr *pMS = NULL;
	
	//list<DataConnection *>::iterator    itdc;
        list<AllocInfo *>::iterator         ita;
        vector<downBurst *>::iterator       itd;	

	list<rsObject *>::iterator          itr;
	vector <ManagementConnection *>::iterator itmc;
	vector <DataConnection *>::iterator itdc;
	
	
	for (int i = 0; i < MAXRS; i++)	
	{
		if (Station->rslist[i] == NULL)
			continue;

		if (Station->rslist[i]->ScanFlag == true)
			continue;

		if (Station->rslist[i]->ResourceStatus != RS_Serving)
			continue;

		pRS = Station->rslist[i];
		pRSm = Station->rslist[i]->myRS;
		pRSm->downTransBurst = (vector<WiMaxBurst *> *)burstCollection;


		//Examine relay Management Connection
		for (itmc = pRSm->DL_relayManalist.begin(); itmc != pRSm->DL_relayManalist.end(); itmc++)
		{
			pConn = *itmc;
			if((pMS = Station->getMSbyManacid(pConn->getCID())) == NULL)
			{
				printf("***(%d):%s:Getting MS(mcid=%d) object failed!***\n", __LINE__, __func__, pConn->getCID());
				exit(1);
			}
			/* Getting burst information between RS to MSs */
			burst = GetCorrespondBurst(burstCollection, pMS->rdiuc);
			pduInfo.clear();
                        allocLen = pConn->GetInformation(&pduInfo);

			if (allocLen > 0)
                        {
				burst->length += allocLen;
				burst->duration = computeNearestSlots(burst->toSlots(burst->length));
				burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));
				allocSlots += burst->duration;
                        }
		}

		//Examine relay Data Connection
		for(itdc = pRSm->DL_relayDtlist.begin();itdc != pRSm->DL_relayDtlist.end();itdc++)
                {
                        pConn = *itdc;
			if((pMS = Station->getMSbyDtcid(pConn->getCID())) == NULL)
			{
				printf("***(%d):%s:Getting MS(dcid=%d) object failed!***\n", __LINE__, __func__, pConn->getCID());
				exit(1);
			}
			/* Getting burst information between RS to MSs */
			burst = GetCorrespondBurst(burstCollection, pMS->rdiuc);
                        pduInfo.clear();
                        thisLen = pConn->GetInformation(&pduInfo);

                        if(thisLen > 0)
                        {
                                burst->ConnectionCollection.push_back(new AllocInfo(pConn, 0, pduInfo));
                        }
                }	

	}

	
	/* We will reverse the connection order to avoid connection starvation */
	if (rand() % 2) {
                for (itd = burstCollection->begin();itd != burstCollection->end(); itd++) {
                        burst = *itd;
                        burst->ConnectionCollection.reverse();
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

						burst->length += aInfo->getNext();	// in bytes
						burst->addCID(pConn->getCID());
						aInfo->len += aInfo->getNext();
						aInfo->position++;
					}
				}
			}
			/* mapping burst length to slots */
			burst->duration = computeNearestSlots(burst->toSlots(burst->length));
			allocSlots += burst->duration;
		}
	}
	return allocSlots;	
}

/*
 * Append downlink allocations to burstCollection.
 */
int BS_16j_Scheduler::DLAccessScheduling(vector<downBurst *> *burstCollection, int availableSlots)
{
	list<DataConnection *>::iterator    itdc;
	list<AllocInfo *>::iterator         ita;
	list<msObject_mr *>::iterator          its;
	vector<downBurst *>::iterator       itd;

	Connection *pConn   = NULL;
	downBurst *burst    = NULL;
	AllocInfo *aInfo    = NULL;
	msObject_mr *pMS       = NULL;
	uint32_t thisLen    = 0;
	uint32_t allocLen   = 0;
	int needSlots       = 0;
	int allocSlots      = 0;
	int broadcastLen    = 0;
	bool isDone         = false;
	vector<int> pduInfo;

	rsObject *pRS       = NULL;
	list<rsObject *>::iterator          itr;

	
	/* Compute total bytes of broadcast connections , including MG_DLMAP, MG_ULMAP, MG_DCD, MG_UCD, MG_CLKCMP, MG_MOB_TR
 	 * -FIND, MG_MOB_NBRADV, MG_MOB_PAGADV 
 	 */
	pConn = Station->broadcastConnection;
	pduInfo.clear();
	broadcastLen = pConn->GetInformation(&pduInfo); // ignore pduInfo here

	if (broadcastLen > 0)
	{
		burst = GetCorrespondBurst(burstCollection, RobustDIUC);	//RobustDIUC=0 , RobustUIUC=1
		burst->length += broadcastLen;
		burst->duration = computeNearestSlots(burst->toSlots(burst->length));
		allocSlots += burst->duration;
	}

	/* Examine Initial Ranging connection and use QPSK 1/2 */
	pConn = Station->initRangingConnection;
	pduInfo.clear();
	allocLen = pConn->GetInformation(&pduInfo);

	if (allocLen > 0)
	{
		burst = GetCorrespondBurst(burstCollection, RobustDIUC);
		burst->length += allocLen;
		burst->duration = computeNearestSlots(burst->toSlots(burst->length));
		burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));
		allocSlots += burst->duration;
		
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

		/* Find the burst using the same FEC profile from burstCollection */
		if (Station->mslist[i]->accessStation != Station)
		{
			pRS = (rsObject*) Station->mslist[i]->accessStation;
			burst = GetCorrespondBurst(burstCollection, pRS->diuc);
		}
		else
			burst = GetCorrespondBurst(burstCollection, pMS->diuc);

		//printf("MS(%d) DIUC = %d\n", pMS->myMS->get_nid(), pMS->diuc);

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
				burst->length += allocLen;
				burst->duration = computeNearestSlots(burst->toSlots(burst->length));
				burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));
				allocSlots += burst->duration;
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
				burst->ConnectionCollection.push_back(new AllocInfo(pConn, 0, pduInfo)); 
			}
		}
	}

	for (int i = 0; i < MAXRS; i++)
	{
		if (Station->rslist[i] == NULL)
			continue;

		if (Station->rslist[i]->ScanFlag == true)
			continue;

		if (Station->rslist[i]->ResourceStatus != RS_Serving)
			continue;

		pRS = Station->rslist[i];

		/* Find the burst using the same FEC profile from burstCollection */
		burst = GetCorrespondBurst(burstCollection, pRS->diuc);

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
				burst->length += allocLen;
				burst->duration = computeNearestSlots(burst->toSlots(burst->length));
				burst->ConnectionCollection.push_back(new AllocInfo(pConn, allocLen, pduInfo));
				allocSlots += burst->duration;
			}
		}

	}

	/* We will reverse the connection order to avoid connection starvation */
	if (rand() % 2) {
                for (itd = burstCollection->begin();itd != burstCollection->end(); itd++) {
                        burst = *itd;
                        burst->ConnectionCollection.reverse();
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
						burst->length += aInfo->getNext();	// in bytes
						burst->addCID(pConn->getCID());
						aInfo->len += aInfo->getNext();
						aInfo->position++;
					}
				}
			}
			/* mapping burst length to slots */
			burst->duration = computeNearestSlots(burst->toSlots(burst->length));
			allocSlots += burst->duration;
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

	return allocSlots;
}

/*
 * Append contention allocations to burstCollection at Ranging Subchannel
 */
int BS_16j_Scheduler::ContentionScheduling(vector<upBurst *> *burstCollection, int ULAccessoccupiedSymbols)
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
	rangOpportunity = ULAccessoccupiedSymbols / 3;

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
	rangOpportunity = ULAccessoccupiedSymbols / 3;

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
int BS_16j_Scheduler::CDMAallocScheduling(vector<upBurst *> *burstCollection, int availableSlots)
{
	int fec         = 0;
	int allocSlots  = 0;
	upBurst *uburst = NULL;
	list<RangingObject *>::iterator its1;

	/* Polled List of Ranging Code RSs & MSs */
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
			//printf("Time:%llu:BS_16j_Scheduler(%d)::%s()__CDMA_allocation\n", GetCurrentTime(), Station->get_nid(), __func__);

			/*****************************
			 * --- CDMA allocation IE ---
			 *****************************/
			fec = Station->UCDProfile[CDMA_Alloc_IE].fec;

			uburst = new upBurst(broadcastCID, CDMA_Alloc_IE, fec);
			uburst->ulmap_ie.ie_14.cid          = broadcastCID;
			uburst->ulmap_ie.ie_14.uiuc         = CDMA_Alloc_IE;
			uburst->ulmap_ie.ie_14.duration     = 5;    // FIXME: slot enough?? 
			uburst->ulmap_ie.ie_14.uiuc_trans   = RobustUIUC;	//1
			uburst->ulmap_ie.ie_14.repeCode     = 0;
			uburst->ulmap_ie.ie_14.frameIndex   = (*its1)->rangingFrameNumber & 0x0F;
			uburst->ulmap_ie.ie_14.rangCode     = (*its1)->rangingCodeIndex & 0xFF;
			uburst->ulmap_ie.ie_14.rangSym      = (*its1)->rangingSymbol & 0xFF;
			uburst->ulmap_ie.ie_14.rangCh       = (*its1)->rangingSubchannel & 0x7F;
			uburst->ulmap_ie.ie_14.bwReq        = 0;
			uburst->ulmap_ie.ie_14.relayRScid   = (*its1)->RS_cid;		//record relay RS cid
			uburst->encapsulateAllField();		//encapsulate ulmap_ie to OFDMA_ULMAP_IE

			burstCollection->push_back(uburst);
			allocSlots += 5;
		}
	}

	return allocSlots;
}

/*
 * Append access zone uplink UGS allocations to burstCollection.
 */
int BS_16j_Scheduler::UGSScheduling_MSs(vector<upBurst *> *burstCollection, int availableSlots)
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
	int tmpSlots            = 0;
	double rate             = 1;

	/* Here, we recommend a simple algorithm for the purpose of dynamic average uplink resoure allocation
 	 * to avoid total demanding resoure of MSs over uplink access zone capacity. 
	 */
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
			
			/* Iff MS is relayed , we should calculate resoure allocation for this MS according to
 			 * modulation scheme between MS->RS , otherwire MS->BS should be used.
 			 */
			if (Station->mslist[i]->accessStation != Station)
				uiuc    = Station->mslist[i]->ruiuc;
			else
				uiuc    = Station->mslist[i]->uiuc;
			fec     = Station->UCDProfile[uiuc].fec;
			mac     = Station->mslist[i]->address();
			sflow   = Station->GetProvisionedFlow(mac->buf());

			// No this MS Qos configure, need check $NAME.sim/$NAME.mobilewimax_cfg
			if (sflow == NULL)
			{
				fprintf(stderr, "\e[1;31mBS[%d] Can't find QoS configure for %s, check XXX.sim/XXX.mobilerelaywimax_cfg\e[0m\n", Station->get_nid(), mac->str());
				exit(1);
			}

			sclass = Station->GetServiceClass(sflow->GetQosIndex());
			bitsPerFrame = (int) ((sclass->MaxSustainedRate * rate) / 1000.0 * frameDuration[Station->FrameDuCode()]);
			slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);

			tmpSlots += slots;

		}
		if (tmpSlots <= availableSlots)
			break;
		else
			rate -= 0.01;

	}while(tmpSlots > availableSlots);

	/* If there is not enough slots for MS UGS traffics , we will schedule UL access zone traffics by FIFO */
	if (rate == 0)
		rate = 1;

	/*if (rate != 1)
		fprintf(stderr, "\e[1;31m%s:BS[%d] Warning...UL slots can not satisfy total sustained currently . average sustained rate = %f\e[0m\n", __func__, Station->get_nid(), rate);*/

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
		cid     = Station->mslist[i]->getBasicCID();	//grant for MS UL traffics
		/* Here , we use the UIUC from MS to RS to calculate access link capacity
		 * iff MS is relayed via RS , otherwise original UIUC will be used
		 */		
		if (Station->mslist[i]->accessStation != Station)
			uiuc    = Station->mslist[i]->ruiuc;
		else
			uiuc    = Station->mslist[i]->uiuc;
		fec     = Station->UCDProfile[uiuc].fec;
		mac     = Station->mslist[i]->address();
		sflow   = Station->GetProvisionedFlow(mac->buf());

		// No this MS Qos configure, need check $NAME.sim/$NAME.mobilewimax_cfg
		if (sflow == NULL)
		{
			fprintf(stderr, "\e[1;31mBS[%d] Can't find QoS configure for %s, check XXX.sim/XXX.mobilerelaywimax_cfg\e[0m\n", Station->get_nid(), mac->str());
			exit(1);
		}

		// MS->MaxSustainedRate may be too high
		sclass = Station->GetServiceClass(sflow->GetQosIndex());
		//MaxSustainedRate = 1000*1024	,frameuration = 5 ms
		bitsPerFrame = (int) ((sclass->MaxSustainedRate * rate) / 1000.0 * frameDuration[Station->FrameDuCode()]);
		slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);

		if (slots <= 0)
		{
			fprintf(stderr, "\e[1;31mWe cannot give this MS any allocation!\e[0m\n");
			exit(1);
		}

		uburst = new upBurst(cid, uiuc, fec);
		uburst->ulmap_ie.ie_other.cid = cid;
		uburst->ulmap_ie.ie_other.uiuc = uiuc;
		uburst->ulmap_ie.ie_other.duration = slots;	// MS PacketScheduling capacity
		uburst->ulmap_ie.ie_other.repeCode = 0;
		uburst->encapsulateAllField();
		
		allocSlots += slots;

		if (allocSlots > availableSlots)
                {
                        fprintf(stderr, "\e[1;31m(%d)%s:Too many UGS connections! Maybe we need some sort of admission control.allocSlots = %d , availableSlots = %d\e[0m\n",__LINE__, __func__, allocSlots, availableSlots);
                        allocSlots -= slots;
			delete uburst;
                        break;

                }
		burstCollection->push_back(uburst);
	}
	
	return allocSlots;
}

/*
 * Append relay zone uplink UGS allocations to burstCollection.
 */
int BS_16j_Scheduler::UGSScheduling_RSs(vector<upBurst *> *burstCollection, int availableSlots)
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
	int tmpSlots            = 0;
	double rate             = 1;

	/* Here, we recommend a simple algorithm for the purpose of dynamic average uplink resoure allocation
 	 * to avoid total demanding resoure of RSs over uplink relay zone capacity.
 	 */
	do{
		tmpSlots = 0;
		for (int i = 0; i < MAXRS; i++)
		{
			if (Station->rslist[i] == NULL)
				continue;

			if (Station->rslist[i]->ScanFlag == true)
				continue;

			if (Station->rslist[i]->ResourceStatus != RS_Serving)
				continue;

			// Get RS's information
			uiuc    = Station->rslist[i]->uiuc;
			fec     = Station->UCDProfile[uiuc].fec;
			bitsPerFrame = (int) (100*1024 / 1000.0 * frameDuration[Station->FrameDuCode()]);
			slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);

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

				if (Station->mslist[j]->accessStation != Station && (rsObject*) Station->mslist[j]->accessStation == Station->rslist[i])
				{
					// Get MS's information
					mac     = Station->mslist[j]->address();
					sflow   = Station->GetProvisionedFlow(mac->buf());

					// No this MS Qos configure, need check $NAME.sim/$NAME.mobilerelaywimax_cfg
					if (sflow == NULL)
					{
						fprintf(stderr, "\e[1;31mBS[%d] Can't find QoS configure for %s, check XXX.sim/XXX.mobilerelaywimax_cfg\e[0m\n", Station->get_nid(), mac->str());
						exit(1);
					}

					// MS->MaxSustainedRate may be too high
					sclass = Station->GetServiceClass(sflow->GetQosIndex());
					bitsPerFrame = (int) ((sclass->MaxSustainedRate * rate) / 1000.0 * frameDuration[Station->FrameDuCode()]);
					/* Here , we use RS MCS to calculate MS relay link capacity */
					slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);	
					tmpSlots += slots;

				}
			}
		}
		if (tmpSlots <= availableSlots)
			break;
		else
			rate -= 0.01;

	}while(tmpSlots > availableSlots);

	/* If there is not enough slots for MS UGS traffics , we will schedule UL access zone traffics by FIFO */
	if (rate == 0)
		rate = 1;

	/*if (rate != 1)
		fprintf(stderr, "\e[1;31m%s:BS[%d] Warning...UL slots can not satisfy total sustained currently . average sustained rate = %f\e[0m\n", __func__, Station->get_nid(), rate);*/

	for (int i = 0; i < MAXRS; i++)	//FIXME
	{
		if (Station->rslist[i] == NULL)
			continue;

		if (Station->rslist[i]->ScanFlag == true)
			continue;

		if (Station->rslist[i]->ResourceStatus != RS_Serving)
			continue;

		// Get RS's information
		cid     = Station->rslist[i]->getBasicCID();
		uiuc    = Station->rslist[i]->uiuc;
		fec     = Station->UCDProfile[uiuc].fec;
		/* FIXME , we currently configure RS uplink capacity enough for network entry, it should
		 * be request by sending BW rwquest ranging to MR-BS for uplink capacity.
		 */

		bitsPerFrame = (int) (100*1024 / 1000.0 * frameDuration[Station->FrameDuCode()]);	//FIXME
		slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);

		if (slots <= 0)
		{
			fprintf(stderr, "\e[1;31mWe cannot give this RS any allocation!\e[0m\n");
			exit(1);
		}

		uburst = new upBurst(cid, uiuc, fec);
		uburst->ulmap_ie.ie_other.cid = cid;
		uburst->ulmap_ie.ie_other.uiuc = uiuc;
		uburst->ulmap_ie.ie_other.duration = slots;
                uburst->ulmap_ie.ie_other.repeCode = 0;
		allocSlots += slots;
		
		/* Compute relay capacity */
		for (int j = 0; j < MAXMS; j++)	
		{
			if (Station->mslist[j] == NULL)
				continue;

			if (Station->mslist[j]->ScanFlag == true)
				continue;

			if (Station->mslist[j]->ResourceStatus != Serving)
				continue;

			if (Station->mslist[j]->accessStation != Station && (rsObject*) Station->mslist[j]->accessStation == Station->rslist[i])
			{
				// Get MS's information
				mac     = Station->mslist[j]->address();
				sflow   = Station->GetProvisionedFlow(mac->buf());

				// No this MS Qos configure, need check $NAME.sim/$NAME.mobilerelaywimax_cfg
				if (sflow == NULL)
				{
					fprintf(stderr, "\e[1;31mBS[%d] Can't find QoS configure for %s, check XXX.sim/XXX.mobilerelaywimax_cfg\e[0m\n", Station->get_nid(), mac->str());
					exit(1);
				}

				// MS->MaxSustainedRate may be too high
				sclass = Station->GetServiceClass(sflow->GetQosIndex());
				bitsPerFrame = (int) ((sclass->MaxSustainedRate * rate) / 1000.0 * frameDuration[Station->FrameDuCode()]);
				/* Here , we use RS MCS to calculate MS relay link capacity */
				slots = WiMaxBurst::bytesToSlots(bitsPerFrame / 8, fec);	

				if (slots <= 0)
				{
					fprintf(stderr, "\e[1;31mWe cannot give this MS any allocation!\e[0m\n");
					exit(1);
				}

				uburst->ulmap_ie.ie_other.duration += slots;
				allocSlots += slots;

			}
		}
		if (allocSlots > availableSlots)
		{
			fprintf(stderr, "\e[1;31m%s:Too many UGS connections! Maybe we need some sort of admission control.allocSlots = %d , availableSlots = %d\e[0m\n", __func__, allocSlots, availableSlots);
			allocSlots -= uburst->ulmap_ie.ie_other.duration;
			delete uburst;
			break;
		}

		uburst->encapsulateAllField();
		burstCollection->push_back(uburst);

	}

	return allocSlots;
}

/*
 * Append access zone uplink BE allocations to burstCollection.
 */
int BS_16j_Scheduler::BEScheduling_MSs(vector < upBurst * >*burstCollection, int availableSymbols)
{
	return 0;
}

/*
 * Append relay zone uplink BE allocations to burstCollection.
 */

int BS_16j_Scheduler::BEScheduling_RSs(vector < upBurst * >*burstCollection, int availableSymbols)
{
        return 0;
}


/*
 * Generate a MAC PDU containing UCD.
 */
void BS_16j_Scheduler::generate_UCD()
{
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

		delete tmptlv;
	}

	Station->broadcastConnection->Insert(ifmm);
}

/*
 * Generate a MAC PDU containing DCD.
 */
void BS_16j_Scheduler::generate_DCD()
{
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
	ifmm->appendTLV(63, 1, Station->EndtoEndMetric);

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
	Station->broadcastConnection->Insert(ifmm);
}

/*
 * Generate a MAC PDU containing MOB_NBR-ADV.
 */
void BS_16j_Scheduler::generate_MOB_NBRADV()
{
	ifmgmt *ifmm          = NULL;
	int tmpBits           = 0;
	uint8_t N_neighbors   = 2;
	uint8_t skipOptField  = 0xFC; // 1111 1100
	uint8_t fragmentIndex = 0;
	uint8_t totalFragment = 1;
	uint8_t *loopLen      = NULL;

	/*loopLen = new uint8_t [N_neighbors];
	memset(loopLen, 0, N_neighbors);*/

	/*
	 * Compute all field bits
	 */
	tmpBits += 8;
	if ((skipOptField & 0x01) == 0)
		tmpBits += 24;

	tmpBits += 24;

	N_neighbors = Station->NeighborBSList->nbrBSs_Index.size();

	loopLen = new uint8_t [N_neighbors];
        memset(loopLen, 0, N_neighbors);

	for (int i = 0;i < N_neighbors;i++)
	{
		int len = 0;
		uint8_t phyProfileID = Station->NeighborBSList->nbrBSs_Index[i]->PHY_ProfileID;

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

	if ((skipOptField & 0x01) == 0)  // skipOptField[0]
	{
		ifmm->appendBitField(24, Station->address()->buf() + 3);  // Operator ID
	}
	ifmm->appendBitField( 8, Station->NeighborBSList->NBRADV_CfgCount);
	ifmm->appendBitField( 4, fragmentIndex);
	ifmm->appendBitField( 4, totalFragment);
	ifmm->appendBitField( 8, N_neighbors);

	for (int i = 0;i < N_neighbors;i++)
	{
		uint8_t FA_Index     = 0;
		uint8_t BS_EIRP      = 0;
		uint8_t phyProfileID = Station->NeighborBSList->nbrBSs_Index[i]->PHY_ProfileID;

		ifmm->appendBitField( 8, loopLen[i]);   // Length of this iteration in bytes
		ifmm->appendBitField( 8, phyProfileID); // Spec 16e. Table 109g

		if (((phyProfileID & 0x40) >> 6) == 1) // FA Index Indicator
		{
			ifmm->appendBitField( 8, FA_Index);
		}

		if (((phyProfileID & 0x10) >> 4) == 1) // BS EIRP Indicator
		{
			ifmm->appendBitField( 8, BS_EIRP);
		}

		if ((skipOptField & 0x02) == 0) // skipOptField[1]
		{
			ifmm->appendBitField(24, Station->NeighborBSList->nbrBSs_Index[i]->addr->buf() + 3); // Neighbor BSID
		}

		ifmm->appendBitField( 8, Station->NeighborBSList->nbrBSs_Index[i]->PreambleIndex);  // Preamble Index

		if ((skipOptField & 0x04) == 0) // skipOptField[2]
		{
			ifmm->appendBitField( 8, Station->NeighborBSList->nbrBSs_Index[i]->HO_Optimize); // HO Process Optimization
		}

		if ((skipOptField & 0x08) == 0) // skipOptField[3]
		{
			ifmm->appendBitField( 8, Station->NeighborBSList->nbrBSs_Index[i]->SchServSupport); // Scheduling Service Supported
		}

		ifmm->appendBitField( 4, Station->NeighborBSList->nbrBSs_Index[i]->DCDCfgCount & 0x0F);  // DCD Configuration Change Count
		ifmm->appendBitField( 4, Station->NeighborBSList->nbrBSs_Index[i]->UCDCfgCount & 0x0F);  // UCD Configuration Change Count

		/*
		 * TLV Encoded Neighbor Information
		 */
	}

	delete [] loopLen;

	Station->broadcastConnection->Insert(ifmm);
}

/*
 * Generate a MAC PDU containing DL-MAP.
 */
void BS_16j_Scheduler::generate_DL_MAP(vector<downBurst *> *burstCollection_a, vector<downBurst *> *burstCollection_t, int DLsymbols)
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

	/* 16j added */
	OFDMA_DLMAP_IE  *ie = NULL;
	DLMAP_IE_u      stc_dlmap_ie;
	struct STC_DL_Zone_Switch_IE stc_ie;

	// Compute total length of DL-MAP
	for (itd = burstCollection_a->begin();itd != burstCollection_a->end(); itd++)
	{
		nBits += (*itd)->ie->getBits();
	}
	//printf("%s>>burstCollection_t->size = %d<<\n", __func__, (int)burstCollection_t->size());
	/* encoding transparent burst IE if existed */
	if (burstCollection_t->size() != 0)
	{
		itd = burstCollection_t->begin();
		memset(&stc_dlmap_ie, 0x00, sizeof(DLMAP_IE_u));
		memset(&stc_ie, 0 , sizeof(struct STC_DL_Zone_Switch_IE));
		stc_dlmap_ie.ie_14.diuc = Extended_DIUC_2;
		stc_dlmap_ie.ie_14.ext2_diuc = 0x01;
		stc_dlmap_ie.ie_14.length = sizeof(struct STC_DL_Zone_Switch_IE);
		stc_dlmap_ie.ie_14.data =(void*)&stc_ie;

		stc_ie.symOff = (*itd)->dlmap_ie.ie_other.symOff;
		stc_ie.permut = 0;	//PUSC permutation
		stc_ie.use_subchan = 1;	//use all subchannels
		stc_ie.stc = 0;		//no STC
		stc_ie.matric_ind = 0;
		stc_ie.dl_permb = 0;
		stc_ie.amc_type = 0;
		stc_ie.midamb_pres = 0;
		stc_ie.midamb_boost = 0;
		stc_ie.antenna_selc = 0;
		stc_ie.dedi_pilot = 0;
		stc_ie.trasparent_txpwr = 0x0F;
		
		ie = new OFDMA_DLMAP_IE(Extended_DIUC_2);	//diuc = 14
		ie->mallocIE(4);	// 4 bytes, 16 + 32 = 48 bits
		ie->appendBitField(4, stc_dlmap_ie.ie_14.diuc);
		ie->appendBitField(4, stc_dlmap_ie.ie_14.ext2_diuc);
		ie->appendBitField(8, stc_dlmap_ie.ie_14.length);
		ie->appendBitField(8, stc_ie.symOff);
		ie->appendBitField(2, stc_ie.permut);
		ie->appendBitField(1, stc_ie.use_subchan);
		ie->appendBitField(2, stc_ie.stc);
		ie->appendBitField(2, stc_ie.matric_ind);
		ie->appendBitField(5, stc_ie.dl_permb);
		ie->appendBitField(2, stc_ie.amc_type);
		ie->appendBitField(1, stc_ie.midamb_pres);
		ie->appendBitField(1, stc_ie.midamb_boost);
		ie->appendBitField(1, stc_ie.antenna_selc);
		ie->appendBitField(1, stc_ie.dedi_pilot);
		ie->appendBitField(4, stc_ie.trasparent_txpwr);

		nBits += ie->getBits();

		for (itd = burstCollection_t->begin();itd != burstCollection_t->end(); itd++)
		{
			nBits += (*itd)->ie->getBits();
		}
			
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
	ifmm->appendBitField( 8, Station->FrameDuCode());       // Frame Duration Code
	ifmm->appendBitField(24, Station->frameNumber);         // Frame Number
	ifmm->appendBitField( 8, Station->DCDCfgCount);         // Configuration change count
	ifmm->appendBitField(48, Station->address()->buf());    // Base Station ID
	ifmm->appendBitField( 8, DLsymbols);                    // OFDMA DL symbols
	Station->saveDLsymbols(DLsymbols);

	/*
	 * DL-MAP IE (Spec 16e. 8.4.5.3)
	 */
	for (itd = burstCollection_a->begin();itd != burstCollection_a->end(); itd++)
	{
		dlmap_ie = (*itd)->ie;
		ifmm->appendBitField(dlmap_ie->getBits(), dlmap_ie->getData());
	}
	
	if (burstCollection_t->size() != 0)
        {
		ifmm->appendBitField(ie->getBits(), ie->getData());

		for (itd = burstCollection_t->begin();itd != burstCollection_t->end(); itd++)
		{
			dlmap_ie = (*itd)->ie;
			ifmm->appendBitField(dlmap_ie->getBits(), dlmap_ie->getData());
		}

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
	if (ie != NULL)
		delete ie;
}

/*
 * Generate a MAC PDU containing UL-MAP.
 */
void BS_16j_Scheduler::generate_UL_MAP(vector<upBurst *> *burstCollection, int ULallocStartTime, int ULsymbols)
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
	 * Generate UL-MAP Management Message (Spec 16j. Table 18)
	 */
	ifmm = new ifmgmt(MG_ULMAP, len);
	ifmm->appendBitField( 8, rsv);                  // Reserved
	ifmm->appendBitField( 8, Station->UCDCfgCount); // Configuration change count
	ifmm->appendBitField(32, ULallocStartTime);     // Uplink Alloation start time
	ifmm->appendBitField( 8, ULsymbols);            // OFDMA UL symbols
	Station->saveULsymbols(ULsymbols);

	/*
	 * UL-MAP IE (Spec 16j. 8.4.5.4)
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
 *  Search corresponding UL-MAP IE and return its related interval in that frame
 */
int BS_16j_Scheduler::SearchULTime(int ULallocTime,int symOffset)
{
	return ULallocTime + Station->symbolsToPSs(symOffset); // in PS
}

int BS_16j_Scheduler::computeNearestSlots(int realSlots)
{
	int usingCh = 10; // FIXME: Assume each burst will occupy 10 subchannels
	int factor = 0;

	while (usingCh * factor < realSlots)
	{
		factor++;
	}

	return usingCh * factor;
}

void BS_16j_Scheduler::mapComputeSymCh(int slots, uint8_t *nSym, uint8_t *nCh)
{
	int totalSymbols = slots * 2;

	if (totalSymbols == 0)
		return;

	if (totalSymbols % 20 != 0)
	{
		fprintf(stderr, "\e[1;31mBS Compute Slot Allocation Error\e[0m\n");
		exit(0);
	}

	*nSym = totalSymbols / 10;
	*nCh = 10;
}

/* ------------------------------------------------------------------------------------ */

vector<WiMaxBurst *> *MS_16j_Scheduler::ULaccess_Scheduling()
{
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

vector<WiMaxBurst *> *RS_16j_Scheduler::ULrelay_Scheduling()
{
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

	vector <ULMAP_IE_u>::iterator itu;

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

	// Examine relay management connection
	for (itm = Station->UL_relayManalist.begin();itm != Station->UL_relayManalist.end();itm++)
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

	// Examine relay data connections
	for (itd = Station->UL_relayDtlist.begin();itd != Station->UL_relayDtlist.end();itd++)
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
