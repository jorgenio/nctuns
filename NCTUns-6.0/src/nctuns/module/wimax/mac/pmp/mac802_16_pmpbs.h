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

#ifndef __NCTUNS_mac802_16_PMPBS_h__
#define __NCTUNS_mac802_16_PMPBS_h__

#include <object.h>
#include <packet.h>
#include <timer.h>
/*
#include <misc/log/logHeap.h>
#include <misc/log/logchktbl.h>
*/

#include "../common.h"
#include "../mac802_16.h"
#include "ssobject.h"

#define	MAXSS		255
#define	MAXCONN		1000

class BSScheduler;
class ClassifierRuleTable;
class ProvisionedSfTable;
class ServiceClass;
class ServiceClassTable;

class mac802_16_PMPBS:public mac802_16 {
      private:

	//      Variable Declaration
	char *BSCfgFile, *CSTYPE, *LinkMode;

	int DCDinterval, UCDinterval;
	double fecSNR[16];	// Index by rate id (fec)

	timerObj *timerDCD;
	timerObj *timerUCD;
	timerObj *downlinkTimer;


	int frameNumber;
	int TimingAdjust;
	char dlmapflag;		// True: Force to transmit a complete DL-MAP
	ssObject *sslist[MAXSS];
	 list < ssObject * >PolledList;
	ManagementConnection *broadcastConnection, *initRangingConnection;
	ManagementConnection *globalConnection[3];
	DataConnection *conlist[MAXCONN];
	 vector < ConnectionReceiver * >ReceiverList;

	struct PHYInfo *LastSignalInfo;
	u_int64_t frameStartTime;

	u_char UCDCfgCount, DCDCfgCount;
	struct DCDBurstProfile DCDProfile[16];	// Index: DIUC
	struct UCDBurstProfile UCDProfile[16];	// Index: UIUC

	ProvisionedSfTable *SfTable;
	ServiceClassTable *ScTable;
	ClassifierRuleTable *CrTable;

	BSScheduler *pScheduler;

	//  Function Declaration    
	ssObject *CreateSS(u_char *);


	bool DownlinkClassifier(Packet * p);
	void PacketScheduling();
	void generate_UCD();
	void generate_DCD();
	void generate_DL_MAP();

	ssObject *getSSbycid(int);

	void procRNGREQ(mgmt_msg *, int, int);

      public:
	 mac802_16_PMPBS(u_int32_t type, u_int32_t id, struct plist *pl,
			 const char *name);
	~mac802_16_PMPBS();

	int init();
	int recv(Event *);
	int send(Event *);

	inline u_char getCSType() {
		return _CSType;
	} Connection *CreateDataConnection(ssObject *);
	ServiceFlow *GetProvisionedFlow(u_char mac[6]);
	ServiceFlow *GetProvisionedFlow(u_int32_t flowId);
	ServiceClass *GetServiceClass(u_int32_t qosIndex);

	friend class BSScheduler;
};
#endif				/* __NCTUNS_mac802_16_PMPBS_h__ */
