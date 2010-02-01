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

#ifndef __NCTUNS_80216J_OFDMA_PMPMS_MR_H__
#define __NCTUNS_80216J_OFDMA_PMPMS_MR_H__

#include <stdio.h>
#include <vector>
#include <assert.h>
#include <object.h>
#include <nctuns_api.h>
#include <con_list.h>
#include <nodetype.h>
#include <event.h>
#include <timer.h>
#include "ofdma_80216j.h"
#include "../mac_16j/structure.h"
#include "../mac_16j/common.h"
#include "../mac_16j/library.h"
#include "../mac_16j/pmp/mac802_16j_pmpms.h"

using namespace std;
using namespace mobileRelayCommon;

class OFDMA_PMPMS_MR:public OFDMA_80216j {
	private:
		timerObj                *_recvTimer;
		ePacket_                *_buffered_ePacket;
		char                    *_burstPtr;
		char			*_burstPtr_trans;
		char                    *_frameEndPtr;

		vector<dlmapinfo *>     _DLMapInfo_access;
		vector<dlmapinfo *>     _DLMapInfo_transparent;
		struct DLFP_except_128  _framePrefix;
		int                     _currProcIE;
		uint64_t                _frameStartTime;

		enum RecvState {Idle, ProcDLFP, ProcAccessIE, ProcTransIE} recvState;

		void    sendToSuperStations        (Packet *, struct LogInfo *);
		void    recvHandler     ();
		int     procCmdFromMAC  (Pkthdr *);
		int     procRNGCode     (Pkthdr *);
		void    decodeDLFP      (char *, struct DLFP_except_128 *);

	public:
		OFDMA_PMPMS_MR(uint32_t type, uint32_t id, struct plist *pl, const char *name);
		~OFDMA_PMPMS_MR();

		int     init    ();
		int     recv    (ePacket_ *);
		int     send    (ePacket_ *);
		int     skipBufferedPkt();
		void    resetState();
		void    dump_buf(int, int);
};

#endif                /* __NCTUNS_80216J_OFDMA_PMPMS_MR_H__ */
