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

#ifndef __NCTUNS_80216J_NT_SCHEDULER_H__
#define __NCTUNS_80216J_NT_SCHEDULER_H__

#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <vector>

#include "mac802_16j_NT_pmpbs.h"
#include "mac802_16j_NT_pmpms.h"
#include "mac802_16j_NT_pmprs.h"
#include "../mac_address.h"
#include "../service_flow.h"
#include "../burst.h"
#include "../../phy_16j_NT/ofdma_80216j_NT.h"

#define MAX_UL_RATIO 0.5
#define MAX_DL_RELAY_RATIO 0.5
#define MAX_UL_RELAY_RATIO 0.5

using namespace std;
using namespace MR_MacAddress_NT;
using namespace MR_ServiceFlow_NT;
using namespace MR_Burst_NT;

class mac802_16j_NT_PMPBS;
class mac802_16j_NT_PMPMS;
class mac802_16j_NT_PMPRS;

class Scheduler_NT{
    private:
        struct DCDBurstProfile  DCDProfile[16];    // Index: DIUC

    public:

		int ULallocStartTime;
		int RelayULallocStartTime;
		int RelayDLallocStartTime;  // R-zone location
        int DLsymbols;
        
        downBurst   *GetCorrespondBurst (vector<downBurst *> *, int);
        
        void mapComputeSymCh(int, uint8_t *, uint8_t *);
        int  computeNearestSlots(int);
        void Clear_UL_BurstCollection(vector <upBurst *> *);
        void Clear_DL_BurstCollection(vector <downBurst *> *);

        Scheduler_NT() 
        {
            ULallocStartTime = 0;
            RelayULallocStartTime = 0;
            RelayDLallocStartTime = 0;
            DLsymbols = 0;

            for (int i = 0, fec = QPSK_1_2; i <= 12 && fec <= QAM64_3_4; i++, fec++)    // Spec 8.4.5.3.1 Table 276
            {
                // Spec 11.4.2 Table 363
                DCDProfile[i].used  = 1;
                DCDProfile[i].fec   = fec;
            }
        }

};

class BS_16j_NT_Scheduler : public Scheduler_NT{
	private:
		mac802_16j_NT_PMPBS    *Station;
		
        vector <upBurst *>   *upBurstCollection;
		vector <upBurst *>   *upRelayBurstCollection;


        int         DLAccess_Scheduling  (vector<downBurst *> *, int);
		int         DLRelay_Scheduling   (vector<downBurst *> *, int);

        void        generate_DL_MAP     (vector<downBurst *> *, int);
		void        generate_UL_MAP     (vector<upBurst *> *,   int, int);

        int         ContentionScheduling(vector<upBurst *> *,   int);
		int         CDMAallocScheduling (vector<upBurst *> *,   int);
		int         UGSScheduling       (vector<upBurst *> *,   int);
		int         BEScheduling        (vector<upBurst *> *,   int);
		
        /* 16j scheduling*/
		void        generate_R_MAP      (vector<downBurst *> *, int);
		void        generate_UL_MAP_relay     (vector<upBurst *> *,   int, int);
	    int         ULAccessScheduling  (vector <upBurst *>*,int);
	    int         ULRelayScheduling   (vector <upBurst *>*,int);

        int         ULRelay_ContentionScheduling(vector<upBurst *> *,   int);
		int         ULRelay_CDMAallocScheduling (vector<upBurst *> *,   int);
		int         ULRelay_UGSScheduling       (vector<upBurst *> *,   int);
		int         ULRelay_BEScheduling        (vector<upBurst *> *,   int);

        // record which BR header is received 
        int        br_header_type2_ext[8];
        vector <uint8_t> *rs_br_hdr_tid;
        vector <upBurst *> *ulmap_ie_queue;
        int num_relay_ir;   
        int num_relay_ho;   
        int num_relay_br;   
        bool relay_diuc[16];

	public:
        vector <downBurst *> *DLRelayBurstCollection;

		BS_16j_NT_Scheduler(mac802_16j_NT_PMPBS *pBS)
		{
			Station = pBS;
			upBurstCollection       = new vector<upBurst *>;
			upRelayBurstCollection  = new vector<upBurst *>;
			ulmap_ie_queue          = new vector<upBurst *>;
            rs_br_hdr_tid           = new vector<uint8_t>;

            num_relay_ir = 0;   
            num_relay_ho = 0;   
            num_relay_br = 0;   

            for(int i=0; i<8 ; i++)
                br_header_type2_ext[i] = 0;

            for(int i=0 ; i< 16 ; i++)
                relay_diuc[i] = false;
        }

		~BS_16j_NT_Scheduler()
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
            
            if(upRelayBurstCollection!=NULL)
            {
                while(!upRelayBurstCollection->empty())
                {
                    delete *(upRelayBurstCollection->begin());
                    upRelayBurstCollection->erase(upRelayBurstCollection->begin());
                }
                delete upRelayBurstCollection;
            }

            if(ulmap_ie_queue!=NULL)
            {
                while(!ulmap_ie_queue->empty())
                {
                    delete *(ulmap_ie_queue->begin());
                    ulmap_ie_queue->erase(ulmap_ie_queue->begin());
                }
                delete ulmap_ie_queue;
            }
            
            if(rs_br_hdr_tid != NULL)
            {
                while(!rs_br_hdr_tid->empty())
                {
                    rs_br_hdr_tid->erase(rs_br_hdr_tid->begin());
                }
                delete rs_br_hdr_tid;
            }
		}

		vector<WiMaxBurst *> *Scheduling();
		void generate_UCD();
		void generate_DCD();
		void generate_UCD_relay(rsObject_NT *);
		void generate_DCD_relay(rsObject_NT *);
		void generate_RCD(rsObject_NT *);
        void generate_RS_AccessMAP(rsObject_NT *);
        void procBR_header(BR_Connection *);
		void generate_MOB_NBRADV();
		int  SearchULTime(int);

		int  SearchULRelayTime(int);

};

/* ------------------------------------------------------------------------------------ */
/*                                                                                      */ 
/*                                  MS Scheduler                                        */
/*                                                                                      */
/* ------------------------------------------------------------------------------------ */
class MS_16j_NT_Scheduler {
	private:
		mac802_16j_NT_PMPMS *Station;

	public:
		MS_16j_NT_Scheduler(mac802_16j_NT_PMPMS *pMS)
		{
			Station = pMS;
		}

		vector<WiMaxBurst *> *Scheduling();
};

/* ------------------------------------------------------------------------------------ */
/*                                                                                      */ 
/*                                  RS Scheduler                                        */
/*                                                                                      */
/* ------------------------------------------------------------------------------------ */
class RS_16j_NT_Scheduler : public Scheduler_NT{
	private:
		mac802_16j_NT_PMPRS *Station;

        void        CDMAallocScheduling (vector<upBurst *> *,   int);
	    void        UGSScheduling       (vector<upBurst *> *);
        int         ContentionScheduling(vector<upBurst *> *,   int);
        void        DLAccess_Scheduling(vector <downBurst *>* ,int);
	
    public:
        vector  <upBurst *>   *RS_ULAccessBurstCollection;
        vector  <rngcode>  saved_ir_code;

        RS_16j_NT_Scheduler(mac802_16j_NT_PMPRS *pRS)
		{
			Station = pRS;
		    RS_ULAccessBurstCollection = new vector<upBurst *>;
        }
		
        ~RS_16j_NT_Scheduler()
		{
            if(RS_ULAccessBurstCollection!=NULL)
            {
                while(!RS_ULAccessBurstCollection->empty())
                {
                    delete *(RS_ULAccessBurstCollection->begin());
                    RS_ULAccessBurstCollection->erase(RS_ULAccessBurstCollection->begin());
                }
                delete RS_ULAccessBurstCollection;
            }
        }
		
        //vector<WiMaxBurst *> *ULScheduling();
		vector<WiMaxBurst *> *UL_Scheduling();
		vector<WiMaxBurst *> *DL_Scheduling();
		
		void        generate_DL_MAP     (vector<downBurst *> *, int);
		void        generate_UL_MAP     (vector<upBurst *> *,   int, int);
		//void        generate_MOB_NBRADV();
		int         SearchULTime(int);
};


#endif  /* __NCTUNS_80216J_NT_SCHEDULER_H__  */
