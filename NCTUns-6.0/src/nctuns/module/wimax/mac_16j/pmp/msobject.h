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

#ifndef __NCTUNS_80216J_MSOBJECT_H__
#define __NCTUNS_80216J_MSOBJECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <list>

#include <timer.h>

#include "mac802_16j_pmpbs.h"
#include "mac802_16j_pmpms.h"
#include "ms_base.h"
#include "../library.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../structure.h"
#include "../timer_mngr.h"

using namespace std;
using namespace mobileRelayMacAddress;
using namespace mobileRelayServiceFlow;
using namespace mobileRelayTimer;
using namespace mobileRelayMSbase;

// MS status
enum MSstatus{
	Serving,
	PerformHO,
	NeedClear
};

class mac802_16j_PMPBS;
class mac802_16j_PMPMS;

	class msObject_mr:public mac802_16MSbase {
		public:
			mac802_16j_PMPMS        *myMS;	// pointer to real MAC module
			uint8_t                 diuc;
			uint8_t                 uiuc;
			uint8_t rdiuc;        // relay diuc from rs to ms
			uint8_t ruiuc;        // relay uiuc from ms to rs
			mac802_16j_PMPBS        *servingBS;
			mac802_16j		*accessStation;
			NbrBS_MR                   *targetBS;
			ManagementConnection    *MnConnections[3];  // 0: Basic, 1: Primary, 2:Secondary
			list<DataConnection *>  DtConnections;
			ManagementConnection    *pCurrentConnect;   // some dirty trick
			ServiceFlow             *Sflow;
			timerObj                *timerResourceRetain;
			uint16_t                DSApending; // Only process one DSA-REQ at time
			uint16_t                lastCID;
			uint16_t                relayRSBcid;
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

			explicit msObject_mr(uint8_t *, int, int, int, mac802_16j_PMPBS *, mac802_16j_PMPMS *);
			~msObject_mr();
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
			void procREPRSP(struct mgmt_msg *, int, int);

			void initDSA_BS(int, ServiceFlow *);
			void Dump();

			inline uint16_t getBasicCID()
			{
				return BasicCID;
			}

			inline uint16_t getPriCID()
			{
				return PriCID;
			}

			inline uint16_t getSecCID()
			{
				return SecCID;
			}
			
			inline uint8_t getDIUC()
			{
				return diuc;
			}
			
			inline uint8_t getUIUC()
                        {
                                return uiuc;
                        }

			friend class mac802_16j_PMPBS;
	};
#endif                /* __NCTUNS_80216J_MSOBJECT_H__ */
