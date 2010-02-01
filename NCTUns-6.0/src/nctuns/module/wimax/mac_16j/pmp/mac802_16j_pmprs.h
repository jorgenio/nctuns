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

#ifndef __NCTUNS_80216J_MAC_80216E_PMPRS_H__
#define __NCTUNS_80216J_MAC_80216E_PMPRS_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <math.h>
#include <vector>

#include <ethernet.h>
#include <nctuns_api.h>
#include <packet.h>
#include <object.h>
#include <tcp.h>
#include <timer.h>

#include "scheduler.h"
#include "rs_base.h"
#include "../common.h"
#include "../library.h"
#include "../mac_address.h"
#include "../timer_mngr.h"
#include "../ofdma_map_ie.h"
#include "../../phy_16j/ofdma_80216j.h"
#include "../../phy_16j/ofdma_pmprs_MR.h"

#define    rsInit       0x000
#define    rsScan       0x001    // Scanning & Synchronization to the DL
#define    rsDLParam    0x002    // Obtain DL parameters
#define    rsULParam    0x004    // Obtain UL parameters
#define    rsRanging    0x008    // Initial ranging & automatic adjustments
#define    rsNegcap     0x010    // Negotiate basic capabilities
#define    rsAuth       0x020
#define    rsRegister   0x040    // Registration
#define    rsProv       0x080    // Establish provisioned connections
#define    rsOp         0x100    // Operational

#define    MAXMS        255
#define    MAXCONN      1000

extern const double frameDuration[];

using namespace std;
using namespace mobileRelayCommon;
using namespace mobileRelayMacAddress;
using namespace mobileRelayTimer;
using namespace mobileRelayRSbase;
using namespace mobileRelayOFDMAMapIE;

class RS_16j_Scheduler;

class mac802_16j_PMPRS:public mac802_16RSbase {
	private:
		enum RangingStatus {
			Init,
			Continue,
			Abort,
			RngSuccess,
			CodeSuccess,
			Rerange,
			REQWaitRSP,
			CodeWaitRSP,
		};
		char    *LinkMode;
		char    *CSTYPE;
		uint8_t myDIUC;
		int     State;
		int     frameNumber;
		int     *_frameDuCode;
		uint8_t ULsymbols;

		int         _ULAccessSymbols;
		int         _ULRelaySymbols;
		int	    _DLAccessSymbols;
		int	    _DLTransSymbols;

		vector<WiMaxBurst *> *downTransBurst;

		timerObj *timerSendRngCode;
		timerObj *timerSendRngReq;
		timerObj *timerSendUL;
		timerObj *timerSendDL;
		timerObj *timerLostDL;
		timerObj *timerLostUL;
		timerObj *timerHORetransmit;
		timerObj *timerPeriodRanging;
		timerObj *timerSendScanReq;
		timerObj *timerStartScan;
		timerObj *timerStartHO;

		int LostULinterval;
		int LostDLinterval;
		int PeriodRangingInterval;
		int HORetransmitInterval;
		int ScanReqInterval;
		int ServerLevelPredict;
		int HOReqRetryAvailable;
		int TimingAdjust;
		double fecSNR[16];    // Index by rate id (fec)

		uint8_t RngStart;
		uint8_t RngEnd;
		uint8_t ReqStart;
		uint8_t ReqEnd;
		uint8_t RngCodeRegion;
		uint8_t UCDCfgCount;
		uint8_t DCDCfgCount;

		uint16_t RngCount;
		uint16_t RngTime;
		uint16_t RngLastStatus;
		uint32_t RngChOffset;
		uint32_t RngSymOffset;

		bool        ScanFlag;
		uint8_t     ServingBSID[6];
		uint8_t     saved_msg[256];
		uint16_t    lastCID;
		ifmgmt      *ifsavedmm;

		list <uint16_t> mem_cidlist;

		RangingObject savedCode;
		ULMAP_IE_u savedULie;
		ULMAP_IE_u savedIHie;
		ULMAP_IE_u savedBPie;
		ULMAP_IE_u savedALie;

		ManagementConnection             *initRangingConnection;
		vector<ManagementConnection *>   MnConnections;    // Basic, Primary Connections
		
		/*record management & data connection relay by this RS*/
		vector <ManagementConnection *>	UL_relayManalist;
		vector <DataConnection *>	UL_relayDtlist;
		vector <ConnectionReceiver *>	UL_relayReceiverList;
		vector <ManagementConnection *> DL_relayManalist;
                vector <DataConnection *>       DL_relayDtlist;
		vector <ConnectionReceiver *>   DL_relayReceiverList;

		struct DCDBurstProfile DCDProfile[16];
		struct UCDBurstProfile UCDProfile[16];
		struct PHYInfo LastSignalInfo;

		NeighborBSs_MR *NeighborBSList;
		RS_16j_Scheduler *pScheduler;

		///////////////////////////////////////////////////////////////// member functions
		void PushPDUintoConnection(uint16_t cid, ifmgmt * pdu);
		Connection *getConnection(uint16_t cid);
		uint64_t computeSendTime(int numSlots, int duration, int fastime);
		uint64_t computeSendTime(const struct OFDMA_ULMAP_IE_12 &ie_12, int txopOffset, int fastime);

		void T1();
		void T2();
		void T3();
		void T6();
		void T8();
		void T12();
		void T18();
		void T42();
		void T67();
		void LostDLMAP();
		void LostULMAP();
		void HOmsgRetransmit();
		void StartPeriodRanging();
		void PacketScheduling_UL();
		void PacketScheduling_DL();
		void StartScanNbrBSs();
		void StartHandover();
		void WaitForRangingOpportunity();

		ManagementConnection * getULRelayManaconn(uint16_t);
		ManagementConnection * getDLRelayManaconn(uint16_t);
		DataConnection *  getULRelayDtconn(uint16_t);
		DataConnection *  getDLRelayDtconn(uint16_t);

		int procDCD(struct mgmt_msg *, int, int);
		int procUCD(struct mgmt_msg *, int, int);
		int procDLMAP(struct mgmt_msg *, int, int);
		int procULMAP(struct mgmt_msg *, int, int);
		int procMOB_NBRADV(struct mgmt_msg *, int, int);
		int procRNGRSP(struct mgmt_msg *, int, int);
		int procSBCRSP(struct mgmt_msg *, int, int);
		int procMOB_SCNRSP(struct mgmt_msg *, int, int);
		int procREGRSP(struct mgmt_msg *, int, int);
		int procDSAREQ(struct mgmt_msg *, int, int);
		int procDSAACK(struct mgmt_msg *, int, int);
		int procMOB_BSHORSP(struct mgmt_msg *, int, int);
		int procRS_ConfigCMD(struct mgmt_msg *, int, int);
		int procRS_MemberListUpdate(struct mgmt_msg *, int, int);
		int procMS_InfoDel(struct mgmt_msg *, int, int);

		int SendRNGCODE();
		int SendRNGREQ();
		int SendSBCREQ();
		int SendREGREQ();
		int SendMOB_SCNREQ();
		int SendMOB_SCNREP();
		int SendMOB_RSHOREQ();
		int SendMOB_HOIND(uint8_t);
		int SendMR_GenericACK();
		int SendMR_RNGREP(Packet*);
		int SendREPREQ();		// purpose for measuring relay diuc from rs to ms

		uint8_t selectDIUC(double);
		uint8_t selectUIUC(double);
		double  getMCSweight(uint8_t);

		inline int frameDuCode() const { return *_frameDuCode; }

	public:
		mac802_16j_PMPRS(uint32_t type, uint32_t id, struct plist *pl, const char *name);
		~mac802_16j_PMPRS();

		int init();
		int recv(ePacket_ *);
		int send(ePacket_ *);


		friend class RS_16j_Scheduler;
		friend class BS_16j_Scheduler;
};

#endif                /* __NCTUNS_80216E_MAC_80216J_PMPRS_H__ */
