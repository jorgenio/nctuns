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

#ifndef __NCTUNS_80216J_NT_MAC_80216J_NT_PMPBS_H__
#define __NCTUNS_80216J_NT_MAC_80216J_NT_PMPBS_H__

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
#include "msobject_NT.h"
#include "rsobject_NT.h"
#include "../library.h"
#include "../common.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../timer_mngr.h"
#include "../mac802_16j_NT.h"
#include "../management_message.h"
#include "../../phy_16j_NT/ofdma_80216j_NT.h"
#include "../../phy_16j_NT/ofdma_pmpbs_NT.h"

#define    MAXMS        255
#define    MAXRS        255
#define    MAXCONN      1000

extern const double frameDuration_NT[];

using namespace MR_Mac80216j_NT;
using namespace MR_Common_NT;
using namespace MR_MacAddress_NT;
using namespace MR_ServiceFlow_NT;
using namespace MR_Timer_NT;
using namespace MR_ManageMsg_NT;

class BS_16j_NT_Scheduler;
class msObject_NT;
class rsObject_NT;
class RangingMSobject_NT;

class mac802_16j_NT_PMPBS:public mac802_16j_NT {
	private:
		char    *BSCfgFile;
		char    *NBRBSCfgFile;
		char    *CSTYPE;
		char    *LinkMode;
		int     *_frameDuCode;
		int     DCDinterval;
		int     UCDinterval;
		int     MOB_NBR_ADVinterval;
		int     RS_BR_header_interval;
        int     frameNumber;
		int     ULoccupiedSymbols;
		int     DLAccessoccupiedSymbols;
		int     DLRelayoccupiedSymbols;
		int     ULAccessoccupiedSymbols;
		int     ULRelayoccupiedSymbols;
		double  fecSNR[16];    // Index by rate id (fec)
        vector <WiMaxBurst *> *DLRelayBurstCollection;

		timerObj *timerDCD;
		timerObj *timerUCD;
		timerObj *downlinkTimer;
		timerObj *timerMOB_NBR_ADV;
		timerObj *timerSetldle;
        timerObj *timerSelectPath;
        timerObj *timerSendDLRelayBurst;

		//list<msObject_NT *>     PolledList;
		//list<rsObject_NT *>     rsPolledList;
		list<RangingObject *>   RangingCodeList;
		list<RangingObject *>   RS_RangingCodeList;
		
		msObject_NT             *mslist[MAXMS];
		rsObject_NT             *rslist[MAXRS];

		ManagementConnection    *broadcastConnection;
		ManagementConnection    *broadcastConnection_relay;
		ManagementConnection    *initRangingConnection;
		ManagementConnection    *initRangingConnection_relay;
		ManagementConnection    *initRangingConnection_relay_MS;
        DataConnection          *conlist[MAXCONN];
		struct PHYInfo          *LastSignalInfo;
		struct DCDBurstProfile  DCDProfile[16];    // Index: DIUC
		struct UCDBurstProfile  UCDProfile[16];    // Index: UIUC
		vector<ConnectionReceiver *> ReceiverList;

		uint64_t    frameStartTime;
		uint8_t     UCDCfgCount;
		uint8_t     DCDCfgCount;
		uint8_t     RCDCfgCount;

		ProvisionedSfTable  *PsTable;
		ServiceClassTable   *ScTable;
		ClassifierRuleTable *CrTable;

		BS_16j_NT_Scheduler *pScheduler;

		//////////////////////////////////////////////////////////// member functions
		void parseCfgFile();
		void RemoveMS(msObject_NT *);
		void saveDLMAP(char *, int);
		void saveRMAP(char *, int);
		void changeStateToIdle();
		void saveDLsymbols(int);
		void saveULsymbols(int);
		void saveDLAccessSymbols(int);
		void saveDLRelaySymbols(int);
		void saveULAccessSymbols(int);
		void saveULRelaySymbols(int);
		bool DownlinkClassifier(Packet *);
		void PacketScheduling();
		void DLRelayPacketScheduling();
		void handleRangingCode();
		void handleRSRangingCode();
		void generate_UCD();
		void generate_DCD();
		void generate_RCD();
		void generate_MOB_NBRADV();
		void generate_RS_AccessMAP();
		void SendMS_InfoDel(uint16_t, uint16_t);
        void procRNGREQ(struct mgmt_msg *, int, int);
        void SelectMSsPath();
        uint8_t selectDIUC(double);
        uint8_t selectUIUC(double);
        double  getWeightbydiuc(uint8_t);
        double  getWeightbyuiuc(uint8_t);

		msObject_NT *CreateMS(uint8_t *,uint8_t,uint8_t);

		rsObject_NT *CreateRS(uint8_t *);
		rsObject_NT *getRSbycid(int);
        mac802_16j_NT_PMPMS *getMS(uint8_t,uint8_t);

	public:
		mac802_16j_NT_PMPBS(uint32_t type, uint32_t id, struct plist *pl, const char *name);
		~mac802_16j_NT_PMPBS();

		NeighborMRBSs_NT *NeighborMRBSList;

		int init();
		int recv(ePacket_ *);
		int send(ePacket_ *);

		inline uint8_t getCSType()
		{
			return _CSType;
		}

		inline int FrameDuCode() const { return *_frameDuCode; }
        void SendRS_MemberListUpdate(list<uint16_t>,bool, uint16_t);     

		Connection      *CreateDataConnection(msObject_NT *);
		Connection      *CreateDataConnection(rsObject_NT *);
		ServiceFlow     *GetProvisionedFlow(uint8_t mac[6]);
		ServiceFlow     *GetProvisionedFlow(uint32_t flowId);
		ServiceClass    *GetServiceClass(uint32_t qosIndex);
        
        uint8_t getMSindex(uint16_t);
		msObject_NT *getMSbycid(int);
		msObject_NT *getMSbyDtcid(uint16_t);

        void ClearRNGCodeList(list<RangingObject *> *RNGCodeList)
        {
            while(!RNGCodeList->empty())
            {
                delete *(RNGCodeList->begin());
                RNGCodeList->erase(RNGCodeList->begin());
            }
        }

		friend class BS_16j_NT_Scheduler;
};

#endif                /* __NCTUNS_80216J_NT_MAC_80216J_NT_PMPBS_H__  */
