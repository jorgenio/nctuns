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

#ifndef __NCTUNS_80216J_COMMON_H__
#define __NCTUNS_80216J_COMMON_H__

#include <stdio.h>
#include <vector>
#include <packet.h>
#include "structure.h"
#include "burst.h"

using namespace std;
using namespace mobileRelayBurst;

#define NORMAL_TYPE 1
#define NOTIFY_PHY 2
#define RNGCODE_INFO_TYPE 3
#define DL_TRANSPARENT_TYPE 4
#define UL_Relay_TYPE 5

namespace mobileRelayCommon {
	class Pkthdr {
		public:
			struct p_hdr p_hdr;
			struct s_exthdr exthdr;
			short flag;        /*  PF_SEND , PF_RECV */
			short type;        // Command Type: 1 for Burst, 2 for notify PHY ,3 for Rangcode infor , 4 for DL transparent burst , 5 for UL relay zone burst
			uint16_t RSBcid;
	};

	class dlmapinfo {
		public:
			uint16_t diuc;
			uint8_t symOff;
			uint8_t chOff;
			uint8_t numSym;
			uint8_t numCh;
			uint8_t repeCode;
	};

	class RangCodeInfo: public Pkthdr {
		public:
			uint8_t *rangingCode; // duplicated
			int codeLen;
			int rangSymOffset;
			int rangChOffset;
			int rangMethod;
			// 16j , added for adptive measure DIUC from BS to MS
			uint16_t MSBcid;
			uint8_t directDIUC;


			RangCodeInfo()
			{
				rangingCode = NULL;
				type = RNGCODE_INFO_TYPE;
				MSBcid = 0;
				directDIUC = RobustDIUC;
			}

			~RangCodeInfo()
			{
				if (rangingCode != NULL)
					delete [] rangingCode;
			}
	};

	class ntfyDLMAP: public Pkthdr {
		public:
			vector<dlmapinfo> info;
			bool transparent_indicator;

			ntfyDLMAP()
			{
				type = NOTIFY_PHY;
				transparent_indicator = false;
			}
	};

	class BurstInfo: public Pkthdr {
		public:
			vector<WiMaxBurst *> *Collection;

			BurstInfo()
			{
				type        = NORMAL_TYPE;
				Collection  = NULL;
			}

			~BurstInfo()
			{
				if (Collection != NULL)
				{
					while (!Collection->empty())
					{
						delete *(Collection->begin());
						Collection->erase(Collection->begin());
					}
					delete Collection;
				}
			}
	};
	//For BS manages Ranging code list
	class RangingObject {
		public:
			uint8_t rangingCodeIndex;
			uint32_t rangingFrameNumber;
			uint8_t rangingSymbol;
			uint8_t rangingSubchannel;
			uint8_t rangingUsage;
			uint64_t recv_time;
			double	 SNR;
			bool collision;
			bool allocated; // if ranging status is success, this flag will be on. (CDMA Allocation)
			/* 16j added */
			int  ULallocStartTime;
			uint16_t  RS_cid; 
			int  MSBcid;	/* For periodic ranging best path selection */
			uint8_t ruiuc;	/* record uiuc between  ms to rs */
			double  relayWeight;

			RangingObject(uint8_t pCodeIndex, uint32_t pFrameNumber, uint8_t pSymbol, uint8_t pSubchannel, uint8_t pUsage, uint64_t pTime, double power , int pULalloctime , uint16_t relaycid, int msbcid, uint8_t relayuiuc)
			{
				rangingCodeIndex    = pCodeIndex;
				rangingFrameNumber  = pFrameNumber;
				rangingSymbol       = pSymbol;
				rangingSubchannel   = pSubchannel;
				rangingUsage        = pUsage;
				recv_time           = pTime;
				collision           = false;
				allocated           = false;
				SNR		    = power;
				ULallocStartTime    = pULalloctime;
				RS_cid = relaycid;
				MSBcid = msbcid;
				ruiuc = relayuiuc;	// robust uiuc
				relayWeight = 2;	// assume worest case of 2 hop realy
			}

			RangingObject()
			{
				;
			}

			~RangingObject()
			{
				;
			}
	};
}

/* for LogInfo */
typedef struct LogInfo {
	uint32_t src_nid;
	uint32_t dst_nid;
	int burst_len;
	int channel_id;
} loginfo_t;

#endif                /* __NCTUNS_80216J_COMMON_H__    */
