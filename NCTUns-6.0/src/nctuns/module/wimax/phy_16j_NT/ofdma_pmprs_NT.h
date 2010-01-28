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

#ifndef __NCTUNS_80216E_OFDMA_PMPRS_NT_H__
#define __NCTUNS_80216E_OFDMA_PMPRS_NT_H__

#include <stdio.h>
#include <vector>
#include <assert.h>
#include <object.h>
#include <nctuns_api.h>
#include <con_list.h>
#include <nodetype.h>
#include <event.h>
#include <timer.h>
#include "ofdma_80216j_NT.h"
#include "../mac_16j_NT/structure.h"
#include "../mac_16j_NT/common.h"
#include "../mac_16j_NT/library.h"

using namespace std;
using namespace MR_Common_NT;

class OFDMA_PMPRS_NT:public OFDMA_80216j_NT {
	private:
		timerObj                *_recvTimer;
		ePacket_                *_buffered_ePacket;
		char                    *_burstPtr;
		char                    *_burstPtr_relay;
		char                    *_frameEndPtr;
        char                    *_dlmapBuf_NT;
        int                     _dlmapLen_NT;
        struct PHYInfo          LastSignalInfo;

		vector<dlmapinfo *>     _DLMapInfo;
		vector<dlmapinfo *>     _DLMapInfo_access;
		vector<dlmapinfo *>     _DLMapInfo_relay;

        struct DLFP_except_128  _framePrefix;
		int                     _currProcIE;
		uint64_t                _frameStartTime;

        bool                    relay_flag;
        bool                    relay_burst_recv;
		
		enum RecvState {Idle, ProcRzonePrefix, ProcRelayIE} recvState;

		void    sendToBS        (Packet *, struct LogInfo *);
		void    recvHandler_superordinate     ();
		void    recvHandler_subordinate (ePacket_ *);
		int     procCmdFromMAC  (Pkthdr *);
		int     procRNGCode     (Pkthdr *);
		void    decodeRzonePrefix      (char *, struct RzonePrefix_except_128 *);
        void    sendToSubStations (Packet *, struct LogInfo *);
        void    buildDLFP       (struct DLFP_except_128 *, int);

	public:
		OFDMA_PMPRS_NT(uint32_t type, uint32_t id, struct plist *pl, const char *name);
		~OFDMA_PMPRS_NT();

		int     init    ();
		int     recv    (ePacket_ *);
		int     send    (ePacket_ *);
		int     skipBufferedPkt();
		void    dump_buf(int, int);
        void    saveDLMAP(char *, int);

};

#endif                /* __NCTUNS_80216E_OFDMA_PMPRS_NT_H__ */
