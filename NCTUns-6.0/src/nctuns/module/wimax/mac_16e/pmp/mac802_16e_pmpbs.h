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

#ifndef __NCTUNS_80216E_MAC_80216E_PMPBS_H__
#define __NCTUNS_80216E_MAC_80216E_PMPBS_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <ethernet.h>
#include <nctuns_api.h>
#include <packet.h>
#include <object.h>
#include <timer.h>
#include <mbinder.h>

#include "scheduler.h"
#include "msobject.h"
#include "../library.h"
#include "../common.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../timer_mngr.h"
#include "../mac802_16e.h"
#include "../management_message.h"
#include "../../phy_16e/ofdma_80216e.h"
#include "../../phy_16e/ofdma_pmpbs.h"

#define    MAXMS        255
#define    MAXCONN      1000

extern const double frameDuration[];

using namespace mobileMac80216e;
using namespace mobileCommon;
using namespace mobileMacAddress;
using namespace mobileServiceFlow;
using namespace mobileTimer;
using namespace mobileManageMsg;

class BS_16e_Scheduler;
class msObject;
class RangingMSobject;

class mac802_16e_PMPBS:public mac802_16e {
	private:
		char    *BSCfgFile;
		char    *NBRBSCfgFile;
		char    *CSTYPE;
		char    *LinkMode;
		int     *_frameDuCode;
		int     DCDinterval;
		int     UCDinterval;
		int     MOB_NBR_ADVinterval;
		int     frameNumber;
		int     ULoccupiedSymbols;
		double  fecSNR[16];    // Index by rate id (fec)

		timerObj *timerDCD;
		timerObj *timerUCD;
		timerObj *downlinkTimer;
		timerObj *timerMOB_NBR_ADV;
		timerObj *timerSetldle;

		list<msObject *>        PolledList;
		list<RangingObject *>   RangingCodeList;
		msObject                *mslist[MAXMS];
		ManagementConnection    *broadcastConnection;
		ManagementConnection    *dlmapConnection;
		ManagementConnection    *initRangingConnection;
		DataConnection          *conlist[MAXCONN];
		struct PHYInfo          *LastSignalInfo;
		struct DCDBurstProfile  DCDProfile[16];    // Index: DIUC
		struct UCDBurstProfile  UCDProfile[16];    // Index: UIUC
		vector<ConnectionReceiver *> ReceiverList;

		uint64_t    frameStartTime;
		uint8_t     UCDCfgCount;
		uint8_t     DCDCfgCount;

		ProvisionedSfTable  *PsTable;
		ServiceClassTable   *ScTable;
		ClassifierRuleTable *CrTable;

		BS_16e_Scheduler *pScheduler;

		//////////////////////////////////////////////////////////// member functions
		void parseCfgFile();
		void RemoveMS(msObject *);
		void saveDLMAP(char *, int);
		void changeStateToIdle();
		void saveDLsymbols(int);
		void saveULsymbols(int);
		bool DownlinkClassifier(Packet *);
		void PacketScheduling();
		void handleRangingCode();
		void generate_UCD();
		void generate_DCD();
		void generate_MOB_NBRADV();
		void procRNGREQ(struct mgmt_msg *, int, int);

		msObject *CreateMS(uint8_t *);
		msObject *getMSbycid(int);

	public:
		mac802_16e_PMPBS(uint32_t type, uint32_t id, struct plist *pl, const char *name);
		~mac802_16e_PMPBS();

		NeighborBSs *NeighborBSList;

		int init();
		int recv(ePacket_ *);
		int send(ePacket_ *);

		inline uint8_t getCSType()
		{
			return _CSType;
		}

		inline int FrameDuCode() const { return *_frameDuCode; }

		Connection      *CreateDataConnection(msObject *);
		ServiceFlow     *GetProvisionedFlow(uint8_t mac[6]);
		ServiceFlow     *GetProvisionedFlow(uint32_t flowId);
		ServiceClass    *GetServiceClass(uint32_t qosIndex);

		friend class BS_16e_Scheduler;
};

#endif                /* __NCTUNS_80216E_MAC_80216E_PMPBS_H__  */
