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

#ifndef __NCTUNS_80216E_OFDMA_PMPMS_H__
#define __NCTUNS_80216E_OFDMA_PMPMS_H__

#include <stdio.h>
#include <vector>
#include <assert.h>
#include <object.h>
#include <nctuns_api.h>
#include <con_list.h>
#include <nodetype.h>
#include <event.h>
#include <timer.h>
#include "ofdma_80216e.h"
#include "../mac_16e/structure.h"
#include "../mac_16e/common.h"
#include "../mac_16e/library.h"

using namespace std;
using namespace mobileCommon;

class OFDMA_PMPMS:public OFDMA_80216e {
	private:
		timerObj                *_recvTimer;
		ePacket_                *_buffered_ePacket;
		char                    *_burstPtr;
		char                    *_frameEndPtr;

		vector<dlmapinfo *>     _DLMapInfo;
		struct DLFP_except_128  _framePrefix;
		int                     _currProcIE;
		uint64_t                _frameStartTime;

		enum RecvState {Idle, ProcDLFP, ProcIE} recvState;

		void    sendToBS        (Packet *, struct LogInfo *);
		void    recvHandler     ();
		int     procCmdFromMAC  (Pkthdr *);
		int     procRNGCode     (Pkthdr *);
		void    decodeDLFP      (char *, struct DLFP_except_128 *);

	public:
		OFDMA_PMPMS(uint32_t type, uint32_t id, struct plist *pl, const char *name);
		~OFDMA_PMPMS();

		int     init    ();
		int     recv    (ePacket_ *);
		int     send    (ePacket_ *);
		int     skipBufferedPkt();
		void    dump_buf(int, int);
};

#endif                /* __NCTUNS_80216E_OFDMA_PMPMS_H__ */
