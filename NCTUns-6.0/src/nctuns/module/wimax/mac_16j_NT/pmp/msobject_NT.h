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

#ifndef __NCTUNS_80216J_NT_MSOBJECT_H__
#define __NCTUNS_80216J_NT_MSOBJECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <list>

#include <timer.h>

#include "mac802_16j_NT_pmpbs.h"
#include "mac802_16j_NT_pmpms.h"
#include "ms_base_NT.h"
#include "../library.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../structure.h"
#include "../timer_mngr.h"

using namespace std;
using namespace MR_MacAddress_NT;
using namespace MR_ServiceFlow_NT;
using namespace MR_Timer_NT;
using namespace MR_MSbase_NT;

// MS status
enum {
	Serving,
	PerformHO,
	NeedClear
};

class mac802_16j_NT_PMPBS;
class mac802_16j_NT_PMPMS;

class msObject_NT:public mac802_16MSbase {
	public:
		uint8_t                 diuc;
		uint8_t                 uiuc;
		mac802_16j_NT           *accessStation;
		mac802_16j_NT_PMPBS     *servingBS;
		mac802_16j_NT_PMPMS     *myMS;      // point to "mac802_16j_NT_PMPMS" module
		NbrMRBS_NT              *targetBS;
		ManagementConnection    *MnConnections[3];  // 0: Basic, 1: Primary, 2:Secondary
		list<DataConnection *>  DtConnections;
		ManagementConnection    *pCurrentConnect;   // some dirty trick
		ServiceFlow             *Sflow;
		timerObj                *timerResourceRetain;
		uint16_t                DSApending; // Only process one DSA-REQ at time
		uint16_t                lastCID;
		uint16_t                PeriodRangingInterval;
		uint16_t                ResourceRetainTime;
		int                     ResourceStatus;
		uint8_t                 ServerLevelPredict;
		uint8_t                 ScanDuration;
		uint8_t                 ScanTimes;
		bool                    ScanFlag;
		int                     ChangeToScanCnt;
		uint8_t                 InterleaveInt;
		uint8_t                 ScanIteration;

		explicit msObject_NT(uint8_t *, int, int, int, mac802_16j_NT_PMPBS *,  mac802_16j_NT_PMPMS *);
		//explicit msObject_NT(uint8_t *, int, int, int, mac802_16j_NT_PMPBS *);
		~msObject_NT();
		int handle(struct mgmt_msg *, int, int);
		void T7();
		void T10();
		void ResourceClear();

		void SendDSAREQ(uint16_t);

		void procSBCREQ(struct mgmt_msg *, int, int);
		void procREGREQ(struct mgmt_msg *, int, int);
		void procDSARSP(struct mgmt_msg *, int, int);
		void procMOB_MSHOREQ(struct mgmt_msg *, int, int);
		void procMOB_SCNREQ(struct mgmt_msg *, int, int);
		void procMOB_SCNREP(struct mgmt_msg *, int, int);
		void procMOB_HOIND(struct mgmt_msg *, int, int);

		void initDSA_BS(int, ServiceFlow *);
		void Dump();

		inline uint16_t getBasicCID()
		{
			return BasicCID;
		}

		friend class mac802_16j_NT_PMPBS;
		friend class mac802_16j_NT_PMPMS;
};

#endif                /* __NCTUNS_80216J_NT_MSOBJECT_H__ */
