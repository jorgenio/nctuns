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

#ifndef __NCTUNS_WIMAX_SCHEDULER_H__
#define __NCTUNS_WIMAX_SCHEDULER_H__

#include <vector>

#include "mac802_16_pmpbs.h"
#include "mac802_16_pmpss.h"
#include "../burst.h"

#define MAX_UL_RATIO	0.5

class BSScheduler {

    private:
    int allocStartTime;
    mac802_16_PMPBS*    Station;
    vector<upBurst*>*   upBurstCollection;

    downBurst* GetCorrespondBurst(vector<downBurst*>*, int);

    void generate_DL_MAP(vector<downBurst*>*);
    void generate_UL_MAP(vector<upBurst*>*, int);

    int DLScheduling(vector<downBurst*>*, int);
    int ContentionScheduling(vector<upBurst*>*, int);
    int UGSScheduling(vector<upBurst*>*, int);
    int BEScheduling(vector<upBurst*>*, int);

    public:

    BSScheduler(mac802_16_PMPBS* pBS) {
        
        Station = pBS;
        upBurstCollection = new vector <upBurst*>;

    }

    ~BSScheduler() {
    
        if (upBurstCollection) {
            
            while (!upBurstCollection->empty()) {
                
                delete *(upBurstCollection->begin());
                upBurstCollection->erase(upBurstCollection->begin());
            }
            
            delete upBurstCollection;
            
        }
    }
    
    vector <WiMaxBurst*>* Scheduling();
    void generate_UCD();
    void generate_DCD();
    int  SearchULIE( struct OFDM_ULMAP_IE& ie);
};

/* ------------------------------------------------------------------------------------ */

class SSScheduler {

    private:
    mac802_16_PMPSS * Station;

    public:

    SSScheduler(mac802_16_PMPSS * pSS) {

        Station = pSS;

    }

    vector < WiMaxBurst * >*Scheduling();

};

#endif  /* __NCTUNS_WIMAX_SCHEDULER_H__ */
