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

#ifndef __NCTUNS_80216J_SCHEDULER_H__
#define __NCTUNS_80216J_SCHEDULER_H__

#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <vector>

#include "mac802_16j_pmpbs.h"
#include "mac802_16j_pmpms.h"
#include "mac802_16j_pmprs.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../burst.h"
#include "../../phy_16j/ofdma_80216j.h"

#define MAX_UL_RATIO 0.5
#define MAX_UL_RELAY_ZONE_RATIO 0.5
#define MAX_TRANSPARENT_ZONE_RATIO 0.5

using namespace std;
using namespace mobileRelayMacAddress;
using namespace mobileRelayServiceFlow;
using namespace mobileRelayBurst;


class mac802_16j_PMPBS;
class mac802_16j_PMPMS;
class mac802_16j_PMPRS;

class BS_16j_Scheduler {
	private:
		mac802_16j_PMPBS    *Station;
		vector<upBurst *>   *upBurstCollection;

		downBurst   *GetCorrespondBurst (vector<downBurst *> *, int);
		int         DLAccessScheduling    (vector<downBurst *> *, int);
		int         TransparentScheduling (vector<downBurst *> *,   int);
		void        generate_DL_MAP     (vector<downBurst *> *, vector<downBurst *> *, int);
		void        generate_UL_MAP     (vector<upBurst *> *,   int, int);
		int         ContentionScheduling(vector<upBurst *> *,   int);
		int         CDMAallocScheduling (vector<upBurst *> *,   int);
		int         UGSScheduling_MSs   (vector<upBurst *> *,   int);
		int         UGSScheduling_RSs   (vector<upBurst *> *,   int);
		int         BEScheduling_MSs        (vector<upBurst *> *,   int);
		int         BEScheduling_RSs        (vector<upBurst *> *,   int);

	public:
		int ULallocStartTime;
		int ULrelayallocStartTime;
		int DLtransparentStartTime;
		int DLsymbols;
		vector<downBurst *> *downTransparentBurstCollection;
		BS_16j_Scheduler(mac802_16j_PMPBS *pBS)
		{
			Station = pBS;
			upBurstCollection = new vector<upBurst *>;
			downTransparentBurstCollection = new vector<downBurst *>;
		}

		~BS_16j_Scheduler()
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

			if (downTransparentBurstCollection != NULL)
                        {
                                while (!downTransparentBurstCollection->empty())
                                {
                                        delete *(downTransparentBurstCollection->begin());
                                        downTransparentBurstCollection->erase(downTransparentBurstCollection->begin());
                                }
                                delete downTransparentBurstCollection;
                        }
		}

		vector<WiMaxBurst *> *Scheduling();
		void generate_UCD();
		void generate_DCD();
		void generate_MOB_NBRADV();
		int  SearchULTime(int,int);
		void mapComputeSymCh(int, uint8_t *, uint8_t *);
		int  computeNearestSlots(int);
};

/* ------------------------------------------------------------------------------------ */

class MS_16j_Scheduler {
	private:
		mac802_16j_PMPMS *Station;

	public:
		MS_16j_Scheduler(mac802_16j_PMPMS *pMS)
		{
			Station = pMS;
		}

		vector<WiMaxBurst *> *ULaccess_Scheduling();
};

/* -------------------------------------------------------------------------------------- */
class RS_16j_Scheduler {
        private:
                mac802_16j_PMPRS *Station;

        public: 
                RS_16j_Scheduler(mac802_16j_PMPRS *pRS)
                {
                        Station = pRS;
                }

                vector<WiMaxBurst *> *ULrelay_Scheduling();
};

#endif  /* __NCTUNS_80216J_SCHEDULER_H__  */
