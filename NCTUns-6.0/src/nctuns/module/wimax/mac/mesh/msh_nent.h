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

#ifndef __NCTUNS_WIMAX_MSH_NENT_H__
#define __NCTUNS_WIMAX_MSH_NENT_H__


#include <inttypes.h>
#include "../management_message.h"


struct NENT_RequestIE {
	u_char MacAddr[6];
	u_char OpConfInfo[8];
	uint16_t msbOpAuth;
	uint16_t lsbOpAuth;
	uint16_t msbNodeSn;
	uint16_t lsbNodeSn;
};

struct MSH_NENT_field {
	u_char entType:3;
	u_char XmtCounter:3;
	u_char reserve1:2;
	//u_int16_t     SponsorID       :16;
	u_char msbSponsorID:8;
	u_char lsbSponsorID:8;
	u_char XmtPower:4;
	u_char XmtAntenna:3;
	u_char reserve2:1;
};


class Mac_address;

class MSH_NENT : public ManagementMessage {

public:
	enum {
		RESERVED		= 0X0,
		NET_ENTRY_ACK		= 0X1,
		NET_ENTRY_REQUEST	= 0X2,
		NET_ENTRY_CLOSE		= 0X3,
	};

private:
	MSH_NENT_field *f;	// fixed length field part
	NENT_RequestIE *req;

public:
	explicit MSH_NENT(u_char pXmtCntr, uint16_t pNID, u_char pXmtPwr, u_char pXmtAnt);
	explicit MSH_NENT(u_char* pBuffer, int pLength);
	virtual ~MSH_NENT();

	void setTypeAck(u_char pXmtCntr) const;
	void setTypeRequest(Mac_address*, u_char pOpInfo[8], uint32_t pOpAuth, uint32_t pNodeSn);
	void setTypeClose() const;
	int copyTo(u_char* dstbuf) const;

	inline int getLen() const
	{
		return 1 + sizeof(MSH_NENT_field) + ((f->entType == NET_ENTRY_REQUEST) ? sizeof(NENT_RequestIE) : 0);
	}
	inline u_char getEntryType() const { return f->entType; }
	inline u_char getXmtCounter() const { return f->XmtCounter; }
	inline uint16_t getSponsorID() const { return (f->msbSponsorID << 8) + f->lsbSponsorID; }
	inline u_char getXmtParameter(u_char& pwr, u_char& ant) const
	{
		pwr = f->XmtPower;
		ant = f->XmtAntenna;
		return f->XmtPower;
	}
	inline const u_char *getMacaddress() const { return req->MacAddr; }
	inline int getOpAuth() const { return (req->msbOpAuth << 16) | (req->lsbOpAuth); }
	inline int getNodeSerailNumber() const { return (req->msbNodeSn << 16) | (req->lsbNodeSn); }
};


#endif /* __NCTUNS_WIMAX_MSH_NENT_H__ */
