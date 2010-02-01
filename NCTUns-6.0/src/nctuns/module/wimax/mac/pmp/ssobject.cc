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
#include <unistd.h>
#include <string.h>

#include "mac802_16_pmpbs.h"
#include "ssobject.h"
#include "../library.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../structure.h"
#include "../timer_mngr.h"

#define VERBOSE_LEVEL   MSG_WARNING
#include "../verbose.h"

ssObject::ssObject(u_char* pMac, int pBasicCID, int pPriCID, int pSecCID, mac802_16_PMPBS* bs)
{
//	memcpy(MacAddr, pMac, 6);
	address()->copy_from(pMac, 6);
	BasicCID = pBasicCID;
	PriCID = pPriCID;
	SecCID = pSecCID;

	MnConnections[0] = new ManagementConnection(pBasicCID, 0);
	MnConnections[1] = new ManagementConnection(pPriCID, 0);
	MnConnections[2] = pSecCID ? new ManagementConnection(pSecCID, 0) : NULL;
	pBS = bs;

	DownChID = UpChID = 1;
	DSApending = broadcastCID;	// broadcast CID: No pending DSA-RSP Pending

	BASE_OBJTYPE(mem_func);
	mem_func = POINTER_TO_MEMBER(ssObject, T7);
	timer_mngr()->set_func_t(7u, this, mem_func);
	mem_func = POINTER_TO_MEMBER(ssObject, T10);
	timer_mngr()->set_func_t(10u, this, mem_func);
}

int ssObject::handle(mgmt_msg * recvmm, int cid, int len)
{
	Connection *c;

	if (cid == BasicCID) {
		pCurrentConnect = MnConnections[0];
		if (recvmm->type == MG_SBCREQ) {
			INFO("SBCREQ len = %d\n", len);
			procSBCREQ(recvmm, cid, len);
		}
	} else if (cid == PriCID) {
		pCurrentConnect = MnConnections[1];
		if (recvmm->type == MG_REGREQ) {
			INFO("REGREQ len = %d\n", len);
			procREGREQ(recvmm, cid, len);

			if (DSApending == broadcastCID) {
				Sflow = pBS->GetProvisionedFlow(address()->buf());
				if (Sflow) {
					c = pBS->
					    CreateDataConnection(this);
					DSApending = c->getCID();
					SendDSAREQ(c->getCID());
					Sflow->SetConnectionID(c->
							       getCID());
					c->SetFID(Sflow->GetSfindex());
				}
			}
		}
		if (recvmm->type == MG_DSARSP) {
			INFO("DSARSP len = %d\n", len);
			DSApending = broadcastCID;
			timer_mngr()->cancel_t(7);
			procDSARSP(recvmm, cid, len);
		}
	} else {
	}
	return 0;
}

void ssObject::T7()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);

	SendDSAREQ(DSApending);
}

void ssObject::T10()
{
	printf("%d: mac802_16_PMPSS::%s()\n", get_nid(), __FUNCTION__);
}

/*
 * Figure 67
 */
void ssObject::procSBCREQ(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	uint8_t t = 0;

	u_char *ptr;
	u_char fbas, fmaxpower[4], fctpower, ffftsize, fdemod, fmod, ffcs, ftc;
	u_int16_t fstg;

	DEBUG("mm=%p, cid=%d len=%d\n", recvmm, cid, len);
	ptr = recvmm->msg;
	ifmm = new ifmgmt((u_char *) recvmm, len, 0);

	while ((t = ifmm->getNextType()) != 0) {
		DEBUG("Type = %d\t", t);
		switch (t) {
		case 1:
			ifmm->getNextValue(&fbas);
			DEBUG("bandwidth allocation = %d\n", fbas);
			break;
		case 2:
			ifmm->getNextValue(&fstg);
			DEBUG("stg = %d\n", fstg);
			break;
		case 3:
			ifmm->getNextValue(fmaxpower);
			DEBUG("maxpower = %d %d %d %d\n", fmaxpower[0],
			       fmaxpower[1], fmaxpower[2], fmaxpower[3]);
			break;
		case 147:
			fctpower = 0;
			ifmm->getNextValue(&fctpower);
			DEBUG("current power = %d\n", fctpower);
			break;
		case 150:
			ffftsize = 0;
			ifmm->getNextValue(&ffftsize);
			DEBUG("current power = %d\n", ffftsize);
			break;
		case 151:
			fdemod = 0;
			ifmm->getNextValue(&fdemod);
			DEBUG("demodulator = %d\n", fdemod);
			break;
		case 152:
			fmod = 0;
			ifmm->getNextValue(&fmod);
			DEBUG("modulator = %d\n", fmod);
			break;
		case 153:
			ffcs = 0;
			ifmm->getNextValue(&ffcs);
			DEBUG("focus contention = %d\n", ffcs);
			break;
		case 154:
			ftc = 0;
			ifmm->getNextValue(&ftc);
			DEBUG("TC = %d\n", ftc);
			break;

		default:
			break;
		}
	}
	delete ifmm;

	ifmm = new ifmgmt(MG_SBCRSP, 0);	// Sec 11.8
	ifmm->appendTLV(1, 1, fbas);
	ifmm->appendTLV(2, 2, fstg);
	ifmm->appendTLV(3, 4, fmaxpower);
	ifmm->appendTLV(147, 1, fctpower);
	ifmm->appendTLV(150, 1, ffftsize);
	ifmm->appendTLV(151, 1, fdemod);
	ifmm->appendTLV(152, 1, fmod);
	ifmm->appendTLV(153, 1, ffcs);
	ifmm->appendTLV(154, 1, ftc);

	pCurrentConnect->Insert(ifmm);
}

/*
 *	Section 6.3.2.3.10
 */
void ssObject::SendDSAREQ(u_short cid)
{
	ifmgmt *ifmm;
	ifTLV *tmptlv;
	u_char fhmac[21];

	ifmm = new ifmgmt(MG_DSAREQ, 2);

	ifmm->appendField(2, 1234);	// Transaction ID

	{
		tmptlv = new ifTLV();
		tmptlv->Add(2, 2, cid);	// CID 11.13.2
		// CS specification, Spec 11.13.19.1
		// 1: Packet, IPv4
		// 2: Packet, IPv6
		// 3: Packet, 802.3/Ethernet
		// 4: Packet, 802.1Q VLAN
		// 5: Packet, IPv4 over 802.3/Ethernet
		tmptlv->Add(28, 1, pBS->getCSType());
	}
	ifmm->appendTLV(145, tmptlv->getLen(), (void *) tmptlv->getBuffer());	// Uplink service flow
	delete tmptlv;

	ifmm->appendTLV(149, 21, fhmac);	// hmac
/*
	ifmm->copyTo(saved_msg);
	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());
	ifsavedmm->getLen();
	lastCID = cid;
*/
	pCurrentConnect->Insert(ifmm);

	resetTimerT(7);
}

/*
 * Figure 70
 */
void ssObject::procREGREQ(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	int t = 0;

	u_char *ptr;
	u_char fhmac[21], fmanage, fipm, farq, fcrc;
	u_int16_t fncid;

	DEBUG("mm=%p, cid=%d len=%d\n", recvmm, cid, len);
	ptr = recvmm->msg;
	ifmm = new ifmgmt((u_char *) recvmm, len, 0);

	while ((t = ifmm->getNextType()) != 0) {
		DEBUG("Type = %d\t", t);
		switch (t) {
		case 2:
			ifmm->getNextValue(&fmanage);
			DEBUG("manage = %d\n", fmanage);
			break;
		case 3:
			ifmm->getNextValue(&fipm);
			DEBUG("IP mode = %d\n", fipm);
			break;
		case 6:
			ifmm->getNextValue(&fncid);
			DEBUG("# of UCID = %d\n", fncid);
			break;
		case 10:
			ifmm->getNextValue(&farq);
			DEBUG("ARQ Support = %d\n", farq);
			AttrARQ = farq;
			break;
		case 12:
			ifmm->getNextValue(&fcrc);
			DEBUG("CRC Support = %d\n", fcrc);
			AttrCRC = fcrc;
			break;
		case 149:
			ifmm->getNextValue(fhmac);
			DEBUG("HMAC = ...\n");
			break;

		default:
			DEBUG("Non-implement Type (%d)\n", t);
			break;
		}
	}
	delete ifmm;

	ifmm = new ifmgmt(MG_REGRSP, 0);	// Sec 11.8

	ifmm->appendTLV(2, 1, 0U);
	ifmm->appendTLV(3, 1, 0U);
	ifmm->appendTLV(6, 2, fncid);
	ifmm->appendTLV(10, 2, AttrARQ);
	ifmm->appendTLV(12, 2, AttrCRC);
	ifmm->appendTLV(149, 21, fhmac);

	pCurrentConnect->Insert(ifmm);
}

void ssObject::procDSARSP(struct mgmt_msg *recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	int type = 0;
	u_int16_t ftransid;
	u_char fcc, fhmac[21];

	DEBUG("mm=%p, cid=%d len=%d\n", recvmm, cid, len);
	ifmm = new ifmgmt((u_char *) recvmm, len, 3);
	ifmm->getField(0, 2, &ftransid);
	ifmm->getField(2, 1, &fcc);

	while ((type = ifmm->getNextType()) != 0) {
		DEBUG("Type = %d\t", type)
		    switch (type) {
		case 149:
			ifmm->getNextValue(fhmac);
			DEBUG("HMAC = ...\n");
			break;
		default:
			break;
		}
	}
	delete ifmm;


	ifmm = new ifmgmt(MG_DSAACK, 3);	// Table 365

	ifmm->appendField(2, ftransid);	// Transcation ID
	ifmm->appendField(1, 0);	// CC: OK, Table 382

	ifmm->appendTLV(149, 21, fhmac);	// HMAC

/*
	ifmm->copyTo(saved_msg);
	ifsavedmm = new ifmgmt(saved_msg, ifmm->getLen(), ifmm->getFLen());
	lastCID = cid;
*/
	pCurrentConnect->Insert(ifmm);
	resetTimerT(10);
}

void ssObject::Dump()
{
	int i;
	list < DataConnection * >::iterator it;
	printf("Dump Subscribe Station %d Info: MacAddr = %s, diuc=%d uiuc=%d\n",
			0, address()->str(), diuc, uiuc);
	for (i = 0; i < 3; i++)
		if (MnConnections[i])
			printf("\tMnConnection[%d], CID=%d\n", i,
			       MnConnections[i]->getCID());
	for (it = DataConnections.begin(); it != DataConnections.end();
	     it++)
		printf("\tDataConnection[], CID=%d\n", (*it)->getCID());
	printf("\n");
}
