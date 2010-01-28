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

#include "GPRS_bts_mac.h"
#include <iostream>
#include <stdlib.h>
#include <nctuns_api.h>
#include <timer.h>
#include <mbinder.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>
#include <gprs/include/burst_type.h>
#include <gprs/include/GPRS_rlcmac_message.h>
#include <gprs/include/partial_queue.h>
#include <gprs/rlc/rlc.h>
#include <gprs/rlc/rlc_impl.h>
#include <gprs/rlc/rlc_shared_def.h>
#include <gprs/rlc/tcb.h>
#include <gprs/llc/llc.h>
#include <gprs/radiolink/radiolink.h>

#include "mac_header.h"
#include "mac_shared_def.h"
//#define __DETAILED_BTS_DEBUG
//#define __BLK_PARTITION_DETAILED_DEBUG
//#define __TICK_OBSERVE_
//#define __USF_DEBUG
//#define __SEQUENCE_DEBUG
//#define __DUMP_ENCODING_DATA
//#define __OTHER_BTS_SEND_OBSERVE_DEBUG
//#define __MODIFY_TLLI_DEBUG
//#define __DETAILED_INFO
    using namespace std;
    MODULE_GENERATOR(GprsBtsMac)

    inline int GprsBtsMac::is_idle_frame() {

        if ( (fn == 12) || (fn == 25) || (fn == 38) || (fn == 51) )
            return true;
        else
            return false;

    }

    inline int GprsBtsMac::is_blkn_boundary() {

        if ( fn == 0  || fn == 4  || fn == 8  || fn == 13 || fn == 17 || fn == 21 ||
             fn == 26 || fn == 30 || fn == 34 || fn == 39 || fn == 43 || fn == 47 )
            return true;
        else
            return false;

    }

    inline int GprsBtsMac::is_uplink_blkn_boundary() {

        if ( uplink_fn == 0  || uplink_fn == 4  || uplink_fn == 8  || uplink_fn == 13 || uplink_fn == 17 || uplink_fn == 21 ||
             uplink_fn == 26 || uplink_fn == 30 || uplink_fn == 34 || uplink_fn == 39 || uplink_fn == 43 || uplink_fn == 47 )
            return true;
        else
            return false;

    }

    inline int GprsBtsMac::fn_to_burst_num(ulong fn1) {

        if ( fn1 == 12 || fn1 == 25 || fn1 == 38 || fn1 == 51 )
            return 0;

        else if ( fn1 == 0  || fn1 == 4  || fn1 == 8  || fn1 == 13 || fn1 == 17 || fn1 == 21 ||
             fn1 == 26 || fn1 == 30 || fn1 == 34 || fn1 == 39 || fn1 == 43 || fn1 == 47 )
            return 0;

        else if ( fn1 == 1  || fn1 == 5  || fn1 == 9  || fn1 == 14 || fn1 == 18 || fn1 == 22 ||
             fn1 == 27 || fn1 == 31 || fn1 == 35 || fn1 == 40 || fn1 == 44 || fn1 == 48 )
            return 1;

        else if ( fn1 == 2 || fn1 == 6  || fn1 == 10  || fn1 == 15 || fn1 == 19 || fn1 == 23 ||
             fn1 == 28 || fn1 == 32 || fn1 == 36 || fn1 == 41 || fn1 == 45 || fn1 == 49 )
            return 2;

        else if ( fn1 == 3  || fn1 == 7  || fn1 == 11  || fn1 == 16 || fn1 == 20 || fn1 == 24 ||
             fn1 == 29 || fn1 == 33 || fn1 == 37 || fn1 == 42 || fn1 == 46 || fn1 == 50 )
            return 3;

        else return -1; /* idle slot or illegal fn1 value */

    }

    GprsBtsMac::GprsBtsMac(u_int32_t type, u_int32_t id, struct plist* pl, const char *name) : NslObject(type, id, pl, name) {

        try {
            state       = NOT_READY;
            /* internal timers for synchronization */
            qn = 0; /* quarter bit number */
            bn = 0; /* bit number */
            tn = 0; /* time slot number */
            fn = 0; /* frame number */
            blkn = 0;
            uplink_tn   = -3;
            uplink_fn   = -1;
            uplink_blkn = -1;

            for ( ulong i=0 ; i<125 ; ++i ) {

                for ( ulong j=0 ; j<8 ; ++j )
                    up_down[i][j] = 0;

            }

            /*GPRS log*/
            _ptrlog     = 0;
            log_flag    = 1;


            bzero( trigger_sending_ind, sizeof(uchar)*8 );

            /* BCCH info list */
            bssdes = new BssDes;
            neighbor_cell_list = NULL;


            /* TDMA scheduler */
            sched_timer = new timerObj;

            /* partially established TCBs */
            sbalist = new SbaFreqList;

            /**** send burst queues *****/
            for ( int i=0 ; i<8 ; ++i)
                dummy_burst_squeue[i] = new SList<Event>[125];

            /* PBCCH send queue */
            psi1_squeue  = new SList<Event>;
            pbcch_squeue = new SList<Event>;
            /* PCCCH group send queues */
            ppch_squeue  = new SList<Event>;
            pnch_squeue  = new SList<Event>;
            pagch_squeue = new SList<Event>;

            /**** recv burst queues ****/

            for (int i=0 ; i<8 ;++i) {

                uplink_rq[i] = new partially_assembled_rlcblk[125];

            }


            tlli_cache = new TlliHashCache;
            pedl = new TbfDesList;

            downlink_tsnum = DOWNLINK_ALLOC_TS;
            uplink_tsnum = UPLINK_ALLOC_TS;

            vBind("BSIC",&bsic);
            vBind("RAI",&rai);

            vBind("start_freq_ch",&sfreq);
            vBind("end_freq_ch",&efreq);

            vBind("downlink_ts_num",&downlink_tsnum);
            vBind("uplink_ts_num",&uplink_tsnum);

            vBind("cfg_filename",&bss_cfg_filename);



        }
        catch(std::exception& e) {
            cout << "GprsBtsMac(): ASSERTION failed: "<< e.what() << endl;
        }
    }

    GprsBtsMac::~GprsBtsMac() {

        if ( bssdes) delete bssdes;
        if ( neighbor_cell_list ) delete neighbor_cell_list;

        //if ( radiolink_obj ) delete radiolink_obj;
	radiolink_obj = NULL;

        /* partially established TCBs */
        if ( sbalist ) delete sbalist;

        /* TDMA scheduler */
        if ( sched_timer ) delete sched_timer;

        /**** send burst queues *****/
        for ( int i= 0 ; i<8 ; ++i ) {
            if ( dummy_burst_squeue[i] )
                delete[] dummy_burst_squeue[i];
            if ( uplink_rq[i] )
                delete[] uplink_rq[i];
        }

        if ( psi1_squeue )  delete psi1_squeue;
        if ( pbcch_squeue ) delete pbcch_squeue;
        if ( ppch_squeue )  delete ppch_squeue;
        if ( pnch_squeue )  delete pnch_squeue;
        if ( pagch_squeue ) delete pagch_squeue;

        if ( tlli_cache )   delete tlli_cache;
        if ( pedl )         delete pedl;

    }

    int GprsBtsMac::init() {

        /* bss management parameters shall be specified by users via Module Configuration
         * dialog.
         */

        sched_timer->init();
        // Disabling the following statement can shutdown BSS:
        sched_timer->setCallOutObj(this, (int (NslObject::*)(Event*)) &GprsBtsMac::update_internal_counters );

        radiolink_obj = reinterpret_cast<radiolink*> (sendtarget_->bindModule());
        IS_NULL_STR(radiolink_obj,"GprsMsMac constructor(): the radiolink obj is not found.\n", -1);


        u_int64_t tick;
        NANO_TO_TICK( tick , BIT_PERIOD);
        printf("BTS MAC [%u]: set a BIT_PERIOD as %lld ticks. A tick = %d ns.\n", get_nid(), tick, TICK );
        sched_timer->start(tick,tick);

        //#define __STATIC_ALLOCATION_FOR_BSS_RESOURCES__
        #ifdef  __STATIC_ALLOCATION_FOR_BSS_RESOURCES__

            /* Allocate radio resources to BSS */
            bssdes->start_ch  = 1;
            bssdes->end_ch    = 5;


            for ( int i=bssdes->start_ch; i<=bssdes->end_ch ;++i) {
                bssdes->ch_map[i]                               = new TaAllocation; /* uplink channels */
                bssdes->ch_map[CORRESPONDING_DOWNLINK_CH(i)]    = new TaAllocation; /* downlink channels */
            }

            bssdes->bcch_no  = CORRESPONDING_DOWNLINK_CH(bssdes->start_ch); /* the first downlink channel */
            cout <<"Set bssdes->bcch_no = "<< (ulong)(bssdes->bcch_no) << endl;
            bssdes->pbcch_tn = 1;

            bssdes->bs_pbcch_blks     = 3;
            bssdes->bs_pag_blks_res   = 3;
            bssdes->bs_prach_blks     = 0;

        #else

            /* Allocate the resources for a base station from the
             * specified configuration file.
             */


            neighbor_cell_list = bsscfg_parser( get_nid() , bss_cfg_filename );

            bssdes = neighbor_cell_list->get_head_elem();
            ASSERTION( bssdes , "GprsBtsMac init(): bssdes is null.\n" );


            for ( int i=bssdes->start_ch; i<=bssdes->end_ch ;++i) {
                bssdes->ch_map[i]                               = new TaAllocation; /* uplink channels */
                bssdes->ch_map[CORRESPONDING_DOWNLINK_CH(i)]    = new TaAllocation; /* downlink channels */
            }

            bssdes->bs_pbcch_blks     = 3;
            bssdes->bs_pag_blks_res   = 3;
            bssdes->bs_prach_blks     = 0;



        #endif

        printf("BTS MAC [%u]: Set Downlink/Uplink time slot numbers for a MS = %d/%d \n", get_nid(),
            downlink_tsnum, uplink_tsnum );

        if ( (sfreq != bssdes->start_ch) || (efreq!=bssdes->end_ch) ) {

            printf("Warning: The parameters for channel assignment in .tcl and .bsscfg files are not the same.\n");
            exit(1);

        }


        /*GPRS log*/
        if ( WirelessLogFlag && !strcasecmp(GPRSLogFlag, "on") ) {
                if ( !ptrlogFileOpenFlag ) {
                        ptrlogFileOpenFlag = true;

                        char    *ptrFile;
                        if( ptrlogFileName ) {
                                ptrFile = (char *)malloc(strlen
                                        (GetConfigFileDir())+strlen
                                        (ptrlogFileName) + 1);
                                sprintf(ptrFile,"%s%s",GetConfigFileDir(),
                                                        ptrlogFileName);
                                fptr = fopen(ptrFile, "w+");
                                free(ptrFile);
                        }
                        else {
                                ptrFile = (char *)malloc(strlen
                                        (GetScriptName())+5);
                                sprintf(ptrFile,"%s.ptr",GetScriptName());
                                fptr = fopen(ptrFile, "w+");
                                free(ptrFile);
                        }

                        if( fptr == NULL ) {
                                printf("Error : Can't create file %s\n",ptrFile);
                                exit(-1);
                        }

                        Event_ *heapHandle = createEvent();
                        u_int64_t time;
                        MILLI_TO_TICK(time, 100);
                        u_int64_t chkInt = GetCurrentTime() + time;
                        setEventTimeStamp(heapHandle, chkInt, 0);

                        int (*__fun)(Event_ *) =
                        (int (*)(Event_ *))&DequeueLogFromHeap;;
                        setEventCallOutFunc(heapHandle, __fun, heapHandle);
                        scheduleInsertEvent(heapHandle);
                }
                _ptrlog = 1;
        }


        return NslObject::init();
    }

    int GprsBtsMac::update_blkn() {
        int blkn_changed;
        int old_blkn = blkn;

        if ( fn>=0 ) {
            if ( fn == 12 || fn == 25 || fn == 38 || fn == 51)
                blkn = -1;
            else
                blkn = ( fn - (fn-(fn/12))/12 ) /4;
        }
        if ( uplink_fn>=0) {
            if ( uplink_fn == 12 || uplink_fn == 25 || uplink_fn == 38 || uplink_fn == 51)
                uplink_blkn = -1;
            else
                uplink_blkn = ( uplink_fn - (uplink_fn-(uplink_fn/12))/12 ) /4;
        }

        if ( old_blkn != blkn)
            blkn_changed = true;
        else
            blkn_changed = false;

        return blkn_changed;
    }

    int GprsBtsMac::update_internal_counters() {
        /* update internal counters */
        bool tn_boundary        = false;

        ++bn;
        if ( bn>=156 ) {

            bn=0;
            ++tn;
            ++uplink_tn;
            tn_boundary = true;

            if (tn>=8) {
                fn = ++fn %52;
                tn=0;
            }

            if (!(uplink_tn%8)) {
                uplink_tn=0;
                uplink_fn = ++uplink_fn%52;
            }

            if ( uplink_fn >= 0 ) {

                if (state == NOT_READY) {
                    state = READY;
                    #ifdef __DETAILED_BTS_DEBUG
                    cout <<"GprsBtsMac update_internal_counters(): state becomes READY" << endl;
                    #endif
                }
            }

            update_blkn();

            if ( (is_blkn_boundary()) )
                trigger_sending_ind[tn] = true;
            else
                trigger_sending_ind[tn] = false;

            return gprs_scheduler(tn_boundary);

        }

        return 1;

    }

    int GprsBtsMac::blkn_to_starting_fn(long blkn1 ) {
        if ( blkn1 < 0 || blkn1 >51 ) {
            cout << "GprsbtsMac blkn_to_starting_fn(): Assertion failed: blkn < 0 " << endl;
            return -1;
        }
        return blkn1*4+(blkn1/3);
    }

    int GprsBtsMac::send(Event* ep) {

        bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        gmm_option* gmm_opt = reinterpret_cast<gmm_option*>  (bssmsg->sndcp_option);
        rlc_option* rlc_opt = reinterpret_cast<rlc_option*>  (bssmsg->rlc_option);

        TbfDes* utcb = NULL;

        /* inspect the tlli values carried by llc_option */
        llc_option* llc_opt = reinterpret_cast<llc_option*> (bssmsg->llc_option);

        if (llc_opt) {

            /* Close the connection. */
            if ( llc_opt->addtional_info == LLGMM_RESET ) {

                TlliCacheElem* elem = tlli_cache->search_by_tlli(llc_opt->tlli);

                if (elem) {

                    TbfDes* tmp1;
                    TbfDes* tmp2;

                    tmp1 = elem->utcb;
                    tmp2 = elem->dtcb;

                    uchar remove_flag = false;

                    if ( tmp1 ) {

                        tmp1->llgmm_reset_flag = true;

                        if ( tmp1->detach_flag ) {
                            bssdes->ch_map[tmp1->uplink_freq]->remove_tbfdes_entry( tmp1 );
                            remove_flag = true;
                        }
                    }

                    if ( tmp2 ) {

                        tmp2->llgmm_reset_flag = true;

                        if ( tmp2->detach_flag ) {
                            bssdes->ch_map[tmp2->downlink_freq]->remove_tbfdes_entry( tmp2 );
                            remove_flag = true;
                        }

                    }

                    if ( remove_flag ) {

                        tlli_cache->remove( llc_opt->tlli );

                        if ( tmp1 )
                            delete tmp1;

                        if ( tmp2 )
                            delete tmp2;

                        cout << "BTS MAC send(): remove TCBs with tlli = " << llc_opt->tlli << endl;
                    }



                }

                FREE_BSS_EVENT(ep);
                return 1;
            }
            else {

                if ( (llc_opt->tlli!=0) && (llc_opt->oldtlli !=0) ) {
                    int res = tlli_cache->rehash(llc_opt->oldtlli,llc_opt->tlli);
                    if ( res>0 ) {
                        #ifdef __MODIFY_TLLI_DEBUG
                        cout <<"GprsBtsMac send(): modify tlli " << llc_opt->oldtlli << " to "<< llc_opt->tlli
                                <<" successfully.\n" << endl;
                        #endif
                    }
                    else {
                        cout <<"GprsBtsMac send(): modify tlli " << llc_opt->oldtlli << " to "<< llc_opt->tlli
                            << " failed.\n" << endl;
                        exit(1);
                    }
                }
                else if ( (llc_opt->tlli==0) && (llc_opt->oldtlli==0) ) {
                    cout << "BTS MAC: Assertion failed: all TLLIs are zeros."<< endl;
                    exit(1);
                }
                else
                    ;

            }
        }


        ASSERTION(rlc_opt,"GprsBtsMac send(): rlc_opt is NULL");

        if ( rlc_opt->cmd == DOWNLINK_TBF_ESTABLISH || rlc_opt->cmd == PAGING_RESPONSE ) {

            TlliCacheElem* elem = tlli_cache->search_by_tlli(llc_opt->tlli);
            if (elem)
                utcb = elem->utcb;
            else
                utcb = NULL;

            if ( utcb ) {

                if (utcb->state == TRANSFER_STATE ) {

                    PDA* pda = downlink_tbf_allocation( utcb , downlink_tsnum);
                    if (!pda) {
                    Event* ereject = create_pkt_access_reject();
                        send_timestamp(ereject,bssdes->bcch_no,0,1);
                    }
                    else {

                        NDistrMsg* nmsg = new NDistrMsg();
                        nmsg->set_pgmode(0x01);
                        /* set tfi */
                        GlobalTfi tfi_ie;
                        tfi_ie.direction    = 1;
                        tfi_ie.tfi          = pda->downlink_tfi_assignment;
                        nmsg->set_addr_info(GLOBAL_TFI_INDICATOR, tfi_ie);
                        nmsg->set_nondistr_msg(PKT_DOWNLINK_ASSIGNMENT, downlink , pda);
                        Event* eburst;
                        CREATE_BSS_EVENT(eburst);
                        bss_message* bmsg       = reinterpret_cast<bss_message*> (eburst->DataInfo_);
                        bmsg->flag              = PF_SEND;

                        //uchar* dummy_data = new uchar[57];
                        uchar* dummy_data = dummy_data_array;
                        bzero(dummy_data,1);
                        insert_user_data(bmsg,dummy_data,1);
                        create_llc_option(bmsg);
                        llc_option* llc_opt1    = reinterpret_cast<llc_option*> (bmsg->llc_option);
                        llc_opt1->tlli = llc_opt->oldtlli = utcb->tlli;

                        create_rlc_option(bmsg);
                        rlc_option* rlc_opt     = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
                        rlc_opt->ctl_msg_type   = PKT_DOWNLINK_ASSIGNMENT;
                        rlc_opt->tfi            = utcb->tfi;
                        rlc_opt->chid           = utcb->downlink_freq;
                        create_mac_option(bmsg);
                        mac_option* mac_opt     = reinterpret_cast<mac_option*> (bmsg->mac_option);
                        //mac_opt->msg            = nmsg;
                        insert_macopt_msg( mac_opt , NDMSG , nmsg );
                        mac_opt->burst_type     = CTL_BURST;

                        if (rlc_opt->chid >250 )
                            mac_opt->channel_id = bssdes->bcch_no;
                        else
                            mac_opt->channel_id = rlc_opt->chid;

                        printf ( "BTSMAC NODE %d: create packet downlink assignment. \n",get_nid());
                        /* here */
                        utcb->pacch_pending_event_sq->insert_tail(eburst);


                    }

                }

                FREE_BSS_EVENT(ep);
                return 1;
            }

            else {

                cout << "BTS MAC: The utbfdes with the same tlli is not found. Should send Paging Request." << endl;
                FREE_BSS_EVENT(ep);
                return 0;
            }

        }

        else if (rlc_opt->cmd == SEND_PAGING_REQUEST ) {

            Event* epage_req = create_paging_request(gmm_opt->imsi);
            /*copy_bss_msg_options( reinterpret_cast<bss_message*> (epage_req->DataInfo_) ,
                reinterpret_cast<bss_message*> (ep->DataInfo_) );*/
            copy_bss_msg_headers( reinterpret_cast<bss_message*> (epage_req->DataInfo_) ,
                reinterpret_cast<bss_message*> (ep->DataInfo_) );

            FREE_BSS_EVENT(ep);

            if (epage_req) {

                #ifdef __SEQUENCE_DEBUG
                cout <<"BTS MAC: packet page request is going to be enqueued." << endl;
                #endif
                //return NslObject::send(epage_req);
                block_partition(epage_req,PPCH_SQUEUE);

            }
            else
                return 1;
        }

        else if (rlc_opt->cmd == RRBP_REQUEST ) {

            /* Downlink TBF sends RRBP_REQUEST to make BTS MAC arrange an uplink block for a Packet
             * Downlink ACKNACK.
             */

            if (rlc_opt->tfi>=32 || rlc_opt->tfi<0) {
                cout <<"GprsBtsMac send(): TFI out of range." << endl;
                exit(1);
            }

            TbfDes* tbfdes = NULL;

            if ( !(tbfdes=bssdes->ch_map[rlc_opt->chid]->get_tbfdes_by_tfi(rlc_opt->tfi)) ) {

                /* TBF doesn't exist */
                cout <<"GprsBtsMac send(): The corresponding TBF description in MAC layer is not found." << endl;
                cout <<" cmd == " << (int)(rlc_opt->cmd) << endl;
                exit(1);

            }
            else {

                tbfdes->rrbp_cur = 0x0;

                /* allocate an uplink block for this downlink TBF.
                 * So far, the timeslot assignment for an uplink is orthogonal to that for an downlink.
                 * Thus, there is no need to explicitly allocate an uplink block because the corresponding
                 * uplink blocks for a downlink should be unused in current design.
                 */


            }


        }

        else if (rlc_opt->cmd == 0 ){

            /* user data : rlc data blocks */
            TbfDes* tbfdes = NULL;
            TaAllocation* ta_alloc = bssdes->ch_map[rlc_opt->chid];

            IS_NULL_STR(ta_alloc,"GprsBtsMac::send() ta_alloc is null\n" ,0);

            if ( !(tbfdes=ta_alloc->get_tbfdes_by_tfi(rlc_opt->tfi)) ) {

                if ( (ta_alloc=bssdes->ch_map[CORRESPONDING_UPLINK_CH(rlc_opt->chid)]) ) {
                    if ( rlc_opt->ctl_msg_type == PKT_UPLINK_ACKNACK ) {

                        /* reserve the transmission right for this TBF */
                        bssdes->ch_map[CORRESPONDING_UPLINK_CH(rlc_opt->chid)]->set_tfi_granted_on_tn(tn, rlc_opt->tfi);

                        block_partition(ep,PACCH_SQUEUE);
                        //cout <<"GprsBtsMac send(): Enqueue PKT_UPLINK_ACKNACK." << endl;
                    }
                    else {
                        cout <<"GprsBtsMac send(): Unsupported command." << endl;
                        cout <<" cmd == " << (int)(rlc_opt->cmd) << endl;
                        exit(1);
                    }
                }
                else {
                    /* TBF doesn't exist */
                    cout <<"GprsBtsMac send(): The corresponding TBF description in MAC layer is not found." << endl;
                    cout <<" cmd == " << (int)(rlc_opt->cmd) << endl;
                    exit(1);
                }
            }

            else {

                /* reserve the transmission right for this TBF */
                bssdes->ch_map[rlc_opt->chid]->set_tfi_granted_on_tn(tn, rlc_opt->tfi);

                if (rlc_opt->ctl_msg_type < 0)
                    block_partition(ep,PDTCH_SQUEUE);
                else
                    block_partition(ep,PACCH_SQUEUE);
            }

            return 1;
        }

        else {
            cout << "GprsBtsMac send(): unknown type blk." << endl;
            return 0;
        }
        return 1;
    }

    int GprsBtsMac::recv(Event* ep) {

        ASSERTION( ep , "BTS MAC::recv() ep is null.\n");
        bss_message* bssmsg = reinterpret_cast<bss_message*>(ep->DataInfo_);
        mac_option* mac_opt = reinterpret_cast<mac_option*>(bssmsg->mac_option);


        if ( log_flag && !strcasecmp(GPRSLogFlag, "on") ) {

            /*MAC recv log*/
            struct logEvent*    logep;
            uchar burst_len;
            char  burst_type;

            struct gprs_log*    log1;
            u_int64_t           delay;

            log1 = (struct gprs_log *)malloc(sizeof(struct gprs_log));
            MILLI_TO_TICK(delay,0.577);

            if( mac_opt->burst_type == NORMAL_BURST ||
                mac_opt->burst_type == DUMMY_BURST  ||
                mac_opt->burst_type == CTL_BURST)

                burst_len = NB_DATA_LENGTH;
            else
                burst_len = 11;

            if(mac_opt->burst_type == NORMAL_BURST)
                burst_type = FRAMETYPE_GPRS_DATA;

            else if(mac_opt->burst_type == ACCESS_BURST)
                burst_type = FRAMETYPE_GPRS_ACCESS;

            else if(mac_opt->burst_type == DUMMY_BURST)
                burst_type = FRAMETYPE_GPRS_DUMMY;

            else
                burst_type = FRAMETYPE_GPRS_CTL;


            LOG_GPRS(log1,GetCurrentTime()+delay,GetCurrentTime(),get_type()
                    ,get_nid(),StartRX,mac_opt->nid,get_nid(),mac_opt->burst_num,
                    burst_type,burst_len,mac_opt->channel_id);

            INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

        }

        return burst_demultiplexer(ep);

    }

    int GprsBtsMac::ack_rlc(int command , void* msg ) {

        IS_NULL_STR(msg,"GprsBtsMac ack_rlc(): msg is null\n",0);

        Event* ep;
        CREATE_BSS_EVENT(ep);
        bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        bssmsg->flag        = PF_RECV;
        bssmsg->imc_flag    = true;

        create_rlc_option(bssmsg);
        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

        if ( command == UPLINK_TBF_ESTABLISHED || command == DOWNLINK_TBF_ESTABLISHED) {
            rlc_opt->cmd        = command;
            bssmsg->user_data   = reinterpret_cast<char*> (msg);
        }
        else if ( command == PKT_PAGING_REQUEST ) {
            rlc_opt->cmd        = command;
            bssmsg->user_data   = reinterpret_cast<char*> (msg);
        }
        else if ( command == PUSH_A_RLCDATA_BLK ) {
            rlc_opt->cmd        = command;
            bssmsg->user_data   = reinterpret_cast<char*> (msg);
        }

        else {
            cout << "GprsBtsMac ack_rlc(): undefined command "<< command <<endl;
            FREE_BSS_EVENT(ep);
            return 0;
        }

        return NslObject::recv(ep);
    }

    int GprsBtsMac::gprs_scheduler(bool tn_boundary) {


        #ifdef __SEQUENCE_DEBUG

        //printf("BTS MAC: scheduler is invoked on tn[%ld]. bn = %ld at tick= %llu \n", tn, bn , GetCurrentTime() );

        #endif
        /* flush dummy burst squeue on block boundary */
        if ( is_blkn_boundary() && (!tn) ) {

            //printf("BTS MAC: flush uplink_rq. fn=%ld uplink_fn=%ld. tick=%lld.\n", fn , uplink_fn , GetCurrentTime() );
            for ( int i=0 ; i<8 ;++i) {
                for ( int j=0 ; j<125 ;++j ) {
                    dummy_burst_squeue[i][j].flush();
                }
            }

        }


        /* flush receive burst queues on uplink block boundary */
        if ( is_uplink_blkn_boundary() && (!uplink_tn) ) {

            //printf("BTS MAC: flush uplink_rq. fn=%ld uplink_fn=%ld. tick=%lld.\n", fn , uplink_fn , GetCurrentTime() );
            for ( int i=0 ; i<8 ;++i) {
                for ( int j=0 ; j<125 ;++j ) {
                    uplink_rq[i][j].flush();
                }
            }

        }



        /**** send the burst based on the current logical channel****/

        if ( state != READY )
            return 1;

        if ( trigger_sending_ind[tn] ) {

            PollInd* poll_ind       = new PollInd;
            poll_ind->polling_tn    = tn;

            for ( int i=0 ; i<250 ;++i) {

                if (bssdes->ch_map[i] && bssdes->ch_map[i]->tn[tn] && (!bssdes->ch_map[i]->tn[tn]->is_empty()) ) {
                    int flag = false;

                    /* check sba tbfdes */
                    if ( sbalist->search( i , tn ) ) {

                        TbfDes* ptr = bssdes->ch_map[i]->get_sba_tbfdes(tn);
                        if (ptr && ptr->ts_alloc[tn] ) {
                            if ( !ptr->pacch_pending_event_sq->is_empty() ) {
                                Event* epkt = (ptr->pacch_pending_event_sq->get_head())->get_elem();
                                block_partition( epkt ,PACCH_SQUEUE);
                                ptr->pacch_pending_event_sq->remove_head();
                                flag = true;
                                printf("BTS MAC: choose SBA sender on CH[%d] TN[%ld] \n", i , tn );

                            }
                        }
                    }

                    /* check utbfdes */
                    for ( int j=0 ; j<32 ;++j ) {
                        TbfDes* ptr = bssdes->ch_map[i]->get_tbfdes_by_tfi(j);
                        if (ptr && ptr->ts_alloc[tn] ) {
                            if ( !ptr->pacch_pending_event_sq->is_empty() ) {

                                Event* epkt = (ptr->pacch_pending_event_sq->get_head())->get_elem();
                                block_partition( epkt ,PACCH_SQUEUE);
                                ptr->pacch_pending_event_sq->remove_head();

                                bssdes->ch_map[i]->set_tfi_granted_on_tn(tn, j);

                                flag = true;

                                printf("BTS MAC: choose sender on CH %d TN %ld tfi %d \n", i , tn , j );

                            }
                        }
                    }

                    if ( !flag )
                        poll_ind->polling_ch_bitmap[i] = true;
                }


            }

            ack_rlc(PUSH_A_RLCDATA_BLK,poll_ind);
        }


        /* clear listening channels */
        clear_listening_channel();

        /* set up listening channels */
        mark_listening_channel(static_cast<long> (CORRESPONDING_UPLINK_CH(bssdes->bcch_no)) );

        for (long i=UPLINK_START_CH(bssdes->start_ch) ; i<= UPLINK_END_CH(bssdes->end_ch) ;++i) {
            if ( !bssdes->ch_map[i] )
                continue;
            if ( !bssdes->ch_map[i]->tn[uplink_tn] )
                continue;
            if ( !bssdes->ch_map[i]->tn[uplink_tn]->is_empty() )
                mark_listening_channel(i);
        }

        for (long i=DOWNLINK_START_CH(bssdes->start_ch) ; i<= DOWNLINK_END_CH(bssdes->end_ch) ;++i) {
            if ( !bssdes->ch_map[i] )
                continue;
            if ( !bssdes->ch_map[i]->tn[uplink_tn] )
                continue;
            if ( !bssdes->ch_map[i]->tn[uplink_tn]->is_empty() )
                mark_listening_channel(CORRESPONDING_UPLINK_CH(i));
        }

        /*if ( GetCurrentTime() >= 80000000 )
            dump_listening_channel();*/

        set_listening_channel();


        if ( !tn_boundary )
            return 1;

        /* scan all of downlink channels */

        uchar sent_flag = false;

        for (int i=DOWNLINK_START_CH(bssdes->start_ch); i<= DOWNLINK_END_CH(bssdes->end_ch) ;++i) {

            sent_flag = false;

            if ( i == bssdes->bcch_no ) {


                if ( (!tn) && (!fn) ) {

                    /* BCCH carrier processing */
                    /* Broadcast SI13 information on BCCH carrier */

                    if ( !blkn ) {

                        Event* ep = create_si13_msg();
                        add_mac_header(ep,CTL_BLK,0x7);
                        send_timestamp(ep,i,0,1);
                        sent_flag = true;

                    }
                }


                else if ( tn == bssdes->pbcch_tn ) {


                    /* PBCCH */
                    if ( is_pbcch() ) {

                        /* broadcast PacketSystemInformation */
                        if ( blkn == 0 || blkn == 6 ) {
                            /* generate a PSI1 message to send */
                            if ( (is_blkn_boundary()) && (psi1_squeue->is_empty()) ) {
                                Event* ep = create_psi1_msg();
                                block_partition(ep,PSI1_SQUEUE);
                            }

                            if ( !psi1_squeue->is_empty() ) {
                                Event* eb = (psi1_squeue->get_head())->get_elem();
                                send_timestamp(eb,i,0,1);
                                psi1_squeue->remove_head();
                                sent_flag = true;
                            }

                        }

                        else {
                            /* dequeue pbcch_squeue to send:
                            * PSI-group messages except PSI1 are enqueued into pbcch_squeue
                            * first. The manner of dequeuing is FIFO.
                            */
                            if ( (is_blkn_boundary()) && (pbcch_squeue->is_empty()) ) {
                                Event* ep = create_pbcch_msg();
                                block_partition(ep,PBCCH_SQUEUE);
                            }

                            if ( !(pbcch_squeue->is_empty()) ) {
                                Event* eb = (pbcch_squeue->get_head())->get_elem();
                                mac_option* tmp_eb_macopt =
                                    reinterpret_cast<mac_option*>(reinterpret_cast<bss_message*>(eb->DataInfo_)->mac_option);
                                if (!tmp_eb_macopt) {
                                    printf("BTS  MAC: the mac option for a burst is not found.\n");
                                    exit(1);
                                }
                                else {

                                    if ( tmp_eb_macopt->burst_num == fn_to_burst_num( fn ) ) {
                                        send_timestamp(eb,i,0,1);
                                        pbcch_squeue->remove_head();
                                        sent_flag = true;
                                    }
                                    else {
                                        printf("BTS MAC: a burst is scheduled to be sent without correct boundary. The burst is re-scheduled.\n");
                                    }
                                }
                            }

                        }

                    }

                    /* PCCCH = PAGCH + PPCH + PNCH */

                    else if ( is_pccch() ) {

                        if ( !pagch_squeue->is_empty() ) {

                            Event* eb = (pagch_squeue->get_head())->get_elem();
                            mac_option* tmp_eb_macopt =
                                reinterpret_cast<mac_option*>(reinterpret_cast<bss_message*>(eb->DataInfo_)->mac_option);
                            if (!tmp_eb_macopt) {
                                printf("BTS  MAC: the mac option for a burst is not found.\n");
                                exit(1);
                            }
                            else {

                                if ( tmp_eb_macopt->burst_num == fn_to_burst_num( fn ) ) {
                                    send_timestamp(eb,i,0,1);
                                    pagch_squeue->remove_head();
                                    sent_flag = true;
                                }
                                else {
                                    printf("BTS MAC: a burst is scheduled to be sent without correct boundary. The burst is re-scheduled.\n");
                                }
                            }


                        }
                        else if (!ppch_squeue->is_empty()) {

                            Event* eb = (ppch_squeue->get_head())->get_elem();
                            send_timestamp(eb,i,0,1);
                            ppch_squeue->remove_head();
                            sent_flag = true;

                        }
                        else
                            ;
                    }


                    else if ( (is_idle_frame()) ) {
                        Event* eb = create_measured_dummy_burst_msg(i);
                        send_timestamp(eb,i,0,1);
                        sent_flag = true;
                    }

                    else {

                        /* In idle timeslots, BSS shall send dummy burst with USF */
                        if ( (is_blkn_boundary()) && (dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].is_empty()) ) {
                            create_dummy_burst_msg(i);
                        }

                        if ( !(dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].is_empty()) ) {
                            Event* eb = (dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].get_head())->get_elem();
                            send_timestamp(eb,i,0,1);
                            dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].remove_head();
                            sent_flag = true;

                        }

                    }

                }


                else { /* other timeslots on BCCH carrier: process as pdtch */


                    /* In idle timeslots, BSS shall send dummy burst with USF */
                    if ( (is_blkn_boundary()) && (dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].is_empty()) ) {
                        create_dummy_burst_msg(i);
                    }

                    if ( !(dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].is_empty()) ) {
                        Event* eb = (dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].get_head())->get_elem();
                        send_timestamp(eb,i,0,1);
                        dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].remove_head();
                        sent_flag = true;

                    }

                    if ( (!sent_flag) && (is_idle_frame()) ) {
                        Event* eb = create_measured_dummy_burst_msg(i);
                        send_timestamp(eb,i,0,1);
                        sent_flag = true;
                    }


                }


            }

            if ( (!sent_flag) ) {

                /* Selecting an output queue in this timeslot follows the rule:
                 * The priority: uplink PACCH > downlink PACCH > downlink PDTCH
                 * If there is no traffic in downlink, a dummy block should be sent
                 * if an uplink TBF exists and is waiting for USF.
                 */

                TbfDes* selected_utbf           = NULL;
                TbfDes* selected_dtbf           = NULL;

                if ( bssdes->ch_map[CORRESPONDING_UPLINK_CH(i)] ) {

                    if ( (bssdes->ch_map[CORRESPONDING_UPLINK_CH(i)]->tn[tn]) ) {

                        if ( is_blkn_boundary() ) {
                            bssdes->ch_map[CORRESPONDING_UPLINK_CH(i)]->merge_ack(tn);
                        }

                        /* SBA is prior to other normal utcb pacch */
                        selected_utbf = bssdes->ch_map[CORRESPONDING_UPLINK_CH(i)]->get_granted_tbfdes(tn);
                    }
                }

                if ( bssdes->ch_map[i] ) {

                    if ( (bssdes->ch_map[i]->tn[tn]) ) {

                        if ( is_blkn_boundary() ) {
                            bssdes->ch_map[CORRESPONDING_UPLINK_CH(i)]->merge_ack(tn);
                        }

                        selected_dtbf = bssdes->ch_map[i]->get_granted_tbfdes(tn);
                    }
                }

                /* Adopt 0.7 - persistence for uplink control packets. And
                 * 0.3 - persistence for downlink data and control packets.
                 */


                if ( (is_blkn_boundary()) ) {

                    long per_num;

                    if ( !selected_utbf && !selected_dtbf ) {
                        up_down[i-125][tn] = 1;
                    }
                    else if ( !selected_utbf && selected_dtbf ) {
                        up_down[i-125][tn] = 2;
                    }
                    else if ( selected_utbf && !selected_dtbf ) {
                        up_down[i-125][tn] = 1;
                    }
                    else {

                        if ( (selected_utbf->pacch_sq[tn]->is_empty()) && (selected_dtbf->pdtch_sq[tn]->is_empty()) ) {
                            up_down[i-125][tn] = 1;
                        }
                        else if ( (selected_utbf->pacch_sq[tn]->is_empty()) && !(selected_dtbf->pdtch_sq[tn]->is_empty()) ) {
                            up_down[i-125][tn] = 2;
                        }
                        else if ( !(selected_utbf->pacch_sq[tn]->is_empty()) && (selected_dtbf->pdtch_sq[tn]->is_empty()) ) {
                            up_down[i-125][tn] = 1;
                        }
                        else {


                            if ( (selected_utbf->pacch_sq[tn]->get_list_num()) >= 12 ) {
                                up_down[i-125][tn] = 1;
                                printf("BTS MAC: chid = %d tn %ld utbf pacch_elem num = %ld \n" ,
                                    i , tn , (selected_utbf->pacch_sq[tn]->get_list_num()));
                            }

                            else {
                                per_num = (random())%100;

                                if ( per_num < 70 )
                                    up_down[i-125][tn] = 1;
                                else
                                    up_down[i-125][tn] = 2;
                            }

                        }

                    }

                }

                if ( (!sent_flag) && (selected_utbf) && (!(is_idle_frame())) && (up_down[i-125][tn] == 1) ) {

                    if ( !selected_utbf->pacch_sq[tn]->is_empty() ) {
                        Event* eb = (selected_utbf->pacch_sq[tn]->get_head())->get_elem();


                        /* remove SBA allocation */
                        bss_message* bmsg   = reinterpret_cast<bss_message*> (eb->DataInfo_);
                        ASSERTION(bmsg,"BTS MAC: eb->DataInfo_ is NULL\n");
                        mac_option* mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
                        ASSERTION(mac_opt, "BTS MAC: mac_opt is NULL\n");
                        uchar burst_num = mac_opt->burst_num;
                        uchar ch_id     = mac_opt->channel_id;
                        NDistrMsg* nmsg = reinterpret_cast<NDistrMsg*> (mac_opt->msg);


                        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
                        ASSERTION(rlc_opt, "BTS MAC: rlc_opt is NULL\n");
                        uchar ctl_msg_type  = rlc_opt->ctl_msg_type;
                        uchar tfi           = rlc_opt->tfi;



                        selected_utbf->pacch_sq[tn]->remove_head();
                        send_timestamp(eb,i,selected_utbf->get_nid(),0);
                        sent_flag = true;

                        /* if msg_type is PKT_UPLINK_ASSIGNMENT and the address info is
                         * global tfi structure, SBA should be removed.
                         */

                        if ( (ctl_msg_type == PKT_UPLINK_ASSIGNMENT) && (burst_num ==3) ) {


                            ASSERTION(nmsg, "BTS MAC: nmsg is NULL\n");
                            uchar addr_type = nmsg->get_addr_info_type();

                            if  ( addr_type == GLOBAL_TFI_INDICATOR ) {

                                /* SBA should be removed since after PKT_UPLINK_ASSIGNMENT is sent,
                                * the MS should use the configuration indicated dynamic_allocation message.
                                */

                                uchar clean_stage_flag = 0;
                                SbaFreq* elem = sbalist->search(CORRESPONDING_UPLINK_CH(ch_id),this->tn);

                                if (elem) {
                                    sbalist->remove_entry(elem);
                                    ++clean_stage_flag;
                                }

                                /* The instance of SBA was inserted into these two list. Notice that
                                * there is only one instance shared by these two list
                                */
                                TbfDes* sba = bssdes->ch_map[ch_id]->get_sba_tbfdes(tn);
                                if (sba) {
                                    bssdes->ch_map[ch_id]->tn[tn]->remove_entry(sba);
                                    ++clean_stage_flag;
                                }

                                sba = bssdes->ch_map[CORRESPONDING_UPLINK_CH(ch_id)]->get_sba_tbfdes(tn);
                                if (sba) {
                                    bssdes->ch_map[CORRESPONDING_UPLINK_CH(ch_id)]->tn[tn]->remove_entry(sba);
                                    ++clean_stage_flag;
                                }

                                if ( sba ) {
                                    delete sba;
                                    sba = NULL;
                                }

                                if ( clean_stage_flag <3 ) {
                                    cout << "BTS MAC: ASSERTION failed: SBA is not removed completely.\n"<< endl;
                                    exit(1);
                                }
                            }

                        }

                        else if ( (ctl_msg_type == PKT_DOWNLINK_ASSIGNMENT) && (burst_num >=3) ) {

                            TbfDes* tmpdes = bssdes->ch_map[ch_id]->get_tbfdes_by_tfi(tfi);
                            ASSERTION(tmpdes, "BTS MAC: downlink tmpdes is NULL.\n" );
                            ack_rlc(DOWNLINK_TBF_ESTABLISHED,tmpdes);

                        }

                        else
                            ;

                        #ifdef __SEQUENCE_DEBUG
                            cout << "GprsBtsMac::gprs_scheduler(): send pkt on UTBF PACCH in chid = " << i <<
                                "tn = " << tn << endl;
                        #endif


                    }

                }

                if (!sent_flag)
                    up_down[i-125][tn] = 2;

                if ( (!sent_flag) && (selected_dtbf) && (!(is_idle_frame())) && (up_down[i-125][tn] == 2) ) {

                    if ( !selected_dtbf->pacch_sq[tn]->is_empty() ) {
                        Event* eb = (selected_dtbf->pacch_sq[tn]->get_head())->get_elem();
                        selected_dtbf->pacch_sq[tn]->remove_head();
                        send_timestamp(eb,i,selected_dtbf->get_nid(),0);

                        sent_flag = true;

                        #ifdef __SEQUENCE_DEBUG
                        cout << "GprsBtsMac::gprs_scheduler(): send pkt on DTBF PACCH in chid = " << i <<
                            "tn = " << tn << endl;
                        #endif

                    }

                    if ( !selected_dtbf->pdtch_sq[tn]->is_empty() ) {
                        Event* eb = (selected_dtbf->pdtch_sq[tn]->get_head())->get_elem();
                        selected_dtbf->pdtch_sq[tn]->remove_head();
                        send_timestamp(eb,i,selected_dtbf->get_nid(),0);

                        sent_flag = true;

                        #ifdef __SEQUENCE_DEBUG
                        cout << "GprsBtsMac::gprs_scheduler(): send pkt on DTBF PDTCH in chid = " << i <<
                            "tn = " << tn << endl;
                        #endif
                    }

                }

                if ( (!sent_flag) && (bssdes->ch_map[CORRESPONDING_UPLINK_CH(i)]->get_tfi_cnt()) && (!(is_idle_frame())) ) {

                    if ( (is_blkn_boundary()) && (dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].is_empty()) ) {
                        create_dummy_burst_msg(i);
                    }

                    if ( !(dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].is_empty()) ) {
                        Event* eb = (dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].get_head())->get_elem();
                        send_timestamp(eb,i,0,1);
                        dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(i)].remove_head();
                        sent_flag = true;

                        #ifdef __SEQUENCE_DEBUG
                        cout << "GprsBtsMac::gprs_scheduler(): send dummy burst in chid = " << i <<
                            "tn = " << tn << endl;
                        #endif
                    }
                }



            }
        }

        return 1;
    }

    int GprsBtsMac::burst_demultiplexer(Event* ep) {

        ASSERTION(ep, "GprsBtsMac::burst_demul(): receive an empty event.\n");

        bss_message* bss_msg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        ASSERTION(bss_msg,"GprsBtsMac burst_demul(): DataInfo is null\n");

        mac_option* mac_opt = reinterpret_cast<mac_option*> (bss_msg->mac_option);
        ASSERTION(mac_opt,"GprsBtsMac burst_demul(): mac_option is null\n");

        rlc_option* rlc_opt = NULL;
        if ( mac_opt->burst_type != ACCESS_BURST ) {
            rlc_opt = reinterpret_cast<rlc_option*> (bss_msg->rlc_option);
            IS_NULL_STR(rlc_opt,"GprsBtsMac burst_demul(): rlc_option is null\n",0);
        }

        if ( (mac_opt->channel_id != static_cast<long> (CORRESPONDING_UPLINK_CH(bssdes->bcch_no)) )  &&
             (!bssdes->channel_match(mac_opt->channel_id)) ) {

            cout << "BtsMac burst_demul(): received channel id mismatches: received mac_opt->chid =  " <<
                mac_opt->channel_id << endl;
            cout << "BtsMac burst_demul(): CORRESPONDING_UPLINK_CH(bssdes->bcch_no) = "
                << static_cast<long> (CORRESPONDING_UPLINK_CH(bssdes->bcch_no)) << endl;

            if ( bss_msg->packet) {
                remove_upper_layer_pkt(bss_msg->packet);
                free_pkt(bss_msg->packet);
                bss_msg->packet     = NULL;
            }


            FREE_BSS_EVENT(ep);
            return 0;

        }

        if ( state == READY ) {

        /* All BSS should do is to check if this burst is a Channel Request */
        if ( mac_opt->burst_type == ACCESS_BURST ) {

            AB *ab = reinterpret_cast<AB*> (bss_msg->user_data);
            ushort chreqinfo = ab->get_ra_info();
            PktChReq* chreq = new PktChReq;
            chreq->unpack(chreqinfo);

            PacketUplinkAssignment* pua = single_blk_allocation(bss_msg);

            if ( !pua ) {
                Event* ereject = create_pkt_access_reject();
                send_timestamp(ereject,bssdes->bcch_no,0,1);
            }
            else {
                NDistrMsg* nmsg         = new NDistrMsg();
                nmsg->set_pgmode(0x01);
                PacketRequestReference req;
                req.ra_info             = chreqinfo;
                req.fn                  = uplink_fn;
                nmsg->set_addr_info(PKT_REQ_REF_INDICATOR,  req);
                nmsg->set_nondistr_msg(PKT_UPLINK_ASSIGNMENT, downlink, pua);
                Event* eburst;
                CREATE_BSS_EVENT(eburst);
                bss_message* bmsg       = reinterpret_cast<bss_message*> (eburst->DataInfo_);
                uchar* dummy_data = (uchar*)(dummy_data_array);
                bzero(dummy_data,1);
                insert_user_data(bmsg,dummy_data,1);
                create_rlc_option(bmsg);
                rlc_option* rlc_opt     = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
                rlc_opt->ctl_msg_type   = PKT_UPLINK_ASSIGNMENT;
                rlc_opt->tfi            = PAGCH_SQUEUE;
                rlc_opt->chid           = bssdes->bcch_no;
                create_mac_option(bmsg);
                mac_option* mac_opt     = reinterpret_cast<mac_option*> (bmsg->mac_option);
                mac_opt->channel_id     = bssdes->bcch_no;
                //mac_opt->msg            = (nmsg);
                insert_macopt_msg( mac_opt , NDMSG , nmsg );
                mac_opt->burst_type     = CTL_BURST;
                block_partition(eburst,PAGCH_SQUEUE);
            }

            delete ab;
            bss_msg->user_data = NULL;
            if ( bss_msg->packet) {
                remove_upper_layer_pkt(bss_msg->packet);
                free_pkt(bss_msg->packet);
                bss_msg->packet     = NULL;
                printf("BTS MAC[%u]: Assertion failed: Access burst contains user data!\n", get_nid());
                exit(1);
            }

            FREE_BSS_EVENT(ep);
        }

            else if ( mac_opt->burst_type == NORMAL_BURST ) {


            int ins_status = uplink_rq[uplink_tn][mac_opt->channel_id].insert(mac_opt->burst_num, ep);
            //printf("BTS MAC: ep addr = %lx \n" , ep );
            ep = NULL;

            if ( !ins_status )
                return 0;

            #ifdef __SEQUENCE_DEBUG
            cout <<"BTS MAC: receive a rlc blk with burst_num=" << (int) (mac_opt->burst_num) <<"at fn=" << fn << endl;
            #endif

                if (mac_opt->burst_num>=3) {

                    Event* user_data_msg = uplink_rq[uplink_tn][mac_opt->channel_id].fast_assembling();

                if ( !user_data_msg ) {
                        cout <<"GprsBtsMac burst_demul(): data_blk is not correctly reassembled." << endl;
                    }
                else {
                    bss_message* udata_bmsg         = reinterpret_cast<bss_message*> (user_data_msg->DataInfo_);
                    rlc_option*  udata_rlc_opt      = reinterpret_cast<rlc_option*> (udata_bmsg->rlc_option);


                    #ifdef  __DUMP_ENCODING_DATA
                    printf( "BTS MAC: decoding data ptr= %lu \n", (ulong) (udata_bmsg->user_data) ) ;
                    for (int i=0 ; i<57 ; ++i)
                        printf(" %d ", (uchar) (udata_bmsg->user_data[i]));
                    printf("\n");
                    printf("BTS MAC: event indicated CS = %d \n\n", udata_rlc_opt->cs);
                    #endif

                    uchar* decoded_data = channel_decoding(udata_rlc_opt->cs, reinterpret_cast<uchar*> (udata_bmsg->user_data) );

                    if ( decoded_data ) {

                        if ( udata_bmsg->user_data ) {
                            delete udata_bmsg->user_data;
                            udata_bmsg->user_data = NULL;
                        }

                        udata_bmsg->user_data = reinterpret_cast<char*> (decoded_data);

                        #ifdef __SEQUENCE_DEBUG
                        cout <<"BTS MAC: reassemble a rlc blk " << endl;
                        #endif

                        return NslObject::recv(user_data_msg);
                    }
                    else {

                        printf("BTS MAC[%u]: burst is corrupted. \n", get_nid());

                        if ( udata_bmsg->packet) {
                            remove_upper_layer_pkt(udata_bmsg->packet);
                            free_pkt(udata_bmsg->packet);
                            udata_bmsg->packet = NULL;
                        }

                        FREE_BSS_EVENT_INCLUDING_USER_DATA( user_data_msg );
                        return 1;

                    }
                }
                }
            }

            else if ( mac_opt->burst_type == CTL_BURST ) {

                //printf("BTS MAC: insert ctl burst. fn = %ld ufn=%ld tick=%lld\n", fn , uplink_fn , GetCurrentTime() );
                int ins_status = uplink_rq[uplink_tn][mac_opt->channel_id].insert(mac_opt->burst_num, ep);
                //printf("BTS MAC: ep addr = %lx \n" , ep );
                ep = NULL;

                if (!ins_status)
                    return 0;

                if ( mac_opt->burst_num >= 3 ) {

                    Event* ctlmsg = uplink_rq[uplink_tn][mac_opt->channel_id].fast_assembling();
                    if ( !ctlmsg ) {
                        cout <<"GprsBtsMac burst_demul(): ctlmsg is not correctly reassembled." << endl;
                    }
                    else {
                        bss_message* ctlmsg_bmsg    = reinterpret_cast<bss_message*> (ctlmsg->DataInfo_);
                        mac_option*  ctlmsg_mac_opt = reinterpret_cast<mac_option*> (ctlmsg_bmsg->mac_option);

                        NDistrMsg*  nmsg    = reinterpret_cast<NDistrMsg*>(ctlmsg_bmsg->user_data);

                        uchar msgtype           = nmsg->get_msgtype();

                        if ( msgtype == PKT_RESOURCE_REQUEST ) {

                            if ( TLLI_INDICATOR != nmsg->get_addr_info_type()) {
                                cout <<"GprsBtsMac burst_demul(): not receive tlli of MS's \n" << endl;
                                return 0;
                            }

                            ulong ms_tlli;
                            nmsg->get_addr_info(&ms_tlli);


                            SbaFreq* elem = sbalist->search(ctlmsg_mac_opt->channel_id,this->uplink_tn);

                            ASSERTION(elem,"GprsBtsMac::burst_demultiplexer(): The corresponding SBA is not found.\n");

                            PUA* pua = uplink_tbf_allocation(ctlmsg_bmsg,ms_tlli,uplink_tsnum);

                            shared_obj_dec_refcnt( NDMSG , nmsg );
                            if ( (shared_obj_release( NDMSG , nmsg )) ) {
                                nmsg = NULL;
                            }

                            if (!pua) {
                                Event* ereject = create_pkt_access_reject();
                                send_timestamp(ereject,mac_opt->channel_id,0,1);
                            }
                            else {
                                #define __DEBUG_PRINT
                                #ifdef  __DEBUG_PRINT
                                cout << "BTS NODE"<< (long)get_nid() <<": pkt uplink assignment: ts_alloc = " ;
                                for (int i=0 ; i<8 ; ++i)
                                    cout << (int) (pua->dynamic_allocation->usf_tn[i]) << " ";
                                cout << endl;
                                #endif

                                NDistrMsg* nmsg1 = new NDistrMsg;
                                nmsg1->set_pgmode(0x01);
                                /* set tfi */
                                GlobalTfi tfi_ie;
                                tfi_ie.direction    = 0;
                                tfi_ie.tfi          = pua->dynamic_allocation->uplink_tfi_assignment;
                                nmsg1->set_addr_info(GLOBAL_TFI_INDICATOR, tfi_ie);
                                nmsg1->set_nondistr_msg(PKT_UPLINK_ASSIGNMENT, downlink, pua);
                                Event* eburst;
                                CREATE_BSS_EVENT(eburst);
                                bss_message* bmsg       = reinterpret_cast<bss_message*> (eburst->DataInfo_);
                                //uchar* dummy_data = new uchar[57];
                                uchar* dummy_data = dummy_data_array;
                                bzero(dummy_data,1);
                                insert_user_data(bmsg,dummy_data,1);
                                create_rlc_option(bmsg);
                                rlc_option* rlc_opt     = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
                                rlc_opt->ctl_msg_type   = PKT_UPLINK_ASSIGNMENT;
                                rlc_opt->tfi            = PACCH_SQUEUE;
                                rlc_opt->chid           = CORRESPONDING_DOWNLINK_CH(elem->fp->arfcn);
                                create_mac_option(bmsg);
                                mac_option* mac_opt1    = reinterpret_cast<mac_option*> (bmsg->mac_option);

                                mac_opt1->channel_id    = CORRESPONDING_DOWNLINK_CH(elem->fp->arfcn);
                                //mac_opt1->msg           = nmsg1;
                                insert_macopt_msg( mac_opt1 , NDMSG , nmsg1 );
                                mac_opt1->burst_type    = CTL_BURST;
                                //block_partition(eburst,pacch_sq[tn]);
                                TbfDes* sba = bssdes->ch_map[ctlmsg_mac_opt->channel_id]->get_sba_tbfdes(uplink_tn);

                                ASSERTION(sba, "BTS MAC: sba tbfdes is null\n");

                                sba->pacch_pending_event_sq->insert_tail(eburst);

                            }
                        }

                        else if ( msgtype == PKT_CONTROL_ACK ) {
                            /* used for polling process */
                            return NslObject::recv(ctlmsg);
                        }
                        else if ( msgtype == PKT_DOWNLINK_ACKNACK ) {

                            rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (ctlmsg_bmsg->rlc_option);

                            /* The demultiplexing of the block sent on blkn reserved by RRBP is
                             * simplified by carry TFI of this TBF
                             */

                            if (!rlc_opt) {

                                shared_obj_dec_refcnt( NDMSG , nmsg );
                                if ( (shared_obj_release( NDMSG , nmsg )) ) {
                                    nmsg = NULL;
                                }

                                if ( ctlmsg_bmsg->packet) {
                                    remove_upper_layer_pkt(ctlmsg_bmsg->packet);
                                    free_pkt(ctlmsg_bmsg->packet);
                                    ctlmsg_bmsg->packet = NULL;
                                }

                                FREE_BSS_EVENT(ctlmsg);
                                return 0;
                            }

                            TbfDesList* tbflist =
                                bssdes->ch_map[CORRESPONDING_DOWNLINK_CH(ctlmsg_mac_opt->channel_id)]->tn[uplink_tn];

                            TbfDes* tbfdes = tbflist->search_by_tfi(rlc_opt->tfi);

                            if (!tbfdes) {
                                /* the received burst doesn't have corresponding tbfdes */
                                shared_obj_dec_refcnt( NDMSG , nmsg );
                                if ( (shared_obj_release( NDMSG , nmsg )) ) {
                                    nmsg = NULL;
                                }

                                if ( ctlmsg_bmsg->packet) {
                                    remove_upper_layer_pkt(ctlmsg_bmsg->packet);
                                    free_pkt(ctlmsg_bmsg->packet);
                                    ctlmsg_bmsg->packet = NULL;
                                }

                                FREE_BSS_EVENT(ctlmsg);
                                return 0;
                            }
                            else {
                                //ctlmsg_bmsg->user_data = reinterpret_cast<char*> (ctlmsg_mac_opt->msg);
                                return NslObject::recv(ctlmsg);
                            }
                        }

                        else{
                            cout << "GprsBtsMac burst_demultiplexer(): unknown message type " << msgtype  << endl;
                        }
                    }
                }
            }
            else {
                cout << "GprsBtsMac burst_demultiplexer(): unknown type of burst. burst type id="
                        << mac_opt->burst_type  << endl;
                return 0;
            }
        }
        else
            /* if state == NOT_READY, do nothing */
            cout << "GprsBtsMac burst_demul(): BTS is not ready. It requires 3 timeslot to initialize uplinks" << endl;

        return 1;
    }

    int GprsBtsMac::block_partition(Event* ep, int queue_type) {

        #ifdef __BLK_PARTITION_DETAILED_DEBUG
            cout << "GprsBtsMac block_partition(): queuetype = "<< queue_type << endl;
        #endif
        IS_NULL_STR(ep,"GprsBtsMac block_partition(): ep is null\n",-1);

        bss_message* bss_msg    = reinterpret_cast<bss_message*> (ep->DataInfo_);
        IS_NULL_STR( bss_msg,"GprsBtsMac block_partition(): ep->DataInfo_ is null\n",-1);

        rlc_option*  rlc_opt    = reinterpret_cast<rlc_option*> (bss_msg->rlc_option);
        IS_NULL_STR( rlc_opt, "GprsBtsMac block_partition(): rlc_opt is null\n",-1);

        SList<Event>* squeue = NULL;

        long    tbf_chid;
        uchar   squeue_det_flag = false;

        if (rlc_opt->ctl_msg_type == PKT_UPLINK_ACKNACK ) {
            tbf_chid = CORRESPONDING_UPLINK_CH(rlc_opt->chid);
        }

        else if ( (rlc_opt->ctl_msg_type == PKT_UPLINK_ASSIGNMENT) && (queue_type == PACCH_SQUEUE) ) {

            mac_option* mac_opt = reinterpret_cast<mac_option*> (bss_msg->mac_option);
            ASSERTION(mac_opt,"BTS MAC: invalid mac option\n");

            NDistrMsg* nmsg = reinterpret_cast<NDistrMsg*> (mac_opt->msg);
            ASSERTION(nmsg,"BTS MAC: invalid msg\n");

            GlobalTfi gtfides;
            if ( nmsg->get_addr_info(&gtfides) ) {
                tbf_chid = CORRESPONDING_UPLINK_CH(rlc_opt->chid);
                TbfDes* sbades = bssdes->ch_map[tbf_chid]->get_sba_tbfdes(tn);
                if (sbades) {
                    squeue = sbades->pacch_sq[tn];
                    squeue_det_flag = true;
                }
                else {
                    cout <<"BTS MAC: ASSERTION failed due to invalid sbades." << endl;
                    exit(1);
                }
            }
        }

        else if ( rlc_opt->ctl_msg_type == PKT_DOWNLINK_ASSIGNMENT ) {

            llc_option* llc_opt = reinterpret_cast<llc_option*> (bss_msg->llc_option);
            IS_NULL_STR( llc_opt , "GprsBtsMac::block_partitoin(): llc_opt is NULL.\n" , -1 );

            TlliCacheElem* elem = tlli_cache->search_by_tlli(llc_opt->tlli);
            TbfDes* utcb = NULL;

            if (elem)
                utcb = elem->utcb;

            if (utcb)
                tbf_chid = utcb->uplink_freq;
            else
                tbf_chid = rlc_opt->chid;
        }
        else
            tbf_chid = rlc_opt->chid;

        uchar usf_value;
        if (bssdes->ch_map[tbf_chid]) {

            uchar usf_tmp_value = bssdes->ch_map[tbf_chid]->choose_usf_in_tn(tn);

            if (usf_tmp_value == 100 )
                usf_value = 0x07;/* random */
            else
                usf_value = usf_tmp_value;

        }
        else
            usf_value = 0x07;/* random */


        #ifdef __USF_DEBUG
        cout << "BTSMac: (A) CH " << (int)(rlc_opt->chid) << " usf_value at tn = " << tn << " usf = " << (int) (usf_value) << endl;
        #endif

        /* range test */
        if (tbf_chid < 0 || tbf_chid >=250 ) {
            cout <<"BTS MAC: tbf_chid =" << tbf_chid << "is out of range" << endl;
            exit(1);
        }

        if ( !squeue_det_flag ) {

            if ( rlc_opt->tfi >=0 ) {

                IS_NULL_STR( bssdes->ch_map[tbf_chid] ,"GprsBtsMac blk_partition(): the channel is not allocated\n" , -1);
                IS_NULL_STR( bssdes->ch_map[tbf_chid]->tbfdes[static_cast<int> (rlc_opt->tfi)],
                    "GprsBtsMac blk_partition(): the TBF is not allocated\n" , -1);

                if ( queue_type == PACCH_SQUEUE ) {
                    squeue = bssdes->ch_map[tbf_chid]->tbfdes[static_cast<int> (rlc_opt->tfi)]->pacch_sq[tn];
                    add_mac_header(ep,CTL_BLK,usf_value);
                    //printf("Node %ld : Pacch Squeue length = %ld \n" , get_nid() , squeue->get_list_num() );
                }
                else if ( queue_type == PDTCH_SQUEUE ) {
                    squeue = bssdes->ch_map[tbf_chid]->tbfdes[static_cast<int> (rlc_opt->tfi)]->pdtch_sq[tn];
                    add_mac_header(ep,DATA_BLK,usf_value);
                }

                else {
                    cout <<"GprsBtsMac blk_partition(): indicated queue_type =  "<< (int)(queue_type) << " is not found." << endl;
                    return -1;
                }
            }

            else {
                if ( queue_type == PSI1_SQUEUE )
                    squeue = psi1_squeue;
                else if ( queue_type == PBCCH_SQUEUE )
                    squeue = pbcch_squeue;
                else if ( queue_type == PPCH_SQUEUE )
                    squeue = ppch_squeue;
                else if ( queue_type == PNCH_SQUEUE )
                    squeue = pnch_squeue;
                else if ( queue_type == PAGCH_SQUEUE )
                    squeue = pagch_squeue;
                else {
                    cout <<"GprsBtsMac blk_partition(): indicated queue_type =  "<< (int)(queue_type) << " is not found." << endl;
                    return -1;
                }

                add_mac_header(ep,CTL_BLK,usf_value);
            }
        }
        else {

            add_mac_header(ep,CTL_BLK,usf_value);

        }

        DMH* mh = reinterpret_cast<DMH*> (bss_msg->mac_header);
        (*bss_msg->user_data) = (char) (mh->encode());

        uchar* encoded_data = channel_encoding(rlc_opt->cs, reinterpret_cast<uchar*>(bss_msg->user_data));

        IS_NULL_STR(encoded_data,"GprsBtsMac blk_partition(): encoding failed\n",-1);

        try {
            ulong  ts=0;
            bool   stealing_bit = 0;

            for (int i=0 ; i<4 ; ++i ) {
                Event* eburst_p;
                CREATE_BSS_EVENT(eburst_p);
                bss_message* bss_msg1 = reinterpret_cast<bss_message*> (eburst_p->DataInfo_);

                /* The exact amount of data carried by a normal burst is 14.25 bytes, but it's tedious
                * if we conform this limitation due to unaligned byte order.
                * Instead, we pretend that we carry these data in the manner of four bursts. The data are
                * are not fragmented actually.
                */

                NormalBurst* nb         = new NormalBurst(encoded_data+i*14,ts,stealing_bit,NORMAL_BURST);
                insert_user_data(bss_msg1,nb,sizeof(NormalBurst));
                copy_bss_msg_options  (bss_msg1, bss_msg);
                copy_bss_msg_headers  (bss_msg1, bss_msg);
                if ( i == 3 ) {

                    /* Before sending out this block, RLC duplicates a packet carried
                     * by a block to prevent from segmentation fault caused by duplicate
                     * sending the same block.
                     */

                    copy_bss_msg_packet_field(bss_msg1,bss_msg);
                    //bss_msg1->packet = bss_msg->packet;
                }
                else
                    bss_msg1->packet = NULL;

                /* mac_option may already exist in CTL BURST */
                if ( !bss_msg1->mac_option ) {
                    create_mac_option(bss_msg1);
                }

                mac_option* mac_opt = reinterpret_cast<mac_option*> (bss_msg1->mac_option);

                if ( i==3 ) {
                    mac_option* src_macopt = reinterpret_cast<mac_option*> (bss_msg->mac_option);
                    if ( src_macopt ) {
                        if ( src_macopt->msg ) {
                            insert_macopt_msg( mac_opt , src_macopt->msg_type , src_macopt->msg );
                        }
                    }

                }

                mac_opt->burst_num = i; /* other information should be filled when the burst is transmitted */

                if ( !mac_opt->burst_type) {
                    if (rlc_opt->tfi>=0)
                        mac_opt->burst_type = NORMAL_BURST;
                    else
                        mac_opt->burst_type = CTL_BURST;

                }
                squeue->insert_tail(eburst_p);
            }
        }
        catch (std::exception& e) {
            cout <<"GprsBtsMac block_partition ():"<< e.what() <<endl;
        }
        return 1;
    }

    int GprsBtsMac::mark_listening_channel(long chid) {
        if (chid>=250 || chid<=0 ) {
            cout <<"GprsBtsMac mark_listening_channel(): chid ="<< chid <<" is out of range"<<endl;
            return -1;
        }
        listen_chmap[chid] = true;
        return 1;
    }

    int GprsBtsMac::set_listening_channel() {
         return radiolink_obj->set_listen_channel(listen_chmap);
    }

    int GprsBtsMac::send_timestamp(Event* ep , ulong chid , u_int32_t dst_nid , uchar btx_flag ) {

        IS_NULL_STR(ep,"GprsBtsMac send_timestamp(): Assertion failed ep is null\n",0);

        /* assertion */
        if ( (ulong)(ep->DataInfo_) == 0x7000001 || ep->timeStamp_ !=0 ) {
            printf("BTS MAC: find a bad event.\n");
            return 0;
        }

        try {
            bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
            IS_NULL_STR(bssmsg,"GprsBtsMac send_timestamp(): Assertion failed bssmsg is null\n",0);
            bssmsg->flag        = PF_SEND;
            if ( !bssmsg->mac_option ) {
                create_mac_option(bssmsg);
                reinterpret_cast<mac_option*>(bssmsg->mac_option)->burst_type = NORMAL_BURST; /* default settings */
            }

            rlc_option* rlc_opt  = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
            mac_option* mac_opt  = reinterpret_cast<mac_option*> (bssmsg->mac_option);
            mac_opt->fn          = fn;
            mac_opt->tn          = tn;
            mac_opt->bn          = bn;
            mac_opt->channel_id  = chid;
            mac_opt->btx_flag    = btx_flag;
            mac_opt->nid         = get_nid();

            /*MAC log*/
            struct logEvent         *logep;

            struct gprs_log *log1;
            u_int64_t       delay;
            uchar burst_len;
            char burst_type;


            if ( log_flag && !strcasecmp(GPRSLogFlag, "on") ) {

                if ( btx_flag != 1 )
                {
                    if( mac_opt->burst_type == NORMAL_BURST ||
                        mac_opt->burst_type == DUMMY_BURST  ||
                        mac_opt->burst_type == CTL_BURST )

                            burst_len = NB_DATA_LENGTH;
                        else
                            burst_len = 11;


                    if(mac_opt->burst_type == NORMAL_BURST)
                            burst_type = FRAMETYPE_GPRS_DATA;
                        else if(mac_opt->burst_type == ACCESS_BURST)
                            burst_type = FRAMETYPE_GPRS_ACCESS;
                        else if(mac_opt->burst_type == DUMMY_BURST)
                            burst_type = FRAMETYPE_GPRS_DUMMY;
                        else
                            burst_type = FRAMETYPE_GPRS_CTL;

                        log1 = (struct gprs_log*)malloc(sizeof(struct gprs_log));
                        MILLI_TO_TICK(delay,0.577);
                        LOG_GPRS(log1,GetCurrentTime()+delay,GetCurrentTime(),get_type()
                                ,get_nid(),StartTX,get_nid(),dst_nid,
                                mac_opt->burst_num,burst_type,burst_len,mac_opt->channel_id);
                        INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);
                }

            }

            /* perform burst number assertion */
            if ( (mac_opt->burst_num != fn_to_burst_num(mac_opt->fn)) ) {
                cout << "BTS MAC: incorrect burst number sent over this fn." << endl ;
                printf(" Dump mac_option: burst_type %d, burst_num %d, chid %ld, fn %ld, tn %d, bn %ld \n",
                        mac_opt->burst_type,mac_opt->burst_num,mac_opt->channel_id,mac_opt->fn,mac_opt->tn,mac_opt->bn);

                printf("      rlc_option: tfi %d ,ctl_msg_type %d ,cmd %d ,chid %ld \n",
                    rlc_opt->tfi,rlc_opt->ctl_msg_type,rlc_opt->cmd,rlc_opt->chid);
                exit(1);
            }

            /* perform channel consistency checking */

            if ( (ulong)(rlc_opt->chid) != chid ) {
                printf( "BTS MAC: rlc_opt->chid %lu != specified chid %lu.\n", rlc_opt->chid , chid );
                rlc_opt->chid = chid;
                exit(1);
            }

            #ifdef __TICK_OBSERVE_

            printf( "BTS MAC: send a burst on chid = %ld on tn[%ld] at tick = %llu \n",
                mac_opt->channel_id , tn , GetCurrentTime() );

            #endif

            #ifdef  __DETAILED_BTS_DEBUG
            {

                printf("GprsBtsMac send_timestamp(): sends a burst with profile as follows:\n");
                printf("    rlc_option: tfi %d ,ctl_msg_type %d ,cmd %d ,chid %ld, user_data_bsn = %d \n",
                    rlc_opt->tfi,rlc_opt->ctl_msg_type,rlc_opt->cmd,rlc_opt->chid, (int)(rlc_opt->user_data_bsn));
                printf("    mac_option: burst_type %d, burst_num %d, chid %ld, fn %ld, tn %ld, bn %ld \n\n",
                    mac_opt->burst_type,mac_opt->burst_num,mac_opt->channel_id,mac_opt->fn,mac_opt->tn,mac_opt->bn);
            }
            #endif

            #ifdef __OTHER_BTS_SEND_OBSERVE_DEBUG
            if (mac_opt->channel_id == 131 ) {

                printf("GprsBtsMac send_timestamp(): sends a burst with profile as follows:\n");
                printf("    rlc_option: tfi %d ,ctl_msg_type %d ,cmd %d ,chid %ld, user_data_bsn = %d \n",
                    rlc_opt->tfi,rlc_opt->ctl_msg_type,rlc_opt->cmd,rlc_opt->chid, (int)(rlc_opt->user_data_bsn) );
                printf("    mac_option: burst_type %d, burst_num %d, chid %ld, fn %ld, tn %ld, bn %ld \n\n",
                    mac_opt->burst_type,mac_opt->burst_num,mac_opt->channel_id,mac_opt->fn,mac_opt->tn,mac_opt->bn);
            }
            #endif

            if ( rlc_opt->detach_flag && ( rlc_opt->blk_position == (rlc_opt->blk_cnt-1) ) && mac_opt->burst_num == 3 ) {

                Event* notify_event;
                CREATE_BSS_EVENT(notify_event);
                bss_message* ne_bmsg = reinterpret_cast<bss_message*> (notify_event->DataInfo_);

                ne_bmsg->flag       = PF_RECV;

                create_rlc_option( ne_bmsg );
                rlc_option* ne_rlc_opt = reinterpret_cast<rlc_option*> (ne_bmsg->rlc_option);

                ne_rlc_opt->tfi         = rlc_opt->tfi;
                ne_rlc_opt->chid        = rlc_opt->chid;
                ne_rlc_opt->detach_tlli = rlc_opt->detach_tlli;
                ne_rlc_opt->cmd         = LLGMM_DETACH_SENT_COMPLETE;

                for ( int i=0 ; i < 250 ; ++i ) {

                    if ( bssdes->ch_map[i] ) {

                        TbfDes* tbfdes = bssdes->ch_map[i]->get_tbfdes_by_tlli( rlc_opt->detach_tlli);

                        if ( tbfdes ) {
                            tbfdes->detach_flag = true;
                        }
                    }
                }

                NslObject::recv( notify_event );

                TlliCacheElem* elem = tlli_cache->search_by_tlli( rlc_opt->detach_tlli );

                if (elem) {

                    TbfDes* tmp1;
                    TbfDes* tmp2;

                    tmp1 = elem->utcb;
                    tmp2 = elem->dtcb;

                    uchar remove_flag = false;

                    if ( tmp1 ) {

                        tmp1->detach_flag = true;

                        if ( tmp1->llgmm_reset_flag ) {
                            bssdes->ch_map[tmp1->uplink_freq]->remove_tbfdes_entry( tmp1 );
                            remove_flag = true;
                        }
                    }

                    if ( tmp2 ) {

                        tmp2->detach_flag = true;

                        if ( tmp2->llgmm_reset_flag ) {
                            bssdes->ch_map[tmp2->downlink_freq]->remove_tbfdes_entry( tmp2 );
                            remove_flag = true;
                        }

                    }

                    if ( remove_flag ) {

                        tlli_cache->remove( rlc_opt->detach_tlli  );

                        if ( tmp1 )
                            delete tmp1;

                        if ( tmp2 )
                            delete tmp2;

                        cout << "BTS MAC send(): remove TCBs with tlli = " << rlc_opt->detach_tlli << endl;
                    }

                }

            }

            return NslObject::send(ep);
        }
        catch (std::exception& e) {
            cout << "GprsBtsMac send_timestamp(): Assertion failed cause:"<< e.what() << endl;
            return 0;
        }
    }

    Event* GprsBtsMac::create_si13_msg() {
        Si13PbcchLocation* si13_msg  = new Si13PbcchLocation;
        si13_msg->si13_location      = 0; /* the si_tn shall always be tn=0 */
        si13_msg->pbcch_location     = bssdes->pbcch_tn-1; /* the pbcch_tn -1 */
        si13_msg->psi1_repeat_period = bssdes->psi1_repeat_period;

        Event* esi13msg;
        CREATE_BSS_EVENT(esi13msg);
        bss_message* bssmsg      = reinterpret_cast<bss_message*> (esi13msg->DataInfo_);
        insert_user_data(bssmsg,si13_msg,sizeof(Si13PbcchLocation));

        create_rlc_option(bssmsg);
        rlc_option* rlc_opt     = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
        rlc_opt->ctl_msg_type   = SI13_INFO;
        rlc_opt->chid           = bssdes->bcch_no;
        rlc_opt->cs             = 1;
        rlc_opt->tfi            = -1;


        create_mac_option(bssmsg);
        mac_option* mac_opt      = reinterpret_cast<mac_option*> (bssmsg->mac_option);
        mac_opt->burst_type      = SI13_BURST;
        mac_opt->channel_id      = bssdes->bcch_no;
        return esi13msg;
    }

    Event* GprsBtsMac::create_psi1_msg() {

        Psi1*           psi1;
        DistrMsg*       dmsg;
        bss_message *bssmsg;
        mac_option  *mac_opt;
        rlc_option  *rlc_opt;

        try {
            Event* epsi1;
            CREATE_BSS_EVENT(epsi1);
            bssmsg = reinterpret_cast<bss_message*> (epsi1->DataInfo_);

            psi1 = new Psi1;
            dmsg = new DistrMsg;
            dmsg->set_pgmode(1);
            dmsg->set_distr_msg(PSI1, downlink , psi1);

            create_rlc_option(bssmsg);
            rlc_opt                 = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
            rlc_opt->ctl_msg_type   = PSI1;
            rlc_opt->chid           = bssdes->bcch_no;
            rlc_opt->cs             = 1;
            rlc_opt->tfi            = -1;

            create_mac_option(bssmsg);
            mac_opt                 = reinterpret_cast<mac_option*> (bssmsg->mac_option);
            mac_opt->channel_id     = rlc_opt->chid;
            //mac_opt->msg            = dmsg;
            insert_macopt_msg( mac_opt , DMSG , dmsg );
            mac_opt->burst_type     = CTL_BURST;

            //uchar* dummy_data = new uchar[1];
            uchar* dummy_data = dummy_data_array;
            bzero(dummy_data,1);
            insert_user_data(bssmsg, dummy_data , 1);

            psi1->pbcch_change_mark = ++bssdes->pbcch_change_mark%7;
            psi1->psi_change_field  = 0x0;
            psi1->psi_repeat_period = 0x6;
            psi1->psi_count_lr      = 0x3;
            psi1->psi_count_hr      = 0;
            psi1->measurement_order = 0; /* the advanced cell-reselection mode will be supported soon */
            psi1->gprs_cell_options.nmo                = 0; /* this field should be consistent with measurement_order in semantics */
            psi1->gprs_cell_options.timer3168          = 1;
            psi1->gprs_cell_options.timer3192          = 1;
            psi1->gprs_cell_options.drx_timer_max      = 0;
            psi1->gprs_cell_options.access_burst_type  = 1;
            psi1->gprs_cell_options.control_ack_type   = 1;
            psi1->gprs_cell_options.bs_cv_max          = 15;
            /*
            psi1->gprs_cell_options->pan_dec;
            psi1->gprs_cell_options->pan_inc;
            psi1->gprs_cell_options->pan_max;
            */

            psi1->prach_control_parameters.acc_ctl_class   = 0xfe; /* this field shall be ignored unless the multi-class GPRS is supported */
            psi1->prach_control_parameters.s               = 0x2;
            psi1->prach_control_parameters.tx_int          = 0x2;
            memset( psi1->prach_control_parameters.max_retrans , 0x3 , 4 );
            memset( psi1->prach_control_parameters.persistence_level , 0x0 , 4);

            psi1->pccch_organization_parameters.bs_pcc_rel         = 0x0;
            psi1->pccch_organization_parameters.bs_pbcch_blks      = bssdes->bs_pbcch_blks;
            psi1->pccch_organization_parameters.bs_pag_blks_res    = bssdes->bs_pag_blks_res;
            psi1->pccch_organization_parameters.bs_prach_blks      = bssdes->bs_prach_blks;

            /* Power control is not implemented so far */
            /* psi1->global_power_ctl_param;*/

            /* The following fields are set as zero.
            psi1->psi_status_ind;
            psi1->msc_r;
            psi1->sgsn_r;
            psi1->band_indicator;
            */

            return epsi1;
        }
        catch (std::exception& e) {
            cout << "GprsBtsMac create_psi1_msg(): Assertion failed :"<< e.what() << endl;
            return NULL;
        }

    }

    Event* GprsBtsMac::create_psi2_msg() {
        Psi2*       psi2;
        Event*      epsi;
        DistrMsg*   dmsg;
        bss_message *bssmsg;
        mac_option  *mac_opt;
        rlc_option  *rlc_opt;

        try {

            CREATE_BSS_EVENT(epsi);
            bssmsg  = reinterpret_cast<bss_message*> (epsi->DataInfo_);

            psi2 = new Psi2;
            dmsg = new DistrMsg;
            dmsg->set_pgmode(1);
            dmsg->set_distr_msg(PSI2, downlink, psi2);

            create_rlc_option(bssmsg);
            rlc_opt                 = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
            rlc_opt->ctl_msg_type   = PSI2;
            rlc_opt->chid           = bssdes->bcch_no;
            rlc_opt->cs             = 1;
            rlc_opt->tfi            = -1;

            create_mac_option(bssmsg);
            mac_opt = reinterpret_cast<mac_option*> (bssmsg->mac_option);
            mac_opt->channel_id     = rlc_opt->chid;
            mac_opt->burst_type     = CTL_BURST;
            //mac_opt->msg            = dmsg;
            insert_macopt_msg( mac_opt , DMSG , dmsg );

            //uchar* dummy_data = new uchar[1];
            uchar* dummy_data = dummy_data_array;
            bzero(dummy_data,1);
            insert_user_data(bssmsg,dummy_data,1);

            psi2->psi2_change_mark  = bssdes->psi2_change_mark;
            psi2->psi2_index        = 0;
            psi2->psi2_count        = 1;
            psi2->cell_identification.lai  = bssdes->lai;
            psi2->cell_identification.rac  = bssdes->rac;
            psi2->cell_identification.cell_identity = bssdes->cell_identity ;

            /* Reference Freq. List sturct */
            psi2->rfl_number                = 0;
            psi2->length_of_rfl_contents    = 0;
            psi2->rfl_contents              = 0;

            /* Gprs mobile allocation struct */
            psi2->ma_number                 = 15;
            /* this information is ignored in this version
                psi2->gprs_ma_struct; */

            /* Pccch description lists struct */
            psi2->tsc                   = 0x01;
            /* Non_hopping_pccch_carriers_struct */
            psi2->arfcn                         = bssdes->bcch_no;
            psi2->time_slot_allocation  = bssdes->pbcch_tn;

            /* Additional PSI message struct */
            psi2->psi8_broadcast                = 0;
            psi2->psi3ter_broadcast             = 0;
            psi2->psi3quater_broadcast          = 0;

            return epsi;
        }
        catch (std::exception& e) {
            cout << "GprsBtsMac create_psi2_msg(): Assertion failed :"<< e.what() << endl;
            return NULL;
        }
    }

    Event* GprsBtsMac::create_psi3_msg() {

        Psi3*           psi3;
        Event*          epsi;
        DistrMsg*       dmsg;
        bss_message*    bssmsg;
        mac_option*     mac_opt;
        rlc_option*     rlc_opt;

        try {

            CREATE_BSS_EVENT(epsi);
            bssmsg  = reinterpret_cast<bss_message*> (epsi->DataInfo_);

            psi3    = new Psi3;
            dmsg    = new DistrMsg;
            dmsg->set_pgmode(1);
            dmsg->set_distr_msg(PSI3, downlink, psi3);

            create_rlc_option(bssmsg);
            rlc_opt = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
            rlc_opt->ctl_msg_type   = PSI3;
            rlc_opt->chid           = bssdes->bcch_no;
            rlc_opt->cs             = 1;
            rlc_opt->tfi            = -1;

            create_mac_option(bssmsg);
            mac_opt = reinterpret_cast<mac_option*> (bssmsg->mac_option);
            mac_opt->channel_id     = rlc_opt->chid;
            mac_opt->burst_type     = CTL_BURST;
            insert_macopt_msg( mac_opt , DMSG , dmsg );

            uchar* dummy_data = dummy_data_array;
            bzero(dummy_data,1);
            insert_user_data(bssmsg,dummy_data,1);
            //printf("dummy_data = %u dummy_data_array = %u \n", dummy_data , dummy_data_array);


            #ifdef __USE_SPEC_STRUCTURE__
                psi3->psi3_change_mark    = bssdes->psi3_change_mark;
                psi3->psi_bis_count       = 0; /* not used */

                /* Serving cell parameter struct */
                psi3->serving_cell_params.cell_bar_access   = 0; /* normal */
                psi3->serving_cell_params.exc_acc           = 0; /* not used for SoLSA exclusive access */
                psi3->serving_cell_params.gprs_rxlevel_access_min   = 0; /* power management will be implemented soon */
                psi3->serving_cell_params.gprs_ms_txpower_max_cch   = 0;

                /* General cell select parameters */
                psi3->gen_cell_sel.gprs_cell_reselect_hysteresis    = 0;
                psi3->gen_cell_sel.c31_hyst = 0;
                psi3->gen_cell_sel.c32_qual = 0;
                psi3->gen_cell_sel.random_access_retry = 1;
                psi3->gen_cell_sel.t_resel = 0; /* 5 sec */
                psi3->gen_cell_sel.ra_reselect_hysteresis = 0;

                /* Neighbor cell parameters struct */
                /* roaming is not implemented so far :
                psi3->neighbor_cell_params.start_frequency = ;
                CellSelectionStruct cell_selection_struct;
                uchar  nr_of_remaining_cells;
                uchar  freq_diff_length;
                CelSelectionStruct    *freq_diff;;*/
            #else

                if ( neighbor_cell_list )
                    neighbor_cell_list->create_d_bssdes_list( psi3->neighbor_cell );
                else {

                    cout << "GprsBtsMac create_psi3_msg(): neighbor cell list does not exist."<< endl;
                }


            #endif

            return epsi;

        }

        catch (std::exception& e) {
            cout << "GprsBtsMac create_psi3_msg(): Assertion failed :"<< e.what() << endl;
            return NULL;
        }

    }

    long GprsBtsMac::fn_to_blkn(ulong fn1) {
        long blkn1;
        if ( fn1>=0 ) {
            if ( fn1 == 12 || fn1 == 25 || fn1 == 38 || fn1 == 51)
                blkn1 = -1;
            else
                blkn1 = ( fn1 - (fn1-(fn1/12))/12 ) /4;
        }
        return blkn1;
    }

    Event* GprsBtsMac::create_pbcch_msg() {
        int sel = blkn%2;
        if ( sel == 0)
            return create_psi2_msg();
        else
            return create_psi3_msg();
    }

    int GprsBtsMac::is_pbcch() {
        for (int i=0; i<bssdes->bs_pbcch_blks ;++i) {
            if ( blkn == bssdes->pdtch_downlink_multi_seq[i] )
                return 1;
        }
        return 0;
    }

   int GprsBtsMac::is_pccch() {
       for (int i=0; i<bssdes->bs_pag_blks_res ;++i) {
           if ( blkn == bssdes->pdtch_downlink_multi_seq[bssdes->bs_pbcch_blks+i] )
               return 1;
       }
       return 0;
   }

    PacketUplinkAssignment* GprsBtsMac::single_blk_allocation(bss_message* bssmsg) {

        /* if uplink resource is available, network shall respond with Packet Uplink Assignment message
         * Otherwise, network shall send a Access Reject message
         * Note: Two phase establishment procedure is adopted by default.
         */

        mac_option* mac_opt = reinterpret_cast<mac_option*> (bssmsg->mac_option);
        ulong ta_value      = bn - 0;

        if ( ta_value >= 64 ) {
            cout <<
                "GprsBtsMac single_blk_allocation(): <Assertion failed> cause: ta_value is too large (MS is too far from this bss)"
                    << endl;
            cout << "       Report is shown as the following:"<< endl;
            printf( "           mac_opt->bn %ld mac_opt->tn %d mac_opt->fn %ld \n", mac_opt->bn , mac_opt->tn , mac_opt->fn);
            printf( "           bss->bn %ld bss->tn %ld bss->fn %ld \n", bn , tn , fn);
            exit(1);
            //return NULL;
        }
        else
            printf("BTS MAC [%u]: calculate ta_value = %ld \n", get_nid(), ta_value);

        TbfDes*  tmpdes = NULL;

        int least_loading_ch;
        int least_tfi_cnt;

        /* try to find a channel with less load */

        least_loading_ch    = 250;
        least_tfi_cnt       = 32;

        for (int i=(UPLINK_START_CH(bssdes->start_ch)); i<=(UPLINK_END_CH(bssdes->end_ch)) ; ++i ) {

            if ( i == CORRESPONDING_UPLINK_CH(bssdes->bcch_no) )
                continue;

            if ( (bssdes->ch_map[i]->get_tfi_cnt()) < least_tfi_cnt ) {
                least_tfi_cnt       = bssdes->ch_map[i]->get_tfi_cnt();
                least_loading_ch    = i;
            }
        }

        if (least_tfi_cnt>=32) {
            cout << "GprsBtsMac::single_block_allocation(): BTS is out of radio resources.\n" << endl;
            return NULL;
        }

        uchar rank[8];
        uchar included[8];
        memset(rank,32,8);
        bzero(included,8);

        for (int i=0 ; i<8 ; ++i) {

            ulong least_utilization = 32;
            ulong least_tn          = 10;

            for (int j=0 ; j<8 ;++j ) {
                if (included[j])
                    continue;
                if ( !bssdes->ch_map[least_loading_ch])
                    continue;
                if ( !bssdes->ch_map[least_loading_ch]->tn[j] )
                    continue;
                if ( (bssdes->ch_map[least_loading_ch]->tn[j]->get_list_num()) < least_utilization ) {
                    least_utilization = (bssdes->ch_map[least_loading_ch]->tn[j]->get_list_num());
                    least_tn = j;
                }
            }

            if ( least_tn >=8 ) {
                printf("Rank Procedure cannot find a valid value.\n");
                break;
            }

            rank[i] = least_tn;
            included[least_tn] = true;
        }
        /* valid count checking */
        ulong valid_cnt = 0;
        for (int i=0 ; i<8 ;++i) {
            if ( included[i] )
                ++valid_cnt;
        }

        if ( valid_cnt < 1 ) {
            cout <<"GprsBtsMac single_block_allocation(): BTS is out of available resource." << endl;
            return NULL;
        }




        int chosen = false;
        for (int i=0 ; i<8 ; ++i ) {

            if ( chosen)
                break;
            if ( sbalist->search(least_loading_ch,rank[i]) ) {
                printf ("BTS MAC: single_block_allocation(): ch[%d] tn[%d] is already assigned to an SBA.",
                    least_loading_ch , rank[i] );
                continue;
            }

            tmpdes                          = new TbfDes(SBA_TYPE,gprs::bts);
            tmpdes->uplink_freq             = least_loading_ch;
            tmpdes->downlink_freq           = CORRESPONDING_DOWNLINK_CH(least_loading_ch);
            tmpdes->ts_alloc[rank[i]]       = true;
            bssdes->ch_map[least_loading_ch]->tn[rank[i]]->insert_tail(tmpdes);
            bssdes->ch_map[CORRESPONDING_DOWNLINK_CH(least_loading_ch)]->tn[rank[i]]->insert_tail(tmpdes);
            chosen = true;
        }

        if (!chosen) {
            cout << "BTS MAC: single_block_allocation(): all timeslots are already assigned. BTS is out of resource" << endl;
            return NULL;
        }

        tmpdes->ch_coding_scheme            = 1; /* SBA shall carry control message that is encoded by CS1 */
        tmpdes->pacch_sq[tn]                = new SList<Event>;
        tmpdes->state                       = PACKET_TRANSFER_STATE;

        SBA* sba                            = new SBA;
        FrequencyParameter* fp              = new FrequencyParameter;

        sba->tn                             = rank[0];

        #ifdef __EXPLICITLY_FN__

        if ( blkn == -1)
            sba->tbf_starting_time          = blkn_to_starting_fn( fn_to_blkn(fn+1) );
        else
            sba->tbf_starting_time          = blkn_to_starting_fn(blkn+1);
        #else

            sba->tbf_starting_time          = 0; /* use count down value */

        #endif

        fp->tsc                             = bssdes->tsc;
        fp->format                          = 0x00;
        fp->arfcn                           = least_loading_ch;

        SbaFreq* sbafreqdes                 = new SbaFreq;
        SBA* sba_dup                        = new SBA;
        FrequencyParameter* fp_dup          = new FrequencyParameter;
        memcpy( sba_dup , sba , sizeof(SBA));
        memcpy( fp_dup, fp , sizeof(FrequencyParameter));

        sbafreqdes->sba                     = sba_dup;
        sbafreqdes->fp                      = fp_dup;
        sbalist->insert_tail(sbafreqdes);
        tmpdes->insert_sba(sbafreqdes,least_loading_ch);

        PUA* pua                                            = new PUA;
        pua->frequency_params                               = fp;
        pua->sba_or_da                                      = 0; /* contains dynamic allocation structure */
        pua->packet_timing_advance.timing_advance_value     = ta_value;
        pua->tlli_block_channel_coding                      = 1; /* channel_coding should reference the value specified */
        pua->single_block_allocation                        = sba;

        for ( int i=0 ; i<4 ;++i)
            pua->persistence_level[i]  = 1;

        #ifdef __SINBLE_BLK_ALLOCATION_DECISION_DEBUG

        #endif

        return pua;
    }

    PacketDownlinkAssignment* GprsBtsMac::downlink_tbf_allocation(TbfDes* utbfdes, uchar num_of_allocated_blk) {

        /* if the uplink tbf for the tlli has been established, the (tn+3) shall not be
         * selected because MS may send data at uplink_tn == tn, i.e. (tn+3).
         * And the PktDownlinkAssignment message shall be sent over PACCH of uplink TBF.
         */
        if (num_of_allocated_blk<=0)
            return NULL;

        int least_loading_ch;
        int least_tfi_cnt;

        /* try to find a channel with less load */

        least_loading_ch    = 250;
        least_tfi_cnt       = 32;

        for (int i=DOWNLINK_START_CH(bssdes->start_ch); i<=DOWNLINK_START_CH(bssdes->end_ch) ; ++i ) {

            if ( i == (bssdes->bcch_no) )
                continue;

            if ( (bssdes->ch_map[i]->get_tfi_cnt()) < least_tfi_cnt ) {
                least_tfi_cnt       = bssdes->ch_map[i]->get_tfi_cnt();
                least_loading_ch    = i;
            }
        }

        if (least_tfi_cnt>=32) {
            cout << "GprsBtsMac::downlink_tbf_allocation(): BTS is out of radio resources.\n" << endl;
            return NULL;
        }

        uchar rank[8];
        uchar included[8];
        memset(rank,32,8);
        bzero(included,8);

        for (int i=0 ; i<8 ; ++i) {

            ulong least_utilization = 32;
            ulong least_tn          = 10;

            for (int j=0 ; j<8 ;++j ) {
                if (included[j])
                    continue;
                if ( !bssdes->ch_map[least_loading_ch])
                    continue;
                if ( !bssdes->ch_map[least_loading_ch]->tn[j] )
                    continue;
                if ( (bssdes->ch_map[least_loading_ch]->tn[j]->get_list_num()) < least_utilization ) {
                    least_utilization = (bssdes->ch_map[least_loading_ch]->tn[j]->get_list_num());
                    least_tn = j;
                }
            }

            if ( least_tn >=8 ) {
                printf("Rank Procedure cannot find a valid value.\n");
                break;
            }

            rank[i] = least_tn;
            included[least_tn] = true;
        }

        /* valid count checking */
        ulong valid_cnt = 0;
        for (int i=0 ; i<8 ;++i) {
            if ( included[i] )
                ++valid_cnt;
        }

        if ( valid_cnt < num_of_allocated_blk ) {
            cout <<"GprsBtsMac downlink_tbf_allocation(): BTS is out of available resource." << endl;
            return NULL;
        }


        TbfDes* tmpdes              = new TbfDes(SENDER,gprs::bts);
        tmpdes->nodetype            = 1; /* bts */
        tmpdes->tfi                 = bssdes->ch_map[least_loading_ch]->get_an_empty_tfi();
        tmpdes->downlink_freq       = least_loading_ch;
        tmpdes->uplink_freq         = CORRESPONDING_UPLINK_CH(least_loading_ch);
        if (utbfdes) {
            tmpdes->tlli = utbfdes->tlli;
            tmpdes->imsi = utbfdes->imsi;
            tmpdes->nid  = utbfdes->get_nid();
        }
        bssdes->ch_map[least_loading_ch]->set_tbfdes(tmpdes->tfi,tmpdes);
        bssdes->ch_map[least_loading_ch]->tbfdes[tmpdes->tfi] = tmpdes;

        tlli_cache->insert(tmpdes);

        int num_of_assigned_blk = 0;

        for (int i=0 ; i<8 ; ++i ) {

            if ( utbfdes ) {

                int index = rank[i]-3;
                if ( index  < 0 )
                    index +=8;

                if ( (utbfdes->ts_alloc[rank[i]]) )
                    continue;

                if ( (utbfdes->ts_alloc[index]) )
                    continue;


            }

            tmpdes->ts_alloc[rank[i]] = true;
            bssdes->ch_map[least_loading_ch]->tn[rank[i]]->insert_tail(tmpdes);

            if ( ++num_of_assigned_blk >= num_of_allocated_blk)
                break;
        }

        tmpdes->ch_coding_scheme    = 2;
        tmpdes->pdtch_sq[tn]        = new SList<Event>;
        tmpdes->pacch_sq[tn]        = new SList<Event>;
        tmpdes->state               = PACKET_TRANSFER_STATE;

        PDA* pda                    = new PDA;
                FrequencyParameter* fp      = new FrequencyParameter;
        fp->tsc                     = bssdes->tsc;
        fp->format                  = 0x0;
        fp->arfcn                   = least_loading_ch;

        pda->freq_param             = fp;

        pda->ts_alloc               = 0;

        for (int i=0; i<8 ;++i) {
            if (tmpdes->ts_alloc[i] == true )
                pda->ts_alloc |= (0x01<<i);
        }
        pda->downlink_tfi_assignment    = tmpdes->tfi;
        pda->tbf_starting_time          = blkn_to_starting_fn(blkn+1);

        /* enqueue tmpdes into partially-established dtbf list */
        pedl->insert_tail(tmpdes);


        /* The report of the establishment of a downlink TBF is performed by gprs_scheduler */

        return pda;
    }

    PacketUplinkAssignment* GprsBtsMac::uplink_tbf_allocation(bss_message* bssmsg,ulong ms_tlli, uchar num_of_allocated_blk) {

        /* if uplink resource is available, network shall respond with Packet Uplink Assignment message
         * Otherwise, network shall send a Packet Access Reject message
         * Note: Two phase establishment procedure is used as default.
         */
        if (num_of_allocated_blk<=0)
            return NULL;

        /* examine whether dtbfdes exists to make the uplink time slot assignment orthogonal to
         * those assigned for downlink.
         */

        TbfDes* dtbfdes;
        if ( (ms_tlli&0x0000001e) == 0x0000001e ) /* random TLLI */
            dtbfdes = NULL;
        else {
            for (int i = DOWNLINK_START_CH(bssdes->start_ch); i<= DOWNLINK_END_CH(bssdes->end_ch) ;++i)
                if ( (dtbfdes = bssdes->ch_map[i]->get_tbfdes_by_tlli(ms_tlli)) ) {
                    break;
                }
        }

        mac_option* mac_opt = reinterpret_cast<mac_option*> (bssmsg->mac_option);
        ulong ta_value = bn - 0;
        if ( ta_value >= 64 ) {
            cout <<
                "GprsBtsMac uplink_tbf_allocation(): <Assertion failed> cause: ta_value is too large (MS is too far from this bss)"
                    << endl;
            exit(1);
        }
        else
            printf("BTS MAC [%u]: calculate ta_value = %ld \n", get_nid(), ta_value);

        FrequencyParameter* fp  = NULL;
        DynAlloc*   dyn_alloc   = NULL;
        TbfDes*     tmpdes      = NULL;

        int least_loading_ch;
        int least_tfi_cnt;

        least_loading_ch    = 250;
        least_tfi_cnt       = 32;

        for (int i=(UPLINK_START_CH(bssdes->start_ch)); i<=(UPLINK_END_CH(bssdes->end_ch)) ; ++i ) {

            if ( i == CORRESPONDING_UPLINK_CH(bssdes->bcch_no) )
                continue;

            if ( (bssdes->ch_map[i]->get_tfi_cnt()) < least_tfi_cnt ) {
                least_tfi_cnt       = bssdes->ch_map[i]->get_tfi_cnt();
                least_loading_ch    = i;
            }
        }

        if (least_tfi_cnt>=32) {
            cout << "GprsBtsMac::uplink_tbf_allocation(): BTS is out of radio resources.\n" << endl;
            return NULL;
        }

        uchar rank[8];
        uchar included[8];
        memset(rank,32,8);
        bzero(included,8);

        for (int i=0 ; i<8 ; ++i) {

            ulong least_utilization = 32;
            ulong least_tn          = 10;

            for (int j=0 ; j<8 ;++j ) {
                if (included[j])
                    continue;
                if ( !bssdes->ch_map[least_loading_ch])
                    continue;
                if ( !bssdes->ch_map[least_loading_ch]->tn[j] )
                    continue;
                if ( (bssdes->ch_map[least_loading_ch]->tn[j]->get_list_num()) < least_utilization ) {
                    least_utilization = (bssdes->ch_map[least_loading_ch]->tn[j]->get_list_num());
                    least_tn = j;
                }
            }

            if ( least_tn >=8 ) {
                printf("Rank Procedure cannot find a valid value.\n");
                break;
            }

            rank[i] = least_tn;
            included[least_tn] = true;
        }

        /* valid count checking */
        ulong valid_cnt = 0;
        for (int i=0 ; i<8 ;++i) {
            if ( included[i] )
                ++valid_cnt;
        }

        if ( valid_cnt < num_of_allocated_blk ) {
            cout <<"GprsBtsMac uplink_tbf_allocation(): BTS is out of available resource." << endl;
            return NULL;
        }

        tmpdes                      = new TbfDes(RECEIVER,gprs::bts);
        tmpdes->nid                 = mac_opt->nid;
        tmpdes->tlli                = ms_tlli;
        tmpdes->nodetype            = gprs::bts; /* bts */
        tmpdes->tfi                 = bssdes->ch_map[least_loading_ch]->get_an_empty_tfi();
        tmpdes->uplink_freq         = least_loading_ch;
        tmpdes->downlink_freq       = CORRESPONDING_DOWNLINK_CH(least_loading_ch);
        bssdes->ch_map[least_loading_ch]->set_tbfdes(tmpdes->tfi,tmpdes);
        bssdes->ch_map[least_loading_ch]->tbfdes[tmpdes->tfi] = tmpdes;

        tlli_cache->insert(tmpdes);

        for (int i=0 ; i<num_of_allocated_blk ; ++i ) {
            tmpdes->ts_alloc[rank[i]] = true;
            bssdes->ch_map[least_loading_ch]->tn[rank[i]]->insert_tail(tmpdes);
        }

        tmpdes->ch_coding_scheme    = 2;
        tmpdes->pacch_sq[tn]        = new SList<Event>;
        tmpdes->state               = PACKET_TRANSFER_STATE;

        fp                                                  = new FrequencyParameter;
        fp->tsc                                 = bssdes->tsc;
        fp->format                              = 0x0;
        fp->arfcn                               = least_loading_ch;

        dyn_alloc                               = new DynAlloc;
        dyn_alloc->extended_dynamic_allocation_indicator = 0; /* normal dynalloc */
        dyn_alloc->p0                               = 0;
        dyn_alloc->pr_mode                          = 0; /* for one addressed MS */
        dyn_alloc->usf_granularity                  = 0;
        dyn_alloc->tbf_starting_time                = blkn_to_starting_fn(blkn+1);
        dyn_alloc->uplink_tfi_assignment            = tmpdes->tfi;

        int num_of_assigned_blk = 0;

        for (int i=0 ; i<8 ;++i) {


            if ( dtbfdes ) {

                if ( dtbfdes->ts_alloc[((rank[i]+3)%8)] )
                    continue;

                if ( dtbfdes->ts_alloc[rank[i]] )
                    continue;
            }

            uchar usf_value;
            bool  is_pbcch_carrier = (i == DOWNLINK_START_CH(bssdes->start_ch))?true:false;
            if ( (usf_value=bssdes->ch_map[least_loading_ch]->get_usf_in_tn (rank[i],tmpdes->tfi,is_pbcch_carrier)) == 100 )
                continue; /* the timeslot has been assigned to the maximal number of users */
            else {
                dyn_alloc->usf_tn[rank[i]] = usf_value;
                dyn_alloc->tn_presence[rank[i]]   = true;
                tmpdes->usf_tn[rank[i]] = usf_value;
            }

            if ( ++num_of_assigned_blk >= num_of_allocated_blk )
                break;
        }

        dyn_alloc->uplink_tfi_assignment_presence   = true;
        dyn_alloc->tbf_starting_time_presence       = true;
        dyn_alloc->ta_or_ta_pow                     = 0; /* without power ctl param */


        PUA* pua                                                                                = new PUA;
        pua->frequency_params                                                           = fp;
        pua->sba_or_da                                                                          = 1; /* contains dynamic allocation structure */
        pua->packet_timing_advance.timing_advance_value         = ta_value;
        pua->tlli_block_channel_coding                                          = 1; /* channel_coding should reference the value specified */
        pua->dynamic_allocation                                                 = dyn_alloc;
        pua->channel_coding_command                         = 2; /* default setting */

        for ( int i=0 ; i<4 ;++i)
            pua->persistence_level[i]  = 1;

        ack_rlc(UPLINK_TBF_ESTABLISHED,tmpdes);
        return pua;
    }

    Event* GprsBtsMac::create_paging_request(ulong imsi) {

        try {
            PacketPagingRequest* ppr    = new PacketPagingRequest;
            ppr->ptmsi_or_imsi          = 1;
            ppr->imsi                   = imsi;
            ppr->imsi_len               = 4;

                        Event* epage;
                        CREATE_BSS_EVENT(epage);
                        bss_message* bssmsg                     = reinterpret_cast<bss_message*> (epage->DataInfo_);
            bssmsg->flag                = PF_SEND;
            DistrMsg* dmsg              = new DistrMsg;
            dmsg->set_pgmode(1);
            dmsg->set_distr_msg( PKT_PAGING_REQUEST , downlink,  ppr );

            //uchar* dummy_data = new uchar[1];
            uchar* dummy_data = dummy_data_array;
            bzero(dummy_data,1);
            insert_user_data(bssmsg,dummy_data,1);

            create_rlc_option(bssmsg);
            rlc_option* rlc_opt         = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

            /* examine whether the uplink TBF for this MS has been already established */


            rlc_opt->ctl_msg_type       = PKT_PAGING_REQUEST;
            rlc_opt->chid               = bssdes->bcch_no;
            rlc_opt->cs                 = 1;
            rlc_opt->tfi                = -1;

            create_mac_option(bssmsg);
            mac_option* mac_opt                 = reinterpret_cast<mac_option*> (bssmsg->mac_option);
            mac_opt->channel_id         = rlc_opt->chid;
            //mac_opt->msg                              = dmsg;
            insert_macopt_msg( mac_opt , DMSG , dmsg );
            mac_opt->burst_type                 = CTL_BURST;

            return epage;
        }
        catch (std::exception& e) {
            cout << "GprsBtsMac create_paging_request(): ASSERTION failed cause = "  << e.what() << endl;
            return NULL;
        }
    }

    int GprsBtsMac::add_mac_header(Event* ep,uchar blktype, uchar usf) {

        IS_NULL_STR(ep,"GprsBtsMac add_mac_header(): ep is null\n" ,-1);

        bss_message* bssmsg     = reinterpret_cast<bss_message*> (ep->DataInfo_);
        rlc_option*  rlc_opt    = reinterpret_cast<rlc_option*>  (bssmsg->rlc_option);
        //mac_option*  mac_opt    = reinterpret_cast<mac_option*>  (bssmsg->mac_option);

        uchar rrbp   = rlc_opt->rrbp;
        uchar sp_bit = true;
        if (rrbp>3) {
            rrbp = 0x0;
            sp_bit = false;
        }

        DMH* dmh;
        if (!bssmsg->mac_header) {
            dmh                     = new DMH;
            insert_mac_header( bssmsg, dmh , sizeof(DMH));
        }
        else {
            dmh                     = reinterpret_cast<DMH*> (bssmsg->mac_header);
            bssmsg->mac_header_len  = sizeof(DMH);
            //cout << "GprsBtsMac add_mac_header(): mac_header has already been allocated"<< endl;
        }

        if ( blktype == DATA_BLK )
            dmh->payload_type  = 0x00;
        else if ( blktype == CTL_BLK )
            dmh->payload_type  = 0x01;
        else
            cout <<"GprsBtsMac add_mac_header(): unknown blktype indication" <<endl;

        dmh->rrbp           = rrbp; /* 3 blks later */
        dmh->sp             = sp_bit;
        dmh->usf            = usf; /* 111 stands for unused state */

        return 1;
    }

    Event* GprsBtsMac::create_pkt_access_reject() {
        try {
            PAR* par                    = new PAR;
            par->wait_indication        = 1;
            par->wait_indication_size   = 0;
            DistrMsg* dmsg              = new DistrMsg;
            dmsg->set_pgmode(1);
            dmsg->set_distr_msg(PKT_ACCESS_REJECT, downlink, par);

            Event* epar;
                        CREATE_BSS_EVENT(epar);
                        bss_message* bssmsg         = reinterpret_cast<bss_message*> (epar->DataInfo_);

            create_rlc_option(bssmsg);
            rlc_option* rlc_opt         = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
            rlc_opt->ctl_msg_type       = PKT_ACCESS_REJECT;
            rlc_opt->chid               = bssdes->bcch_no;
            rlc_opt->cs                 = 1;
            rlc_opt->tfi                = -1;

            create_mac_option(bssmsg);
            mac_option* mac_opt         = reinterpret_cast<mac_option*> (bssmsg->mac_option);
            mac_opt->channel_id         = rlc_opt->chid;
            //mac_opt->msg                = dmsg;
            insert_macopt_msg( mac_opt , DMSG , dmsg );
            mac_opt->burst_type         = CTL_BURST;

            //uchar* dummy_data = new uchar[1];
            uchar* dummy_data = dummy_data_array;
            bzero(dummy_data,1);
            insert_user_data(bssmsg,dummy_data,1);

            return epar;
        }
        catch (std::exception& e) {
            cout << "GprsBtsMac create_pkt_access_reject(): Assertion failed :"<< e.what() << endl;
            return NULL;
        }
    }

    int GprsBtsMac::create_dummy_burst_msg( long channel_id ) {

        if ( (channel_id <125) || (channel_id >= 250) ) {
            cout <<"GprsBtsMac create_dummy_burst(): channel_id " << channel_id << " is out of range." << endl;
            return -1;
        }

        uchar usf_value = bssdes->ch_map[CORRESPONDING_UPLINK_CH(channel_id)]->choose_usf_in_tn(tn);

        if (usf_value == 100 )
            usf_value = 0x07;/* random */

        #ifdef __USF_DEBUG
        cout << "BTSMac: (B) CH " << channel_id << " usf_value at tn = " << tn << " usf = " << (int) (usf_value) << endl;
        #endif

        for (int i=0 ; i<4 ;++i) {

            Event* edb;
            CREATE_BSS_EVENT(edb);

            bss_message* dummy_bmsg = reinterpret_cast<bss_message*> (edb->DataInfo_);


            //uchar* dummy_data = new uchar[1];
            uchar* dummy_data = dummy_data_array;
            bzero(dummy_data,1);
            insert_user_data(dummy_bmsg, dummy_data , 1 );

            create_rlc_option(dummy_bmsg);
            rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (dummy_bmsg->rlc_option);
            rlc_opt->chid       = channel_id;
            create_mac_option(dummy_bmsg);

            mac_option* mac_opt = reinterpret_cast<mac_option*> (dummy_bmsg->mac_option);
            mac_opt->burst_type = DUMMY_BURST;
            mac_opt->burst_num  = i;

            if ( i == 3) {
                DB* db = new DB(1, DUMMY_BURST);
                //mac_opt->msg        = db;
                insert_macopt_msg( mac_opt , DUMMYBURST , db );
            }
            else
                mac_opt->msg_type   = DUMMYBURST;

            mac_opt->channel_id = channel_id;

            add_mac_header(edb,CTL_BLK,usf_value);

            dummy_burst_squeue[tn][CORRESPONDING_UPLINK_CH(channel_id)].insert_tail(edb);
        }
        return 1;
    }

    Event* GprsBtsMac::create_measured_dummy_burst_msg( long channel_id ) {

        if ( (channel_id <125) || (channel_id >= 250) ) {
            cout <<"GprsBtsMac create_measured_dummy_burst(): channel_id " << channel_id << " is out of range." << endl;
            exit(1);
        }

        uchar usf_value = bssdes->ch_map[CORRESPONDING_UPLINK_CH(channel_id)]->choose_usf_in_tn(tn);

        if (usf_value == 100 )
            usf_value = 0x07;/* random */

        #ifdef __USF_DEBUG
        cout << "BTSMac: (B) CH " << channel_id << " usf_value at tn = " << tn << " usf = " << (int) (usf_value) << endl;
        #endif

        Event* edb;
        CREATE_BSS_EVENT(edb);

        bss_message* dummy_bmsg = reinterpret_cast<bss_message*> (edb->DataInfo_);

        //uchar* dummy_data = new uchar[1];
        uchar* dummy_data = dummy_data_array;
        bzero(dummy_data,1);
        insert_user_data(dummy_bmsg, dummy_data , 1 );

        create_rlc_option(dummy_bmsg);
        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (dummy_bmsg->rlc_option);
        rlc_opt->chid       = channel_id;
        create_mac_option(dummy_bmsg);

        mac_option* mac_opt = reinterpret_cast<mac_option*> (dummy_bmsg->mac_option);
        mac_opt->burst_type = DUMMY_BURST;
        mac_opt->burst_num  = 0;

        /*if ( i == 3) {
            DB* db = new DB(1, DUMMY_BURST);
            //mac_opt->msg        = db;
            insert_macopt_msg( mac_opt , DUMMYBURST , db );
        }
        else*/
        mac_opt->msg_type   = DUMMYBURST;

        mac_opt->channel_id = channel_id;

        add_mac_header(edb,CTL_BLK,usf_value);

        return edb;
    }


    /********* BssDescription member functions *************/
    BssDescription::BssDescription() {
        for (int i=0 ; i<250 ; ++i)
            ch_map[i] = NULL; /* 0 ~ 124 are uplink channels. 125 ~ 249 are downlink channels */

        node_id     = 0;
        bcch_no     = 0;
        pbcch_tn        = 1;
        start_ch        = -1;
        end_ch      = -1;
        tsc         = 0;
        rai         = 0;
        bsic        = 0;

        /* pccch organization parameters */
        bs_pbcch_blks     = 3;
        bs_pag_blks_res   = 0;
        bs_prach_blks     = 0;

        /* message change marks */
        bcch_change_mark  = -1;
        pbcch_change_mark = -1;
        psi2_change_mark  = -1;
        psi3_change_mark  = -1;

        psi1_repeat_period= 26;
        char tmp_multiplexing_seq [] = { 0,6,3,9,1,7,4,10,2,8 ,5,11 };
        memcpy( pdtch_downlink_multi_seq , tmp_multiplexing_seq , 12*sizeof(char) );

    }

    uchar   BssDescription::channel_match(ushort chid) {
        if ( chid <250 )
            return (ch_map[chid])?true:false;
        else
            return false;
    }


    /************** TaAllocation member functions **************/

    TaAllocation::TaAllocation() {

        for (int i=0 ; i<32 ;++i) {
            tbfdes[i] = NULL;
        }
        for (int i=0 ; i<8; ++i) {
            tn[i] = new TbfDesList;
            usf_granted_ind_in_tn[i] = new uchar[8];
            memset(usf_granted_ind_in_tn[i], gprs::usf_unused ,8);
        }

        memset ( tfi_granted_tn , 255  , 8*sizeof(uchar) );
        tfi_cnt  = 0;
    }

    TaAllocation::~TaAllocation() {
        for (int i=0; i<8 ;++i) {
            if (tn[i]) delete tn[i];
        }
    }

    int TaAllocation::remove_tbfdes_entry( TbfDes* tbfdes1 ) {

        ASSERTION( tbfdes1, "TaAllocation remove_tbfdes_entry(): tbfdes1 is NULL." );

        if ( tbfdes[ tbfdes1->tfi ] ) {
            tbfdes[ tbfdes1->tfi ] = NULL;
            --tfi_cnt;
        }
        else {
            cout << "TaAllocation::remove_tbfdes_entry(): Error. tbfdes1->tfi was not registered. "<< endl;
            exit(1);
        }



        for ( int i=0 ; i<8 ; ++i ) {

            if ( tfi_granted_tn[i] == tbfdes1->tfi )
                tfi_granted_tn[i] = 255;

            if ( tbfdes1->usf_tn[i] < 8 ) {

                if ( usf_granted_ind_in_tn[i][ tbfdes1->usf_tn[i] ]  )
                    usf_granted_ind_in_tn[i][tbfdes1->usf_tn[i]] = gprs::usf_unused;

            }

            if ( tbfdes1->ts_alloc[i] )
                tn[i]->remove_entry( tbfdes1 );
        }

        return 1;

    }

    int TaAllocation::get_an_empty_tfi() {
        for (int i=0 ; i<32 ;++i) {
            if ( !tbfdes[i] )
                return i;
        }
        return -1;
    }

    int TaAllocation::set_tbfdes(ulong tfi,TbfDes* tbfdes1) {
        IS_NULL_STR(tbfdes1, "TaAllocation set_tbfdes(): Assertion failed: parameter tbfdes1 is null\n" , -1 );
        if ( tbfdes[tfi] ) {
            cout << "TaAllocation set_tbfdes(): Error: tbfdes["<< tfi << "] has been allocated" << endl;
            return -1;
        }
        else {
            tbfdes[tfi] = tbfdes1;
            for (int i=0 ; i<8 ; ++i) {
                if ( tbfdes1->ts_alloc[i] )
                    tn[i]->insert_tail(tbfdes1);
            }
            ++tfi_cnt;
            return 1;
        }
    }

    int TaAllocation::unset_tbfdes(ulong tfi) {
        IS_NULL_STR(tbfdes[tfi], "TaAllocation unset_tbfdes(): Warning: the tbf with specified tfi doesn't exist. \n" , 1 );

                ListElem<TbfDes>* tmp = new ListElem<TbfDes>(tbfdes[tfi]);
        for (int i=0 ; i<8 ; ++i) {
            if ( tbfdes[tfi]->ts_alloc[i] )
                tn[i]->remove_entry(tmp);
        }
        --tfi_cnt;
        delete tmp;
                return 1;
    }
    uchar TaAllocation::inquiry_usf_in_tn(uchar tn_num,uchar tfi) {
        if (tfi>=32)
            return gprs::usf_unused;

        if (tn_num>=8)
            return gprs::usf_unused;

        for (int i=0 ; i<8 ; ++i ) {
            if ( usf_granted_ind_in_tn[tn_num][i] == tfi ) {
                return i;
            }
        }

        return gprs::usf_unused;
    }

    /* Select an USF for a TBF */
    uchar TaAllocation::get_usf_in_tn (uchar tn_num,uchar tfi,bool is_pbcch_carrier) {

        if (tfi>=32)
            return gprs::usf_unused;

        if (tn_num>=8)
            return gprs::usf_unused;

        /* note that USF = 0x07 stands for free timeslot on PCCCH */
        int usf_limit;
        if (is_pbcch_carrier)
            usf_limit = 0x07;
        else
            usf_limit = 0x08;
        for (int i=0 ; i<usf_limit; ++i) {
            if (usf_granted_ind_in_tn[tn_num][i] == gprs::usf_unused )
                usf_granted_ind_in_tn[tn_num][i] = tfi;
                return i;
        }
        return gprs::usf_unused;
    }

    /* choose an USF to be polled */
    uchar TaAllocation::choose_usf_in_tn (uchar tn_num) {

        uchar cnt=0;
        for (int i=0 ; i<8 ; ++i ) {
            if ( usf_granted_ind_in_tn[tn_num][i] == gprs::usf_unused ) {
                ++cnt;
            }
        }

        /* Any USF value is not used */
        if (cnt>=8)
            return gprs::usf_unused;

        uchar seq[8];
        bzero (seq,8);
        uchar included = 0;
        uchar index = 0;

        /* generate a sequence for polling */
        while(index<8) {

            uchar sel = random()%8;

            if ( !((included>>sel)&0x01) ) { /* Not included */
                included |= (0x01<<sel);
                seq[index++] = sel;
            }

        }

        /* polling usf_granted if the value is available */
        for ( int i=0 ; i<8 ;++i) {
            if ( usf_granted_ind_in_tn[tn_num][seq[i]] != gprs::usf_unused ) {

                uchar   selected_tfi = usf_granted_ind_in_tn[tn_num][seq[i]];
                TbfDes* sel_tbfdes = get_tbfdes_by_tfi(selected_tfi);

                if ( !sel_tbfdes ) {
                    printf("TaAllcation::choose_usf_in_tn(): sel_tbfdes does not exist.\n");
                    exit(1);
                }
                else
                    return sel_tbfdes->usf_tn[tn_num];

            }
        }

        return gprs::usf_unused;
    }

    TbfDes* TaAllocation::get_tbfdes_by_tn_usf(uchar tn1, uchar usf) {
        return tn[tn1]->search_by_tn_usf(tn1,usf);
    }

    TbfDes* TaAllocation::get_sba_tbfdes(uchar tn1) {
        return tn[tn1]->search_sba(tn1);
    }

    TbfDes* TaAllocation::get_tbfdes_by_tfi(uchar tfi) {
        if (tfi>=32)
            return NULL;
        else
            return tbfdes[tfi];
    }

    TbfDes* TaAllocation::get_tbfdes_by_tlli(ulong tlli) {
        for (int i=0 ; i<32; ++i) {
            if (!tbfdes[i])
                continue;
            if (tbfdes[i]->tlli == tlli )
                return tbfdes[i];
        }
        return NULL;
    }

    TbfDes* TaAllocation::get_granted_tbfdes(uchar tn1) {
        TbfDes* tbfdes = NULL;
        if ( (tbfdes=this->get_sba_tbfdes(tn1)) ) {
            if ( !((tbfdes->pacch_sq[tn1])->is_empty()) )
            return tbfdes;
        }
        else {
            tbfdes = get_tbfdes_by_tfi( (get_tfi_granted_on_tn(tn1)) );
            if (tbfdes) {
                return tbfdes;
            }
        }

        return NULL;
    }

    int TaAllocation::merge_ack(ulong tn) {

        for ( ulong i=0 ; i<32 ; ++i ) {
            if ( tbfdes[i] )
                tbfdes[i]->merge_ack(tn);
        }

        return 1;
    }

    TbfDes* TbfDesList::search_by_tfi(uchar tfi) {
        ListElem<TbfDes>* ptr;
        TbfDes* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            if (tmp->tfi == tfi )
                return tmp;
        }
        return NULL;
    }

    TbfDes* TbfDesList::search_by_tn_usf(uchar tn1, uchar usf) {
        ListElem<TbfDes>* ptr;
        TbfDes* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            if (tmp->usf_tn[tn1] == usf )
                return tmp;
        }
        return NULL;
    }

    TbfDes* TbfDesList::search_sba(uchar tn1) {
        ListElem<TbfDes>* ptr;
        TbfDes* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            if (tmp->sr_bit == SBA_TYPE)
                return tmp;
        }
        return NULL;
    }

    /*TbfDes* TbfDesList::search(uchar pacch_tn) {
        ListElem<TbfDes>* ptr;
        TbfDes* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            if (tmp->pacch_blkn_bitmap[pacch_tn] )
                return tmp;
        }
        return NULL;
    }*/

    BssDes* BssDesList::search(ulong bsic) {
        ListElem<BssDes>* ptr;
        BssDes* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            if (tmp->bsic == bsic )
                return tmp;
        }
        return NULL;
    }

    SbaFreq* SbaFreqList::search(ulong ch_no, uchar tn) {
        ListElem<SbaFreq>* ptr;
        SbaFreq* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            if (tmp->fp->arfcn == ch_no ) {
                if (tmp->sba->tn == tn)
                    return tmp;
            }
        }
        return NULL;
    }

    TlliCacheElem::TlliCacheElem(TlliCacheElem* tce) {
        if (!tce)
            printf("TCE constructor(): tce is NULL.\n");
        tlli = tce->tlli;
        utcb = tce->utcb;
        dtcb = tce->dtcb;
    }

    TlliCacheElem* TlliCacheList::search_by_tlli(ulong tlli) {
        ListElem<TlliCacheElem>* ptr;
        TlliCacheElem* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            IS_NULL_STR(tmp,"TCE: TlliCacheElem is NULL.\n",NULL);

            if (tmp->tlli == tlli )
                return tmp;
        }
        return NULL;
    }

    int TlliCacheList::remove_cache_entry( ulong tlli1) {
        ListElem<TlliCacheElem>* ptr;
        TlliCacheElem* tmp;
        for (ptr = head; ptr ; ptr=ptr->get_next()) {
            tmp = ptr->get_elem();
            IS_NULL_STR(tmp,"TCE: TlliCacheElem is NULL.\n",0);

            if (tmp->tlli == tlli1 ) {
                remove_entry(tmp);
            }
        }

        return 1;
    }

  /*  class TlliHashCache {
        TlliCacheList* hash_table[20];
    public:
        TlliHashCache()  { for (int i=0 ; i<20 ;++i ) hash_table[i] = new TlliCacheList; }
        ~TlliHashCache()    { for (int i=0 ; i<20 ;++i ) if (hash_table[i]) delete hash_table[i]; }
        int insert( TbfDes* tbfdes);
        int search_by_tlli(ulong tlli);
        int rehash(ulong old_tlli, ulong new_tlli );
};
*/

    TlliHashCache::TlliHashCache()  {
        hash_size   = 20;
        hash_table  =  new TlliCacheList*[hash_size];
        for (ulong i=0 ; i<hash_size ;++i )
            hash_table[i] = new TlliCacheList;
    }

    TlliHashCache::~TlliHashCache()  {
        hash_size   = 20;

        for (ulong i=0 ; i<hash_size ;++i ) {
            if (hash_table[i]) delete hash_table[i];
        }
        delete hash_table;
    }

    int TlliHashCache::insert(TbfDes* tbfdes) {
        IS_NULL_STR(tbfdes, "THC: tbfdes is NULL\n", -1 );

        if (tbfdes->tlli == 0) {
            cout <<"THC: Warning: utbf" << endl;
        }

        ulong hash_index = tbfdes->tlli % hash_size;

        TlliCacheElem* elem = hash_table[hash_index]->search_by_tlli(tbfdes->tlli);

        if (!elem ) {
            elem = new TlliCacheElem;

            elem->tlli = tbfdes->tlli;

            if ( tbfdes->nodetype == gprs::ms ) {

                if ( tbfdes->sr_bit == SENDER ) {
                    elem->utcb = tbfdes;
                }
                else {
                    elem->dtcb = tbfdes;
                }
            }
            else {

                if ( tbfdes->sr_bit == SENDER ) {
                    elem->dtcb = tbfdes;
                }
                else {
                    elem->utcb = tbfdes;
                }
            }

            hash_table[hash_index]->insert_tail(elem);
        }
        else {

            if ( tbfdes->nodetype == gprs::ms ) {

                if ( tbfdes->sr_bit == SENDER ) {
                    if (!elem->utcb)
                        elem->utcb = tbfdes;
                    else
                        cout <<"THC: utcb has been inserted" << endl;
                }
                else {
                    if (!elem->dtcb)
                        elem->dtcb = tbfdes;
                    else
                        cout <<"THC: dtcb has been inserted" << endl;
                }
            }
            else {

                if ( tbfdes->sr_bit == SENDER ) {
                    if (!elem->dtcb)
                        elem->dtcb = tbfdes;
                    else
                        cout <<"THC: dtcb has been inserted" << endl;
                }
                else {
                    if (!elem->utcb)
                        elem->utcb = tbfdes;
                    else
                        cout <<"THC: utcb has been inserted" << endl;
                }
            }
        }

        return 1;
    }

    TlliCacheElem* TlliHashCache::search_by_tlli(ulong tlli1) {

        if ( tlli1 == 0) {
            cout <<"THC: Warning: tlli == 0 " << endl;
            return NULL;
        }

        ulong index = tlli1 % hash_size;
        return hash_table[index]->search_by_tlli(tlli1);
    }

    int TlliHashCache::remove( ulong tlli1) {
        if ( tlli1 == 0) {
            cout <<"THC: Warning: tlli == 0 " << endl;
            return -1;
        }

        ulong index = tlli1 % hash_size;
        return (hash_table[index]->remove_cache_entry(tlli1));
    }

    int TlliHashCache::rehash(ulong old_tlli , ulong new_tlli ) {

        if ( old_tlli == 0) {
            cout <<"THC: Warning: old_tlli == 0 " << endl;
            return -1;
        }

        if ( new_tlli == 0) {
            cout <<"THC: Warning: new_tlli == 0 " << endl;
            return -1;
        }

        ulong index = old_tlli%hash_size;
        TlliCacheElem* elem = hash_table[index]->search_by_tlli(old_tlli);

        if ( elem ) {

            TlliCacheElem* elem1 = new TlliCacheElem(elem);
            hash_table[index]->remove_entry(elem); /* only remove the entry ifself, not its content */

            if ( elem1->utcb )
                elem1->utcb->tlli = new_tlli;
            if ( elem1->dtcb )
                elem1->dtcb->tlli = new_tlli;

            elem1->tlli = new_tlli;

            index = new_tlli%hash_size;
            hash_table[index]->insert_tail(elem1);
            return 1;

        }
        else {
            index = new_tlli%hash_size;
            elem = hash_table[index]->search_by_tlli(new_tlli);

            /* This case is a fatal error for the current design of GMM procedures. All the downlink
             * TBFs should have a corresponding uplink TBF, because the paging mode is disabled thus far.
             */
            IS_NULL_STR(elem,"THC rehash(): elem is NULL.\n" , -1);



            #ifdef  __DETAILED_INFO
            cout << "THC: Info: tlli is already set to new_value = " << new_tlli << endl;
            #endif

            /* In this case, the TLLI is already set to local TLLI */
            return 1;
        }

    }

/**************************************************************/
int BssDesList::create_d_bssdes_list(DBssDes* area) {

    int list_num = 6;
    DBssDes* candidate_array = area;

    ASSERTION( candidate_array , "BssDesList::create_d_bssdes_list(): candidate_array is null.\n" );
    ListElem<BssDes>* ptr = head;

    for ( int i=0 ; i<list_num ; ++i ) {

        if ( ptr ) {

            BssDes* ptr_elem = ptr->get_elem();
            ASSERTION( ptr_elem , "BssDesList::create_d_bssdes_list(): ptr_elem is null.\n");

            candidate_array[i].nid      =   ptr_elem->node_id;
            candidate_array[i].rai      =   ptr_elem->rai;
            candidate_array[i].bsic     =   ptr_elem->bsic;
            candidate_array[i].bcch_no  =   ptr_elem->bcch_no;
            candidate_array[i].start_ch =   ptr_elem->start_ch;
            candidate_array[i].end_ch   =   ptr_elem->end_ch;
            candidate_array[i].tsc      =   ptr_elem->tsc;


        }
        else {

            candidate_array[i].nid      =   0;
            candidate_array[i].rai      =   0;
            candidate_array[i].bsic     =   0;
            candidate_array[i].bcch_no  =   0;
            candidate_array[i].start_ch =   -1;
            candidate_array[i].end_ch   =   -1;
            candidate_array[i].tsc      =   0;

        }

        if ( ptr )
                ptr = ptr->get_next();

    }

    return 1;

}

int GprsBtsMac::dump_listening_channel() {

    printf("\n******* dump monitored channel ID ****** \n");
    printf("******* tick=%lld bn =%ld tn=%ld fn=%ld blkn=%ld ****** \n", GetCurrentTime() , bn , tn, fn, blkn );

    for ( ulong i=0 ; i<250 ; ++i ) {

        if ( listen_chmap[i] )
            printf(" %lu ", i);

    }

    printf("\n******* End of dumping monitored channel ID ****** \n");
    return 1;
}
