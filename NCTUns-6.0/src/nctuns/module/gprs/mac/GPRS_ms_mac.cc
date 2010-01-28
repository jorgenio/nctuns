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

#include <iostream>
#include <stdlib.h>
#include <nctuns_api.h>
#include <timer.h>
#include <mbinder.h>
#include <wphyInfo.h>
#include "GPRS_ms_mac.h"
#include "GPRS_bts_mac.h"
#include <gprs/include/GPRS_rlcmac_message.h>
#include <gprs/include/channel_coding.h>
#include <gprs/include/partial_queue.h>
#include <gprs/radiolink/radiolink.h>
#include <gprs/rlc/rlc.h>
#include <gprs/rlc/rlc_shared_def.h>
#include <gprs/llc/llc.h>
#include <gprs/mac/two_slot_usf_granted_ind.h>

using namespace std;

#define     __MS_ROAMING_DEBUG
//#define   __SHOW_MEASURED_RSSI
//#define   __RSSI_DUMP
//#define   __SEQUENCE_DEBUG
//#define   __SEQUENCE_DEBUG_PART_2
//#define   __CH_DETERMINE_DEBUG
//#define   __DETAILED_DEBUG
//#define   __CTL_MSG_DEBUG
//#define   __TIMING_DEBUG
//#define   __TRIGGER_RLC_DEBUG
//#define   __DUMP_ENCODING_DATA
//#define   __RRBP_DEBUG
//#define   DUMMY_BURST_DEBUG
//#define   __BLK_PARTITION_DETAILED_DEBUG
//#define   USF_DEBUG
//#define   __SEQUENCE_DEBUG_3
//#define   __SET_LISTENING_CH_DEBUG

    MODULE_GENERATOR(GprsMsMac);

    inline int GprsMsMac::is_idle_frame() {

        if ( (fn == 12) || (fn == 25) || (fn == 38) || (fn == 51) )
            return true;
        else
            return false;

    }

    inline int GprsMsMac::test_is_idle_frame(ulong fn1) {

        if ( (fn1 == 12) || (fn1 == 25) || (fn1 == 38) || (fn1 == 51) )
            return true;
        else
            return false;

    }

    inline int GprsMsMac::is_blkn_boundary() {

        if ( fn == 0  || fn == 4  || fn == 8  || fn == 13 || fn == 17 || fn == 21 ||
            fn == 26 || fn == 30 || fn == 34 || fn == 39 || fn == 43 || fn == 47 )
            return true;
        else
            return false;

    }

    inline int GprsMsMac::fn_to_burst_num(ulong fn1) {

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

    inline int GprsMsMac::test_is_blkn_boundary(ulong fn1) {

        if ( fn1 == 0  || fn1 == 4  || fn1 == 8  || fn1 == 13 || fn1 == 17 || fn1 == 21 ||
            fn1 == 26 || fn1 == 30 || fn1 == 34 || fn1 == 39 || fn1 == 43 || fn1 == 47 )
            return true;
        else
            return false;

    }


    GprsMsMac::GprsMsMac  (u_int32_t type, u_int32_t id, struct plist* pl, const char *name): NslObject(type, id, pl, name) {

        try {
            state                               = POWER_ON_NOT_SYNC;
            roaming_flag                        = false;
            tlli                                = 0;
            cur_used_bss                        = NULL;
            bcch_list                           = new BcchList;
            cur_monitored_neighbor_list_index   = -1;

            /*GPRS log*/
            _ptrlog     = 0;
            log_flag    = 1;

            /* internal timers for synchronization */
            qn          = 0; /* quarter bit number */
            bn          = 0; /* bit number */
            tn          = 0; /* time slot number */
            fn          = 0; /* frame number */
            blkn        = 0;
            uplink_tn   = -3;
            uplink_fn   = -1;
            uplink_blkn = -1;
            triggered_tn    = 0;
            triggered_fn    = 0;
            triggered_blkn  = 0;

            scanning_mode   = false;
            cur_ch_no       = 0;
            psi3_info_recv_flag = false;

            /* partially-established TCBs */
            dtcb        = NULL; /* this field temporarily stores a TBF control block that are performing attachment */
            utcb        = NULL;


            for ( ulong i=0 ; i<8 ; ++i )
                up_down[i] = 0;


            /* The indicator for trigger sending of the upper layer */
            bzero(trigger_sending_ind,8);

            /* Downlink transmission control parameters:
            * page_mode,
            */
            page_mode           = 0x00;
            bzero( persistence_level , 4 );

            /* TDMA scheduler */
            sched_timer         = new timerObj;
            second_timer        = new timerObj;
            random_timer        = NULL;
            chreq_timer         = NULL;
            pua_timer           = NULL;

            /* send burst queues */
            prach_squeue        = new SList<Event>;

            /* recv burst queues: divided by timeslot number*/
            for ( int i=0 ; i<8 ; ++i ) {
                downlink_rq[i]      = new partially_assembled_rlcblk;
                drq_flush_ind[i]    = true;
            }

            dtcb_rrbp_in_trans_flag = 0;
            pre_recv_timestamp = 0;
            printf( "GprsMsMac constructor(): Node %d PHONE MAC instance is created. \n" , (get_nid()) );

        }
        catch(std::exception& e) {
            cout << "GprsMsMac(): ASSERTION failed:"<< e.what() << endl;
            exit(1);
        }
    }

    int GprsMsMac::init() {

        sched_timer->init();
        sched_timer->setCallOutObj(reinterpret_cast<NslObject*>(this), (int (NslObject::*)(Event*)) &GprsMsMac::update_bn );

        second_timer->init();
        second_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),(int (NslObject::*)(Event*)) &GprsMsMac::show_rssi );

        #ifdef __SHOW_MEASURED_RSSI

        u_int64_t sec_tick;
        SEC_TO_TICK(sec_tick,1);
        second_timer->start( sec_tick,sec_tick);

        #else

        delete second_timer;
        second_timer = NULL;

        #endif

        /* bcch prestored list initialization
         * not implemented
         */

        /* start to scan possible bss based on bcch prestored list */
        if (!cur_used_bss)
            cur_used_bss = new BcchInfo;

        cur_used_bss->clear();


        uchar preset_bcch_list_flag = false;
        if ( bcch_list ) {
            if ( bcch_list->get_head() ) {
                if ( bcch_list->get_head()->get_elem() ) {
                    cur_used_bss->update( (bcch_list->get_head()->get_elem()) );
                    scanning_mode = false;
                    preset_bcch_list_flag = true;
                }
            }
        }

        if (!preset_bcch_list_flag) {
            scanning_mode = true;
            cur_used_bss->set_bcch_no( 126 );
        }

        radiolink_obj = reinterpret_cast<radiolink*> (sendtarget_->bindModule());
        ASSERTION(radiolink_obj,"GprsMsMac init(): the radiolink obj is not found.\n");

        cur_ch_no = cur_used_bss->get_bcch_no();
        printf ("GprsMsMac init(): set current listening channel to ch %u \n", cur_ch_no );
        clear_listening_channel();
        mark_listening_channel(cur_ch_no);
        set_listening_channel();

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
                                sprintf(ptrFile, "%s.ptr",GetScriptName());
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

    int GprsMsMac::recv(Event* ep) {

        IS_NULL_STR(ep,"GprsMsMac recv(): ep is null\n",-1);
        bss_message* bssmsg = reinterpret_cast<bss_message*>(ep->DataInfo_);
        mac_option* mac_opt = reinterpret_cast<mac_option*>(bssmsg->mac_option);


        /*MAC recv log*/
        if ( log_flag && !strcasecmp(GPRSLogFlag, "on") ) {

            struct logEvent*        logep;
            uchar                   burst_len;
            char                    burst_type;


            struct gprs_log*        log1;
            u_int64_t               delay;

            if(mac_opt->btx_flag != 1)
            {
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




                LOG_GPRS(log1,GetCurrentTime()+delay,GetCurrentTime(),get_type(),
                    get_nid(),StartRX,
                    cur_used_bss->get_nid(),get_nid(),
                    mac_opt->burst_num,burst_type,burst_len,mac_opt->channel_id);

                INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

            }

        }

        burst_demultiplexer(ep);
        //dmalloc_log_stats();
        //dmalloc_log_unfreed();
        return 1;
    }

    int GprsMsMac::send(Event* ep) {

        IS_NULL_STR(ep,"GprsMsMac send(): ep is null\n",-1);
        IS_NULL_STR(ep->DataInfo_,"GprsMsMac send(): ep->DataInfo_ is null\n",-1);

        /* parse command from RLC */
        bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        llc_option* llc_opt = reinterpret_cast<llc_option*> (bssmsg->llc_option);
        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

        if ( llc_opt ) {

            if ( llc_opt->addtional_info == LLGMM_RESET ) {

                uchar remove_flag = false;
                if ( utcb ) {

                    utcb->llgmm_reset_flag = true;

                    if ( true ) {

                        if ( utcb->tlli == llc_opt->tlli ) {
                            delete utcb;
                            utcb = NULL;
                            remove_flag = true;
                        }

                    }

                }

                if ( dtcb ) {

                    dtcb->llgmm_reset_flag = true;

                    if ( true ) {

                        if ( dtcb->tlli == llc_opt->tlli ) {
                            delete dtcb;
                            dtcb = NULL;
                            remove_flag = true;
                        }

                    }

                }

                if ( remove_flag )
                    cout << "MsMAC: remove TCBs with TLLI = " << llc_opt->tlli << endl;

                FREE_BSS_EVENT( ep );
                return 1;
            }

            tlli = llc_opt->tlli;

            if (utcb) {
                if (utcb->tlli == llc_opt->oldtlli)
                    utcb->tlli = llc_opt->tlli;
            }

            if (dtcb) {
                if (dtcb->tlli == llc_opt->oldtlli)
                    dtcb->tlli = llc_opt->tlli;
            }

        }


        IS_NULL_STR(rlc_opt,"GprsMsMac send(): rlc_opt is null\n",-1);

        if ( state == PACKET_IDLE_STATE ) {

            if ( rlc_opt->cmd == UPLINK_TBF_ESTABLISH ) {
                establish_uplink_tbf(TWOPHASE_ACCESS, cur_used_bss->get_bcch_no(), tlli);
                return 1;
            }
            else if (rlc_opt->cmd == PAGING_RESPONSE ) {
                establish_downlink_tbf(PAGING_RESPONSE, cur_used_bss->get_bcch_no() , tlli);
                return 1;
            }
            else if ( rlc_opt->cmd == GMM_ROAMING_REQUEST ){

                ASSERTION( llc_opt , "GprsMsMac::send(): llc_opt is null while MAC needs the indicated request channel.\n");

                if ( llc_opt->req_bsic < 0 ) {
                    printf("MS MAC::send(): Assertion failed.");
                    printf("\nThe Upper layer requests a roaming_procedure but without valid req_bsic =%ld.\n", llc_opt->req_bsic);
                    exit(1);
                }

                if ( llc_opt->req_rai < 0 ) {
                    printf("MS MAC::send(): Assertion failed.");
                    printf("The Upper layer requests a roaming_procedure but without valid req_rai =%ld.\n", llc_opt->req_rai);
                    exit(1);
                }

                BcchInfo* tmp = bcch_list->search_by_bsic_rai( llc_opt->req_bsic , llc_opt->req_rai );
                ASSERTION( tmp , "GprsMsMac::send(): Cannot find new BSS structure by given BSIC and RAI.\n");
                gmm_roaming_procedure(tmp->get_bcch_no());
                return 1;
            }
            else
                return 1;
        }

        else if ( state == PACKET_TRANSFER_STATE) {

            if ( rlc_opt->cmd == UPLINK_TBF_ESTABLISH ) {
                establish_uplink_tbf(TWOPHASE_ACCESS,cur_used_bss->get_bcch_no(), tlli);
                return 1;
            }
            else if (rlc_opt->cmd == PAGING_RESPONSE ) {
                establish_downlink_tbf(PAGING_RESPONSE, cur_used_bss->get_bcch_no(), tlli);
                return 1;
            }
            else if ( rlc_opt->cmd == UPLINK_TBF_RELEASE ) {
                release_uplink_tbf();
            }
            else if (rlc_opt->cmd == PAGING_RESPONSE ) {
                block_partition(ep,PACCH_SQUEUE);
            }
            else if ( rlc_opt->cmd == GMM_ROAMING_REQUEST ){

                ASSERTION( llc_opt , "GprsMsMac::send(): llc_opt is null while MAC needs the indicated request channel.\n");

                if ( llc_opt->req_bsic < 0 ) {

                    printf("MS MAC::send(): Assertion failed. The Upper layer requests a roaming_procedure but without valid req_bsic =%ld.\n", llc_opt->req_bsic);

                    exit(1);
                }

                if ( llc_opt->req_rai < 0 ) {

                    printf("MS MAC::send(): Assertion failed. The Upper layer requests a roaming_procedure but without valid req_rai =%ld.\n", llc_opt->req_rai);

                    exit(1);
                }

                BcchInfo* tmp = bcch_list->search_by_bsic_rai( llc_opt->req_bsic , llc_opt->req_rai );
                ASSERTION( tmp , "GprsMsMac::send(): Cannot find new BSS structure by given bsic and rai.\n");
                gmm_roaming_procedure(tmp->get_bcch_no());
            }
            else {

                /* Partition a RLC block into four MAC bursts, enqueue them to
                 * the corresponding burst send queue.
                 */
                if ( rlc_opt->ctl_msg_type < 0 )
                    block_partition(ep,PDTCH_SQUEUE); /* User Data */
                else
                    block_partition(ep,PACCH_SQUEUE); /* downlink TBF ACK */
            }
            return 1;
        }

        else if ( state == POWER_ON_NOT_SYNC ) {
            cout << "MS is not synchronized with any bss" << endl;
            return 0;
        }

        else if ( state == ABNORMAL )
            return 0;

        else {
            cout << "GprsMsMac send():unknown state"<<endl;
            return 0;
        }

    }


    int GprsMsMac::burst_demultiplexer(Event* ep) {

        IS_NULL_STR(ep,"GprsMsMac burst_demultiplexer(): ep is null\n",-1);

        bss_message* bss_msg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        IS_NULL_STR(bss_msg,"GprsMsMac burst_demul(): DataInfo_ is null\n",-1);

        /*
         *   rlc_option*  rlc_opt = reinterpret_cast<rlc_option*> (bss_msg->rlc_option);
         *   IS_NULL_STR(bss_msg,"GprsMsMac burst_demul(): rlc_option is null\n",-1);
         */

        mac_option* mac_opt = reinterpret_cast<mac_option*>(bss_msg->mac_option);
        ASSERTION(mac_opt,"GprsMsMac burst_demul(): mac_option is null\n");

        if ( (validate_mac_option(mac_opt)) < 0 )  {
            exit(1);
        }

        incoming_event_nid = mac_opt->nid;

        double rssi_val_v = (double) (10 * log10(bss_msg->wphyinfo->RxPr_ * 1e3)); // dbm

        #ifdef __RSSI_DUMP
        printf("Burst RSSI = %f NID= %u timestamp=%ll\n", rssi_val_v, mac_opt->nid, GetCurrentTime() );

        #endif

        if ( (detect_collision(mac_opt , mac_opt->tn)) > 0 ) {

            printf("\n* * * * Dump current mac_opt: * * * *\n");
            printf("mac_opt: bn %ld tn %lu fn %ld burst_num %lu burst_type %lu src_nid %u \n\n",
                mac_opt->bn , (ulong)(mac_opt->tn) , (ulong)(mac_opt->fn) , (ulong)(mac_opt->burst_num) , (ulong)(mac_opt->burst_type), mac_opt->nid);

            if ( rb_rec[(int)(mac_opt->tn)].recv_rssi > rssi_val_v ) {

                if ( bss_msg->packet) {
                    remove_upper_layer_pkt(bss_msg->packet);
                    free_pkt(bss_msg->packet);
                    bss_msg->packet     = NULL;
                }
                FREE_BSS_EVENT(ep);
                return 0;
            }


        }

        if (mac_opt->channel_id != cur_ch_no ) {

            cout << "MsMac burst_demul(): received channel id mismatches."  <<
                    "The channel of this burst="                            << mac_opt->channel_id <<
                    "  The channel of this MS = "                           << cur_ch_no << endl;

            if ( bss_msg->packet) {
                remove_upper_layer_pkt(bss_msg->packet);
                free_pkt(bss_msg->packet);
                bss_msg->packet     = NULL;
            }
            FREE_BSS_EVENT(ep);
            exit(1);

        }

        /* synchronization procedure */
        if ( state == POWER_ON_NOT_SYNC ) {

            if ( mac_opt->channel_id == (cur_used_bss->get_bcch_no()) ) {

                update_counters(mac_opt->tn,mac_opt->fn);
                record_recv_burst(mac_opt, rssi_val_v );

                #ifdef   __SEQUENCE_DEBUG
                printf("MS MAC: sync with BSS of which nid is %ld at tick = %llu tn = %ld fn = %ld\n",
                    mac_opt->nid , ( GetCurrentTime() ) , tn , fn);
                #endif

                state           = PACKET_IDLE_STATE;
                scanning_mode   = false;
                cur_used_bss->set_nid( mac_opt->nid );

                gprs_scheduler();

                sched_timer->cancel();
                u_int64_t bn_tick;
                NANO_TO_TICK( bn_tick , BIT_PERIOD);
                printf("MS MAC [%u]: set a BIT_PERIOD as %lld ticks. A tick = %d ns.\n", get_nid(), bn_tick, TICK );
                sched_timer->start( bn_tick, bn_tick);

            }
        }
        else {

            /* Emulate the functionality of Synchronization Channel */
            if ( (!mac_opt->tn) &&
                 //(mac_opt->channel_id == (cur_used_bss->get_bcch_no()) )
                 ( mac_opt->nid == (cur_used_bss->get_nid()) ) ) {


                update_counters(mac_opt->tn,mac_opt->fn); /* sync in the first timeslot */
                record_recv_burst(mac_opt, rssi_val_v );

                #ifdef   __SEQUENCE_DEBUG
                printf("MS MAC: sync at tick = %llu tn = %ld fn = %ld\n", ( GetCurrentTime() ) , tn , fn);
                #endif

                gprs_scheduler();

                sched_timer->cancel();
                u_int64_t bn_tick;
                NANO_TO_TICK( bn_tick , BIT_PERIOD);
                //printf("MS MAC [%u]: set a BIT_PERIOD as %lld ticks. A tick = %d ns.\n", get_nid(), bn_tick, TICK );
                sched_timer->start( bn_tick, bn_tick);

            }

        }

        /* signal measurement recording process */
        if ( state == PACKET_IDLE_STATE || state == PACKET_TRANSFER_STATE ) {

            if ( bcch_list ) {
                BcchInfo* elem = bcch_list->search( mac_opt->channel_id );

                if ( elem ) {
                    elem->rssi = elem->rssi*(0.3) + rssi_val_v*(0.7);
                    ++elem->received_burst_cnt;
                }
            }

        }

        int res = 1;

        #ifdef  __DETAILED_DEBUG
        {

            printf("GprsMsMac burst_demul(): receives a burst with profile as follows:\n");
            printf("mac_option: burst_type %d, burst_num %d, chid %ld, fn %ld, tn %d, bn %ld \n",
                mac_opt->burst_type,mac_opt->burst_num,mac_opt->channel_id,mac_opt->fn,mac_opt->tn,mac_opt->bn);

        }
        #endif



        /* Check if this burst is SI13_INFO or not.
         * SI13-type burst shall not be inserted into partial received queue,
         * since it's not a GPRS RLC/MAC block.
         */

        if ( mac_opt->burst_type == SI13_BURST ) {

            Si13PbcchLocation* si13_msg         = reinterpret_cast<Si13PbcchLocation*> (bss_msg->user_data);
            //si13_msg->si13_location             = 0; /* the si_tn shall always be tn=0 */
            cur_used_bss->pbcch_tn              = si13_msg->pbcch_location + 1;
            cur_used_bss->psi1_repeat_period    = si13_msg->psi1_repeat_period ;

            if (utcb) {
                if ( ( CORRESPONDING_DOWNLINK_CH(utcb->uplink_freq) == (cur_used_bss->get_bcch_no()) ) &&
                    ( !mac_opt->tn || (mac_opt->tn==1) ) )
                    utcb->usf_granted_ind->set(mac_opt->tn, ((uplink_blkn+1)%12) );
            }

            if ( bss_msg->packet) {
                remove_upper_layer_pkt(bss_msg->packet);
                free_pkt(bss_msg->packet);
                bss_msg->packet     = NULL;
            }
            FREE_BSS_EVENT_INCLUDING_USER_DATA(ep);
            ep = NULL;
        }

        else {

            if ( mac_opt->burst_type == DUMMY_BURST ) {
                DB* tmpdb = reinterpret_cast<DB*> (mac_opt->msg);
                if (tmpdb) {

                    if ( mac_opt->msg ) {

                        shared_obj_dec_refcnt( mac_opt->msg_type , mac_opt->msg );
                        int res = shared_obj_release( mac_opt->msg_type , mac_opt->msg );
                        if ( res ) {
                            mac_opt->msg = 0;
                        }

                    }

                }
            }

            uchar recv_burst_number = mac_opt->burst_num;
            uchar recv_tn           = mac_opt->tn;
            uchar recv_fn           = mac_opt->fn;
            uchar recv_bn           = mac_opt->bn;

            /* perform burst number checking */
            if ( (recv_burst_number != fn_to_burst_num(recv_fn)) ) {
                cout << "MS MAC: incorrect burst number sent over this fn." << endl ;
                printf(" Dump mac_option: burst_type %d, burst_num %d, chid %ld, fn %lu, tn %lu, bn %lu \n",
                        mac_opt->burst_type, recv_burst_number , mac_opt->channel_id,
                            (ulong)(recv_fn) , (ulong)(recv_tn) , (ulong)(recv_bn) );

                exit(1);
            }

            if ( (test_is_idle_frame(recv_fn)) ) {

                //printf("MS: fn = %ld \n", fn);

                if ( bss_msg->packet) {
                    remove_upper_layer_pkt(bss_msg->packet);
                    free_pkt(bss_msg->packet);
                    bss_msg->packet     = NULL;
                }

                FREE_BSS_EVENT(ep);
                return 0;
            }


            int ins_status = downlink_rq[(uchar)(recv_tn)]->insert(recv_burst_number,ep);
            //printf("MS MAC: ep addr = %lx \n" , ep );
            ep = NULL;

           /* If the burst is received on a block boundary, the flush of the
            * burst queue containing this burst should be forbidden on this bit period.
            * Because, under NCTUns events triggered
            */

            if ( (bn == 155) && (tn==7) ) {
                drq_flush_ind[(uchar)(recv_tn)] = false;
            }
            else {
                drq_flush_ind[(uchar)(recv_tn)] = true;
            }

            if (!ins_status)
                return 0;

            Event* blk = NULL;

            if (mac_opt->burst_num>=3) {
                blk = downlink_rq[(uchar)(recv_tn)]->fast_assembling();

                if (!blk) {
                    printf("MS MAC: Node ID %u cannot reassemble the block \n", get_nid() );
                }
                else {

                    uchar*  decoded_data    = NULL;
                    DMH*    dmh             = NULL;
                    bss_message* tmp_bssmsg = reinterpret_cast<bss_message*> (blk->DataInfo_);
                    mac_option*  tmp_macopt = reinterpret_cast<mac_option*> (tmp_bssmsg->mac_option);
                    rlc_option*  tmp_rlcopt = reinterpret_cast<rlc_option*> (tmp_bssmsg->rlc_option);

                    ulong burst_tn      = tmp_macopt->tn;
                    //ulong burst_type    = tmp_macopt->burst_type;

                    if ( tmp_macopt->burst_type == NORMAL_BURST) {

                        IS_NULL_STR(tmp_macopt,"GprsMsMac burst_demul(): tmp_bssmsg->mac_opt is null\n",-1);
                        IS_NULL_STR(tmp_rlcopt,"GprsMsMac burst_demul(): tmp_bssmsg->rlc_opt is null\n",-1);
                        decoded_data = channel_decoding(tmp_rlcopt->cs,reinterpret_cast<uchar*> (tmp_bssmsg->user_data) );

                        if (decoded_data) {
                            dmh = new DMH;
                            dmh->decode(decoded_data[0]);
                        }
                        else {
                            /* received block is corrupted */
                            printf("MsMac(nid %u) burst_demul(): burst is corrupted.\n", get_nid());

                            if ( tmp_bssmsg->packet) {
                                remove_upper_layer_pkt(tmp_bssmsg->packet);
                                free_pkt(tmp_bssmsg->packet);
                                tmp_bssmsg->packet     = NULL;
                            }
                            FREE_BSS_EVENT( blk );
                            return 1;
                        }

                    }
                    else if ( (tmp_macopt->burst_type == CTL_BURST) ) {

                        IS_NULL_STR(tmp_macopt,"GprsMsMac burst_demul(): tmp_bssmsg->mac_opt is null\n",-1);
                        IS_NULL_STR(tmp_rlcopt,"GprsMsMac burst_demul(): tmp_bssmsg->rlc_opt is null\n",-1);
                        decoded_data   = reinterpret_cast<uchar*> (tmp_bssmsg->user_data);
                        dmh            = reinterpret_cast<DMH*>   (tmp_bssmsg->mac_header);

                    }
                    else if ( (tmp_macopt->burst_type == DUMMY_BURST) ) {

                        IS_NULL_STR(tmp_macopt,"GprsMsMac burst_demul(): tmp_bssmsg->mac_opt is null\n",-1);

                        decoded_data   = reinterpret_cast<uchar*> (tmp_macopt->msg);
                        dmh            = reinterpret_cast<DMH*>   (tmp_bssmsg->mac_header);


                        #ifdef  DUMMY_BURST_DEBUG
                        cout << "MS MAC: receive a dummy block" << endl;
                        #endif
                    }
                    else
                        cout << "GprsMsMac burst_demul(): unknown burst type" << (int)(tmp_macopt->burst_type) << endl;

                    if (dmh) {

                        if (utcb) {


                            /**** process usf ****/
                            /* Due to precise time synchronization in NCTUns, receiving an incoming
                            * burst may occur at the boundary of a time slot. The sequence
                            * of scheduled events at the same tick is not predictable in NCTUns. Thus,
                            * the MAC header processing is based on tn value time-stamped in BTS side,
                            * not the value in MS side.
                            */


                            #ifdef  USF_DEBUG
                            cout << "GprsMsMac burst_demul(): dmh->usf = "                  << (int)(dmh->usf)
                                << " utcb->usf_fn[" << (int) (tmp_macopt->tn) << "] = "
                                << (int) (utcb->usf_tn[(int)(tmp_macopt->tn)])
                                << " received_freq = "                                     << tmp_macopt->channel_id
                                << " at tick = ";
                                printf ( " %llu \n", GetCurrentTime() );
                            #endif

                            if ( (dmh->usf != 0x7) && (dmh->usf == utcb->usf_tn[(int)(tmp_macopt->tn)]) ) {

                                /* derive the assigned blkn with tn value in the incoming block */
                                long tmp_blkn = fn_to_blkn(tmp_macopt->fn);

                                if ( (tmp_blkn!=-1) && (blkn_to_starting_fn(tmp_blkn) == ((ulong)tmp_macopt->fn-3)) ) {

                                    utcb->usf_granted_ind->set(tmp_macopt->tn, ((tmp_blkn+1)%12) );

                                    /* tmp_macopt is stamped by the first burst */
                                    #ifdef USF_DEBUG
                                    printf("MS MAC (nid %ld): utcb->usf_granted[%d] = %ld \n",
                                        get_nid(), tmp_macopt->tn , (tmp_blkn+1)%12 );
                                    #endif
                                }
                                else {

                                    /* tmp_macopt is stamped by the first burst */
                                    utcb->usf_granted_ind->set(tmp_macopt->tn, ((tmp_blkn+1)%12) );

                                    printf("MS MAC: Warning: USF is not assigned with block boundary alignment. utcb->usf_granted[%d] = %ld \n", tmp_macopt->tn , tmp_blkn+1);
                                }


                            }

                            else if ( dmh->usf == 0x7 ) {

                                #ifdef USF_DEBUG
                                printf("MS MAC: utcb->usf_granted[%d] = random \n", tmp_macopt->tn);
                                #endif
                                long tmp_blkn = fn_to_blkn(tmp_macopt->fn);
                                utcb->usf_granted_ind->set ( tmp_macopt->tn , (tmp_blkn+1)%12 );

                            }

                            else {
                                #ifdef USF_DEBUG
                                printf("MS MAC: utcb->usf_granted[%d] = not_granted \n", tmp_macopt->tn);
                                #endif
                                long tmp_blkn = fn_to_blkn(tmp_macopt->fn);
                                utcb->usf_granted_ind->unset( tmp_macopt->tn , (tmp_blkn+1)%12 );
                            }

                            /* process rrbp field */
                            #ifdef __RRBP_UTCB_USED
                            /* UTCB may be notified of the assignment of a block for polling
                            * procedure.
                            */

                            if ( dmh->sp ) {

                                /* process rrbp field */
                                if ( dmh->rrbp == 0 ) {
                                    /*utcb->pacch_blkn    = fn_to_blkn((fn+13)%52);
                                    utcb->pacch_tn      = tmp_macopt->tn;*/

                                    int calculated_blkn = fn_to_blkn((fn+13)%52);
                                    utcb->pacch_blkn_bitmap[(uchar)calculated_blkn][tmp_macopt->tn] = 1;

                                }
                                else if ( dmh->rrbp == 1 ) {
                                    /*utcb->pacch_blkn    = fn_to_blkn((fn+17)%52);
                                    utcb->pacch_tn      = tmp_macopt->tn;*/
                                    int calculated_blkn = fn_to_blkn((fn+17)%52);
                                    utcb->pacch_blkn_bitmap[(uchar)calculated_blkn][tmp_macopt->tn] = 1;
                                }
                                else if ( dmh->rrbp == 2 ) {
                                    /*utcb->pacch_blkn    = fn_to_blkn((fn+21)%52);
                                    utcb->pacch_tn      = tmp_macopt->tn;*/
                                    int calculated_blkn = fn_to_blkn((fn+21)%52);
                                    utcb->pacch_blkn_bitmap[(uchar)calculated_blkn][tmp_macopt->tn] = 1;
                                }
                                else {
                                    /*utcb->pacch_blkn    = fn_to_blkn((fn+26)%52);
                                    utcb->pacch_tn      = tmp_macopt->tn;*/
                                    int calculated_blkn = fn_to_blkn((fn+26)%52);
                                    utcb->pacch_blkn_bitmap[(uchar)calculated_blkn][tmp_macopt->tn] = 1;
                                }
                            }
                            else
                                utcb->rrbp_cur = gprs::rrbp_unused;
                            #endif
                        }

                        if ( dtcb ) {

                            /* process rrbp field:
                            * The assignment of rrbp field is triggerd by dmh->sp is 1.
                            * Clear rrbp field when the indicated block passes.
                            */

                            if ( (dtcb->ts_alloc[(uchar)(tmp_macopt->tn)]) && dmh->sp ) {

                                if ( dtcb->rrbp_cur != dmh->rrbp ) {

                                    /* process rrbp field */
                                    if ( dmh->rrbp == 0 ) {

                                        int calculated_blkn = fn_to_blkn((fn+13)%52);
                                        dtcb->pacch_blkn_bitmap[calculated_blkn][(uchar)tmp_macopt->tn] = 1;

                                    }
                                    else if ( dmh->rrbp == 1 ) {
                                        int calculated_blkn = fn_to_blkn((fn+17)%52);
                                        dtcb->pacch_blkn_bitmap[calculated_blkn][(uchar)tmp_macopt->tn] = 1;
                                    }
                                    else if ( dmh->rrbp == 2 ) {
                                        int calculated_blkn = fn_to_blkn((fn+21)%52);
                                        dtcb->pacch_blkn_bitmap[calculated_blkn][(uchar)tmp_macopt->tn] = 1;
                                    }
                                    else {
                                        int calculated_blkn = fn_to_blkn((fn+26)%52);
                                        dtcb->pacch_blkn_bitmap[calculated_blkn][(uchar)tmp_macopt->tn] = 1;
                                    }

                                }

                                #ifdef __RRBP_DEBUG

                                if ( tmp_macopt->channel_id != cur_used_bss->get_bcch_no() )  {

                                    cout << "*** At bn = " << bn << " tn = " << tn << " fn = " << fn << " blkn = " << blkn << endl;
                                    cout << "MS MAC: sp = " << (int)(dmh->sp) << " rrbp value = " << (long) (dtcb->rrbp_cur)
                                            << "    rlc_opt->rrbp = " << (int)(tmp_rlcopt->rrbp) << endl;


                                }
                                #endif

                                dtcb->rrbp_cur = dmh->rrbp;

                                /* Use polling_indicator to notify MS RLC that the ack should be sent on the
                                * specified block.
                                */
                            }
                            else {
                                   dtcb->rrbp_cur = gprs::rrbp_unused;
                            }


                        }



                        if (dmh->payload_type == 0x00 ) { /* user data */

                            if ( tmp_macopt->burst_type == NORMAL_BURST) {

                                if ( tmp_bssmsg->user_data ) {
                                    if ( tmp_rlcopt->tfi == dtcb->tfi ) {
                                        ;//delete tmp_bssmsg->user_data;
                                    }
                                    tmp_bssmsg->user_data = NULL;
                                }

                                tmp_bssmsg->user_data = reinterpret_cast<char*> (decoded_data);
                                if (dmh) delete dmh;

                                if ( tmp_rlcopt->tfi == dtcb->tfi ) {
                                    res = NslObject::recv(blk);
                                }
                                else {

                                    if ( tmp_bssmsg->packet) {
                                        remove_upper_layer_pkt(tmp_bssmsg->packet);
                                        free_pkt(tmp_bssmsg->packet);
                                        tmp_bssmsg->packet     = NULL;
                                    }

                                    FREE_BSS_EVENT_INCLUDING_USER_DATA(blk);
                                    res = 0;
                                }



                            }
                            else if ( tmp_macopt->burst_type == CTL_BURST) {
                                cout <<"GprsMsMac burst_demul(): control burst should not carry user data\n"<<endl;
                                res = 0;
                            }
                            else {
                                cout <<"GprsMsMac burst_demul(): unknown burst type with payload type = 0x00 \n"<<endl;
                                res = 0;
                            }

                        }
                        else if ( dmh->payload_type == 0x01 ){ /* control data */

                            if ( tmp_macopt->burst_type == CTL_BURST ) {

                                if ( tmp_rlcopt->ctl_msg_type == PKT_UPLINK_ACKNACK ) {

                                    if ( tmp_rlcopt->tfi == utcb->tfi ) {
                                        tmp_bssmsg->user_data   = reinterpret_cast<char*> (decoded_data);
                                        tmp_bssmsg->flag        = PF_RECV;
                                        return NslObject::recv(blk);
                                    }
                                    else {

                                        if ( tmp_bssmsg->user_data ) {

                                            shared_obj_dec_refcnt( tmp_macopt->msg_type , tmp_bssmsg->user_data );
                                            shared_obj_release( tmp_macopt->msg_type , tmp_bssmsg->user_data );
                                            tmp_bssmsg->user_data = 0;

                                        }

                                        if ( tmp_bssmsg->packet) {
                                            remove_upper_layer_pkt(tmp_bssmsg->packet);
                                            free_pkt(tmp_bssmsg->packet);
                                            tmp_bssmsg->packet     = NULL;
                                        }

                                        FREE_BSS_EVENT(blk);
                                        res = 0;
                                    }

                                }
                                else {

                                    res = ctrmsg_demultiplexer (decoded_data,tmp_rlcopt->ctl_msg_type );
                                    if ( tmp_bssmsg->packet) {
                                        remove_upper_layer_pkt(tmp_bssmsg->packet);
                                        free_pkt(tmp_bssmsg->packet);
                                        tmp_bssmsg->packet     = NULL;
                                    }

                                    FREE_BSS_EVENT(blk);

                                }
                            }
                            else if (tmp_macopt->burst_type == DUMMY_BURST) {
                                DB* tmpdb = reinterpret_cast<DB*> (decoded_data);
                                if (tmpdb) {

                                    if ( tmp_macopt->msg ) {

                                        shared_obj_dec_refcnt( tmp_macopt->msg_type , tmp_macopt->msg );
                                        int res = shared_obj_release( tmp_macopt->msg_type , tmp_macopt->msg );
                                        if ( res ) {
                                            tmp_macopt->msg = 0;
                                        }

                                    }

                                }
                                decoded_data = NULL;

                                if ( tmp_bssmsg->packet) {
                                    remove_upper_layer_pkt(tmp_bssmsg->packet);
                                    free_pkt(tmp_bssmsg->packet);
                                    tmp_bssmsg->packet     = NULL;
                                    printf("MS MAC[%u]: Assertion failed: dummy burst carries a user packet.\n",
                                        get_nid());
                                    exit(1);
                                }
                                FREE_BSS_EVENT( blk );
                            }

                            else {
                                cout <<"GprsMsMac burst_demul(): unknown burst type with payload type = 0x01 \n"<<endl;
                                res = 0;
                            }
                        }
                        else
                            cout <<"GprsMsMac burst_demul(): unsupported payload type = " << dmh->payload_type << endl;
                    }
                    else {
                        #ifdef __DEBUG_
                        cout <<"GprsMsMac burst demultiplexer(): mac header is missing or uplink tcb does not exist.\n" << endl;
                        #endif
                    }


                    downlink_rq[burst_tn]->flush();

                }

            }

        } /* end of burst type except SI13_BURST */

        return res;
    }

    int GprsMsMac::update_blkn() {
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
        return 1;
    }

    int GprsMsMac::update_counters(uchar tn1,ulong fn1) {

        bn = 0;
        if ((tn1!=tn) || (fn1!=fn)) {
            fn              = fn1;
            uplink_fn       = fn1;
            tn              = tn1;
            uplink_tn       = tn1 - 3;

            if (uplink_tn<0) {
                uplink_tn +=8;
                uplink_fn--;
                if (uplink_fn<0)
                    uplink_fn +=52;
            }

            update_fn(fn);
            update_uplink_fn(uplink_fn);

        }

        set_trigger_sending_indicator();

        return 1;
    }
    int GprsMsMac::update_tn() {

        ++tn;
        ++uplink_tn;
        if (tn>=8) {
            update_fn(-1);
            tn=0;
        }

        if ((uplink_tn%8) == 0) {
            update_uplink_fn(-1);
            uplink_tn=0;
        }

        set_trigger_sending_indicator();

        return 1;
    }

    int GprsMsMac::update_fn(long fn1) {

        if ( fn1 <0)
            fn = ++fn%52;
        else
            fn = fn1;

        update_blkn();

        return 1;
    }

    int GprsMsMac::update_uplink_fn(long fn1) {

        if (fn1<0)
            uplink_fn = ++uplink_fn%52;
        else
            uplink_fn = fn1;

        update_blkn();

        /* count down tbf_starting_time_count_down_value */
        if (utcb) {
            if (utcb->sbafreq) {
                if (utcb->tbf_starting_time_count_down_value>0) {
                    if ( fn1<0)
                        --utcb->tbf_starting_time_count_down_value;
                    else
                        utcb->tbf_starting_time_count_down_value-=fn1;
                }
            }
        }
        if (dtcb) {
            if (dtcb->sbafreq) {
                if (dtcb->tbf_starting_time_count_down_value>0) {
                    if ( fn1 <0)
                        --dtcb->tbf_starting_time_count_down_value;
                    else
                        dtcb->tbf_starting_time_count_down_value-=fn1;
                }
            }
        }
        return 1;
    }

    int GprsMsMac::update_bn() {


        /* update internal counters */
        ++bn;
        if ( bn>=156 ) {
            bn=0;
            update_tn();

            #ifdef  __TIMING_DEBUG
                printf ("*** timing info. *****\n");
                printf ("bn = %ld tn = %ld fn = %ld  blkn=%ld \n", bn , tn , fn , blkn);
            #endif

        }
        else if ( bn == 147 ) { /* 156.25 - 8.25 = 148 : guard period*/
            /* configure channels to be monitored for next timeslot */
            configure_listened_channels();
        }
        else
            ;

        /*if ( utcb && roaming_flag ) {
            if ( utcb->ta_ie != 0 ) {
                printf("Bug: here\n");
                exit(1);
            }
        }*/

        gmm_update();
        gprs_scheduler();

        /*if ( utcb && roaming_flag ) {
            if ( utcb->ta_ie != 0 ) {
                printf("Bug: here\n");
                exit(1);
            }
        }*/

        return 1;
    }

    ulong GprsMsMac::blkn_to_starting_fn(long blkn1 ) {
        if ( blkn1 < 0 || blkn1 >51 ) {
            cout << "GprsbtsMac blkn_to_starting_fn(): Assertion failed: blkn < 0 || blkn > 51 " << endl;
            return 1000;
        }
        return blkn1*4+(blkn1/3);
    }

    int GprsMsMac::mark_listening_channel(long chid) {
        if (chid>=250) {
            cout <<"GprsMsMac mark_listening_channel(): chid ="<< chid <<" is out of range"<<endl;
            return -1;
        }

        listen_chmap[chid] = true;


        #ifdef  __SET_LISTENING_CH_DEBUG

        printf("MS MAC: freq = %d is monitored on tn[%ld] bn = %ld . at tick= %llu \n",
            chid , tn , bn , GetCurrentTime() );

        #endif

        return 1;
    }

    int GprsMsMac::set_listening_channel() {

        return radiolink_obj->set_listen_channel(listen_chmap);
    }


    int GprsMsMac::gprs_scheduler() {


        /* flush the receive burst queue of the preceding timeslot if it is on blkn_boundary */
        if ( is_blkn_boundary() && (bn==0) && (!tn) ) {

            for ( int i=0 ; i<8 ; ++i ) {
                if ( drq_flush_ind[i] )
                    downlink_rq[i]->flush();

                drq_flush_ind[i] = true;
            }

        }

        /* get timing advance value */
        ulong ta_value = 0;
        #ifdef __SCHEDULER_TIMING_DEBUG
        printf( "MS MAC: gprs_scheduler is invoked on tn[%ld] bn = %ld at tick = %llu \n" , tn , bn , GetCurrentTime() );
        #endif

        if ( utcb ) {

            if ( (utcb->ta_ie)  ) {

                /*printf (" MS MAC gprs scheduler(): ta_ie  = %lu value = %d \n", (ulong) (utcb->ta_ie) ,
                    (utcb->ta_ie)->timing_advance_value );*/

                if ( ((utcb->ta_ie)->timing_advance_value<156) && ((utcb->ta_ie)->timing_advance_value>0)  )
                    ta_value = utcb->ta_ie->timing_advance_value;

                else if ((utcb->ta_ie)->timing_advance_value>=156 ) {
                    ta_value = 0;
                    ASSERTION( false , "GprsMsMac::gprs_scheduler(): ta_value is larger than 155.");
                }

                else
                    ta_value = 0 ;
            }
            else
                ta_value = 0;
        }
        else if ( dtcb ) {

            if ( (dtcb->ta_ie) ) {
                if ( ((dtcb->ta_ie)->timing_advance_value<156) && ((dtcb->ta_ie)->timing_advance_value>0)  )
                    ta_value = dtcb->ta_ie->timing_advance_value;

                else if ((dtcb->ta_ie)->timing_advance_value>=156 ) {
                    ta_value = 0;
                    ASSERTION( false , "GprsMsMac::gprs_scheduler(): ta_value is larger than 155.");
                }

                else
                    ta_value = 0 ;
            }
            else
                ta_value = 0;

        }
        else
            ta_value = 0;


        //if (ta_value)
        //printf("MS MAC: ta_value =%ld \n", ta_value);

        if ( ta_value ) {
            triggered_tn    = uplink_tn + 1;
            triggered_fn    = uplink_fn;
            triggered_blkn  = fn_to_blkn(triggered_fn);

            if ( triggered_tn >7 ) {

                triggered_tn = 0;
                ++triggered_fn;

                if ( triggered_fn >=52 ) {
                    triggered_fn    = 0;
                }

                triggered_blkn  = fn_to_blkn(triggered_fn);
            }
        }
        else {
            triggered_tn    = uplink_tn;
            triggered_fn    = uplink_fn;
            triggered_blkn  = fn_to_blkn(triggered_fn);
        }

        if ( bn == ((156-ta_value)%156) ) {

            if ( trigger_sending_ind[triggered_tn] ) {

                #ifdef  __TIMING_DEBUG
                    printf ("*** timing info. *****\n");
                    printf ("bn = %ld tn = %ld fn = %ld  blkn=%ld \n", bn , tn , fn , blkn);
                #endif


                /* If the state of RLC is PACKET_TRANSFER_STATE,
                 * MAC shall notify RLC of sending data
                 */

                if ( state == PACKET_TRANSFER_STATE ) {

                    bool need_polling = false;

                    PollInd* poll_ind = new PollInd;

                    /* set up polling channel */
                    if( dtcb && (test_is_blkn_boundary(triggered_fn))  && (triggered_blkn>=0 && triggered_blkn < 12)){

                        if ( (dtcb->pacch_sq[triggered_tn]->is_empty()) &&
                             ( dtcb->pacch_blkn_bitmap[(uchar)(triggered_blkn)][triggered_tn] )
                             ) {

                             poll_ind->polling_ch_bitmap[dtcb->downlink_freq] = true;
                             poll_ind->rrbp_ind = 0;
                             dtcb->rrbp_cur = gprs::rrbp_unused;
                             need_polling = true;
                             //printf("MS MAC: choose DTCB to send ACK \n");
                        }
                    }

                    if ( utcb && (test_is_blkn_boundary(triggered_fn)) ) {

                        uchar utbf_pacch_req = false;
                        uchar utbf_pdtch_req = false;

                        if ( !utcb->pacch_pending_event_sq->is_empty() ) {

                            if ( utcb->sbafreq ) {

                                if ( (utcb->sbafreq->sba->tn == triggered_tn) ) {
                                    utbf_pacch_req = true;
                                }

                            }

                            if ( (utcb->ts_alloc[triggered_tn]) ) {

                                utbf_pacch_req = true;

                            }
                        }

                        if ( utcb->pdtch_sq[triggered_tn]->is_empty() && utcb->ts_alloc[triggered_tn] )
                            utbf_pdtch_req = true;

                        if ( utbf_pacch_req ) {

                            Event* epkt = (utcb->pacch_pending_event_sq->get_head())->get_elem();
                            block_partition( epkt ,PACCH_SQUEUE);
                            utcb->pacch_pending_event_sq->remove_head();

                            printf("MS MAC(nid %d): UTBF starts to send control message on CH %d on TN %ld\n",
                                get_nid(),utcb->uplink_freq,tn);

                        }
                        else if ( utbf_pdtch_req ) {
                            poll_ind->polling_ch_bitmap[utcb->uplink_freq] = true;
                            need_polling = true;
                        }
                        else
                            ;

                    }

                    poll_ind->polling_tn = triggered_tn;

                    if ( need_polling ) {

                        #ifdef   __TRIGGER_RLC_DEBUG
                        cout<<"GprsMsMac::gprs_scheduler(): Trigger rlc to send 'ONE' rlc block, ta_value = 0 :"<< endl;
                        SHOW_TIME;
                        #endif

                        ack_rlc(PUSH_A_RLCDATA_BLK,poll_ind);
                    }
                    else {
                        delete poll_ind;
                    }

                }
            }

            /**** the following specifies the process of sending ****/
            /* Assumption: BTS shall not assign the same time slot for both uplink and downlik TBFs of
            * a MS, because MS with some classes are usually not able to send/receive at the same time.
            */


            /* Adopt 0.7 - persistence for downlink control packets. And
            * 0.3 - persistence for uplink data and control packets.
            */


            /*if ( test_is_blkn_boundary(triggered_fn) ) {
                long  per_num = (random())%100;
                if ( per_num < 70 )
                    up_down[triggered_tn] = 1;
                else
                    up_down[triggered_tn] = 2;
            }*/

            if (dtcb /*&& (up_down[triggered_tn] == 1)*/ ) {

                if ( utcb->state == RETRY_PHASE ) {
                    /*
                    --utcb->wait_time_cnt;
                    if (utcb->wait_time_cnt <=0 )
                        establish_uplink_tbf(TWOPHASE_ACCESS,cur_used_bss->get_bcch_no(),tlli);

                    */
                    ;/* Do nothing. The task in this clause is replaced by chreq_timer handler. */
                }

                else if ( utcb->state == CHANNEL_REQ_PHASE ) {

                    if ( !prach_squeue->is_empty() ) {

                            /* consider tn and blkn:
                            * prach is indicated by USF = 0x7 and blkn != -1 ( idle slots).
                            */

                            if( (utcb->usf_granted_ind->hit_search( triggered_tn , triggered_blkn)) && (blkn != -1 )) {

                                Event* eb = (prach_squeue->get_head())->get_elem();
                                send_timestamp(eb,CORRESPONDING_UPLINK_CH((cur_used_bss->get_bcch_no())) );
                                prach_squeue->remove_head();

                            }
                    }

                }

                else {

                    if ( !dtcb->tbf_starting_time_count_down_value ) {

                        /* examine whether pacch has data to send:
                         * sbafreq is valid only when two-phase access is being performed.
                         */

                        if (dtcb->sbafreq) {

                            if ( triggered_tn == dtcb->sbafreq->sba->tn ) {

                                if ( !dtcb->pacch_sq[triggered_tn]->is_empty() ) {
                                    Event* eb = (dtcb->pacch_sq[triggered_tn]->get_head())->get_elem();
                                    send_timestamp(eb,CORRESPONDING_UPLINK_CH(dtcb->downlink_freq));
                                    dtcb->pacch_sq[triggered_tn]->remove_head();
                                }

                            }
                            else
                                ;
                        }
                        else {

                            if ( (triggered_blkn>=0 && triggered_blkn < 12) ) {

                                if ( (dtcb->pacch_blkn_bitmap[(uchar)(triggered_blkn)][triggered_tn] )  ||
                                      (dtcb_rrbp_in_trans_flag == 1) ) {

                                    if ( !dtcb->pacch_sq[triggered_tn]->is_empty() ) {
                                        Event* eb = (dtcb->pacch_sq[triggered_tn]->get_head())->get_elem();

                                        mac_option* mac_opt = reinterpret_cast<mac_option*>
                                            (reinterpret_cast<bss_message*>(eb->DataInfo_)->mac_option);

                                        //printf("ms mac: pacch_sq_len = %ld \n", dtcb->pacch_sq[triggered_tn]->get_list_num() );
                                        /* perform assertion */
                                        if ( mac_opt->burst_num != fn_to_burst_num(triggered_fn) ) {


                                            dtcb_rrbp_in_trans_flag = 0;
                                            dtcb->pacch_blkn_bitmap[(uchar)(triggered_blkn)][triggered_tn] = 0;
                                            printf("MS MAC::gprs_scheduler(): The sending action is postponed until the block boundary occurs.\n");
                                            return 0;
                                            /*if ( mac_opt->burst_num == 0 ) {
                                                dtcb_rrbp_in_trans_flag = 0;
                                                dtcb->pacch_blkn_bitmap[(uchar)(triggered_blkn)][triggered_tn] = 0;
                                                printf("MS MAC::gprs_scheduler(): The sending action is postponed until the block boundary occurs.\n");
                                                return 0;
                                            }
                                            else {

                                                dtcb_rrbp_in_trans_flag = 0;
                                                dtcb->pacch_blkn_bitmap[(uchar)(triggered_blkn)][triggered_tn] = 0;

                                                printf("MS MAC::gprs_scheduler(): The sending action is postponed until the block boundary occurs.\n");

                                                printf("mac_opt->burst_num = %ld mac_opt->nid = %ld , expected_burst_num = %ld , uplink_fn = %ld \n",
                                                    mac_opt->burst_num, mac_opt->nid, fn_to_burst_num(triggered_fn) , triggered_fn );
                                                printf("MS MAC: ta_value = %ld \n" , ta_value);

                                                exit(1);
                                            }*/
                                        }

                                        if ( mac_opt->burst_num == 0 ) {
                                            /*printf("At uplink_fn == %ld switch rrbp_in_trans_flag to 1 uplink_tn=%ld.\n",
                                                uplink_fn,uplink_tn);*/
                                            dtcb_rrbp_in_trans_flag = 1;
                                        }
                                        /*if ( mac_opt->burst_num == 1 ) {
                                            printf("At uplink_fn == %ld bnum = 1 uplink_tn=%ld.\n",
                                                uplink_fn,uplink_tn);
                                        }
                                        if ( mac_opt->burst_num == 2 ) {
                                            printf("At uplink_fn == %ld bnum = 2 uplink_tn=%ld.\n",
                                                uplink_fn,uplink_tn);
                                        }*/

                                        if ( mac_opt->burst_num >=3 ) {
                                            /* The end of this PACCH block period. */
                                            dtcb->pacch_blkn_bitmap[(uchar)(triggered_blkn)][triggered_tn] = 0;
                                            dtcb_rrbp_in_trans_flag = 0;
                                            //printf("At uplink_fn == %ld switch rrbp_in_trans_flag to 0.\n", uplink_fn);
                                        }

                                        send_timestamp(eb,CORRESPONDING_UPLINK_CH(dtcb->downlink_freq));
                                        dtcb->pacch_sq[triggered_tn]->remove_head();

                                    }
                                    else {
                                        ;//dtcb_rrbp_in_trans_flag = 0;
                                        /*printf("At uplink_fn == %ld switch rrbp_in_trans_flag to 0 uplink_tn=%ld. (case_2)\n",
                                            uplink_fn, uplink_tn );*/
                                    }
                                }
                            }
                        }
                    }
                }


            } /* end of dtcb */


            /* examine whether utcb has data to send */
            if ( utcb /*&& (up_down[triggered_tn] == 2)*/) {

                if ( utcb->state == RETRY_PHASE ) {
                    /*
                    --utcb->wait_time_cnt;
                    if (utcb->wait_time_cnt <=0 )
                        establish_uplink_tbf(TWOPHASE_ACCESS,cur_used_bss->get_bcch_no(),tlli);
                    */
                    ;/* Do nothing. The task in this clause is replaced by chreq_timer handler. */
                }
                else if ( utcb->state == CHANNEL_REQ_PHASE ) {

                    if ( !prach_squeue->is_empty() ) {

                        if ( (utcb->usf_granted_ind->hit_search( triggered_tn , triggered_blkn) ) && (blkn != -1 )) {
                            Event* eb = (prach_squeue->get_head())->get_elem();
                            send_timestamp(eb,CORRESPONDING_UPLINK_CH((cur_used_bss->get_bcch_no())));
                            prach_squeue->remove_head();

                            if ( !chreq_timer ) {
                                chreq_timer = new timerObj;
                                chreq_timer->init();
                                chreq_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),
                                    (int (NslObject::*)(Event*)) &GprsMsMac::retry_establish_uplink_tbf );

                            }

                            chreq_timer->cancel();
                            u_int64_t duration_tick;
                            u_int32_t duration_sec = 1;
                            SEC_TO_TICK(duration_tick, duration_sec);
                            chreq_timer->start(duration_tick,0);
                            printf("MS MAC: Setup Channel REQ. Timer with duration = %u sec.\n", duration_sec);
                        }
                    }
                }
                else { /* For other states, timing advancing shall be performed */

                    if ( !utcb->tbf_starting_time_count_down_value ) {

                        #ifdef __SEQUENCE_DEBUG_PART
                        cout <<"GprsMsMac gprs_scheduler(): tbf_starting_time_count_down_value="<<
                            (int) (utcb->tbf_starting_time_count_down_value) << endl;
                        #endif
                        /* examine whether pacch has data to send:
                        * sbafreq is valid only when two-phase access is being performed.
                        */
                        if (utcb->sbafreq) {

                            if ( utcb->sba_in_transmission<0 ) {
                                if ( (test_is_blkn_boundary(triggered_fn)) ) {
                                    utcb->sba_in_transmission = triggered_blkn;
                                }
                            }

                            if ( (utcb->sba_in_transmission == triggered_blkn) && (triggered_tn == utcb->sbafreq->sba->tn) ) {

                                if ( !utcb->pacch_sq[triggered_tn]->is_empty() ) {
                                    Event* eb = (utcb->pacch_sq[triggered_tn]->get_head())->get_elem();
                                    send_timestamp(eb, utcb->sbafreq->fp->arfcn );
                                    utcb->pacch_sq[triggered_tn]->remove_head();

                                    #ifdef __SEQUENCE_DEBUG
                                    cout <<"GprsMsMac gprs_scheduler(): send PACCH (UL) DATA indicated by SBA " << endl;
                                    #endif
                                }
                                else {
                                    #ifdef __SEQUENCE_DEBUG
                                    cout <<"GprsMsMac gprs_scheduler(): send PACCH (UL) DATA indicated by SBA but no data"
                                        << endl;
                                    #endif

                                }
                            }

                        }

                        /* if sbafreq is not valid but usf_granted is asserted, the
                        * two-phase access procedure is completed.
                        */

                        else if ( (utcb->usf_granted_ind->hit_search ( triggered_tn , triggered_blkn)) ) {

                            if ( utcb->pdtch_sq[triggered_tn] ) {

                                if ( utcb->pdtch_sq[triggered_tn]->is_empty() ) {
                                    #ifdef __SEQUENCE_DEBUG_PART_2
                                    cout <<"GprsMsMac gprs_scheduler(): send PDTCH (UL) DATA indicated by PUA but no data"
                                        << endl;
                                    #endif
                                    return 1;
                                }
                                else {
                                    Event* eb = (utcb->pdtch_sq[triggered_tn]->get_head())->get_elem();
                                    send_timestamp(eb, utcb->uplink_freq );
                                    utcb->pdtch_sq[triggered_tn]->remove_head();
                                    #ifdef __SEQUENCE_DEBUG_PART_2
                                    cout <<"GprsMsMac gprs_scheduler(): send PDTCH (UL) DATA indicated by PUA " << endl;
                                    #endif
                                }
                            }

                        }
                        else
                            ;
                    }
                }

            }/* end of utcb */

        }
        return 1;
    }


    long GprsMsMac::fn_to_blkn(ulong fn1) {

        long blkn1 = -1;
        if ( fn1>=0 ) {
            if ( fn1 == 12 || fn1 == 25 || fn1 == 38 || fn1 == 51)
                blkn1 = -1;
            else
                blkn1 = ( fn1 - (fn1-(fn1/12))/12 ) /4;
        }
        return blkn1;
    }

    int GprsMsMac::update_ts_map(bool d_bit,uchar timeslot_allocation,uchar tfi) {
        ulong bit_indicator = 0x1;
        if ( tfi >=32 ) {
            cout << "illegal tfi value="<< tfi <<endl;
            return -1;
        }
        bit_indicator<<=tfi;

        if (d_bit == D_BIT_DOWNLINK ) {
            for (int i=0 ; i<8 ; ++i) {
                if ((timeslot_allocation&0x01)) {
                    downlink_ta_map[i] |= bit_indicator;
                }
            }
        }
        else {
                for (int i=0 ; i<8 ; ++i) {
                if ((timeslot_allocation&0x01)) {
                    uplink_ta_map[i] |= bit_indicator;
                }
            }
        }
        return 1;
    }

    int GprsMsMac::block_partition(Event* ep, int queue_type) {

        ASSERTION(ep,"GprsMsMac blk_partition(): ep is null\n");
        ASSERTION(ep->DataInfo_,"GprsMsMac blk_partition(): ep->DataInfo_ is null\n");

        try {

            bss_message* bss_msg    = reinterpret_cast<bss_message*> (ep->DataInfo_);
            rlc_option* rlc_opt     = reinterpret_cast<rlc_option*>  (bss_msg->rlc_option);
            ASSERTION(rlc_opt,"GprsMsMac blk_partition(): rlc_opt is null\n");

            SList<Event>* squeue = NULL;


            #ifdef  __BLK_PARTITION_DETAILED_DEBUG

            cout << "GprsMsMac blk_partition(): rlc_opt->tfi = " << static_cast<int>(rlc_opt->tfi)
                << " rlc_opt->ctl_msg_type = "  << rlc_opt->ctl_msg_type
                << " rlc_opt->user_data_bsn = " << static_cast<int> (rlc_opt->user_data_bsn)
                << " tn = " << static_cast<long> (this->tn)
                << endl;

            #endif

            if (queue_type == PDTCH_SQUEUE ) {

                if ( rlc_opt->tfi >=0 )
                    squeue = utcb->pdtch_sq[triggered_tn];

                else {
                    cout <<"GprsMsMac blk_partition(): indicated PDTCH_SQUEUE , but tfi is illegal." << endl;
                    return -1;
                }
            }

            else if ( queue_type == PRACH_SQUEUE )
                squeue = prach_squeue;

            else if ( queue_type == PACCH_SQUEUE ) {

                if ( rlc_opt->ctl_msg_type == PKT_DOWNLINK_ACKNACK )
                    squeue = dtcb->pacch_sq[triggered_tn];
                else
                    squeue = utcb->pacch_sq[triggered_tn];
            }

            else {
                cout <<"GprsMsMac blk_partition(): indicated queue_type "<< queue_type << " is not found." << endl;
                return -1;
            }


            /* add mac header and perform channel_encoding */

            UMH mh;
            if ( rlc_opt->ctl_msg_type == DATA ) {
                mh.payload_type         = 0x00;
                mh.datablk_or_ctlblk    = 0x00; /* 0 stands for datablk, 1 stands for ctlblk */
            }
            else {
                mh.payload_type         = 0x01;
                mh.datablk_or_ctlblk    = 0x01; /* 0 stands for datablk, 1 stands for ctlblk */
            }

            mh.countdown_value  = 15;
            mh.si               = rlc_opt->stalled;
            mh.rbit             = 0;

            *bss_msg->user_data = mh.encode();


            #ifdef  __DUMP_ENCODING_DATA
            printf ("MS MAC: Pre encoding data ptr=%ld \n", (bss_msg->user_data) );
            for (int i=0 ; i<33 ; ++i)
                printf(" %d ", (uchar) (bss_msg->user_data[i]));
            printf("\n rlc_opt->CS = %d \n",rlc_opt->cs);
            #endif


            uchar* encoding_data = channel_encoding(rlc_opt->cs,reinterpret_cast<uchar*> (bss_msg->user_data) );

            IS_NULL_STR(encoding_data,"GprsMsMac blk_partition(): encoding failed\n",-1);

            #ifdef  __DUMP_ENCODING_DATA
            printf ("MS MAC: encoding data ptr=%ld \n", encoding_data);
            for (int i=0 ; i<57 ; ++i)
                printf(" %d ", encoding_data[i]);
            printf("\n rlc_opt->CS = %d \n",rlc_opt->cs);
            #endif

            ulong  ts = 0;
            bool   stealing_bit = 0;

            for (int i=0 ; i<4 ; ++i ) {

                Event* eburst_p;
                CREATE_BSS_EVENT(eburst_p);
                bss_message* bss_msg1   = reinterpret_cast<bss_message*> (eburst_p->DataInfo_);
                bss_msg1->flag          = PF_SEND;
                NormalBurst* nb         = new NormalBurst( encoding_data+i*14,ts,stealing_bit,NORMAL_BURST);
                insert_user_data(bss_msg1, nb , sizeof(NormalBurst) );
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

                mac_option *mac_opt     = NULL;

                /* mac_option may already exist in CTL BURST */
                if ( !bss_msg1->mac_option ) {
                    create_mac_option(bss_msg1);
                    mac_opt = reinterpret_cast<mac_option*> (bss_msg1->mac_option);
                }
                else {
                    mac_opt = reinterpret_cast<mac_option*>(bss_msg1->mac_option);
                    bss_msg1->mac_option_len    = sizeof(mac_option);
                }


                if ( i==3) {
                    mac_option* src_macopt = reinterpret_cast<mac_option*> (bss_msg->mac_option);
                    if ( src_macopt ) {
                        if ( src_macopt->msg ) {
                            insert_macopt_msg( mac_opt , src_macopt->msg_type , src_macopt->msg );
                        }
                    }

                }

                mac_opt->burst_num = i; /* other information should be filled when the burst is transmitted */

                if ( !mac_opt->burst_type)
                    mac_opt->burst_type = NORMAL_BURST;

                squeue->insert_tail(eburst_p);
            }
        }
        catch (std::exception& e) {
            cout <<"MsMac block_partition ():"<< e.what() <<endl;
        }
        return 1;
    }

    /* It is not neccessary for ctrmsg_demultiplexer() to reference message_type.
     * This additional parameter distinguishes from the normal decoding mode and
     * the simple decoding mode.
     */

    int GprsMsMac::ctrmsg_demultiplexer (uchar* decoded_data , char message_type ) {

        /* process ctr msg */
        if ( !decoded_data ) {
            IS_NULL_STR(decoded_data,"GprsMsMac ctrmsg_demult(): decoded_data is null\n",-1);
        }

        uchar    distr_or_nondistr;
        uchar*   ctrmsg = NULL;
        DistrMsg*  dmsg = NULL;
        NDistrMsg* nmsg = NULL;

        if ( message_type < 0 ) {
            /* if message_type < 0, it represents the message_type
             * should be obtained by decoding contents of decoded_data.
             */

            printf("MS MAC: Decode Data by contents carried in RLC/DATA block.\n");

            ctrmsg = decoded_data+3;

            if ( ( static_cast<uchar>(*ctrmsg) &0x20) >>5 >0 ) {

                distr_or_nondistr = true;
                dmsg = new DistrMsg;
                dmsg->unpack(ctrmsg);

            }
            else {

                distr_or_nondistr   = false;
                NDistrMsg *nmsg     = new NDistrMsg;
                nmsg->unpack(ctrmsg);

            }

        }
        else {

            ctrmsg = decoded_data;

            if ( ((message_type & 0x20)>>5) >0 ) {

                distr_or_nondistr = true;
                dmsg = reinterpret_cast<DistrMsg*> (ctrmsg);

            }
            else {

                distr_or_nondistr = false;
                nmsg = reinterpret_cast<NDistrMsg*> (ctrmsg);

            }
        }

        if ( distr_or_nondistr ) {

            /* distributed msg */
            uchar msgtype = dmsg->get_msgtype();

            if ( (msgtype == PKT_ACCESS_REJECT ) ) {

                    PAR* par = reinterpret_cast<PAR*> (dmsg->get_distr_msg());

                    if ( utcb ) {

                        utcb->state = RETRY_PHASE;

                        if ( chreq_timer )
                            chreq_timer->cancel();

                        else {
                            chreq_timer = new timerObj;
                            chreq_timer->init();
                            chreq_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),
                                (int (NslObject::*)(Event*)) &GprsMsMac::retry_establish_uplink_tbf );

                        }

                        u_int64_t duration_tick;
                        u_int32_t duration;

                        /* If par->wait_indication_size == 0, the field, wait_indication,
                         * is in units of second. If par->wait_indication_size == 1, wait_indication
                         * is in units of 20 milli-second.
                         *
                         */

                        if (par->wait_indication_size) {

                            duration = par->wait_indication*20; /* 20 ms */
                            MILLI_TO_TICK(duration_tick, duration);
                            //utcb->wait_time_cnt = par->wait_indication*;


                        }

                        else {

                            duration = par->wait_indication; /* sec */
                            SEC_TO_TICK(duration_tick, duration);
                            //utcb->wait_time_cnt = par->wait_indication*;

                        }


                        chreq_timer->start(duration_tick,0);
                        {
                            double dur_time;
                            TICK_TO_SEC(dur_time, duration_tick);
                            printf("MS MAC: Setup Channel REQ. Timer with duration = %g sec.\n", dur_time );
                        }
                    }

                }

            else if ( msgtype == PKT_PAGING_REQUEST ) {

                try {

                    #ifdef __SEQUENCE_DEBUG
                    cout <<"MS MAC: recv packet page request." << endl;
                    #endif

                    /* copy the information carried by a distributed message */
                    PacketPagingRequest* ppr    = new PacketPagingRequest;
                    PacketPagingRequest* ppr_p  = reinterpret_cast<PacketPagingRequest*>(dmsg->get_distr_msg());
                    IS_NULL_STR(ppr_p, "GprsMsMac ctrmsg_demul(): paging request is null\n" ,-1);
                    memcpy(ppr,ppr_p,sizeof(PacketPagingRequest));

                }
                catch (std::exception& e) {
                    cout << "GprsMsMac ctrmsg_demul(): ASSERTION failed:" << e.what() << endl;
                }

            }
            else if ( (msgtype == PKT_PDCH_RELEASE) ) {
                ;
            }
            else if ( (msgtype == PKT_PRACH_PARAM) ) {
                ;
            }
            else if ( (msgtype == PKT_DOWNLINK_DUMMY_CONTROL_BLK) ) {
                ;
            }
            else if ( (msgtype == PSI1) ) {

                Psi1* psi1_p = reinterpret_cast<Psi1*> (dmsg->get_distr_msg());
                ASSERTION(psi1_p, "GprsMsMac ctrmsg_demul(): psi1 message pointer refers to null\n");

                /* Copy contents from distributed message */

                cur_used_bss->pbcch_change_mark         = psi1_p->pbcch_change_mark;
                cur_used_bss->psi1_repeat_period        = psi1_p->psi_repeat_period;
                psi1_p->psi_count_lr      = 0x3;
                psi1_p->psi_count_hr      = 0;
                psi1_p->measurement_order = 0; /* The advanced cell-reselection mode will be supported in the furture*/



                memcpy( &(cur_used_bss->gprs_cell_opt), &(psi1_p->gprs_cell_options) , sizeof(GprsCellOptions));

                memcpy( &(cur_used_bss->prach_ctl_param), &(psi1_p->prach_control_parameters) , sizeof(PrachControlParameters));

                memcpy( &(cur_used_bss->pccch_org_param) , &(psi1_p->pccch_organization_parameters) , sizeof(PccchOrgParam));

            }
            else if ( (msgtype == PSI2) ) {

                Psi2* psi2 = reinterpret_cast<Psi2*> (dmsg->get_distr_msg());
                ASSERTION(psi2, "GprsMsMac ctrmsg_demul(): psi2 is null\n");

                cur_used_bss->psi2_change_mark = psi2->psi2_change_mark;
                memcpy( &(cur_used_bss->cell_id) , &(psi2->cell_identification) , sizeof(CellId) );

            }
            else if ( (msgtype == PSI3) ) {

                Psi3* psi3 = reinterpret_cast<Psi3*> (dmsg->get_distr_msg());
                ASSERTION(psi3, "GprsMsMac ctrmsg_demul(): psi3 is null\n");
                //printf("Node %d MsMac: receive psi message.\n" , get_nid() );

                bcch_list->construct_from_psi3( psi3 );

                if ( (++psi3_info_recv_flag) > 10 )
                    psi3_info_recv_flag = 5;


                /* if roaming procedure is being performed */
                if ( roaming_flag && (psi3_info_recv_flag == 1) ) {
                    /* The value of tlli may be changed as a random TLLI */
                    establish_uplink_tbf(TWOPHASE_ACCESS, cur_used_bss->get_bcch_no(), tlli );
                }

            }
            else if ( (msgtype == PSI3_BIS) ) {
                ;
            }
            else if ( (msgtype == PSI4) ) {
                ;
            }
            else if ( (msgtype == PSI5) ) {
                ;
            }
            else if ( (msgtype == PSI6) ) {
                ;
            }
            else if ( (msgtype == PSI7) ) {
                ;
            }
            else if ( (msgtype == PSI8) ) {
                ;
            }
            else if ( (msgtype == PSI13) ) {
                ;
            }
            else if ( (msgtype == PSI14) ) {
                ;
            }
            else if ( (msgtype == PSI3_TER) ) {
                ;
            }
            else if ( (msgtype == PSI3_QUATER) ) {
                ;
            }
            else if ( (msgtype == PSI15) ) {
                ;
            }
            else if ( (msgtype == PSI16) ) {
                ;
            }
            else {

                cout << "GprsMsMac ctrmsg_demultiplexer(): Unknown message type = "<< msgtype <<
                cout << "  indicated type is "<< static_cast<int> (msgtype) << endl;
                }
        }
        else {
            /* non-distributed msg */
            page_mode = nmsg->get_pgmode();

            /* process address info field */
            uchar addrtype = nmsg->get_addr_info_type();
            GlobalTfi               gtfi_ie;
            ulong                   tlli;
            ushort                  tqi;
            PacketRequestReference  prr_ie;

            if (addrtype == GLOBAL_TFI_INDICATOR) {
                nmsg->get_addr_info(&gtfi_ie);
            }
            else if (addrtype == TLLI_INDICATOR) {
                nmsg->get_addr_info(&tlli);
            }
            else if (addrtype == TQI_INDICATOR) {
                nmsg->get_addr_info(&tqi);
            }
            else if (addrtype == PKT_REQ_REF_INDICATOR) {
                nmsg->get_addr_info(&prr_ie);
            }
            else ;

            /**** process message content ****/

            uchar msgtype = nmsg->get_msgtype();

            if ( (msgtype == PKT_CELL_CHANGE_ORDER ) ) {
                ;
            }
            else if ( (msgtype == PKT_DOWNLINK_ASSIGNMENT ) ) {

                if (!dtcb) {

                    /* Create a new Downlink TBF Control Block (DTCB) to store
                     * the resource assignment of downlink time slots.
                     */

                    dtcb = new TbfDes(RECEIVER,gprs::ms);

                    //#ifdef __SEQUENCE_DEBUG
                    printf( "MS MAC(nid %u): recv packet downlink assignment.\n", get_nid());
                    //#endif

                    PDA* pda = reinterpret_cast<PDA*> (nmsg->get_nondistr_msg());

                    memcpy( persistence_level, pda->persistence_level , 4 );
                    /* operating mode is fixed so far */
                    mac_mode = pda->mac_mode;

                    dtcb->downlink_freq         = pda->freq_param->arfcn;
                    dtcb->uplink_freq           = CORRESPONDING_UPLINK_CH(pda->freq_param->arfcn);
                    dtcb->tfi                   = pda->downlink_tfi_assignment;
                    dtcb->tbf_starting_time     = pda->tbf_starting_time;
                    dtcb->tbf_starting_time_count_down_value = 0;
                    for (int i=0 ; i<8 ;++i) {
                        if ( ((pda->ts_alloc>>i) & 0x01) )
                            dtcb->ts_alloc[i] = true;
                        else
                            dtcb->ts_alloc[i] = false;
                    }
                    state                   = PACKET_TRANSFER_STATE; /* MS itself state */
                    dtcb->state             = PACKET_TRANSFER_STATE;
                    dtcb->tlli              = this->tlli;
                    dtcb->imsi              = 0;
                    /* generate corresponding TCB */

                    if ( dtcb->sbafreq ) {
                        delete dtcb->sbafreq;
                        dtcb->sbafreq   = NULL;
                        dtcb->sr_bit    = RECEIVER;
                    }

                    /* notify the upper layer */
                    ack_rlc(DOWNLINK_TBF_ESTABLISHED,dtcb);

                    #ifdef __SEQUENCE_DEBUG_3
                    printf( "PHONE %d : get the required downlink resources. \n", get_nid()  );
                    #endif

                }
                else if ( dtcb->state == CHANNEL_REQ_PHASE ) {

                    delete dtcb;
                    dtcb = new TbfDes(RECEIVER,gprs::ms);

                    //#ifdef __SEQUENCE_DEBUG
                    printf( "MS MAC(nid %u): recv packet downlink assignment\n.", get_nid());
                    //#endif

                    PDA* pda = reinterpret_cast<PDA*> (nmsg->get_nondistr_msg());

                    memcpy( persistence_level, pda->persistence_level , 4 );
                    /* operating mode is fixed so far */
                    mac_mode = pda->mac_mode;

                    dtcb->downlink_freq         = pda->freq_param->arfcn;
                    dtcb->uplink_freq           = CORRESPONDING_UPLINK_CH(pda->freq_param->arfcn);
                    dtcb->tfi                   = pda->downlink_tfi_assignment;
                    dtcb->tbf_starting_time     = pda->tbf_starting_time;
                    dtcb->tbf_starting_time_count_down_value = 0;
                    for (int i=0 ; i<8 ;++i) {
                        if ( ((pda->ts_alloc>>i) & 0x01) )
                            dtcb->ts_alloc[i] = true;
                        else
                            dtcb->ts_alloc[i] = false;
                    }
                    state                   = PACKET_TRANSFER_STATE; /* MS itself state */
                    dtcb->state             = PACKET_TRANSFER_STATE;
                    dtcb->tlli              = this->tlli;
                    dtcb->imsi              = 0;
                    /* generate corresponding TCB */

                    if ( dtcb->sbafreq ) {
                        delete dtcb->sbafreq;
                        dtcb->sbafreq   = NULL;
                        dtcb->sr_bit    = RECEIVER;
                    }

                    /* notify the upper layer */
                    ack_rlc(DOWNLINK_TBF_ESTABLISHED,dtcb);

                    #ifdef __SEQUENCE_DEBUG_3
                    printf( "PHONE %d : get the required downlink resources. \n", get_nid()  );
                    #endif

                }
                else {

                    shared_obj_dec_refcnt( NDMSG , nmsg );

                    if ( (shared_obj_release( NDMSG , nmsg )) ) {
                        nmsg = NULL;
                    }

                    return 0;
                }

            }
            else if ( (msgtype == PKT_MEASUREMENT_ORDER) ) {
                ;
            }
            else if ( (msgtype == PKT_POLLING_REQUEST) ) {
                ;
            }
            else if ( (msgtype == PKT_POWER_CTL_TIMING_ADVANCE) ) {
                ;
            }
            else if ( (msgtype == PKT_QUEUEING_NOTIFICATION) ) {
                ;
            }
            else if ( (msgtype == PKT_TIMESLOT_RECONFIGURATION) ) {
                ;
            }
            else if ( (msgtype == PKT_TBF_RELEASE) ) {
                ;
            }
            else if ( (msgtype == PKT_UPLINK_ACKNACK) ) {
                /* Assertion: The processing of PKT_UPLINK_ACKNACK is bypassed to RLC layer. */
                cout <<"MS MAC: Assertion failed. PKT_UPLINK_ACKNACK is found here." << endl;
            }
            else if ( (msgtype == PKT_UPLINK_ASSIGNMENT) ) {

                PacketUplinkAssignment* pua = reinterpret_cast<PUA*> (nmsg->get_nondistr_msg());

                TbfDes* cur_tbfdes = utcb;

                if (addrtype == PKT_REQ_REF_INDICATOR ) {

                    if (!cur_tbfdes) {

                        shared_obj_dec_refcnt( NDMSG , nmsg );

                        if ( (shared_obj_release( NDMSG , nmsg )) ) {
                            nmsg = NULL;
                        }

                        return 0;

                    }

                    if ( cur_tbfdes->state != CHANNEL_REQ_PHASE ) {

                        shared_obj_dec_refcnt( NDMSG , nmsg );

                        if ( (shared_obj_release( NDMSG , nmsg )) ) {
                            nmsg = NULL;
                        }

                        return 0;
                    }

                    if ( !cur_tbfdes->stored_req ) {

                        shared_obj_dec_refcnt( NDMSG , nmsg );

                        if ( (shared_obj_release( NDMSG , nmsg )) ) {
                            nmsg = NULL;
                        }

                        return 0;
                    }

                    if ( !memcmp(cur_tbfdes->stored_req, &prr_ie, sizeof(PacketRequestReference)) ) {

                        if (!cur_tbfdes->sbafreq)
                            cur_tbfdes->sbafreq   = new SbaFreq;

                        cur_tbfdes->sbafreq->sba  = new SBA;
                        cur_tbfdes->sbafreq->fp   = new FrequencyParameter;

                        memcpy( cur_tbfdes->sbafreq->sba  , pua->single_block_allocation   , sizeof(SingleBlockAllocation));
                        memcpy( cur_tbfdes->sbafreq->fp   , pua->frequency_params          , sizeof(FrequencyParameter));

                        cur_tbfdes->tbf_starting_time_count_down_value = 0;

                        if (!cur_tbfdes->ta_ie)
                            cur_tbfdes->ta_ie = new PacketTimingAdvance;

                        memcpy( cur_tbfdes->ta_ie , &pua->packet_timing_advance, sizeof(PacketTimingAdvance));

                        if (dtcb) {

                            if (dtcb->state == CHANNEL_REQ_PHASE) {
                                dtcb->state = PAGING_RESPONSING;
                                establish_downlink_tbf(PAGING_RESPONSE,cur_used_bss->get_bcch_no(),tlli);
                            }

                        }

                        if (cur_tbfdes) {

                            if (cur_tbfdes->state == CHANNEL_REQ_PHASE) {
                                #ifdef   __SEQUENCE_DEBUG
                                    cout <<"GprsMsMac: receive SBA from BTS." << endl;
                                #endif
                                cur_tbfdes->state = RESOURCE_REQ_PHASE;
                                establish_uplink_tbf(RESOURCE_REQ_PHASE,cur_used_bss->get_bcch_no(),tlli);
                                cur_used_bss->set_nid(incoming_event_nid);
                            }
                        }

                        if ( chreq_timer ){
                            chreq_timer->cancel();
                            delete chreq_timer;
                            chreq_timer = NULL;
                            printf("MS MAC: Cancel Channel REQ. Timer.\n");
                        }

                    }
                    else {

                        shared_obj_dec_refcnt( NDMSG , nmsg );

                        if ( (shared_obj_release( NDMSG , nmsg )) ) {
                            nmsg = NULL;
                        }

                        return 0;

                    }


                }
                else if (addrtype == GLOBAL_TFI_INDICATOR ) {

                    if ( cur_tbfdes->state != RESOURCE_REQ_PHASE ) {

                        shared_obj_dec_refcnt( NDMSG , nmsg );

                        if ( (shared_obj_release( NDMSG , nmsg )) ) {
                            nmsg = NULL;
                        }

                        return 0;
                    }

                    if ( gtfi_ie.direction == 0 ) { /* uplink */

                        IS_NULL_STR(cur_tbfdes,"GprsMsMac::ctrmsg_demul(): cur_tbfdes doesn't exist\n" ,-1);
                        cur_tbfdes->tfi = gtfi_ie.tfi;
                        PacketUplinkAssignment* pua = reinterpret_cast<PUA*> (nmsg->get_nondistr_msg());
                        memcpy( persistence_level, pua->persistence_level , 4 );

                        if ( true )
                            cur_tbfdes->ch_coding_scheme = pua->channel_coding_command;
                        else
                            cur_tbfdes->ch_coding_scheme = 1;

                        if ( !cur_tbfdes->ta_ie )
                            cur_tbfdes->ta_ie = new PacketTimingAdvance;

                        memcpy(cur_tbfdes->ta_ie, &pua->packet_timing_advance , sizeof(PacketTimingAdvance) );



                        if ( pua->sba_or_da == 1) { /* 0 stands for sba , 1 stands for da */

                            cur_tbfdes->uplink_freq       = pua->frequency_params->arfcn;
                            cur_tbfdes->downlink_freq     = CORRESPONDING_DOWNLINK_CH(pua->frequency_params->arfcn);
                            cur_tbfdes->tfi               = pua->dynamic_allocation->uplink_tfi_assignment;
                            cur_tbfdes->tbf_starting_time = pua->dynamic_allocation->tbf_starting_time;
                            cur_tbfdes->tbf_starting_time_count_down_value = 0;

                            memcpy( cur_tbfdes->usf_tn , pua->dynamic_allocation->usf_tn , 8);

                            for ( int i=0 ; i<8 ;++i ) {

                                if ( cur_tbfdes->usf_tn[i] < gprs::free )
                                    cur_tbfdes->ts_alloc[i] = true;
                                else
                                    cur_tbfdes->ts_alloc[i] = false;

                            }

                            state                   = PACKET_TRANSFER_STATE; /* MS itself state */
                            cur_tbfdes->state       = PACKET_TRANSFER_STATE;

                        }
                        else {

                           /* some mechanisms may use sba. Ex.
                            * BTS only receives Pkt Resource Request partially corrected.
                            * So far, ignore this condition. Unsuccessful resource request
                            * will lead to a waiting-timer timeout and then re-performs channel
                            * request procedure.
                            */

                            cout<<"MsMac ctlmsg_demultiplexer(): Single Block Allocation is not allowed here"<<endl;

                            return -1;

                        }

                        if ( cur_tbfdes->sbafreq ) {

                            delete cur_tbfdes->sbafreq;
                            cur_tbfdes->sbafreq       = NULL;
                            cur_tbfdes->sr_bit        = SENDER;
                            cur_tbfdes->usf_granted_ind->clear();

                        }

                        #ifdef   __SEQUENCE_DEBUG
                            cout <<"GprsMsMac: receive PktUplinkAssignment from BTS." << endl;
                            cout << "cur_tbfdes->tfi=" << (int) (cur_tbfdes->tfi)
                                    << " cur_tbfdes->tlli = " << cur_tbfdes->tlli << endl;
                            cout << "cur_tbfdes->ts_alloc ";
                            for (int i=0 ; i<8 ;++i)
                                cout << static_cast<int> (cur_tbfdes->ts_alloc[i]) << " ";

                            cout << endl << "cur_tbfdes->usf_tn ";
                            for (int i=0 ; i<8 ;++i)
                                cout << static_cast<int> (cur_tbfdes->usf_tn[i]) << " ";
                            cout << " ufreq = " << cur_tbfdes->uplink_freq << endl;
                        #endif

                        cur_used_bss->set_nid(incoming_event_nid);

                        if ( roaming_flag ) {

                            ack_rlc( ROAMING_COMPLETED , cur_tbfdes );
                            roaming_flag    = false;
                            printf("\nMS MAC: roaming is completed.\n");
                        }
                        else
                            ack_rlc(UPLINK_TBF_ESTABLISHED, cur_tbfdes );

                        if ( pua_timer )
                            pua_timer->cancel();

                        #ifdef __SEQUENCE_DEBUG_3
                        printf( "PHONE %d : get the required uplink resources. \n", get_nid()  );
                        #endif

                    }
                    else { /* downlink */
                        ;/* downlink assignment is carried by PACKET DOWNLINK ASSIGNMENT message */
                    }
                }

                else {
                    cout  <<
                        "MsMac ctlmsg_demultiplexer(): undefined addressing mode "<< static_cast<int> (addrtype) <<
                            " for Uplink TBF" << endl;
                    return -1;
                }
            }
            else if ( (msgtype == PKT_CELL_CHANGE_CONTINUE) ) {
                ;
            }
            else if ( (msgtype == PKT_NEIGHBOR_CELL_DATA) ) {
                ;
            }
            else if ( (msgtype == PKT_SERVING_CELL_DATA) ) {
                ;
            }
            else if ( (msgtype == PKT_DBPSCH_ASSIGNMENT) ) {
                ;
            }
            else if ( (msgtype == MULTIPLE_TBF_DOWNLINK_ASSIGNMENT) ) {
                ;
            }
            else if ( (msgtype == MULTIPLE_TBF_UPLINK_ASSIGNMENT) ) {
                ;
            }
            else if ( (msgtype == MULTIPLE_TBF_TIMESLOT_RECONFIG) ) {
                ;
            }
            else if ( (msgtype == PKT_CELL_CHANGE_FAILURE) ) {
                ;
            }
            else if ( (msgtype == PKT_CONTROL_ACK) ) {
                ;
            }
            else if ( (msgtype == PKT_DOWNLINK_ACKNACK) ) {
                ;
            }
            else if ( (msgtype == PKT_UPLINK_DUMMY_CONTROL_BLK) ) {
                ;
            }
            else if ( (msgtype == PKT_MEASUREMENT_REPORT) ) {
                ;
            }
            else if ( (msgtype == PKT_ENHANCED_MEASUREMENT_REPORT) ) {
                ;
            }
            else if ( (msgtype == PKT_RESOURCE_REQUEST) ) {
                ;
            }
            else if ( (msgtype == PKT_MOBILE_TBF_STATUS) ) {
                ;
            }
            else if ( (msgtype == PKT_PSI_STATUS) ) {
                ;
            }
            else if ( (msgtype == EGPRS_PKT_DOWNLINK_ACKNACK) ) {
            ;
            }
            else if ( (msgtype == PKT_PAUSE) ) {
                ;
            }
            else if ( (msgtype == ADDITIONAL_MS_RA_CAPABILITIES) ) {
                ;
            }
            else if ( (msgtype == PKT_CELL_CHANGE_NOTIFICATION) ) {
                ;
            }
            else if ( (msgtype == PKT_SI_STATUS) ) {
                ;
            }
            else
                ;
        }

        /* Do not invoke the destructors of Distributed Messages
         * and Nondistributed Messages directly.
         */


        if ( message_type < 0 ) {

            delete ctrmsg;

        }

        if (dmsg) {

            shared_obj_dec_refcnt( DMSG , dmsg );

            if ( (shared_obj_release( DMSG , dmsg )) ) {
                dmsg = NULL;
            }

        }

        if (nmsg) {

            shared_obj_dec_refcnt( NDMSG , nmsg );

            if ( (shared_obj_release( NDMSG , nmsg )) ) {
                nmsg = NULL;
            }

        }

        return 1;
    }

    int GprsMsMac::establish_uplink_tbf(uchar cause, long chid, ulong tlli) {

        /* While a MS tries to attach to a GPRS network, module GprsMsMac automatically
         * enters PACKET_TRANSFER_STATE.
         */

        state = PACKET_TRANSFER_STATE;

        /* Create an Uplink TBF Control Block in MAC module. Some information in this control
         * block should be consistent with that in TBF Control Block in RLC module.
         * A TBF Control Block in MAC module also manages the time slot scheduling and burst
         * transmission, while a TBF Control Block in RLC layer only takes care of RLC blocks
         * and SACK algorithm.
         */

        if (!utcb) {

            #ifdef   __SEQUENCE_DEBUG

            cout <<"GprsMsMac::establish_uplink_tbf(): create a new UTbfDes for tlli= " << tlli << endl;

            #endif

            utcb            = new TbfDes(SENDER,gprs::ms);
            utcb->state     = CHANNEL_REQ_PHASE;
            utcb->nodetype  = gprs::ms;  /* MS */
            utcb->sr_bit    = SENDER;    /* sender */
            utcb->tlli      = tlli;
            ++utcb->ch_req_cnt;

        }

        if (utcb->state == RETRY_PHASE ) {

            utcb->state         = CHANNEL_REQ_PHASE;
            ++utcb->ch_req_cnt;

            if ( utcb->ch_req_cnt >= MAX_CHANNEL_REQ_TIMES ) {
                printf("MS MAC: The number of channel requests reaches the maximum allowed times. \n\n\n");
                return 0;
            }
        }

        if (utcb->state == CHANNEL_REQ_PHASE ) {

            random_timer = new timerObj;
            random_timer->init();
            random_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),
                (int (NslObject::*)(Event*)) &GprsMsMac::randomized_utcb_channel_request );

            u_int64_t duration_tick;
            ulong ran_value = random()%100; /* in units of 5 ms */

            printf("[%u] MS MAC: Start randomized timer with %lu ms.\n", get_nid(), ran_value*5 );
            MILLI_TO_TICK(duration_tick, ran_value*5 );
            random_timer->start( duration_tick, 0 );

        }
        else if (utcb->state == RESOURCE_REQ_PHASE )
            resource_request(utcb,chid);
        else {
            cout<<
                "GprsMsMac::establish_uplink_tbf(): Unknown utcb state or multiple channel requests within the waiting interval."
                    << endl;
        }


        return 1;
    }

    int GprsMsMac::retry_establish_uplink_tbf() {

        utcb->state = RETRY_PHASE;
        return establish_uplink_tbf(TWOPHASE_ACCESS, cur_used_bss->get_bcch_no(), tlli);

    }

    int GprsMsMac::retry_resource_request() {

        utcb->state = RESOURCE_REQ_PHASE;
        return establish_uplink_tbf(TWOPHASE_ACCESS, cur_used_bss->get_bcch_no(), tlli);

    }

    int GprsMsMac::establish_downlink_tbf(uchar cause, long chid, ulong tlli) {

        state = PACKET_TRANSFER_STATE;

        if (!dtcb) {
            dtcb            = new TbfDes( 2 , gprs::ms );
            dtcb->state     = CHANNEL_REQ_FOR_PAGE_RES;
            dtcb->nodetype  = gprs::bts; /* BTS */
            dtcb->sr_bit    = RECEIVER; /* sender */
            dtcb->tlli      = tlli;
            ++dtcb->ch_req_cnt;
        }

        if (dtcb->state == CHANNEL_REQ_FOR_PAGE_RES ) {

            random_timer = new timerObj;
            random_timer->init();
            random_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),
                (int (NslObject::*)(Event*)) &GprsMsMac::randomized_dtcb_channel_request );

            u_int64_t duration_tick;
            ulong ran_value = random()%100; /* in units of 5 ms */
            MILLI_TO_TICK(duration_tick, ran_value*5 );
            random_timer->start( duration_tick, 0 );

        }

        else if (dtcb->state == PAGING_RESPONSING )
            ack_rlc(PAGING_RESPONSING,dtcb);
        else if (dtcb->state == PAGING_RESPONSED )
            ;
        else
            cout<<"GprsMsMac establish_downlink_tbf(): unknown dtcb state." << endl;
        return 1;
    }


    int GprsMsMac::randomized_utcb_channel_request() {

        if ( random_timer ) {

            random_timer->cancel();
            delete random_timer;
            random_timer = NULL;

        }

        return channel_request(utcb, cur_used_bss->get_bcch_no(), TWOPHASE_ACCESS );

    }

    int GprsMsMac::randomized_dtcb_channel_request() {

        if ( random_timer ) {

            random_timer->cancel();
            delete random_timer;
            random_timer = NULL;

        }

        return channel_request(dtcb, cur_used_bss->get_bcch_no(), PAGING_RESPONSE );

    }

    int GprsMsMac::channel_request(TbfDes* utcb, long chid , ulong cause ) {


        if ( !utcb ) {

            if ( this->utcb )
                utcb = this->utcb;

            else {
                IS_NULL_STR(utcb, "GprsMsMac channel_req(): utcb is null\n" , -1);
            }

        }

        try {
            /* create a packet channel request */
            PktChReq* chreq     = new PktChReq;
            chreq->length_type  = ELEVEN_BITS;

            if ( cause == TWOPHASE_ACCESS ) {

                chreq->format_type      = cause;
                chreq->priority     = 0x00;
                chreq->random_bits      = (random())%8;

            }
            else if ( cause == PAGING_RESPONSE ) {

                chreq->format_type      = cause;
                chreq->random_bits      = (random())%8;

            }
            else {

                delete chreq;
                chreq = NULL;
                return -1;

            }

            utcb->stored_req            = new PacketRequestReference;
            utcb->stored_req->ra_info   = chreq->pack();
            //utcb->stored_req->fn        = fn; update in send_timestamp
            delete chreq;
            chreq = NULL;

            AB *ab = new AB( utcb->stored_req->ra_info, 0x01 , 0x03 );

            Event* e_ab;
            CREATE_BSS_EVENT(e_ab);
            bss_message* bssmsg     = reinterpret_cast<bss_message*>(e_ab->DataInfo_);
            insert_user_data(bssmsg,ab,sizeof(AB));
            create_mac_option(bssmsg);
            mac_option* mac_opt     = reinterpret_cast<mac_option*> (bssmsg->mac_option);

            /* Access burst need not form a RLC block. */
            mac_opt->burst_num      = 0 ;  /* ranged from 0 to 3 */
            mac_opt->burst_type     = ACCESS_BURST;
            mac_opt->channel_id     = chid;
            prach_squeue->insert_tail(e_ab);

            #ifdef   __SEQUENCE_DEBUG
            cout <<"GprsMsMac::channel_request(): insert channel_req into prach_squeue." << endl;
            #endif

        }
        catch(std::exception& e) {
            cout << "GprsMsMac channel_req(): ASSERTION failed: " << e.what() << endl;
            return -1;
        }
        return 1;
    }

    int GprsMsMac::resource_request(TbfDes* tbfdes,long chid) {

        ASSERTION( tbfdes , "GprsMsMac::resource_request(): tbfdes is null.\n");

        /* Create Packet Resource Request message */
        NDistrMsg* nmsg_pkt_rr = new NDistrMsg;

        nmsg_pkt_rr->set_pgmode(page_mode);
        nmsg_pkt_rr->set_addr_info(TLLI_INDICATOR,this->tlli);

        PacketResourceRequest* pkt_rr = new PacketResourceRequest;
        nmsg_pkt_rr->set_nondistr_msg(PKT_RESOURCE_REQUEST, uplink , pkt_rr);

        pkt_rr->access_type = 0x00;

        /* fill in channel request description */
        pkt_rr->ch_req_des.peak_throughput_class    = 1; /* omit the value in this field thus far */
        pkt_rr->ch_req_des.radio_priority           = 1; /* omit the value in this field thus far */
        pkt_rr->ch_req_des.rlc_mode                 = 0; /* acknowledge mode */
        pkt_rr->ch_req_des.llc_pdu_type             = 1; /* not SACK or ACK protected */
        pkt_rr->ch_req_des.rlc_octet_count          = 0; /* only implement open-ended TBF */
        pkt_rr->change_mark                         = cur_used_bss->psi2_change_mark;
        pkt_rr->c_value                             = 0; /* not supported */

        pkt_rr->access_type_ind                     = 1 ;
        pkt_rr->ms_ra_ca2_ie_ind                    = 0 ;
        pkt_rr->change_mark_ind                     = 1 ;
        pkt_rr->sign_var_ind                        = 0 ;

        bzero( pkt_rr->i_level_tn_ind, 8 );

        UMH* mh = new UMH;
        mh->payload_type = 0x01;
        mh->rbit = 0;

        {
            Event* eblk;
            CREATE_BSS_EVENT(eblk);
            bss_message* bss_msg            = reinterpret_cast<bss_message*>(eblk->DataInfo_);
            uchar* dummy_data = dummy_data_array;
            bzero(dummy_data, sizeof(dummy_data_array) );

            insert_user_data(bss_msg,dummy_data, sizeof(dummy_data_array) );
            insert_mac_header(bss_msg,mh,sizeof(UMH));

            create_rlc_option(bss_msg);
            rlc_option* rlc_opt         = reinterpret_cast<rlc_option*> (bss_msg->rlc_option);
            rlc_opt->ctl_msg_type       = PKT_RESOURCE_REQUEST;
            create_mac_option(bss_msg);
            mac_option* mac_opt         = reinterpret_cast<mac_option*> (bss_msg->mac_option);
            mac_opt->burst_type         = CTL_BURST;

            insert_macopt_msg( mac_opt , NDMSG , nmsg_pkt_rr );
            mac_opt->channel_id         = chid;

            tbfdes->pacch_pending_event_sq->insert_tail(eblk);

            #ifdef   __SEQUENCE_DEBUG
            cout <<"GprsMsMac::resource_request(): partition resource_request." << endl;
            #endif
        }
        return 1;
    }

    int GprsMsMac::send_timestamp( Event* ep, long chid ) {

        IS_NULL_STR(ep,"GprsBtsMac send_timestamp(): Assertion failed ep is null\n",-1);

        /* Check if the range of chid is legal. Channel 0 is guard channel and can not be used for transmission. */
        if ( chid<=0 || chid>=125 ) {
            cout <<"GprsMsMac send_timestamp(): chid " << chid << " is illegal." << endl;
            return -1;
        }

        try {

            bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
            IS_NULL_STR(bssmsg,"GprsMsMac send_timestamp(): Assertion failed bssmsg is null\n",-1);
            bssmsg->flag = PF_SEND;

            if ( !bssmsg->mac_option ) {
                create_mac_option(bssmsg);
                reinterpret_cast<mac_option*> (bssmsg->mac_option)->burst_type  = NORMAL_BURST; /* default settings */

            }

            mac_option* mac_opt     = reinterpret_cast<mac_option*> (bssmsg->mac_option);
            mac_opt->fn             = fn;
            mac_opt->tn             = tn;
            mac_opt->bn             = bn;
            mac_opt->channel_id     = chid;
            mac_opt->nid                = get_nid();

            /*MAC log*/
            if ( log_flag && !strcasecmp(GPRSLogFlag, "on") ) {

                struct logEvent         *logep;
                uchar burst_len;
                char  burst_type;

                struct gprs_log *log1;
                u_int64_t       delay;
                log1 = (struct gprs_log*)malloc
                                        (sizeof(struct gprs_log));
                MILLI_TO_TICK(delay,0.577);

                if(mac_opt->burst_type == NORMAL_BURST
                    || mac_opt->burst_type == DUMMY_BURST || mac_opt->burst_type == CTL_BURST)
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
                    ,get_nid(),StartTX,get_nid(),cur_used_bss->get_nid(),
                        mac_opt->burst_num,burst_type,burst_len,mac_opt->channel_id);

                INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

            }

            rlc_option*  rlc_opt    = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

            #ifdef  __SEQUENCE_DEBUG
            {


                printf("GprsMsMac send_timestamp(): (nid %ld)sends a burst with profile as follows: tick=%lld\n",
                    get_nid() , GetCurrentTime() );

                if (rlc_opt) {
                    printf("rlc_option: tfi %d ,ctl_msg_type %d ,cmd %d ,chid %ld \n",
                        rlc_opt->tfi,rlc_opt->ctl_msg_type,rlc_opt->cmd,rlc_opt->chid);
                }

                printf("mac_option: burst_type %d, burst_num %d, chid %ld, fn %ld, tn %ld, bn %ld \n\n",
                    mac_opt->burst_type,mac_opt->burst_num,mac_opt->channel_id,mac_opt->fn,mac_opt->tn,mac_opt->bn);
            }
            #endif

            if (mac_opt->burst_type == ACCESS_BURST) {
                utcb->stored_req->fn = uplink_fn;

                //#ifdef   __SEQUENCE_DEBUG
                printf( "[%u] MS MAC::send_timestamp(): send access_burst at tick %lld. \n" , get_nid() , GetCurrentTime() );
                //#endif
            }


            #ifdef  __EVENT_DUP_DEBUG

            printf("MS MAC: send event addr = %lx \n", ep );

            #endif

            if ( rlc_opt ) {

                if ( (rlc_opt->ctl_msg_type == PKT_RESOURCE_REQUEST) && (mac_opt->burst_type == CTL_BURST) &&
                     (mac_opt->burst_num == 3) ) {

                    if ( !pua_timer ) {

                        pua_timer = new timerObj;
                        pua_timer->init();
                        pua_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),
                            (int (NslObject::*)(Event*)) &GprsMsMac::retry_resource_request );

                    }

                    pua_timer->cancel();
                    u_int64_t duration_tick;
                    u_int32_t duration_sec = 1;
                    SEC_TO_TICK(duration_tick, duration_sec);
                    pua_timer->start(duration_tick,0);
                    printf("[%u] MS MAC: Setup Packet Resource REQ. Timer with duration = %u sec.\n", get_nid() ,duration_sec);


                }

            }
            return NslObject::send(ep);
        }

        catch (std::exception& e) {
            cout << "GprsBtsMac send_timestamp(): Assertion failed cause:"<< e.what() << endl;
            return -1;
        }
    }

    int GprsMsMac::ack_rlc(ulong command , void* msg ) {

        IS_NULL_STR(msg, "GprsMsMac ack_rlc(): Assertion failed: cause: field msg is null\n" , -1);

        Event* ep;
        CREATE_BSS_EVENT(ep);

        bss_message* bssmsg     = reinterpret_cast<bss_message*> (ep->DataInfo_);
        bssmsg->flag            = PF_RECV;
        bssmsg->imc_flag        = true;

        create_rlc_option(bssmsg);
        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

        if ( command == UPLINK_TBF_ESTABLISHED || command == DOWNLINK_TBF_ESTABLISHED ||
                command == ROAMING_COMPLETED ) {

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

            cout << "GprsMsMac ack_rlc(): Undefined command "<< command << endl;
            FREE_BSS_EVENT(ep);
            exit(1);

        }
        return NslObject::recv(ep);
    }

    int GprsMsMac::is_idle_frame_for_ta_update() {

        if ( (tn == 7 && fn == 11) || ( tn==7 && fn==37) )
            return true;
        else
            return false;
    }

    int GprsMsMac::is_idle_frame_for_signal_measurement() {

        if ( (tn == 7 && fn == 24) ||
             (tn == 0 && fn == 25) ||
             (tn == 1 && fn == 25) ||
             (tn == 2 && fn == 25) ||
             (tn == 3 && fn == 25) ||
             (tn == 4 && fn == 25) ||
             (tn == 5 && fn == 25) ||
             (tn == 6 && fn == 25) ||
             (tn == 7 && fn == 50) ||
             (tn == 0 && fn == 51) ||
             (tn == 1 && fn == 51) ||
             (tn == 2 && fn == 51) ||
             (tn == 3 && fn == 51) ||
             (tn == 4 && fn == 51) ||
             (tn == 5 && fn == 51) ||
             (tn == 6 && fn == 51)     )
            return true;

        else
            return false;


    }

    int GprsMsMac::configure_listened_channels() {

        /* Configure channels to be monitored for the next timeslot */

        clear_listening_channel();

        if ( state == POWER_ON_NOT_SYNC ) {

            mark_listening_channel((cur_used_bss->get_bcch_no()));
            set_listening_channel();
            cur_ch_no = (cur_used_bss->get_bcch_no());
            return 1;

        }
        else if ( state == PACKET_IDLE_STATE ) {

            /* If the next frame is idle frames for TA update. */

            if ( (is_idle_frame_for_ta_update()) ) {

                mark_listening_channel((cur_used_bss->get_bcch_no()));
                set_listening_channel();
                cur_ch_no = (cur_used_bss->get_bcch_no());

            }
            /* If the next frame is idle frames for signal measurements */
            else if ( (is_idle_frame_for_signal_measurement()) ) {

                BcchInfo* tmp = get_cur_monitored_neighbor_cell();
                if ( !tmp ) {

                    mark_listening_channel((cur_used_bss->get_bcch_no()));
                    set_listening_channel();
                    cur_ch_no = (cur_used_bss->get_bcch_no());

                }
                else {

                    uchar ncell_bcchno = tmp->get_bcch_no();

                    if ( ncell_bcchno >=250 ) {

                        printf("GprsMsMac::configure_listening_channel(): The indicated BCCH of neighbor cell %d is out of range",
                            ncell_bcchno );

                        exit(1);
                    }
                    else {
                        mark_listening_channel(ncell_bcchno);
                        set_listening_channel();
                        cur_ch_no = ncell_bcchno;

                    }

                }

            }
            /* Otherwise, listen the bcch of the current cell */
            else {

                mark_listening_channel((cur_used_bss->get_bcch_no()));
                set_listening_channel();
                cur_ch_no = (cur_used_bss->get_bcch_no());

            }
            return 1; /* in the future, measurement report mechanism shall be considered */

        }

        else if ( state == PACKET_TRANSFER_STATE ) {

            /* The determination for listening channels */
            uchar ch_determined = false;


            if ( utcb ) {

                /* Thus far, the timeslot allocation for MS's uplink TBF and downlink TBF should
                 * be orthogonal to each other. If a conflict is allowed, the priority of uplink TBF
                 * should be higher.
                 */


                if ( utcb->state == PACKET_TRANSFER_STATE ) {

                    if ( utcb->usf_tn[(tn+1)%8] < 7 ) {
                        /* USF = 0x111 stands for PKT random access channel */
                        mark_listening_channel(CORRESPONDING_DOWNLINK_CH(utcb->uplink_freq));

                        #ifdef __CH_DETERMINE_DEBUG
                            printf("MS MAC(nid %ld): freq = %d is monitored on tn[%ld] bn = %ld . at tick= %llu \n",
                                get_nid(), CORRESPONDING_DOWNLINK_CH(utcb->uplink_freq) , tn , bn , GetCurrentTime());
                        #endif

                        cur_ch_no = CORRESPONDING_DOWNLINK_CH(utcb->uplink_freq);
                        ch_determined = true;
                    }

                }

                else if ( utcb->state == RESOURCE_REQ_PHASE && utcb->sbafreq ) {

                    if ( utcb->sbafreq->sba->tn == ((tn+1)%8) ) {
                        mark_listening_channel(CORRESPONDING_DOWNLINK_CH(utcb->sbafreq->fp->arfcn));

                        #ifdef __CH_DETERMINE_DEBUG
                            printf("MS MAC(nid %ld): SBA freq = %d is monitored on tn[%ld] bn = %ld. at tick= %llu \n",
                                get_nid(), CORRESPONDING_DOWNLINK_CH(utcb->sbafreq->fp->arfcn) , tn , bn , GetCurrentTime() );
                        #endif


                        cur_ch_no = CORRESPONDING_DOWNLINK_CH(utcb->sbafreq->fp->arfcn);
                        ch_determined = true;
                    }
                }

                else
                    ;

            }

            if ( dtcb && (!ch_determined) ) {

                if ( dtcb->state == PACKET_TRANSFER_STATE ) {

                    if ( dtcb->sbafreq ) {

                        if ( dtcb->sbafreq->sba->tn == ((tn+1)%8) ) {
                            mark_listening_channel(dtcb->sbafreq->fp->arfcn);
                            cur_ch_no = dtcb->sbafreq->fp->arfcn;
                            ch_determined = true;
                        }

                    }
                    else if ( dtcb->ts_alloc[(tn+1)%8] ) {

                        mark_listening_channel(dtcb->downlink_freq);
                        cur_ch_no = dtcb->downlink_freq;
                        ch_determined = true;

                    }
                    else
                        ;
                }
            }

            if ( !ch_determined) {


                if ( (is_idle_frame_for_signal_measurement()) ) {

                    BcchInfo* tmp = get_cur_monitored_neighbor_cell();
                    if ( !tmp ) {

                        mark_listening_channel((cur_used_bss->get_bcch_no()));
                        cur_ch_no = (cur_used_bss->get_bcch_no());
                        ch_determined = true;

                    }
                    else {

                        uchar ncell_bcchno = tmp->get_bcch_no();

                        if ( ncell_bcchno >=250 ) {

                            printf("MSMAC configure_listening_channel(): The indicated BCCH of neighbor cell %d is out of range",
                                ncell_bcchno );

                            exit(1);
                        }
                        else {
                            mark_listening_channel(ncell_bcchno);
                            cur_ch_no = ncell_bcchno;
                            ch_determined = true;

                        }

                    }

                }

            }

            if (!ch_determined) {

                mark_listening_channel((cur_used_bss->get_bcch_no()));

                #ifdef __CH_DETERMINE_DEBUG
                    printf("MS MAC(nid %ld): bcch is monitored on tn[%ld] bn = %ld . at tick= %llu \n",
                        get_nid(), tn, bn , GetCurrentTime() );
                    printf("MS MAC(nid %ld): bcch is %d \n" , get_nid() , (cur_used_bss->get_bcch_no()));
                #endif


                cur_ch_no = (cur_used_bss->get_bcch_no());
            }

            set_listening_channel();
            return 1;
        }

        return 1;
    }

    int GprsMsMac::set_trigger_sending_indicator() {

        if ( utcb ) {

            if ( (!utcb->ta_ie) || (!utcb->ta_ie->timing_advance_value) ) {

                if ( (test_is_blkn_boundary(uplink_fn)) )
                    trigger_sending_ind[uplink_tn] = true;
                else
                    trigger_sending_ind[uplink_tn] = false;

            }
            else { /* timing advancing should be performed */
                long next_uplink_tn = uplink_tn + 1;
                long next_uplink_fn = uplink_fn;
                if (next_uplink_tn >7) {
		    long tmp = ++next_uplink_fn;
                    next_uplink_tn = 0;
                    next_uplink_fn = tmp % 52;
                }

                if ( (test_is_blkn_boundary(next_uplink_fn)) )
                    trigger_sending_ind[next_uplink_tn] = true;
                else
                    trigger_sending_ind[next_uplink_tn] = false;
            }
        }
        else if ( dtcb ) {

            if ( (!dtcb->ta_ie) || (!dtcb->ta_ie->timing_advance_value) ) {

                if ( (test_is_blkn_boundary(uplink_fn)) )
                    trigger_sending_ind[uplink_tn] = true;
                else
                    trigger_sending_ind[uplink_tn] = false;

            }
            else { /* timing advancing should be performed */

                long next_uplink_tn = uplink_tn + 1;
                long next_uplink_fn = uplink_fn;
                if (next_uplink_tn >7) {
		    long tmp = ++next_uplink_fn;
                    next_uplink_tn = 0;
                    next_uplink_fn = tmp % 52;
                }

                if ( (test_is_blkn_boundary(next_uplink_fn)) )
                    trigger_sending_ind[next_uplink_tn] = true;
                else
                    trigger_sending_ind[next_uplink_tn] = false;
            }

        }

        else
            trigger_sending_ind[uplink_tn] = false;

        return 1;

    }

    int GprsMsMac::create_ctl_ack(uchar ctlack_value) {

        IS_NULL_STR(dtcb, "GprsMsMac::create_ctl_ack(): dtcb is NULL.\n" ,-1);
        IS_NULL_STR(utcb, "GprsMsMac::create_ctl_ack(): utcb is NULL.\n" ,-1);

        if ( dtcb->tlli != utcb->tlli ) {
            cout << "GprsMsMac::create_ctl_ack() Assertion failed: utbf and dtbf have different TLLIs " << endl;
            exit(1);
        }

        PktCtlAck* pca  = new PktCtlAck;
        pca->tlli       = dtcb->tlli;
        pca->ctlack     = 0x03;

        NDistrMsg* nmsg = new NDistrMsg();
        nmsg->set_pgmode(0x01);
        /* set tfi */
        GlobalTfi tfi_ie;
        tfi_ie.direction    = 0;
        tfi_ie.tfi          = utcb->tfi;
        nmsg->set_addr_info(GLOBAL_TFI_INDICATOR, tfi_ie);
        nmsg->set_nondistr_msg(PKT_CONTROL_ACK, uplink, pca);

        Event* eburst;
        CREATE_BSS_EVENT(eburst);
        bss_message* bmsg       = reinterpret_cast<bss_message*> (eburst->DataInfo_);
        bmsg->flag              = PF_SEND;


        uchar* dummy_data = dummy_data_array;
        bzero(dummy_data,1);
        insert_user_data(bmsg,dummy_data,1);


        create_rlc_option(bmsg);
        rlc_option* rlc_opt     = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
        rlc_opt->ctl_msg_type   = PKT_CONTROL_ACK;
        rlc_opt->tfi            = utcb->tfi;
        rlc_opt->chid           = utcb->uplink_freq;

        create_mac_option(bmsg);
        mac_option* mac_opt     = reinterpret_cast<mac_option*> (bmsg->mac_option);

        insert_macopt_msg( mac_opt , NDMSG , nmsg );
        mac_opt->burst_type     = CTL_BURST;

        if (rlc_opt->chid >250 ) {
            cout << "GprsMsMac::create_ctl_ack(): rlc_opt->chid is out of range." << endl;
            exit(1);
        }
        else
            mac_opt->channel_id = rlc_opt->chid;

        cout <<"MS MAC: enqueue a PKT CONTROL ACK." << endl;

        /*buggy if we don't take the block boundary into account */
        block_partition(eburst,PACCH_SQUEUE);

        return 1;
    }

    /******************************************************************/
    BcchInfo::BcchInfo() {
        bzero(this,sizeof(BcchInfo));
        rssi                = -10000.0;
        start_ch            = -1;
        end_ch              = -1;
        pbcch_tn            = -1; /* un-synchronized state */
        bcch_change_mark    = -1;
        pbcch_change_mark   = -1;
        psi2_change_mark    = -1;
        psi3_change_mark    = -1;
    }

    int BcchInfo::clear() {
        bzero(this,sizeof(BcchInfo));
        rssi                = -10000.0;
        start_ch            = -1;
        end_ch              = -1;
        pbcch_tn            = -1; /* un-synchronized state */
        bcch_change_mark    = -1;
        pbcch_change_mark   = -1;
        psi2_change_mark    = -1;
        psi3_change_mark    = -1;

        return 1;
    }

    int BcchInfo::copy_from_dbssdes( DBssDes* dbssdes ) {

        nid         = dbssdes->nid;
        rai         = dbssdes->rai;
        bsic        = dbssdes->bsic;
        tsc         = dbssdes->tsc;
        bcch_no     = dbssdes->bcch_no;
        start_ch    = dbssdes->start_ch;
        end_ch      = dbssdes->end_ch;

        return 1;
    }

    int BcchInfo::update(BcchInfo* new_entry ) {

        ASSERTION( new_entry , "BcchInfo::update(): new_entry is null.\n");

        nid      = new_entry->nid;
        rai      = new_entry->rai;
        bsic     = new_entry->bsic;
        tsc      = new_entry->tsc;
        bcch_no  = new_entry->bcch_no;
        start_ch = new_entry->start_ch;
        end_ch   = new_entry->end_ch;

        return 1;
    }

    int BcchList::construct_from_psi3( Psi3* psi3msg ) {

        IS_NULL_STR( psi3msg , "contruct_from_psi3(): Argument psi3msg is null.\n" , -1 );

        int elem_num = 6;

        for ( int i=0 ; i<elem_num ; ++i ) {

            BcchInfo* cur_elem = get_elem(i);

            if ( cur_elem ) {
                cur_elem->copy_from_dbssdes( &psi3msg->neighbor_cell[i] );
            }
            else {
                BcchInfo* tmp = new BcchInfo;
                tmp->copy_from_dbssdes( &psi3msg->neighbor_cell[i] );
                insert_tail( tmp );
            }

        }

        return 1;

    }

    BcchInfo* BcchList::search(ulong bcch_no) {

        ListElem<BcchInfo>* ptr;
        for ( ptr=head ; ptr ; ptr=ptr->get_next() ) {

            BcchInfo* elem = ptr->get_elem();
            ASSERTION( elem , "BcchList search(): elem is null.\n" );
            if ( elem->get_bcch_no() == bcch_no ) {
                return elem;
            }
        }

        return NULL;

    }

    BcchInfo* BcchList::search_by_bsic_rai(uchar bsic, uchar rai) {

        ListElem<BcchInfo>* ptr;
        for ( ptr=head ; ptr ; ptr=ptr->get_next() ) {

            BcchInfo* elem = ptr->get_elem();
            ASSERTION( elem , "BcchList search(): elem is null.\n" );
            if ( (elem->get_bsic() == bsic ) && (elem->get_rai() == rai) ) {
                return elem;
            }
        }

        return NULL;

    }

    int BcchList::show_rssi() {

        #ifdef __SHOW_MEASURED_RSSI
        ListElem<BcchInfo>* ptr;
        printf("\n---------------------------------------------------------------------------\n");
        for ( ptr=head ; ptr ; ptr=ptr->get_next() ) {

            BcchInfo* elem = ptr->get_elem();
            if ( elem ) {
                printf("BSS NID =%d : rssi = %f \n", elem->get_nid(), elem->get_rssi() );
            }
        }

        printf("\n---------------------------------------------------------------------------\n");
        #endif
        return 1;

    }

    /************************************************************************************/
    int GprsMsMac::gmm_update() {

        /* return immediately for debugging */
        //return 1;

        if ( !bcch_list)
            return 0;

        if ( !bcch_list->get_head() )
            return 0;

        if ( state != PACKET_TRANSFER_STATE )
            return 0;

        struct preferred_list* mm_upd_list = (struct preferred_list*) malloc(sizeof(struct preferred_list));
        bzero(mm_upd_list,sizeof(struct preferred_list));

        bss_info* ptr  = mm_upd_list->list_head_p;

        long rank[6];
        long included[6];
        bzero( rank , 6*sizeof(long) );
        bzero( included ,6*sizeof(long) );

        for ( int i=0 ; i<6 ; ++i ) {

            double max     = -10000.0;
            ulong  max_ind = 100;

            ListElem<BcchInfo>* bl_ptr1 = bcch_list->get_head();
            ASSERTION( bl_ptr1 , "GprsMsMac::gmm_update(): bl_ptr1 is null.\n");

            for ( int j=0 ; j<6 ;++j ) {

                if ( included[j] )
                    continue;

                BcchInfo* elem1 = bl_ptr1->get_elem();
                ASSERTION( elem1 , "GprsMsMac::gmm_update(): pointer 'elem1' points to null.\n");

                if ( elem1->rssi > max ) {

                    max = elem1->rssi;
                    max_ind = j;

                }

                bl_ptr1 = bl_ptr1->get_next();

            }

            if ( max_ind >=6 ) {
                printf("MS MAC::gmm_update(): Rank Procedure cannot find a valid value, max_ind = %ld.\n" , max_ind );
                break;
            }

            rank[i] = max_ind;
            included[max_ind] = 1;

        }

        /* compare the current rank and the previous rank */
        uchar change_mark = false;
        for ( int i=0 ; i<6 ; ++i ) {

            if ( neighbor_signal_rank[i] != rank[i] ) {

                change_mark = true;
                neighbor_signal_rank[i] = rank[i];

            }
        }

        if ( !change_mark) {

            for ( int i=0 ; i<6 ; ++i ) {
                neighbor_signal_rank[i] = rank[i];
            }

            free(mm_upd_list);
            return 1;

        }



        for ( int i=0 ; i<6 ; ++i ) {

            bss_info* elem  = new bss_info;
            BcchInfo* bl_elem   = bcch_list->get_elem( rank[i] );
            ASSERTION( bl_elem , "GprsMsMac::gmm_update(): pointer 'bl_elem' points to null.\n");

            elem->bsic  = bl_elem->bsic;
            elem->rai   = bl_elem->rai;
            elem->next  = NULL;

            if ( i == 0 ) {
                ptr  = elem;
                mm_upd_list->list_head_p = ptr;
            }

            else {
                ptr->next = elem;
                ptr = ptr->next;
            }

            ++mm_upd_list->num_of_list;
        }

        Event* ep;
        CREATE_BSS_EVENT(ep);
        bss_message* bmsg       = reinterpret_cast<bss_message*> (ep->DataInfo_);
        bmsg->flag              = PF_RECV;
        bmsg->imc_flag          = true;
        bmsg->sndcp_option      = reinterpret_cast<char*> (mm_upd_list);
        bmsg->sndcp_option_len  = sizeof(struct preferred_list);

        create_rlc_option( bmsg );
        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
        rlc_opt->cmd = GMM_UPDATE;

        const int buf_len = 30;
        char *buf = new char[buf_len];
        bzero( buf , buf_len );
        memcpy ( buf , "route_area_update_rq" , 21 );

        bmsg->user_data     = buf;
        bmsg->user_data_len = 30;

        printf("MS MAC::gmm debug\n");
        fflush(stdout);
        return NslObject::recv(ep);

    }

int GprsMsMac::gmm_roaming_procedure(long bcch_no) {

    if ( bcch_no < 0 ) {

        printf("MS MAC [%u]::gmm_roaming_procedure(): specified bcch number is negative.", get_nid() );
        return 0;

    }

    ulong tlli;
    if ( utcb )
        tlli = utcb->tlli;
    else
        tlli = 0;

    roaming_flag            = true;
    state                   = POWER_ON_NOT_SYNC;
    psi3_info_recv_flag     = false;

    cur_used_bss->clear();
    cur_used_bss->set_bcch_no( bcch_no );

    #ifdef __MS_ROAMING_DEBUG
    printf("\n\nMS [%u]: start to roam to another BSS with bcch_no == %ld .\n", get_nid() , bcch_no );
    printf("roaming flag is %u \n" , roaming_flag );
    #endif

    if ( utcb ) {
        delete utcb;
        utcb = NULL;
        //utcb->reset_for_roaming();
    }

    if ( dtcb ) {
        delete dtcb;
        dtcb = NULL;
        //utcb->reset_for_roaming();
        //dtcb->reset_for_roaming();
    }

    return 1;
}

/**** Functions for keeping track of received bursts ****/
int GprsMsMac::validate_mac_option(mac_option* mac_opt) {

    ASSERTION(mac_opt , "validate_mac_option(): mac_opt is null.\n");

    if ( mac_opt->channel_id > 250 || mac_opt->channel_id < 0 ) {

        printf("mac_opt:the channel number in the mac_opt is invalid. Detail: ch_id = %ld \n", mac_opt->channel_id);
        return -1;
    }

    if ( mac_opt->channel_id == 0 ) {
        printf("mac_opt:channel number 0 is reserved for preventing from interference by other radio system. \n");
        return -1;
    }

    if ( mac_opt->fn >= 52 || mac_opt->fn < 0 ) {
        printf("mac_opt: fn is illegal %ld\n", mac_opt->fn);
        return -1;
    }

    if ( mac_opt->tn >= 8 || mac_opt->tn < 0 ) {
        printf("mac_opt: tn is illegal %d\n", mac_opt->tn);
        return -1;
    }

    if ( mac_opt->bn >= 156 || mac_opt->bn < 0 ) {
        printf("mac_opt: bn is illegal %ld\n", mac_opt->bn);
        return -1;
    }

    if ( mac_opt->burst_num >= 4 || mac_opt->burst_num < 0 ) {
        printf("mac_opt: burst_num is illegal %d\n", mac_opt->burst_num);
        return -1;
    }

    if ( mac_opt->burst_type >= 52 || mac_opt->burst_type < 0 ) {
        printf("mac_opt: burst_type is illegal %d\n", mac_opt->burst_type);
        return -1;
    }


    return 1;
}

int GprsMsMac::record_recv_burst(mac_option* mac_opt, double rssi_val) {

        rb_rec[tn].recv_tn     = tn;
        rb_rec[tn].recv_bn     = bn;
        rb_rec[tn].recv_rssi   = rssi_val;
        rb_rec[tn].src_nid     = mac_opt->nid;

        return 1;

}

int GprsMsMac::detect_collision(mac_option* mac_opt , uchar recv_tn ) {

    u_int64_t cur_recv_timestamp = GetCurrentTime();
    u_int64_t trans_period;


    if ( mac_opt->burst_type == ACCESS_BURST ) {
        trans_period = 18; /* need to recalculate */
    }
    else if ( mac_opt->burst_type == SI13_BURST ) {
        trans_period = 36;
    }
    else {
        trans_period = 36;
    }

    if ( !pre_recv_timestamp ) {
        pre_recv_timestamp = GetCurrentTime();
        return 0;
    }

    if ( cur_recv_timestamp < pre_recv_timestamp ) {

        printf("GprsMsMac::detect_collision(): ASSERTION failed: the new timestamp for a burst is less than the old one.\n");
        exit(1);

    }

    else if ( (cur_recv_timestamp - pre_recv_timestamp) < trans_period ) {

        printf("Node %u MS MAC: Collision occurs: \n", get_nid() );
        printf("Detail: the receiving profile of the previous received burst: \n");
        printf("recv_tn = %d recv bn = %d recv rssi = %f src_nid = %ld \n",
            rb_rec[recv_tn].recv_tn, rb_rec[recv_tn].recv_bn, rb_rec[recv_tn].recv_rssi, rb_rec[recv_tn].src_nid);

        printf("WARNING: The locations of BSS may be rearranged.\n");
        return 1;
    }
    else
        return 0;

}

int GprsMsMac::show_rssi() {
    if ( !bcch_list ) {
        printf("MS MAC::show_rssi(): bcch_list is null");
        return 0;
    }

    bcch_list->show_rssi();

    return 1;
}


BcchInfo* GprsMsMac::get_cur_monitored_neighbor_cell() {

    if ( bcch_list ) {

        for (int i=0 ; i<6 ; ++i ) {

            if ( (cur_monitored_neighbor_list_index == -1) || (cur_monitored_neighbor_list_index >= 6) )
                cur_monitored_neighbor_list_index =1;
                /* skip the first elemenet because the first one stands for the current camping cell. */


             BcchInfo* tmp = bcch_list->get_elem(cur_monitored_neighbor_list_index);
             ASSERTION(tmp,"GprsMsMac get_cur_monitored_neighbor_cell(): the object of neighbor cell information is missing.\n" );

             if ( (tmp->start_ch < 0) && (tmp->end_ch < 0) ) {
                 ++cur_monitored_neighbor_list_index;
                 continue;
             }
             else {
                 ++cur_monitored_neighbor_list_index;
                 return tmp;
             }
        }

        return NULL;

    }
    else
        return NULL;

}
