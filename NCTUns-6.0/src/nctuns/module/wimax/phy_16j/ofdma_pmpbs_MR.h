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

#ifndef __NCTUNS_80216J_OFDMA_PMPBS_MR_H__
#define __NCTUNS_80216J_OFDMA_PMPBS_MR_H___

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

using namespace mobileRelayCommon;
using namespace mobileRelayBurst;

class OFDMA_PMPBS_MR:public OFDMA_80216j {
	private:
		enum RecvState {Idle_access,Idle_relay, Busy_access, Busy_transparent} recvState;
		void    sendToSubStations   (Packet *);
		void    recvHandler (Event *);
		void    buildDLFP   (struct DLFP_except_128 *, int);
		void    buildDLFP   (struct DLFP_for_128 *, int);

		timerObj *timerULSwitcher;
		timerObj *timerDLSwitcher;

		void ULzoneSwitch();
		void DLzoneSwitch();

	public:
		OFDMA_PMPBS_MR(uint32_t type, uint32_t id, struct plist *pl, const char *name);
		~OFDMA_PMPBS_MR();

		int init();
		int recv(ePacket_ *);
		int send(ePacket_ *);
		void saveDLMAP(char *, int);
		void setStateBusy();
		void setStateIdle();
};

#endif  /* __NCTUNS_80216J_OFDMA_PMPBS_MR_H__ */
