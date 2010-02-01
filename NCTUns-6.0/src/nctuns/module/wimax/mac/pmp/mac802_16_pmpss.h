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

#ifndef __NCTUNS_mac_802_16_PMPSS_h__
#define __NCTUNS_mac_802_16_PMPSS_h__

#include <vector>

#include <object.h>
#include <packet.h>
#include <timer.h>
/*
#include <misc/log/logHeap.h>
#include <misc/log/logchktbl.h>
*/
#include "ss_base.h"


#define	ssInit		0x000
#define	ssScan		0x001	// Scanning & Synchronization to the DL
#define	ssDLParam	0x002	// Obtain DL parameters
#define	ssULParam	0x004	// Obtain UL parameters
#define	ssRanging	0x008	// Initial ranging & automatic adjustments
#define	ssNegcap	0x010	// Negotiate basic capabilities
#define	ssAuth		0x020
#define	ssRegister	0x040	// Registration
#define	ssProv		0x080	// Establish provisioned connections
#define	ssOp		0x100	// Operational

using namespace std;

class SSScheduler;
class mac802_16_PMPSS:public mac802_16SSbase {

private:
	char *LinkMode;

	int State;
	u_char myDIUC;

	timerObj *timerSendRnging, *timerSendUL;
	timerObj *timerLostDL, *timerLostUL;

	int LostULinterval, LostDLinterval;
	int TimingAdjust;
	double fecSNR[16];	// Index by rate id (fec)
	u_char RngStart, RngEnd, ReqStart, ReqEnd;	// Random Backoff Parameters
	u_int16_t RngCount, RngTime, RngLastStatus;

	u_char saved_msg[256];
	ifmgmt *ifsavedmm;
	u_int16_t lastCID;

	OFDM_ULMAP_IE savedRNGie, savedBWie, savedULie;

	ManagementConnection *initRangingConnection;
	 vector < ManagementConnection * >MnConnections;	// Basic, Primary Connections
	 vector < DataConnection * >ConnectionList;
	 vector < ConnectionReceiver * >ReceiverList;

	struct PHYInfo LastSignalInfo;

	u_char UCDCfgCount, DCDCfgCount;
	u_int16_t UCDBWOpp, UCDRngOpp;
	struct DCDBurstProfile DCDProfile[16];
	struct UCDBurstProfile UCDProfile[16];
	SSScheduler *pScheduler;

	void PushPDUintoConnection(u_int16_t cid, ifmgmt * pdu);
	Connection *getConnection(u_int16_t cid);
	u_int64_t computeSendTime(const OFDM_ULMAP_IE & ie, int fastime);

	void T1();
	void T3();
	void T6();
	void T8();
	void T18();
	void LostDLMAP();
	void LostULMAP();
	void PacketScheduling();
	void WaitForInitRangingOpportuniy();

	int procDCD(struct mgmt_msg *, int, int);
	int procUCD(struct mgmt_msg *, int, int);
	int procDLMAP(struct mgmt_msg *, int, int);
	int procULMAP(struct mgmt_msg *, int, int);

	int procRNGRSP(struct mgmt_msg *, int, int);
	int procSBCRSP(struct mgmt_msg *, int, int);

	int procDSAREQ(struct mgmt_msg *, int, int);
	int procDSAACK(struct mgmt_msg *, int, int);

	int SendRNGREQ();
	int SendSBCREQ();
	int SendREGREQ();

	u_char selectDIUC(double);

      public:
	 mac802_16_PMPSS(u_int32_t type, u_int32_t id, struct plist *pl,
			 const char *name);
	~mac802_16_PMPSS();

	int init();
	int recv(Event *);
	int send(Event *);

	friend class SSScheduler;
};
#endif				/* __NCTUNS_mac802_16_PMPSS_h__ */
