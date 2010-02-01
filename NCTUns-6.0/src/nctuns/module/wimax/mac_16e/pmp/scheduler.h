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

#ifndef __NCTUNS_80216E_SCHEDULER_H__
#define __NCTUNS_80216E_SCHEDULER_H__

#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <vector>

#include "mac802_16e_pmpbs.h"
#include "mac802_16e_pmpms.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../burst.h"
#include "../../phy_16e/ofdma_80216e.h"

#define MAX_UL_RATIO 0.5

using namespace std;
using namespace mobileMacAddress;
using namespace mobileServiceFlow;
using namespace mobileBurst;

class mac802_16e_PMPBS;
class mac802_16e_PMPMS;

class BS_16e_Scheduler {
	private:
		mac802_16e_PMPBS    *Station;
		vector<upBurst *>   *upBurstCollection;

		downBurst   *GetCorrespondBurst (vector<downBurst *> *, int);
		int         DLScheduling        (vector<downBurst *> *, int);
		void        generate_DL_MAP     (vector<downBurst *> *, int);
		void        generate_UL_MAP     (vector<upBurst *> *,   int, int);
		int         ContentionScheduling(vector<upBurst *> *,   int);
		int         CDMAallocScheduling (vector<upBurst *> *,   int);
		int         UGSScheduling       (vector<upBurst *> *,   int);
		int         BEScheduling        (vector<upBurst *> *,   int);

	public:
		int ULallocStartTime;

		BS_16e_Scheduler(mac802_16e_PMPBS *pBS)
		{
			Station = pBS;
			upBurstCollection = new vector<upBurst *>;
		}

		~BS_16e_Scheduler()
		{
			if (upBurstCollection != NULL)
			{
				while (!upBurstCollection->empty())
				{
					delete *(upBurstCollection->begin());
					upBurstCollection->erase(upBurstCollection->begin());
				}
				delete upBurstCollection;
			}
		}

		vector<WiMaxBurst *> *Scheduling();
		void generate_UCD();
		void generate_DCD();
		void generate_MOB_NBRADV();
		int  SearchULTime(int);
		void mapComputeSymCh(int, uint8_t *, uint8_t *);
		int  computeNearestSlots(int);
};

/* ------------------------------------------------------------------------------------ */

class MS_16e_Scheduler {
	private:
		mac802_16e_PMPMS *Station;

	public:
		MS_16e_Scheduler(mac802_16e_PMPMS *pMS)
		{
			Station = pMS;
		}

		vector<WiMaxBurst *> *Scheduling();
};

#endif  /* __NCTUNS_80216E_SCHEDULER_H__  */
