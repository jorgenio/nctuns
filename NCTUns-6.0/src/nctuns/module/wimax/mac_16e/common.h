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

#ifndef __NCTUNS_80216E_COMMON_H__
#define __NCTUNS_80216E_COMMON_H__

#include <stdio.h>
#include <vector>
#include <packet.h>
#include "structure.h"
#include "burst.h"

using namespace std;
using namespace mobileBurst;

namespace mobileCommon {
	class Pkthdr {
		public:
			struct p_hdr p_hdr;
			struct s_exthdr exthdr;
			short flag;        /*  PF_SEND , PF_RECV */
			short type;        // Command Type: 1 for Burst, 2 for notify PHY
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

			RangCodeInfo()
			{
				rangingCode = NULL;
				type = 3;
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

			ntfyDLMAP()
			{
				type = 2;
			}
	};

	class BurstInfo: public Pkthdr {
		public:
			vector<WiMaxBurst *> *Collection;

			BurstInfo()
			{
				type        = 1;
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

/* for LogInfo */
typedef struct LogInfo {
	uint32_t src_nid;
	uint32_t dst_nid;
	int burst_len;
	int channel_id;
} loginfo_t;

#endif                /* __NCTUNS_80216E_COMMON_H__    */
