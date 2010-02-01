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

#ifndef __NCTUNS_ssObject_h__
#define __NCTUNS_ssObject_h__

#include <list>

#include <timer.h>

#include "ss_base.h"

#include <stdint.h>

using namespace std;

class mac802_16_PMPBS;
class ServiceFlow;

class ssObject:public mac802_16SSbase {

public:
	u_char diuc, uiuc;
//      int     StateID;

	mac802_16_PMPBS *pBS;
	ManagementConnection *MnConnections[3];	// 0: Basic, 1: Primary, 2:Secondary
	list<DataConnection*>DataConnections;
	ManagementConnection *pCurrentConnect;	// some dirty trick
	ServiceFlow *Sflow;

	u_int16_t DSApending;	// Only process one DSA-REQ at time

	explicit ssObject(u_char*, int, int, int, mac802_16_PMPBS*);

	inline uint16_t getBasicCID() {
		return BasicCID;
	};
	int handle(mgmt_msg *, int, int);


	void T7();
	void T10();

	void SendDSAREQ(u_int16_t);
	void procSBCREQ(mgmt_msg *, int, int);
	void procREGREQ(mgmt_msg *, int, int);
	void procDSARSP(mgmt_msg *, int, int);

	u_char saved_msg[256];
	ifmgmt *ifsavedmm;
	u_int16_t lastCID;

	void initDSA_BS(int, ServiceFlow *);
	void Dump();

	friend class mac802_16_PMPBS;
};

#endif				/* __NCTUNS_ssObject_h__ */
