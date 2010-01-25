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

#ifndef __NCTUNS_80216J_NT_COMMON_H__
#define __NCTUNS_80216J_NT_COMMON_H__

#include <stdio.h>
#include <vector>
#include <packet.h>
#include "structure.h"
#include "burst.h"
#include "../../misc/log/logpack.h"
#define NORMAL_TYPE     1
#define NOTIFY_PHY      2
#define RANGING_INFO    3
#define DL_ACCESS       4
#define DL_RELAY        5
#define UL_ACCESS       6
#define UL_RELAY        7


// RNG-REQ burst type
#define MS_RNGREQ       10
#define RS_RNGREQ       20
#define RELAY_MS_RNGREQ 30

using namespace std;
using namespace MR_Burst_NT;

namespace MR_Common_NT {

	class Pkthdr {
		public:
			struct p_hdr p_hdr;
			struct s_exthdr exthdr;
			short flag;        /*  PF_SEND , PF_RECV */
			uint8_t type;
            // Command Type: 1 for Burst,
            //               2 for notify PHY
            //               3 for Ranging info
            //               4 for DL relay burst
            //	             5 for UL relay burst
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

			RangCodeInfo(int len,int symOff,int chOff,int method)
			{
				rangingCode = NULL;
				type = RANGING_INFO;
                codeLen = len;
                rangSymOffset = symOff;
                rangChOffset = chOff;
                rangMethod = method;
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
            bool relay_ind;     // indicate whether relay from RS or not

			ntfyDLMAP()
			{
				type = NOTIFY_PHY;
                relay_ind = false;
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

	class RangingObject {
		public:
			uint32_t rangingFrameNumber;
			uint8_t rangingCodeIndex;
			uint8_t rangingSymbol;
			uint8_t rangingSubchannel;
			uint8_t rangingUsage;
			uint64_t recv_time;
			bool collision;
			bool allocated; // if ranging status is success, this flag will be on. (CDMA Allocation)

			RangingObject(uint8_t pCodeIndex, uint32_t pFrameNumber, uint8_t pSymbol, uint8_t pSubchannel, uint8_t pUsage, uint64_t pTime)
			{
				rangingCodeIndex    = pCodeIndex;
				rangingFrameNumber  = pFrameNumber;
				rangingSymbol       = pSymbol;
				rangingSubchannel   = pSubchannel;
				rangingUsage        = pUsage;
				recv_time           = pTime;
				collision           = false;
				allocated           = false;
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
struct msInfo{
    uint8_t  nid;
    uint8_t  pid;
    uint8_t  diuc;  // from RS->MS
    uint8_t  uiuc;  // from MS->RS
};

/* for LogInfo */
typedef struct LogInfo {
	uint32_t src_nid;
	uint32_t dst_nid;
	int burst_len;
	int channel_id;
} loginfo_t;

#endif                /* __NCTUNS_80216J_NT_COMMON_H__    */
