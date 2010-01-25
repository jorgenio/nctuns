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

#ifndef __NCTUNS_80216J_NT_NEIGHBOR_BS_H__
#define __NCTUNS_80216J_NT_NEIGHBOR_BS_H__

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "mac_address.h"

#define HO_THRESHOLD 8

using namespace std;
using namespace MR_MacAddress_NT;

class NbrMRBS_NT {
	public:
		/* MOB_NBR-ADV */
		uint8_t PHY_ProfileID;
		uint8_t PreambleIndex;  // save channel ID
		uint8_t HO_Optimize;
		uint8_t SchServSupport;
		uint8_t DCDCfgCount;
		uint8_t UCDCfgCount;

		/* MOB_SCN-REP */
		uint8_t ScanType;
		uint8_t CINR;           // save SNR
		uint8_t RSSI;
		uint8_t RelativeDelay;
		uint8_t RTD;

		/* Identify */
		uint8_t Index;
		Mac_address *addr;
		int nid;
		bool targetHO;

	public:
		NbrMRBS_NT(int, uint8_t *);
		~NbrMRBS_NT();

		void dump();
};

class NeighborMRBSs_NT {
	public:
		uint8_t NBRADV_CfgCount; // Configuration Change Count of MOB_NBR-ADV
		uint8_t ScanDuration;
		uint8_t ReportMode;
		uint8_t ReportMetric;
		uint8_t ReportPeriod;

		uint8_t StartFrame;
		uint8_t InterleavingInterval;
		uint8_t ScanIteration;
		uint8_t ScanTimes;

		uint8_t ServingBSchID;
		double ServingCINR;
		uint8_t ScanningChID;

		vector<NbrMRBS_NT *> nbrBSs_Index;
		vector<NbrMRBS_NT *> nbrBSs_Full;
		vector<NbrMRBS_NT *> nbrBSs_Curr;

	public:
		NeighborMRBSs_NT();
		~NeighborMRBSs_NT();

		NbrMRBS_NT *getNbrbyBSID(uint8_t *);
		NbrMRBS_NT *getNbrbyChID(uint8_t);
		int getNextScanChID();
		bool checkHOterms();
		NbrMRBS_NT *getTargetBS();
		void dump();
};

#endif  /* __NCTUNS_80216J_NT_NEIGHBOR_BS_H__ */
