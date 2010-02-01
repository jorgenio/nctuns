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
#include <math.h>
#include <algorithm>

#include "scheduler.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../../phy/ofdm_80216.h"

#define VERBOSE_LEVEL   MSG_WARNING
#include "../verbose.h"

using namespace std;

static bool compare(const void *a, const void *b)
{
	downBurst *oa = (downBurst *) a;
	downBurst *ob = (downBurst *) b;
	return (oa->ie.diuc) < (ob->ie.diuc);
}

downBurst*
BSScheduler::GetCorrespondBurst(vector<downBurst*>* c, int diuc)
{
	vector<downBurst*>::iterator it;
	downBurst*                   pBurst;

	for (it = c->begin(); it != c->end(); it++) {
		if ((*it)->ie.diuc == diuc)
			return *it;
	}
	pBurst = new downBurst(diuc, Station->DCDProfile[diuc].fec);
	c->push_back(pBurst);
	return pBurst;
}

/* ------------------------------------------------------------------------------------ */

vector < WiMaxBurst * >*BSScheduler::Scheduling()
{
	char *buf;
	u_int allocLen, requestLen, len;
	vector < int >PduInfo;
	downBurst *burst;
	Connection *pConn;

	vector < downBurst * >*downBurstCollection =
	    new vector < downBurst * >;
	vector < downBurst * >::iterator it1;
	list < AllocInfo * >::iterator ita;

	int symbolsPerFrame =
	    Station->symbolsPerFrame() - LONG_PREAMBLE - DLFP_LEN;
	int ULAvailableSymbols = (int) (symbolsPerFrame * MAX_UL_RATIO);
	int DLAvailableSymbols = symbolsPerFrame - ULAvailableSymbols;
	int resvSymbols = 30;
	int allocSymbols;

	DEBUG("\tDLAvailableSymbols=%d, ULAvailableSymbols=%d\n",
	       DLAvailableSymbols, ULAvailableSymbols);

	while (!upBurstCollection->empty()) {
		delete *(upBurstCollection->begin());
		upBurstCollection->erase(upBurstCollection->begin());
	}

	// Scheduling contetion period
	allocSymbols =
	    ContentionScheduling(upBurstCollection, ULAvailableSymbols);
	ULAvailableSymbols -= allocSymbols;

	// Scheduling UL UGS traffic
	allocSymbols =
	    UGSScheduling(upBurstCollection, ULAvailableSymbols);
	ULAvailableSymbols -= allocSymbols;

	// Scheduling DL traffic
	DLAvailableSymbols += ULAvailableSymbols;
	DLAvailableSymbols -= resvSymbols;
	DEBUG("\tDLAvailableSymbols=%d, ULAvailableSymbols=%d, resvSymbols=%d\n", DLAvailableSymbols, ULAvailableSymbols, resvSymbols);
	allocSymbols =
	    DLScheduling(downBurstCollection, DLAvailableSymbols);
//      printf("DLAvail=%d, DLAlloc=%d\n", DLAvailableSymbols, allocSymbols);

	// Scheduling BE traffic
	//ULAvailableSymbols = DLAvailableSymbols - allocSymbols;
	//DLAvailableSymbols = 0;
	//BEScheduling(upBurstCollection, DLAvailableSymbols+ULAvailableSymbols);

	allocStartTime =
	    Station->symbolsToPSs(LONG_PREAMBLE + 1 + allocSymbols +
				  resvSymbols);
	DEBUG("\tallocStartTime=%dPSs (%d symbols)\n", allocStartTime,
	       LONG_PREAMBLE + 1 + allocSymbols + resvSymbols);

	// Generate UL-MAP
	generate_UL_MAP(upBurstCollection, allocStartTime);

	// Generate DL-MAP
	sort(downBurstCollection->begin(), downBurstCollection->end(),
	     compare);

	generate_DL_MAP(downBurstCollection);


	// Examine Broadcast connection and use robust profile
	pConn = Station->globalConnection[0];
	PduInfo.clear();
	allocLen = pConn->GetInformation(&PduInfo);

	if (allocLen > 0) {
		if (downBurstCollection->size() > 0)
			burst = (*downBurstCollection)[0];
		else {
			burst = GetCorrespondBurst(downBurstCollection, RobustDIUC);
		}

		INFO("\t=> Broadcast Connection: %d bytes burst=%p\n",
			 allocLen, burst);
		burst->ConnectionCollection.
		    push_front(new AllocInfo(pConn, allocLen, PduInfo));
		burst->length += allocLen;
		burst->ie.cid = broadcastCID;
	}

	len = LONG_PREAMBLE + DLFP_LEN;
	for (it1 = downBurstCollection->begin();
	     it1 != downBurstCollection->end(); it1++) {
		burst = *it1;
		burst->ie.stime = len;
		burst->duration = burst->toSymbol(burst->length);
		len += burst->duration;
	}

	for (it1 = downBurstCollection->begin();
	     it1 != downBurstCollection->end(); it1++) {
		burst = *it1;
//              burst->Dump();
		if (burst->duration == 0) {
			downBurstCollection->erase(it1, it1 + 1);
			delete burst;
			it1--;
		}
	}
	// Generate DL-MAP
	generate_DL_MAP(downBurstCollection);

	for (it1 = downBurstCollection->begin();
	     it1 != downBurstCollection->end(); it1++) {
		burst = *it1;
		buf = new char[burst->length];
		burst->payload = buf;

		for (ita = burst->ConnectionCollection.begin();
		     ita != burst->ConnectionCollection.end(); ita++) {
			pConn = (Connection *) (*ita)->conn;
			requestLen = (*ita)->len;
			len = pConn->EncapsulateAll(buf, requestLen);
			buf += len;
		}
//              burst->length = buf-burst->payload;
//              burst->Dump();
	}

	return (vector < WiMaxBurst * >*)downBurstCollection;
}

//
// Append downlink allocations to BurstCollection.
//
int BSScheduler::DLScheduling(vector < downBurst * >*BurstCollection,
			      int availableSymbols)
{
	vector < downBurst * >::iterator it1, it2;
	list < AllocInfo * >::iterator ita;
	list < ssObject * >::iterator its;

	Connection *pConn;
	vector < int >PduInfo;
	u_int i, j, thisLen, allocLen;
	downBurst *burst;
	AllocInfo *aInfo;
	bool isDone;
	int symbols, allocSymbols;
	ssObject *pSS;

	DEBUG("\tavailableSymbols=%d\n", availableSymbols);

	// Examine Initial Ranging connection and use BPSK 1/2
	pConn = Station->initRangingConnection;

	PduInfo.clear();
	allocLen = pConn->GetInformation(&PduInfo);

	if (allocLen > 0) {
		burst = GetCorrespondBurst(BurstCollection, RobustDIUC);
		burst->ie.cid = initRangingCID;
		INFO("\t=> Initial Ranging Connection: %d bytes\n", allocLen);
		burst->ConnectionCollection.
		    push_back(new AllocInfo(pConn, allocLen, PduInfo));
	}

	for (i = 0; i < MAXSS; i++) {
		if (Station->sslist[i]) {
			pSS = Station->sslist[i];
			// Find the burst using the same FEC profile from BurstCollection
			burst =
			    GetCorrespondBurst(BurstCollection, pSS->diuc);
			for (j = 0; j < 2; j++) {
				if (!pSS->MnConnections[j])
					continue;

				pConn = pSS->MnConnections[j];

				PduInfo.clear();
				allocLen = pConn->GetInformation(&PduInfo);

				if (allocLen > 0) {
					INFO("\t=> sslist[%d]->conlist[%d]: %d bytes\n",
						 i, j, allocLen);
					burst->ConnectionCollection.
					    push_back(new
						      AllocInfo(pConn,
								allocLen,
								PduInfo));
				}
			}

			list < DataConnection * >::iterator itdc =
			    pSS->DataConnections.begin();
			while (itdc != pSS->DataConnections.end()) {
				pConn = *itdc;
				itdc++;
				PduInfo.clear();
				thisLen = pConn->GetInformation(&PduInfo);

				if (thisLen > 0) {
					INFO("Examine data connections: conlist[%d]: %d bytes\n",
						 i, thisLen);

					burst->ConnectionCollection.
					    push_back(new
						      AllocInfo(pConn, 0,
								PduInfo));
				}
			}
		}
	}
	if (rand() % 2) {
		for (it1 = BurstCollection->begin();
		     it1 != BurstCollection->end(); it1++) {
			burst = *it1;
			burst->ConnectionCollection.reverse();
		}
	}
	// Selection
	allocSymbols = 0;
	isDone = false;

	while (!isDone) {
		isDone = true;

		for (it1 = BurstCollection->begin();
		     it1 != BurstCollection->end(); it1++) {
			burst = *it1;
			allocSymbols -= burst->duration;

			for (ita = burst->ConnectionCollection.begin();
			     ita != burst->ConnectionCollection.end();
			     ita++) {
				aInfo = *ita;
				pConn = (Connection *) aInfo->conn;

				if (aInfo->position >=
				    aInfo->pduInfo.size())
					continue;

//                              burst->ie.cid = broadcastCID;
				if (burst->ie.cid != broadcastCID
				    && burst->ie.cid != pConn->getCID())
					burst->ie.cid = pConn->getCID();

				symbols =
				    burst->toSymbol(burst->length +
						    aInfo->getNext());
				if (allocSymbols + symbols >
				    availableSymbols) {
					isDone = true;
					aInfo->len +=
					    burst->
					    toByte(availableSymbols -
						   allocSymbols) -
					    burst->length;
					burst->length =
					    burst->
					    toByte(availableSymbols -
						   allocSymbols);
					break;
				}

				DEBUG("\t sec len=%d, diuc=%d, symbols=%d, allocSymbols=%d\n", aInfo->getNext(), burst->ie.diuc, symbols, allocSymbols);

				burst->length += aInfo->getNext();
				aInfo->len += aInfo->getNext();
				aInfo->position++;

				isDone = false;
			}
			burst->duration = burst->toSymbol(burst->length);
			allocSymbols += burst->duration;
		}
		if (isDone) {
			break;
		}
	}
#if 0
	for(it1=BurstCollection->begin(); it1!=BurstCollection->end(); it1++)
	{
		burst = *it1;

		for(ita=burst->ConnectionCollection.begin(); ita!=burst->ConnectionCollection.end(); ita++)
		{
			aInfo = *ita;
			pConn = (Connection*) aInfo->conn;
		}
	}
#endif
	DEBUG("\tallocSymbols=%d\n", allocSymbols);
	return allocSymbols;
}

//
// Append contention allocations to BurstCollection.
//
int BSScheduler::ContentionScheduling(vector < upBurst * >*BurstCollection,
				      int availableSymbols)
{
	upBurst *uburst;
	list < ssObject * >::iterator its;
	int cid;
	int stime = 0;
	int allocSymbols = 0;
	int i, fec;

	DEBUG("\tavailableSymbols=%d\n", availableSymbols);

	// Compute the start time for the next allocation
	if (BurstCollection->empty()) {
		stime = 0;
	} else {
		uburst = *(BurstCollection->end());
		stime = uburst->ie.stime + uburst->ie.duration;
	}

	DEBUG("\tstart time=%d\n", stime);

	// Initial Ranging IE
	fec = Station->UCDProfile[Initial_Ranging].fec;
	for (its = Station->PolledList.begin();
	     its != Station->PolledList.end(); its++) {
		cid = (*its)->getBasicCID();
		BurstCollection->
		    push_back(new
			      upBurst(cid, Initial_Ranging, stime, 5,
				      fec));
		stime += 5;
		allocSymbols += 5;
		DEBUG("\tInvited initial ranging for CID %d => 5 symbols\n", cid);
	}
	for (i = 0; i < 1; i++) {
		BurstCollection->
		    push_back(new
			      upBurst(broadcastCID, Initial_Ranging, stime,
				      5, fec));
		stime += 5;
		allocSymbols += 5;
		DEBUG("\tInitial ranging => 5 symbols\n");
	}

	// Request IE, each with one opportunity
	fec = Station->UCDProfile[REQ_Full].fec;
	for (i = 0; i < 1; i++) {
		BurstCollection->
		    push_back(new
			      upBurst(broadcastCID, REQ_Full, stime, 2,
				      fec));
		stime += 2;
		allocSymbols += 2;
		DEBUG("\tBW request => 2 symbols\n");
	}

	DEBUG("\tallocSymbols=%d\n", allocSymbols);
	return allocSymbols;
}


//
// Append uplink UGS allocations to BurstCollection.
//
int BSScheduler::UGSScheduling(vector < upBurst * >*BurstCollection,
			       int availableSymbols)
{
	upBurst *uburst;
	ServiceFlow *Sflow;
	ServiceClass *Sclass;
	int cid;
	int stime = 0;
	int bitsPerFrame;
	int symbols, allocSymbols = 0;
	int i, uiuc, fec;
	DEBUG("\tavailableSymbols=%d\n", availableSymbols);

	// Compute the start time for the next allocation
	if (BurstCollection->empty()) {
		stime = 0;
	} else {
		uburst = BurstCollection->back();
		stime = uburst->ie.stime + uburst->ie.duration;
	}

	DEBUG("\tstart time=%d\n", stime);

	// Data Grant Burst Type IEs
	for (i = 0; i < MAXSS; i++) {
		if (!Station->sslist[i])
			continue;

		cid = Station->sslist[i]->getBasicCID();
		uiuc = Station->sslist[i]->uiuc;
		fec = Station->UCDProfile[uiuc].fec;

		//mac = Station->sslist[i]->getMacAddr();
		Mac_address* mac = Station->sslist[i]->address();
		Sflow = Station->GetProvisionedFlow(mac->buf());
		if (Sflow == NULL) {
			printf("Can't find QoS configure for %s\n", mac->str());
			continue;
		}
		Sclass = Station->GetServiceClass(Sflow->GetQosIndex());
		bitsPerFrame = (int) (Sclass->MaxSustainedRate / 1000.0 *
				      Station->frame_duration());

		// # of symbols allocated to this SS
		symbols = WiMaxBurst::bytesToSymbols(bitsPerFrame / 8, fec);

		DEBUG("\tSS %d fec=%d => MaxSustainedRate=%dbps, bitsPerFrame=%d, symbols=%d\n", i, fec, Sclass->MaxSustainedRate, bitsPerFrame, symbols);

		if (symbols <= 0) {
			printf("We cannot give this SS any allocation!\n");
			exit(1);
		}

		symbols += SHORT_PREAMBLE;
		allocSymbols += symbols;

		if (allocSymbols > availableSymbols) {
			printf("Too many UGS connections! Maybe we need some sort of admission control.\n");
			exit(1);
		}

		BurstCollection->
		    push_back(new upBurst(cid, uiuc, stime, symbols, fec));
		stime += symbols;
	}

	DEBUG("\tallocSymbols=%d\n", allocSymbols);
	return allocSymbols;
}

//
// Append uplink BE allocations to BurstCollection.
//
int BSScheduler::BEScheduling(vector < upBurst * >*BurstCollection,
			      int availableSymbols)
{
	return 0;
}

/*
* Generate a MAC PDU containing UCD.
*/
void BSScheduler::generate_UCD()
{
	ifmgmt *ifmm;
	ifTLV *tmptlv;
	u_char burstProfileBuffer[128], i;
	struct UCDBurstProfile *pf;

	ifmm = new ifmgmt(MG_UCD, 5);

	ifmm->appendField(1, Station->UCDCfgCount);	// Configuration change count
	ifmm->appendField(1, 2);	// Ranging backoff start
	ifmm->appendField(1, 10);	// Ranging backoff end
	ifmm->appendField(1, 0);	// Request backoff start
	ifmm->appendField(1, 10);	// Request backoff end

	/*
	 * TLV encoded information (Spec 11.3.1)
	 */

	/*
	 * Setting uplink burst profile (Spec 8.3.5.5 and
	 */
	for (i = 0; i < 16; i++) {
		if (!Station->UCDProfile[i].used)
			continue;
		pf = Station->UCDProfile + i;
		INFO(" UCD[%d] uiuc=%d fec=%d, boost=%d tcs=%d\n", i,
			 i, pf->fec, pf->powerBoost, pf->tcs);
		burstProfileBuffer[0] = i;	// UIUC
		tmptlv = new ifTLV(burstProfileBuffer + 1, 0);
		tmptlv->Add(150, 1, pf->fec);
		tmptlv->Add(151, 1, pf->powerBoost);
		tmptlv->Add(152, 1, pf->tcs);
		ifmm->appendTLV(1, tmptlv->getLen() + 1,
				burstProfileBuffer);
		delete tmptlv;
	}

	ifmm->appendTLV(4, 2, Station->symbolsToPSs(5));	// Ranging request opportunity size
	ifmm->appendTLV(3, 2, Station->symbolsToPSs(2));	// Bandwidth request opportunity size

	Station->broadcastConnection->Insert(ifmm);
}

/*
* Generate a MAC PDU containing DCD.
*/
void BSScheduler::generate_DCD()
{
	ifmgmt *ifmm;
	ifTLV *tmptlv;
	u_char burstProfileBuffer[128], i;
	struct DCDBurstProfile *pf;

	ifmm = new ifmgmt(MG_DCD, 2);

	ifmm->appendField(1, 0x0a);	// Downlink channel ID
	ifmm->appendField(1, Station->DCDCfgCount);	// Configuration change count

	/*
	 * TLV encoded information (Spec 11.3.1)
	 */
	ifmm->appendTLV(14, 1, 4);	// Frame duration code, 4:10ms Table 230
	ifmm->appendTLV(15, 3, Station->frameNumber);	// Frame number

	/*
	 * Setting downlink burst profile (Spec 8.3.5.5 and 11.4)
	 */
	for (i = 0; i < 16; i++) {
		if (!Station->DCDProfile[i].used)
			continue;
		pf = Station->DCDProfile + i;
		INFO(" DCD[%d] diuc=%d fec=%d, exit=%d entry=%d tcs=%d\n",
			 i, i, pf->fec, pf->exitThreshold, pf->entryThreshold, pf->tcs);
		burstProfileBuffer[0] = i;	// DIUC
		tmptlv = new ifTLV(burstProfileBuffer + 1, 0);
		tmptlv->Add(150, 1, pf->fec);
		tmptlv->Add(151, 1, pf->exitThreshold);
		tmptlv->Add(152, 1, pf->entryThreshold);
		tmptlv->Add(153, 1, pf->tcs);
		ifmm->appendTLV(1, tmptlv->getLen() + 1,
				burstProfileBuffer);
		delete tmptlv;
	}
	Station->broadcastConnection->Insert(ifmm);
}

/*
* Generate a MAC PDU containing DL-MAP.
*/
void BSScheduler::generate_DL_MAP(vector < downBurst * >*c)
{
	unsigned int i, len, nBurst, flag;
	ifmgmt *ifmm;
	struct OFDM_DLMAP_IE ie;

	nBurst = c->size();
	flag = Station->dlmapflag;	// Generate Complete DL-MAP
	if (nBurst <= 4 && flag == 0)	// DL-MAP is not necessary
		return;

	if (nBurst == 0)	//  No burst, only broadcast message
		len = 15;
	else {
		if (flag)
			len = 7 + (nBurst + 1) * sizeof(OFDM_DLMAP_IE);
		else
			len = 7 + (nBurst - 4 + 1) * sizeof(OFDM_DLMAP_IE);
	}
	ifmm = new ifmgmt(MG_DLMAP, len);
	ifmm->appendField(1, Station->DCDCfgCount);	// Configuration change count
	ifmm->appendField(6, Station->address()->buf());	// Base Station ID

	/*
	 * DL-MAP IE (Spec 8.3.6.2)
	 */
	if (nBurst == 0) {
		ie.cid = broadcastCID;
		ie.diuc = 1;	// robust profile
		ie.preamble = 0;
		ie.stime = DLFP_LEN + LONG_PREAMBLE;
		ifmm->appendField(sizeof(ie), &ie);
	}
	if (flag)
		i = 0;
	else
		i = 4;
	for (; i < c->size(); i++) {
		ie = (*c)[i]->ie;
		len = ie.stime + (*c)[i]->duration;
		ifmm->appendField(sizeof(ie), &ie);
	}

	// Pad an empty DL-MAP IE for finish
	ie.cid = broadcastCID;
	ie.diuc = EndOfMap;
	ie.preamble = 0;
	ie.stime = len;
	ifmm->appendField(sizeof(ie), &ie);

	Station->broadcastConnection->Insert(ifmm);
}

/*
* Generate a MAC PDU containing UL-MAP.
*/
void BSScheduler::generate_UL_MAP(vector < upBurst * >*c,
				  int allocStartTime)
{
	ifmgmt *ifmm;
	unsigned int i, len, nBurst;
	struct OFDM_ULMAP_IE ie;

	nBurst = c->size();
	len = 6 + (nBurst + 1) * sizeof(OFDM_ULMAP_IE);
	ifmm = new ifmgmt(MG_ULMAP, len);

	ifmm->appendField(1, 0);
	ifmm->appendField(1, Station->UCDCfgCount);	// Configuration change count
	ifmm->appendField(4, allocStartTime);	// Alloation start time

	/*
	 * UL-MAP IE (Spec 8.3.6.3)
	 */
	for (i = 0; i < c->size(); i++) {
		ie = (*c)[i]->ie;
		ie.midamble = 0;
		len = ie.stime + ie.duration;
		ifmm->appendField(sizeof(ie), &ie);
	}

	// Pad an empty UL-MAP IE for finish
	ie.cid = broadcastCID;
	ie.stime = len;
	ie.chidx = 0;
	ie.uiuc = 14;		// End of Map
	ie.duration = 0;
	ie.midamble = 0;
	ifmm->appendField(sizeof(ie), &ie);

	Station->broadcastConnection->Insert(ifmm);
}

/*
*  Search corresponding UP-MAP IE and return its related interval in that frame
*/
int BSScheduler::SearchULIE(struct OFDM_ULMAP_IE &ie)
{
	vector < upBurst * >::iterator itu;

	for (itu = upBurstCollection->begin();
	     itu != upBurstCollection->end(); itu++) {
		if (ie.stime == (*itu)->ie.stime) {
			return allocStartTime +
			    Station->symbolsToPSs(ie.stime);
		}
	}

	printf("Error: Cannot find corresponding IE!\n");
	return -1;
}

/* ------------------------------------------------------------------------------------ */

vector < WiMaxBurst * >*SSScheduler::Scheduling()
{
	char *buf;
	int BurstLen, maxSymbols, fec;
	unsigned int i, thisLen, allocLen, len;
	vector < upBurst * >*BurstCollection = NULL;
	vector < upBurst * >::iterator it1, it2;
	list < AllocInfo * >::iterator ita;
	vector < int >PduInfo;
	upBurst *burst = NULL;
	Connection *pConn;

	fec = Station->UCDProfile[Station->savedULie.uiuc].fec;
	burst = new upBurst(Station->savedULie, fec);
	allocLen = 0;

	// Examine other management connections
	for (i = 0; i < Station->MnConnections.size(); i++) {
		pConn = Station->MnConnections[i];

		PduInfo.clear();
		thisLen = pConn->GetInformation(&PduInfo);

		if (thisLen > 0) {
			INFO("\t=> sslist[%d]->conlist (%d): %d bytes\n",
				 i, pConn->getCID(), thisLen);
			burst->ConnectionCollection.
			    push_back(new
				      AllocInfo(pConn, thisLen, PduInfo));
			allocLen += thisLen;
		}
	}

	// Examine data connections
	for (i = 0; i < Station->ConnectionList.size(); i++) {
		pConn = Station->ConnectionList[i];

		PduInfo.clear();
		thisLen = pConn->GetInformation(&PduInfo);

		if (thisLen > 0) {
			INFO("\t=> conlist[%d]: %d bytes\n", i, thisLen);
			burst->ConnectionCollection.
			    push_back(new AllocInfo(pConn, 0, PduInfo));
			allocLen += thisLen;
		}
	}
	if (burst->ConnectionCollection.empty()) {
		delete burst;
		return NULL;
	}
	BurstCollection = new vector < upBurst * >;
	BurstCollection->push_back(burst);

	BurstLen = 0;
	burst->length = 0;
	maxSymbols = burst->ie.duration - SHORT_PREAMBLE;

	thisLen = burst->toByte(maxSymbols);

	if (thisLen < allocLen)
		allocLen = thisLen;
	burst->length = allocLen;
	buf = new char[burst->length];
	burst->payload = buf;

	for (ita = burst->ConnectionCollection.begin();
	     ita != burst->ConnectionCollection.end(); ita++) {
		pConn = (Connection *) (*ita)->conn;
		len = pConn->EncapsulateAll(buf, allocLen);
		buf += len;
		allocLen -= len;
	}

	return (vector < WiMaxBurst * >*)BurstCollection;
}
