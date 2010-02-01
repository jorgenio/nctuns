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

#include <cstdio>
#include <string.h>

#include "msh_nent.h"
#include "../mac_address.h"

MSH_NENT::MSH_NENT(u_char pXmtCntr, u_int16_t pNID, u_char pXmtPwr, u_char pXmtAnt)
: ManagementMessage(CTLMSG_TYPE_MSH_NENT)
{

	f = new MSH_NENT_field;
	bzero(f, sizeof(MSH_NENT_field));
	req = NULL;

	f->XmtCounter = pXmtCntr;
	f->msbSponsorID = pNID >> 8;
	f->lsbSponsorID = pNID & 0xFF;
	f->XmtPower = pXmtPwr;
	f->XmtAntenna = pXmtAnt;
}

MSH_NENT::MSH_NENT(u_char* pBuffer, int pLength)
: ManagementMessage(pBuffer)
{
	int pos = 1 + sizeof(MSH_NENT_field);
	req = NULL;

	f = reinterpret_cast<MSH_NENT_field*>(pBuffer + 1);

	if (f->entType == 0x2) {
		req = reinterpret_cast<NENT_RequestIE*>(pBuffer + pos);
	}
}

MSH_NENT::~MSH_NENT()
{
	if (flag) {
		delete f;
		delete req;
	}
}

void
MSH_NENT::setTypeAck(u_char pXmtCntr) const
{
	f->entType = 0x1;
	f->XmtCounter = pXmtCntr;
}

void
MSH_NENT::setTypeRequest(
        Mac_address* mac_addr, u_char pOpInfo[8], u_int32_t pOpAuth, u_int32_t pNodeSn)
{
	req = new NENT_RequestIE;
	f->entType = 0x2;
	memcpy(req->MacAddr, mac_addr->buf(), 6);
	memcpy(req->OpConfInfo, pOpInfo, 8);
	req->msbOpAuth = pOpAuth >> 16;
	req->lsbOpAuth = pOpAuth & 0xffff;
	req->msbNodeSn = pNodeSn >> 16;
	req->lsbNodeSn = pNodeSn & 0xffff;
}

void
MSH_NENT::setTypeClose() const
{
	f->entType = 0x3;
}


int
MSH_NENT::copyTo(u_char* dstbuf) const
{
	int pos = 0;
	memcpy(dstbuf, &Type, 1);
	memcpy(dstbuf + 1, f, sizeof(MSH_NENT_field));
	pos = 1 + sizeof(MSH_NENT_field);

	if (f->entType == 0x2) {
		memcpy(dstbuf + pos, req, sizeof(NENT_RequestIE));
		pos += sizeof(NENT_RequestIE);
	}
	return pos;		// Total Length
}
