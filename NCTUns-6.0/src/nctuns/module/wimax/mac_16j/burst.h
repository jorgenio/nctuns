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

#ifndef __NCTUNS_80216j_BURST_H__
#define __NCTUNS_80216j_BURST_H__

#include <vector>
#include <list>

#include "structure.h"
#include "connection.h"
#include "ofdma_map_ie.h"
#include "../phy_16j/ofdma_80216j.h"

using namespace std;
using namespace mobileRelayConnection;
using namespace mobileRelayOFDMAMapIE;

namespace mobileRelayBurst {
	class AllocInfo {
		public:
			Connection *conn;
			int len;
			vector<int> pduInfo;
			uint32_t position;

			AllocInfo(Connection *c, int l, vector<int> p)
			{
				conn = c;
				len = l;
				pduInfo = p;
				position = 0;
			}

			inline int getNext()
			{ 
				return pduInfo[position]; 
			}
	};

	class WiMaxBurst {
		protected:
			char _fec;

			WiMaxBurst(char fec)
			{
				_fec = fec;
				payload = NULL;
				length = 0;
			}

		public:
			int length;        // in Bytes
			char *payload;
			list<AllocInfo *> ConnectionCollection;

			virtual ~WiMaxBurst()
			{
				AllocInfo *a = NULL;
				while (ConnectionCollection.size())
				{
					a = ConnectionCollection.back();
					delete a;
					ConnectionCollection.pop_back();
				}

				if (payload != NULL)
				{
					delete [] payload;
				}
			}

			inline int toSlots(int bytes)
			{
				return bytesToSlots(bytes, _fec);
			}

			inline int toByte(int slots)
			{
				return slotsToBytes(slots, _fec);
			}

			static int bytesToSlots(int bytes, int fec)
			{
				static int uncodedSizePerSlot[] = {6, 9, 12, 18, 18, 24, 27};

				if (bytes == 0)
					return 0;

				if (bytes % uncodedSizePerSlot[fec] == 0)
					return (bytes / uncodedSizePerSlot[fec]);
				else
					return (bytes / uncodedSizePerSlot[fec]) + 1;
			}

			static int slotsToBytes(int slots, int fec)
			{
				static int uncodedSizePerSlot[] = {6, 9, 12, 18, 18, 24, 27};

				return (slots * uncodedSizePerSlot[fec]);
			}
	};

	class downBurst:public WiMaxBurst {
		public:
			OFDMA_DLMAP_IE  *ie;
			DLMAP_IE_u      dlmap_ie;
			int             duration;        // in slots
			int             nCid;
			uint16_t        cid[MaxCID];	// MaxCID = 0xFFFF

			downBurst(char diuc, char fec)
				: WiMaxBurst(fec)
			{
				length      = 0;
				payload     = NULL;
				ie          = new OFDMA_DLMAP_IE(diuc);
				memset(&dlmap_ie, 0x00, sizeof(DLMAP_IE_u));
				duration    = 0;
				nCid        = 0;
				memset(cid, 0x00, MaxCID * sizeof(uint16_t));
			}

			~downBurst()
			{
				if (ie != NULL)
				{
					if ((ie->_diuc != Extended_DIUC_2 && ie->_diuc != Extended_DIUC) && (nCid != 0))
					{
						delete [] dlmap_ie.ie_other.cid;
					}
					delete ie;
				}
			}

			inline void addCID(uint16_t pcid)
			{
				for(int i = 0;i < nCid;i++)
				{
					if(cid[i] == pcid)
					{
						return;
					}
				}

				cid[nCid] = pcid;
				nCid++;
			}

			inline void encapsulateAllField()
			{
				if (ie->_diuc != Extended_DIUC_2 && ie->_diuc != Extended_DIUC)
				{
					ie->mallocIE(nCid);
					ie->appendBitField( 4, dlmap_ie.ie_other.diuc);
					ie->appendBitField( 8, dlmap_ie.ie_other.numCid);
					for (int i = 0;i < nCid;i++)
						ie->appendBitField(16, dlmap_ie.ie_other.cid[i]);
					ie->appendBitField( 8, dlmap_ie.ie_other.symOff);
					ie->appendBitField( 6, dlmap_ie.ie_other.chOff);
					ie->appendBitField( 3, dlmap_ie.ie_other.boosting);
					ie->appendBitField( 7, dlmap_ie.ie_other.numSym);
					ie->appendBitField( 6, dlmap_ie.ie_other.numCh);
					ie->appendBitField( 2, dlmap_ie.ie_other.repeCode);
				}

				if(ie->_ie_bits % 8 != 0)
				{
					ie->_ie_data[ie->_ie_bits / 8] >>= (8 - ie->_ie_bits % 8);
				}
			}
	};

	class upBurst:public WiMaxBurst {
		public:
			OFDMA_ULMAP_IE  *ie;
			ULMAP_IE_u      ulmap_ie;

			upBurst(ULMAP_IE_u *pie, char fec)
				: WiMaxBurst(fec)
			{
				length = 0;
				payload = NULL;
				memcpy(&ulmap_ie, pie, sizeof(ULMAP_IE_u));
				ie = new OFDMA_ULMAP_IE(ulmap_ie.ie_14.cid, ulmap_ie.ie_14.uiuc);
				encapsulateAllField();
			}

			upBurst(uint16_t cid, uint8_t uiuc, char fec)
				: WiMaxBurst(fec)
			{
				length = 0;
				payload = NULL;
				ie = new OFDMA_ULMAP_IE(cid, uiuc);
			}

			~upBurst()
			{
				if (ie != NULL)
				{
					delete ie;
				}
			}

			inline void encapsulateAllField()
			{
				if (ie->_uiuc == CDMA_BWreq_Ranging) //CDMA ranging code IE
				{
					ie->mallocIE(0);
					ie->appendBitField(16, ulmap_ie.ie_12.cid);
					ie->appendBitField( 4, ulmap_ie.ie_12.uiuc);
					ie->appendBitField( 8, ulmap_ie.ie_12.symOff);
					ie->appendBitField( 7, ulmap_ie.ie_12.chOff);
					ie->appendBitField( 7, ulmap_ie.ie_12.numSym);
					ie->appendBitField( 7, ulmap_ie.ie_12.numCh);
					ie->appendBitField( 2, ulmap_ie.ie_12.rangMethod);
					ie->appendBitField( 1, ulmap_ie.ie_12.rangIndicator);
				}
				else if (ie->_uiuc == CDMA_Alloc_IE)	//MR-BS allocates to RS/MS initial RNG-REQ
				{
					ie->mallocIE(0);
					ie->appendBitField(16, ulmap_ie.ie_14.cid);
					ie->appendBitField( 4, ulmap_ie.ie_14.uiuc);
					ie->appendBitField( 6, ulmap_ie.ie_14.duration);
					ie->appendBitField( 4, ulmap_ie.ie_14.uiuc_trans);
					ie->appendBitField( 2, ulmap_ie.ie_14.repeCode);
					ie->appendBitField( 4, ulmap_ie.ie_14.frameIndex);
					ie->appendBitField( 8, ulmap_ie.ie_14.rangCode);
					ie->appendBitField( 8, ulmap_ie.ie_14.rangSym);
					ie->appendBitField( 7, ulmap_ie.ie_14.rangCh);
					ie->appendBitField( 1, ulmap_ie.ie_14.bwReq);
					ie->appendBitField( 16, ulmap_ie.ie_14.relayRScid);

				}
				else	//normal traffics
				{
					ie->mallocIE(0);
					ie->appendBitField(16, ulmap_ie.ie_other.cid);
					ie->appendBitField( 4, ulmap_ie.ie_other.uiuc);
					ie->appendBitField(10, ulmap_ie.ie_other.duration);
					ie->appendBitField( 2, ulmap_ie.ie_other.repeCode);

				}

				if(ie->_ie_bits % 8 != 0)
				{
					ie->_ie_data[ie->_ie_bits / 8] >>= (8 - ie->_ie_bits % 8);
				}
			}
	};
}

#endif                /* __NCTUNS_80216j_BURST_H__ */
