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
#include <assert.h>
#include <udp.h>
#include <nctuns_api.h>
#include <timer.h>
#include <gprs/include/types.h>
#include <gprs/include/burst_type.h>
#include <gprs/include/GPRS_rlcmac_message.h>
#include <gprs/include/partial_queue.h>
#include <gprs/llc/llc.h>
#include <gprs/rlc/rlc.h>
#include <gprs/rlc/state_array.h>
#include <gprs/rlc/rlc_shared_def.h>
#include <gprs/mac/two_slot_usf_granted_ind.h>

#include "tcb.h"

//#define   __PREPARE_ACK_DEBUG
//#define RLC_OPT_HEADER_CMP
//#define __RECV_DATA_SEQ_DEBUG
//#define __UTCB_ACK_CONTENT_DEBUG
//#define __DTCB_ACK_CONTENT_DEBUG
//#define __TCB_SEND_BLK_PROFILE_DEBUG

    using namespace std;

    DownlinkTbfControlBlock::DownlinkTbfControlBlock(bool sr_bit, uchar tfi1, void* nslobj, int (Rlc::*send_func1) (Event*) , int (Rlc::*recv_func1) (Event*)) :
        TbfControlBlock( sr_bit , tfi1 , nslobj , send_func1 , recv_func1) {
        return ;
    }


    UplinkTbfControlBlock::UplinkTbfControlBlock(bool sr_bit, uchar tfi1, void* nslobj, int (Rlc::*send_func1) (Event*) , int (Rlc::*recv_func1) (Event*)) :
        TbfControlBlock( sr_bit , tfi1 , nslobj , send_func1 , recv_func1 ) {
        return ;
    }

    TbfControlBlock::TbfControlBlock(bool sr_bit, uchar tfi1, void* nslobj1, int (Rlc::*send_func1) (Event*) , int (Rlc::*recv_func1) (Event*) ) :
    NslObject(0, 0, 0, "TCB") {

        /* node type is determined in transformation function */

        tfi                      = tfi1;
        channel_coding_scheme    = 2; /* default scheme */
        window_size              = GPRS_WS;
        elem_num                 = GPRS_SNS;
        state                    = READY;
        detach_flag              = false;
        llgmm_reset_flag         = false;

        /* variables used in sending side */
        send_list                = new SList<Event>;
        tx_cnt                   = 0;
        vs                       = 0;
        va                       = 0;
        vcs                      = 0;

        /* variables used in receiving side */
        vr                       = 0;
        vq                       = 0;

        if (!sr_bit) {
            vb                   = new SendBuf;
            vn                   = NULL;
            send_list            = new SList<Event>;
        }
        else {
            /* TbfControlBlock block queue */
            vb                   = NULL;
            vn                   = new RecvBuf;
            partial_queue        = new partially_assembled_llcframe_list;
            //ack_queue            = new SList<Event>;
        }

        ack_event                = NULL;
        ack_threshold            = 10;

        nslobj                   = nslobj1;
        recv_func                = recv_func1;
        send_func                = send_func1;

        ASSERTION(nslobj    , "TCB: nsl obj is NULL\n" );
        ASSERTION(recv_func , "TCB: recv func is NULL\n" );
        ASSERTION(send_func , "TCB: send func is NULL\n" );
        ack_timer                = NULL;
        rrbp_value               = 0x0;
    }

    TbfControlBlock::~TbfControlBlock() {
    	if (vb) delete vb;
    	if (vn) delete vn;
    
    	if ( nodetype == gprs::bts ) {
    		cancel_ack_timer();
    	}
    
    	if ( ack_timer )
    		delete ack_timer;
    }

    int TbfControlBlock::init_ack_timer() {
        if ( nodetype == gprs::bts ) {
            ack_timer = new timerObj;
            IS_NULL_STR(ack_timer,"TCB: ack_timer is not allocated.\n" ,-1);
            ack_timer->init();
            ack_timer->setCallOutObj(this, (int (NslObject::*)(Event*)) &TbfControlBlock::prepare_ack );
            return 1;
        }
        else {
            cout << "TCB: Only BTS TCB can initialize an ACK timer. nodetype = " << (int)(nodetype) << endl;
            return -1;
        }
    }

    int TbfControlBlock::start_ack_timer() {

        IS_NULL_STR( ack_timer , "ack_timer is NULL.\n" , -1 );

        if ( nodetype == gprs::bts ) {

            IS_NULL_STR(ack_timer,"TCB: ack_timer is not allocated.\n" ,-1);
            /* The period of ACK period is 2 second by default */
            u_int64_t tick;
            SEC_TO_TICK(tick,ACK_TIMER_PERIOD);
            ack_timer->start(tick,tick);
            return 1;
        }
        else {
            cout << "TCB: Only BTS TCB can start an ACK timer. nodetype = " << (int)(nodetype) << endl;
            return -1;
        }

    }

    int TbfControlBlock::cancel_ack_timer() {

        IS_NULL( ack_timer , -1 );

        if ( nodetype == gprs::bts ) {
            IS_NULL_STR(ack_timer,"TCB: ack_timer is not allocated.\n" ,-1);
            ack_timer->cancel();
            return 1;
        }
        else {
            cout << "TCB: Only BTS TCB can manipulate an ACK timer. nodetype = " << (int)(nodetype) << endl;
            return -1;
        }

    }

    int TbfControlBlock::restart_ack_timer() {

        IS_NULL( ack_timer , -1 );

        if ( nodetype == gprs::bts ) {
            IS_NULL_STR(ack_timer,"TCB: ack_timer is not allocated.\n" ,-1);
            cancel_ack_timer();
            start_ack_timer();
            return 1;
        }
        else {
            cout << "TCB: Only BTS TCB can manipulate an ACK timer. nodetype = " << (int)(nodetype) << endl;
            return -1;
        }

    }

    int TbfControlBlock::transform_tcb_descriptions(TbfDes* tbfdes) {

        IS_NULL_STR(tbfdes,"TCB transform_tcb_des(): tbfdes is null.\n",-1);

        state                   = READY;
        nodetype                = tbfdes->nodetype;
        tlli                    = tbfdes->tlli;
        uplink_freq             = tbfdes->uplink_freq;
        downlink_freq           = tbfdes->downlink_freq;

        if ( tbfdes->ch_coding_scheme >=1 && tbfdes->ch_coding_scheme <=4 )
            channel_coding_scheme   = tbfdes->ch_coding_scheme;
        else {
            cout <<"TCB transform tbfdes(): Warning: channel coding scheme is not within legal range." << endl;
            channel_coding_scheme   = 2;
        }

        sr_bit                  = tbfdes->sr_bit;

        set_datasize(channel_coding_scheme);

        timeslot_allocation     = 0;

        for (int i=0 ; i<8 ;++i ) {

            if ( tbfdes->ts_alloc[i] ) {
                timeslot_allocation |= (0x01<<i);
                //cout << "Debug: ta_bit: 0x01<<(i="<< i << ")" << "=" << (0x01<<i)<< endl;
                printf("TCB::transform_tbfdes(): TN %d is granted.\n", i );
            }
        }

        return 1;

    }

    uchar* TbfControlBlock::get_datablk_buf() {

        /* NOTE: CS2,CS3,and CS4 are involved with padding bits
         * see ts 44.060
         */
        uchar* buf;

        try {
            if (channel_coding_scheme == 1 ) {
                buf = new uchar[CS1_BLKSIZE+1];
                bzero(buf,CS1_BLKSIZE+1);
            }
            else if ( channel_coding_scheme == 2 ) {
                buf = new uchar[CS2_BLKSIZE+1];
                bzero(buf,CS2_BLKSIZE+1);
            }
            else if ( channel_coding_scheme == 3 ) {
                buf = new uchar[CS3_BLKSIZE+1];
                bzero(buf,CS3_BLKSIZE+1);
            }
            else if ( channel_coding_scheme == 4 ) {
                buf = new uchar[CS4_BLKSIZE+1];
                bzero(buf,CS4_BLKSIZE+1);
            }
            else {
                cout <<"TCB get_data_blk_buf(): unknown coding scheme\n" <<endl;
                buf = NULL;
            }
        }
        catch (std::exception& e) {
            cout << "TCB get_datablk_buf():" << e.what() << endl;
            return NULL;
        }
        return buf;
    }

    int TbfControlBlock::get_datablk_buf_len() {
            if (channel_coding_scheme == 1 ) {
                return CS1_BLKSIZE+1;
            }
            else if ( channel_coding_scheme == 2 ) {
                return CS2_BLKSIZE+1;
            }
            else if ( channel_coding_scheme == 3 ) {
                return CS3_BLKSIZE+1;
            }
            else if ( channel_coding_scheme == 4 ) {
                return CS4_BLKSIZE+1;
            }
            else {
                cout <<"TCB get_datablk_buf_len(): datablk size is not define." << endl;
                return 0;
            }
    }


    int TbfControlBlock::send(Event* ep) {
        int res = this->partition_upper_layer_pdu(ep);
        if ( res < 0 ) {
            //printf("TCB::send(): enqueue failed. \n");
        }
        return res;
    }

    int TbfControlBlock::recv_data(Event* ep , uchar* decoded_data ) {
        int res;
        bss_message *msg    = reinterpret_cast<bss_message*> (ep->DataInfo_);
        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (msg->rlc_option);
        long bsn            = rlc_opt->user_data_bsn;
        //printf("TCB::recv_data(): recv  bsn = %ld tick=%lld\n", bsn , GetCurrentTime() );
        /*  enqueue the received block */
        int ins_res = vn->insert( ep , bsn );

        if ( ins_res < 0 ) {

            /* In the case of RECV Buffer being full */
            if ( msg->packet) {
                remove_upper_layer_pkt(msg->packet);
                free_pkt(msg->packet);
                msg->packet = NULL;
            }

            FREE_BSS_EVENT_INCLUDING_USER_DATA(ep);
            return 0;
        }

        vr = vn->get_ssn();
        vq = vn->get_vq();

        #ifdef __RECV_DATA_SEQ_DEBUG

        if ( nodetype == gprs::ms )
            cout << "MS ";
        else
            cout << "BTS ";
        cout << "TCB: recv data blk with bsn = " << bsn
             << " pkt_ts=" << rlc_opt->pkt_time_stamp
             << " cur_ts = " << GetCurrentTime() << endl;

        #endif

        uchar start_bsn = static_cast<uchar> (compute_start_bsn(rlc_opt));
        partially_assembled_llcframe_list_elem* p_list = partial_queue->get_list(start_bsn);

        if ( !p_list ) {
            partial_queue->create( start_bsn , rlc_opt->blk_cnt , rlc_opt->sndcp_header_len ,
                    rlc_opt->llc_header_len , rlc_opt->user_data_len, rlc_opt->pkt_time_stamp );

            p_list = partial_queue->get_list(start_bsn);
        }

        if ( !p_list ) {
            printf("p_list creating failed");
            exit(1);
        }

        uchar* decoded_data_portion = new uchar[datasize_];
        bzero ( decoded_data_portion , datasize_);
        memcpy ( decoded_data_portion , decoded_data+3 , datasize_ );
        delete decoded_data;
        decoded_data = NULL;
        res = p_list->insert(bsn,decoded_data_portion, rlc_opt->pkt_time_stamp); /* strip mac and rlc headers */;

        if ( res < 0 ) {

            if ( res == -1 ) {

                //printf("TCB::recv_data: duplicated blk.\n");

            }
            else if ( res == -10 ) {

                printf("TCB::recv_data: p_list is out-of-date.\n");
                int start_bsn_v = p_list->get_start_bsn();
                p_list->release_partial_event();
                partial_queue->destroy(start_bsn_v);

                p_list = partial_queue->get_list(start_bsn_v);

                if ( !p_list ) {

                    partial_queue->create( start_bsn_v , rlc_opt->blk_cnt , rlc_opt->sndcp_header_len ,
                            rlc_opt->llc_header_len , rlc_opt->user_data_len, rlc_opt->pkt_time_stamp );

                    p_list = partial_queue->get_list(start_bsn_v);

                    if ( !p_list ) {
                        printf("TCB::recv_data(): p_list creating failed");
                        exit(1);
                    }

                    res = p_list->insert(bsn,decoded_data_portion, rlc_opt->pkt_time_stamp); /* strip mac and rlc headers */;

                    if ( res < 0 ) {

                        printf("TCB::recv_data: p_list is out-of-date.\n");
                        exit(1);

                    }

                    start_bsn = start_bsn_v;
                }
                else {
                    printf("TCB::recv_data(): p_list destroying failed\n");
                    exit(1);
                }


            }
            else {
                printf("TCB::recv_data: undefined condition.\n");
                exit(1);
            }
        }

        if ( rlc_opt->blk_position == (rlc_opt->blk_cnt-1) )
            p_list->create_partial_event( ep );

        Event* llc_frame = p_list->fast_assembling(datasize_);



        /* the llc frame assembling is finished successfully */
        if ( llc_frame ) {

            bss_message* msg1 = reinterpret_cast<bss_message*> (llc_frame->DataInfo_);
            ASSERTION(msg1 , "TCB recv_data(): msg is null.\n");
            msg1->flag = PF_RECV;


            /* destroy the slot in the partial queue */

            ulong   total_blk_cnt   = partial_queue->get_blk_cnt( start_bsn );
            partial_queue->destroy(start_bsn);

            /* recycle the memory allocated to each block */
            if ( nodetype == gprs::ms )
                cout << "MS ";
            else
                cout << "BTS ";
            printf("TCB: recv pdu [%d:%lu] \n", start_bsn, (start_bsn+total_blk_cnt-1)%128);
            vn->recycle_current_event(start_bsn,total_blk_cnt);
            vn->recycle();


            /* send this event */
            if ( nodetype == gprs::ms ) {
                Rlc* obj = reinterpret_cast<Rlc*> (nslobj);
                res = (obj->*recv_func) (llc_frame);
            }
            else {
                Rlc* obj = reinterpret_cast<Rlc*> (nslobj);
                res = (obj->*recv_func) (llc_frame);
            }

            #ifdef __RECV_DATA_SEQ_DEBUG
            if ( nodetype == gprs::ms )
                cout << "MS ";
            else
                cout << "BTS ";

            cout << "TCB: recv data blk with bsn = " << bsn
                << " pkt_ts=" << rlc_opt->pkt_time_stamp
                << " cur_ts = " << GetCurrentTime() << endl;

            #endif
            if ( nodetype == gprs::bts ) {
                prepare_ack();
                //restart_ack_timer();
            }

        }

        return res;
    }

    int TbfControlBlock::recv_ack(void* ack_info) {
        /* invoke each TCB dedicated process_ack() function */
        return this->process_ack(ack_info);
    }

    int DownlinkTbfControlBlock::process_ack(void* pda ) {

        PDACK* pdack = reinterpret_cast <PDACK*> (pda); /* uplink control header only contains 1 byte mac header */
        AckNack* ack_nack_des = pdack->get_acknack();
        ack_nack_des->get_fai();

        /* Get SSN (starting sequence number of this RBB) */
        /* Read RBB (Received Block Bitmap) */
        uchar  ssn   = ack_nack_des->get_ssn();
        uchar* rbb   = ack_nack_des->get_rbb();

        //printf("BTS::recv an ACK at tick = %lld \n", GetCurrentTime() );

        #ifdef __DTCB_ACK_CONTENT_DEBUG
        printf("\nBTS DTCB recv ack: ssn = %d \n", (int)(ssn));
        #endif

        /* Update vb,va according to the obtained information */
        long head_ind = vb->get_head();
        uchar boundary_flag = false;
        for ( ulong i=0 ; i<window_size ; ++i ) {

            long bsn = (ssn-1) - i;
            if (bsn<0)
                bsn += elem_num;

            if ( boundary_flag || (bsn == head_ind) )
                break;

            if ( bsn == (vb->get_tail()) )
                boundary_flag = true;

            if ( rbb[i] == ACKED ) {

                if ( (vb->get_state(bsn)) == PENDING_ACK ) {
                    vb->set_state( bsn , ACKED );
                    vb->del(bsn);
                }

            }

            #ifdef __DTCB_ACK_CONTENT_DEBUG

            printf("rbb[%d] = %d , ", i , rbb[i] );

            if ( !(i % 5) )
                printf("\n");

            #endif
        }


        vb->forward_tail();

        #ifdef __DTCB_ACK_CONTENT_DEBUG
        printf("\n\n");
        #endif

        return 1;
    }

    int UplinkTbfControlBlock::process_ack(void* pua ) {
        PUACK* puack = reinterpret_cast<PUACK*> (pua); /* strip off MAC and RLC headers */
        AckNack* ack_nack_des = puack->get_acknack();
        ack_nack_des->get_fai();


        /* Get SSN (starting sequence number of this RBB) */
        /* Read RBB (Received Block Bitmap) */
        uchar ssn    = ack_nack_des->get_ssn();
        uchar* rbb   = ack_nack_des->get_rbb();

        #ifdef __UTCB_ACK_CONTENT_DEBUG
        printf("\nMS UTCB recv ack: ssn = %d \n", (int)(ssn));
        #endif

        /* Update vb,va according to the obtained information */
        long head_ind       = vb->get_head();
        uchar boundary_flag = false;
        for ( ulong i=0 ; i<window_size ; ++i ) {

            long bsn = (ssn-1) - i;
            if (bsn<0)
                bsn +=elem_num;

            if ( boundary_flag || (bsn == head_ind) )
                break;

            if ( bsn == (vb->get_tail()) )
                boundary_flag = true;

            if ( rbb[i] == ACKED ) {
                if ( (vb->get_state(bsn)) == PENDING_ACK ) {
                    vb->set_state( bsn , ACKED );
                    vb->del(bsn);
                }
            }

            #ifdef __UTCB_ACK_CONTENT_DEBUG

            printf("rbb[%d] = %d    , ", i , rbb[i] );

            if ( !(i % 5) )
                printf("\n");
            #endif

        }

        //printf("UTCBstate: array[65] = %ld \n" , vb->get_state(65) );

        vb->forward_tail();

        #ifdef __UTCB_ACK_CONTENT_DEBUG
        printf("\n\n");
        #endif

        return 1;
    }

    int TbfControlBlock::prepare_ack() {

        if (nodetype != gprs::bts ) {
            cout <<"Only BTS can prepare ACK." << endl;
            return -1;
        }

        prepare_ack_timestamp = GetCurrentTime();


        #ifdef __PREPARE_ACK_DEBUG
        printf("prepare_ack(): ts = %lld \n",prepare_ack_timestamp);
        if ( this->nodetype == gprs::ms ) {
            printf("MS: ");
        }
        else if ( nodetype == gprs::bts ) {
            printf("BTS: ");
        }
        else
            printf("Unknown side:");

        if ( sr_bit == SENDER ) {
            cout << "Sender TBF prepare ACK "<< endl;
        }
        else {
            cout << "Receiver TBF prepare ACK "<< endl;
        }

        #endif

        ack_req     = true;
        rrbp_value  = 0x00;

        start_ack_timer();

        return 1;
    }


    int DownlinkTbfControlBlock::create_ack() {

        /* only Receiver has an allocated RecvBuf */
        if (!sr_bit) {
            return false;
        }

        /* Create an Acknowledgement */
        long ssn = vn->get_ssn();
        if (ssn<0)
            return false;

        NonDistributedMsg* d_ack_msg;

        try {

            GlobalTfi gtfi_ie;
            gtfi_ie.tfi         = tfi;
            gtfi_ie.direction   = D_BIT_DOWNLINK;

            d_ack_msg = new NonDistributedMsg;
            d_ack_msg->set_addr_info(GLOBAL_TFI_INDICATOR, gtfi_ie);

            PDACK* dack = new PDACK;
            dack->set_ssn(ssn);
            dack->set_fai(0);
            /* fai is set to 1 if the current block of downlink is the last one data block */
            uchar* rbb = new uchar[window_size];
            bzero(rbb,window_size*sizeof(uchar));

            /* rbb is ranged from (ssn -1 ) mod GPRS_SNS to (ssn-64) mod GPRS_SNS */
            for (ulong i=1 ; i<=window_size ; ++i ) {

                long ind = ssn - i;
                if ( ind < 0 )
                    ind += elem_num;
                rbb[i-1] = vn->get_state(ind);

            }



            dack->set_rbb(rbb);
            d_ack_msg->set_nondistr_msg(PKT_DOWNLINK_ACKNACK, uplink , dack );

            /* create an event to convey this ack */
            Event* eack;
            CREATE_BSS_EVENT(eack);
            bss_message* bssmsg     = reinterpret_cast<bss_message*> (eack->DataInfo_);
            bssmsg->flag            = PF_SEND;
            //char* dummy_data        = new char[1];
            char* dummy_data = (char*)(dummy_data_array);
            bzero( dummy_data , 1 );
            insert_user_data(bssmsg,dummy_data,1);

            rlc_option* rlc_opt     = new rlc_option;
            rlc_opt->ctl_msg_type   = PKT_DOWNLINK_ACKNACK;
            rlc_opt->direction      = 0;
            rlc_opt->chid           = uplink_freq;
            rlc_opt->tfi            = tfi;

            mac_option* mac_opt     = new mac_option;
            mac_opt->burst_type     = CTL_BURST;
            //mac_opt->msg            = d_ack_msg;
            insert_macopt_msg( mac_opt , NDMSG , d_ack_msg );

            bssmsg->rlc_option      = reinterpret_cast<char*> (rlc_opt);
            bssmsg->rlc_option_len  = sizeof(rlc_option);
            bssmsg->mac_option      = reinterpret_cast<char*> (mac_opt);
            bssmsg->mac_option_len  = sizeof(mac_option);
            bssmsg->rlc_header      = NULL;

            /* An uplink control block doesn't have RLC header. */

            return send_ack(eack);

        }
        catch (std::exception& e) {
            cout <<"TCB create_ack():" << e.what() << endl;
            return false;
        }
    }

    int UplinkTbfControlBlock::create_ack() {

        /* only Receiver has an allocated RecvBuf */
        if (!sr_bit) {
            return false;
        }

        /* Create an Acknowledgement */
        long ssn = vn->get_ssn();
        //printf("UTCB: create ack_ssn = %ld \n",ssn);

        if (ssn<0)
            return false;

        NonDistributedMsg* u_ack_msg;
        try {

            GlobalTfi gtfi_ie;
            gtfi_ie.tfi         = tfi;
            gtfi_ie.direction   = D_BIT_DOWNLINK;

            u_ack_msg = new NonDistributedMsg;
            u_ack_msg->set_addr_info(GLOBAL_TFI_INDICATOR, gtfi_ie);

            PUACK* uack = new PUACK;
            uack->set_ssn(ssn);
            uack->set_fai(0);
            /* fai is set to 1 if the current block of downlink is the last one data block */
            uchar* rbb = new uchar[window_size];
            bzero(rbb,window_size*sizeof(uchar));

            /* rbb is ranged from (ssn -1 ) mod GPRS_SNS to (ssn-64) mod GPRS_SNS */
            for (ulong i=1 ; i<=window_size ; ++i ) {
                long ind = ssn - i;
                if ( ind < 0 )
                    ind += elem_num;
                rbb[i-1] = vn->get_state(ind);
            }
            uack->set_rbb(rbb);

            u_ack_msg->set_nondistr_msg(PKT_UPLINK_ACKNACK, downlink , uack );

            /* create an event to convey this ack */
            Event* eack;
            CREATE_BSS_EVENT(eack);
            bss_message* bssmsg     = reinterpret_cast<bss_message*> (eack->DataInfo_);
            bssmsg->flag            = PF_SEND;
            //char* dummy_data        = new char[1];
            char* dummy_data = (char*)(dummy_data_array);
            insert_user_data(bssmsg,dummy_data,1);

            rlc_option* rlc_opt     = new rlc_option;
            rlc_opt->ctl_msg_type   = PKT_UPLINK_ACKNACK;
            rlc_opt->direction      = 1;
            rlc_opt->chid           = downlink_freq;
            rlc_opt->tfi            = tfi;

            mac_option* mac_opt     = new mac_option;
            mac_opt->burst_type     = CTL_BURST;
            //mac_opt->msg            = u_ack_msg;
            insert_macopt_msg( mac_opt , NDMSG , u_ack_msg );
            RDCBH* rdcbh            = new RDCBH;
            rdcbh->tfi              = tfi;

            bssmsg->rlc_option      = reinterpret_cast<char*> (rlc_opt);
            bssmsg->rlc_option_len  = sizeof(rlc_option);
            bssmsg->mac_option      = reinterpret_cast<char*> (mac_opt);
            bssmsg->mac_option_len  = sizeof(mac_option);
            insert_rlc_header( bssmsg , rdcbh , sizeof(RDCBH) );
            return send_ack(eack);
        }
        catch (std::exception& e) {
            cout <<"TCB create_ack():" << e.what() << endl;
                return false;
        }

    }

    int TbfControlBlock::send_ack(Event* ep) {
        ASSERTION(ep,"TCB send_ack(): ep is null\n");

        Rlc* obj = reinterpret_cast<Rlc*> (nslobj);
        int res = (obj->*send_func) (ep);
        FREE_BSS_EVENT( ep );
        return res;
    }

    Event* TbfControlBlock::scheduler() {

       /* If vs < (va+SWS) mod SNS , then
        *     if there is an entry with BSN in vb set as NACKED, then
        *         if BSN < (va+SWS)modSNS , then
        *             invoke send_the_oldest_nack_blk();
        *         endif
        *     else
        *         send the oldest block with state= PENDING_ACK
        *     endif
        *
        * Else if vs == (va+SWS)mod SNS
        *     send the oldest block with state = PENDING_ACK;
        * Else exception occurs.
        */


        ASSERTION( vb , "TCB scheduler(): SendBuf does not exist.\n");

        Event* res = vb->get_transmitted_blk();

        /* indicate the peer to prepare send ACK */
        if ( nodetype == gprs::bts ) {

            if ( (vb->trigger_ack_scheduler()) )
                prepare_ack();

        }

        if ( res ) {

            ASSERTION( res->DataInfo_ , "TCB scheduler(): res->DataInfo_ does not exist.\n");

            rlc_option* rlc_opt =
                reinterpret_cast<rlc_option*> (reinterpret_cast<bss_message*> (res->DataInfo_)->rlc_option);

            ASSERTION( rlc_opt , "TCB scheduler(): rlc_opt does not exist.\n");

            rlc_opt->stalled = false;

            #ifdef __TCB_SEND_BLK_PROFILE_DEBUG
            printf("TCB::scheduler(): send bsn = %ld pkt_ts = %lld tick=%lld vs=%d va=%d cur_pending_ind=%d\n",
                rlc_opt->user_data_bsn , rlc_opt->pkt_time_stamp, GetCurrentTime(), vb->get_vs(),
                vb->get_va(), vb->get_cur_sent_pending_acked_bsn() );
            #endif
        }

        return res;
    }


    int UplinkTbfControlBlock::partition_upper_layer_pdu(Event* ep) {

        /*  Fragment the LLC frame into four RLC/MAC data blocks. The size of each segment is dependent on
         *  what channel coding scheme that RLC currently uses.
         */
        int datasize = set_datasize(channel_coding_scheme);

        bss_message* bss_msg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        llc_option*  llc_opt = reinterpret_cast<llc_option*>  (bss_msg->llc_option);

        /* packet length should include the length of SNDCP, LLC header */
        int pkt_len = bss_msg->user_data_len + bss_msg->sndcp_header_len + bss_msg->llc_header_len;

        if ( !pkt_len) {
            printf("UTCB partition(): Assertion failed! Message length = 0 !\n");
            //FREE_BSS_EVENT(ep);
            assert(0);
        }

        /* compute the number of blocks obtained by dividing the length used by current scheme. */
        uchar blk_cnt = pkt_len / datasize;

        if ( (pkt_len%datasize) >0 )
            ++blk_cnt;

        /* if these fragments of the llc frame exceed the TbfControlBlock data block queue,
         * this frame should be rejected and return it to llc layer.
         */

        if ( vb->test_full(blk_cnt) ) {
            //printf("UTCB::partition_upper_layer_pdu(): send buffer is full.\n");
            return -1; /* reject the sending request because vb cannot store these blocks completely. */
        }


        printf("node [%u]: UTCB enqueue bsn[%ld:%ld] p_ts=%lld\n", 
	    get_nid(), vb->get_head(),(vb->get_head()+(blk_cnt-1))%128, GetCurrentTime() );

        uchar *llc_frame_buf= new uchar[pkt_len];
        bzero(llc_frame_buf,pkt_len);

        if (!llc_frame_buf) {
            printf("UTCB partition(): Assertion failed: llc_frame_buf is Null\n");
            return 0;
        }

        memcpy( llc_frame_buf , bss_msg->sndcp_header , bss_msg->sndcp_header_len );
        memcpy( llc_frame_buf + bss_msg->sndcp_header_len , bss_msg->llc_header , bss_msg->llc_header_len );
        memcpy( llc_frame_buf + bss_msg->sndcp_header_len + bss_msg->llc_header_len , bss_msg->user_data , bss_msg->user_data_len );

        if ( bss_msg->packet ) {

            Packet* pkt = bss_msg->packet;

            const char *sep = "+";
	    char *brkt, *str, tok[4][50];
            char *buf = (char*) (pkt->pkt_sget());

            int header_len = sizeof(struct ip) + sizeof(struct udphdr);

            buf = buf + header_len;
            int i = 0;

            for( str=strtok_r(buf, sep, &brkt); str; str=strtok_r(NULL, sep, &brkt))
            {
                strcpy( tok[i], str);
                i++;
            }

            int pid = (int)atoi( tok[0] );

            printf("node [%u]: TCB send pkt no. %d packet \n", get_nid(), pid );
        }


        /* fullfill the block header as default values */
        RUDBH header;

        uchar *tmp_block;

        /* assign the rlcmac header size: See page 117 in TS 44.060 */
        int rlcmac_datablk_header_size = 3;

        /* examine whether this packet with llc_opt->command == LLGMM_DETACH */
        uchar detach_flag = false;
        ulong detach_tlli = 0;
        ASSERTION ( llc_opt , "UTCB partition_upper_layer_pdu(): llc_opt is NULL." );

        /*if ( llc_opt->addtional_info == LLGMM_DETACH ) {
            detach_flag = true;
            detach_tlli = llc_opt->tlli;
        }*/

        /* 4. Fragment LLC frame and enqueue the four RLC/MAC data blocks just fragmented. */
        for ( int cnt=0 ; cnt < blk_cnt ; ++cnt ) {

            long bsn = vb->get_head();

            if ( bsn < 0 ) {

                /* this condition shall be avoided by vb->test_full() function */
                cout <<"UTCB partition_upper_layer_pdu(): send buffer is full." << endl;
                exit(1);
            }

            header.tfi      = tfi;
            header.ti       = 0; /* 1 represents the presence of TLLI */
            header.bsn      = bsn;
            header.mbit     = 0;
            header.ebit     = 1;

            tmp_block = get_datablk_buf(); /* get data block size including header fields */
            int tmp_block_len = (get_datablk_buf_len());
            IS_NULL_STR(tmp_block,"UTCB partition_upper_layer_pdu(): tmp_blk is null\n",-1);
            bzero(tmp_block,datasize);

            /* the first one octet is reserved for mac header */
            header.pack_header( (tmp_block+1) );

            if ( cnt< (blk_cnt-1) ) {
                memcpy ( tmp_block + rlcmac_datablk_header_size , llc_frame_buf+cnt*datasize , datasize );
            }
            else { /* last blk */
                int tail_len  = pkt_len - cnt*datasize;
                tmp_block_len = tail_len;
                memcpy( tmp_block + rlcmac_datablk_header_size , llc_frame_buf+cnt*datasize, tail_len );
            }

            Event* blk;
            CREATE_BSS_EVENT(blk);
            IS_NULL_STR(blk,"UTCB partition_upper_layer_pdu(): creating a blk failed\n",-1);
            bss_message *rlcblk = reinterpret_cast<bss_message*> (blk->DataInfo_);
            IS_NULL_STR(rlcblk,"UTCB partition_upper_layer_pdu(): creating a rlcblk failed\n",-1);
            rlcblk->flag = PF_SEND;

            insert_user_data(rlcblk,tmp_block,tmp_block_len);

            rlc_option *rlc_opt;

            try {

                if ( cnt == (blk_cnt-1) ) { /* the last block should carry the "out-of-band" information for upper layers */
                    copy_bss_msg_options  (rlcblk,bss_msg);
                    //copy_bss_msg_packet_field(rlcblk,bss_msg);
                    // try this
                    rlcblk->packet  = bss_msg->packet;
                    bss_msg->packet = NULL;
                }

                create_rlc_option(rlcblk);
                rlc_opt = reinterpret_cast<rlc_option*> (rlcblk->rlc_option);
            }
            catch (std::exception& e) {
                cout<<"TCB partition_upper_layer_pdu():" << e.what() <<endl;
                return 0;
            }


            /* store the header lengths of upper layer headers for fast memory-partition */
            rlc_opt->sndcp_header_len   = bss_msg->sndcp_header_len;
            rlc_opt->llc_header_len     = bss_msg->llc_header_len;

            rlc_opt->blk_cnt            = blk_cnt;
            rlc_opt->blk_position       = cnt;
            rlc_opt->ctl_msg_type       = DATA;
            rlc_opt->user_data_bsn      = header.bsn;
            rlc_opt->user_data_len      = bss_msg->user_data_len;
            rlc_opt->cs                 = channel_coding_scheme;
            rlc_opt->tfi                = tfi;
            rlc_opt->stalled            = false; /* may be updated when this block is dequeued from sendbuf */
            rlc_opt->detach_flag        = detach_flag;
            rlc_opt->detach_tlli        = detach_tlli;
            rlc_opt->pkt_time_stamp     = GetCurrentTime();

            if ( nodetype == gprs::ms )
                rlc_opt->chid = uplink_freq;
            else
                rlc_opt->chid = downlink_freq;



            #ifdef RLC_OPT_HEADER_CMP

            printf ("UTCB: header tfi %d header bsn %d \n", header.tfi , header.bsn );
            printf ("UTCB: rlc_opt->tfi %d rlc_opt->user_data_bsn %d \n", rlc_opt->tfi , rlc_opt->user_data_bsn);

            #endif

            if ( (vb->insert(blk , bsn)) < 0 ) {
                cout <<"RLC SendBuf: Assertion failed. Send buffer is full." << endl;
                exit(1);
            }
        }

        delete llc_frame_buf;
	FREE_BSS_EVENT_INCLUDING_USER_DATA(ep);
        return 1;
   }

   int DownlinkTbfControlBlock::partition_upper_layer_pdu(Event* ep) {

        /*  Fragment the LLC frame into four RLC/MAC data blocks. The size of each segment is dependent on
         *  what channel coding scheme that RLC currently uses.
         */
        int datasize = set_datasize(channel_coding_scheme);

        bss_message* bss_msg = reinterpret_cast<bss_message*>(ep->DataInfo_);
        llc_option*  llc_opt = reinterpret_cast<llc_option*>  (bss_msg->llc_option);

        /* packet length should include the length of SNDCP, LLC header */
        int pkt_len = bss_msg->user_data_len + bss_msg->sndcp_header_len + bss_msg->llc_header_len;

        if ( !pkt_len) {
            printf("DTCB partition(): Assertion failed! Message length = 0 !\n");
            FREE_BSS_EVENT(ep);
            return 0;
        }

        /* compute the number of blocks obtained by dividing the length used by current scheme. */
        uchar blk_cnt = pkt_len / datasize;

        if ( (pkt_len%datasize) >0 )
            ++blk_cnt;

        /* if these fragments of the llc frame exceed the TbfControlBlock data block queue,
         * this frame should be rejected and return it to llc layer.
         */

        if ( vb->test_full(blk_cnt) ) {

            return -1; /* reject the sending request because vb cannot store these blocks completely. */
        }


        uchar *llc_frame_buf= new uchar[pkt_len];
        bzero(llc_frame_buf,pkt_len);

        if (!llc_frame_buf) {
            printf("DTCB partition(): Assertion failed: llc_frame_buf is Null\n");
            assert(0);
        }

        memcpy( llc_frame_buf , bss_msg->sndcp_header , bss_msg->sndcp_header_len );
        memcpy( llc_frame_buf + bss_msg->sndcp_header_len , bss_msg->llc_header , bss_msg->llc_header_len );
        memcpy( llc_frame_buf + bss_msg->sndcp_header_len + bss_msg->llc_header_len , bss_msg->user_data , bss_msg->user_data_len );



        if ( bss_msg->packet ) {

            Packet* pkt = bss_msg->packet;

            const char *sep = "+";
	    char *brkt, *str, tok[4][50];
            char *buf = (char*) (pkt->pkt_sget());

            int header_len = sizeof(struct ip) + sizeof(struct udphdr);

            buf = buf + header_len;
            int i = 0;

            for( str=strtok_r(buf, sep, &brkt); str; str=strtok_r(NULL, sep, &brkt))
            {
                strcpy( tok[i], str);
                i++;
            }

            int pid = (int)atoi( tok[0] );

            printf("node [%u]: DTCB enqueue pkt no. %d bsn[%ld:%ld] p_ts=%lld\n", 
	        get_nid(), pid , vb->get_head(), (vb->get_head()+(blk_cnt-1))%128, GetCurrentTime() );
        }
        else
            printf("node [%u]: DTCB enqueue bsn[%ld:%ld] p_ts=%lld\n", 
	        get_nid(), vb->get_head(),(vb->get_head()+(blk_cnt-1))%128, GetCurrentTime() );


        /* fullfill the block header as default values */
        RDDBH header;

        uchar* tmp_block;

        /* assign the rlcmac header size: See page 117 in TS 44.060 */
        int rlcmac_datablk_header_size = 3;

        /* examine whether this packet with llc_opt->command == LLGMM_DETACH */
        uchar detach_flag = false;
        ulong detach_tlli = 0;
        ASSERTION ( llc_opt , "DTCB partition_upper_layer_pdu(): llc_opt is NULL." ) ;
        if ( llc_opt->addtional_info == LLGMM_DETACH ) {
            detach_flag = true;
            detach_tlli = llc_opt->tlli;
        }

        /* 4. Fragment LLC frame and enqueue the four RLC/MAC data blocks just fragmented. */
        for ( int cnt=0 ; cnt < blk_cnt ; ++cnt ) {
            long bsn  = vb->get_head();

            if ( bsn < 0 ) {

                /* this condition shall be avoided by vb->test_full() function */
                cout <<"DTCB partition_upper_layer_pdu(): send buffer is full." << endl;
                exit(1);
            }


            header.power_reduction      = 0x0; /* see ts44.060 table 10.4.10a.1 */
            header.tfi                  = tfi;
            header.fbi                  = 0;   /* final block identifier: 1 indicates this is last block */
            header.bsn                  = bsn;/* block sequence number */
            header.length_indicator     = 0;
            header.mbit                 = 0;
            header.ebit                 = 1;

            tmp_block = get_datablk_buf(); /* get data block size including header fields */
            IS_NULL_STR(tmp_block,"DTCB partition_upper_layer_pdu(): tmp_blk is null\n",-1);

            int tmp_block_len = (get_datablk_buf_len());
            IS_NULL_STR(tmp_block,"UTCB partition_upper_layer_pdu(): tmp_blk is null\n",-1);
            bzero(tmp_block,datasize);

            /* the first one octet is reserved for mac header */
            header.pack_header( (tmp_block+1) );

            if ( cnt< (blk_cnt-1) ) {
                memcpy ( tmp_block + rlcmac_datablk_header_size , llc_frame_buf+cnt*datasize , datasize );
            }
            else { /* last blk */
                int tail_len  = pkt_len - cnt*datasize;
                tmp_block_len = tail_len;
                memcpy( tmp_block + rlcmac_datablk_header_size , llc_frame_buf+cnt*datasize, tail_len );
            }

            Event* blk;
            CREATE_BSS_EVENT(blk);
            IS_NULL_STR(blk,"DTCB partition_upper_layer_pdu(): creating a blk failed\n",-1);
            bss_message *rlcblk = reinterpret_cast<bss_message*> (blk->DataInfo_);
            IS_NULL_STR(rlcblk,"DTCB partition_upper_layer_pdu(): creating a rlcblk failed\n",-1);

            rlcblk->flag            = PF_SEND;

            insert_user_data(rlcblk,tmp_block,tmp_block_len);

            rlc_option *rlc_opt;

            try {
                if ( cnt == (blk_cnt-1)) { /* the last block should carry the "out-of-band" information for upper layers */

                    copy_bss_msg_options  (rlcblk,bss_msg);
                    //copy_bss_msg_packet_field(rlcblk,bss_msg);
                    // try this
                    rlcblk->packet  = bss_msg->packet;
                    bss_msg->packet = NULL;
                }

                /* push a data block down to MAC layer and provide a callback function
                 * for MAC layer to inform RLC of the completion of a block transfer.
                 */

                create_rlc_option(rlcblk);
                rlc_opt = reinterpret_cast<rlc_option*> (rlcblk->rlc_option);
            }
            catch (std::exception& e) {
                cout<<"DTCB partition_upper_layer_pdu():" << e.what() <<endl;
                return 0;
            }

            rlc_opt->blk_cnt            = blk_cnt;
            rlc_opt->blk_position       = cnt;
            rlc_opt->ctl_msg_type       = DATA;
            rlc_opt->user_data_bsn      = header.bsn;

            rlc_opt->sndcp_header_len   = bss_msg->sndcp_header_len;
            rlc_opt->llc_header_len     = bss_msg->llc_header_len;
            rlc_opt->user_data_len      = bss_msg->user_data_len;
            rlc_opt->tfi                = tfi;
            rlc_opt->cs                 = channel_coding_scheme;
            rlc_opt->stalled            = false; /* may be updated when this block is dequeued from sendbuf */
            rlc_opt->detach_flag        = detach_flag;
            rlc_opt->detach_tlli        = detach_tlli;
            rlc_opt->pkt_time_stamp     = GetCurrentTime();


            if ( nodetype == gprs::ms )
                rlc_opt->chid = uplink_freq;
            else
                rlc_opt->chid = downlink_freq;

            #ifdef RLC_OPT_HEADER_CMP

            printf ("DTCB: header tfi %d header bsn %d \n", header.tfi , header.bsn );
            printf ("DTCB: rlc_opt->tfi %d rlc_opt->user_data_bsn %d \n", rlc_opt->tfi , rlc_opt->user_data_bsn);

            #endif

            vb->insert(blk , bsn);
        }

        printf("DTCB: tick = %lld ", GetCurrentTime() );
        delete llc_frame_buf;
	FREE_BSS_EVENT_INCLUDING_USER_DATA(ep);
        return 1;
   }


    ulong TbfControlBlock::set_datasize(int coding_scheme_num ) {
        if ( coding_scheme_num == 2 ) /* common case */
            datasize_ = CS2_DATASIZE;
        else if ( coding_scheme_num == 1 )
            datasize_ = CS1_DATASIZE;
        else if ( coding_scheme_num == 3 )
            datasize_ = CS3_DATASIZE;
        else if ( coding_scheme_num == 4 )
            datasize_ = CS4_DATASIZE;
        else
            datasize_ = 1 ; /* future extension */

        return datasize_;
    }


    uchar* TbfControlBlock::decoding(Event* e_p) {

        /* extract the encoded data:
         * 1. decode the data of a rlc block.
         * 2. perform error detection: if passed, copy the data to reassembling buffer.
         *                             else, give up the assembing.
         * 3. store the raw data into a buffer to wait for assembling.
         */

    bss_message *tmp_bssmsg = reinterpret_cast<bss_message*> (e_p->DataInfo_);
    IS_NULL_STR(tmp_bssmsg,"TbfControlBlock assemble(): abnormality! DataInfo_ is NULL \n",NULL);
    uchar* decoded_blkdata;
    decoded_blkdata = channel_decoding( channel_coding_scheme , reinterpret_cast<uchar*> (tmp_bssmsg->user_data) );
    IS_NULL_STR(decoded_blkdata,"TbfControlBlock assemble(): decoded block data failed\n",NULL);
        return decoded_blkdata;
    }

    /****************************************************************/
    TbfDescription::TbfDescription(uchar srbit, bool nodetype1 ) {

        bzero(this,sizeof(TbfDes));
        state                   = CHANNEL_REQ_PHASE;
        sr_bit                  = srbit;
        nodetype                = nodetype1;
        ch_coding_scheme        = 2;
        ta_ie                   = NULL;

        /* Uplink transmission control parameters:
            * USF allocation
            */
        ch_req_cnt      = -1;
        usf_granted_ind = new TwoSlotUsfGrantedInd(gprs::not_granted);
        memset( usf_tn , gprs::free , 8);
        /* PACCH description:
         * (1)PktResourceRequest (use the same configuration of PAGCH (single block assignment) )
         * (2)ControlACKs (indicated by RRBP)
         */
        /*pacch_blkn                          = -1 ;
        pacch_tn                            = -1;*/

        for ( int i=0; i<12 ;++i) {
            pacch_blkn_bitmap[i] = new uchar[8];
            bzero( pacch_blkn_bitmap[i] , 8*sizeof(uchar) );
        }


        tbf_starting_time_count_down_value  = -1;
        sba_in_transmission                 = -1;
        rrbp_cur                            = gprs::rrbp_unused;

        /* receiving queues are maintained by MAC module
         * because the demultiplexing of a burst should be performed
         * upon the receipt of all four bursts forming a RLC block.
         */

        pacch_pending_event_sq  = new SList<Event>;
        for ( int i=0 ; i<8 ; ++i ) {
            pdtch_sq[i] = new SList<Event>;
            pacch_sq[i] = new SList<Event>;
        }
    }

    TbfDescription::~TbfDescription() {
        if (stored_req)
            delete stored_req;
        if (ta_ie)
            delete ta_ie;
        if (sbafreq)
            delete sbafreq;

        for ( int i=0 ; i<8 ;++i ) {
            if (pdtch_sq[i])
                delete pdtch_sq[i];
            if (pacch_sq[i])
                delete pacch_sq[i];
        }

    }

    int TbfDescription::insert_sba(SbaFreq* sbafreqdes, ulong uplink_fn ) {

        IS_NULL_STR(sbafreqdes,"TbfDescription insert_sba(): sabfreqdes is null\n" , -1);
        sbafreq = sbafreqdes;
        long cv = sbafreq->sba->tbf_starting_time - uplink_fn;

        if ( cv<0)
            cv = (52-uplink_fn) + sbafreq->sba->tbf_starting_time;
        tbf_starting_time_count_down_value = cv;
        sba_in_transmission = -1;
        return 1;
    }

    int TbfDescription::merge_ack(ulong tn) {

        if ( !pacch_sq || !pacch_sq[tn] )
            return 0;


        /* Start Seq. Number Analysis */
        ulong  max_ack_num;

        max_ack_num  = pacch_sq[tn]->get_list_num();

        ListElem<Event>* ptr_listelem = pacch_sq[tn]->get_head();
        Event* ptr_elem = NULL;

        uchar* ssn_table = new  uchar[max_ack_num];
        uchar* merged_rbb = new uchar[64];
        bzero( merged_rbb , sizeof(uchar)*64);

        for ( ulong j=0 ; (j<max_ack_num) ; ++j ) {

            ptr_elem = ptr_listelem->get_elem();
            ASSERTION( ptr_elem , "TbfDes::merge_ack(): ptr_elem(): is null. (1) \n");

            bss_message* bmsg   = reinterpret_cast<bss_message*> (ptr_elem->DataInfo_);
            ASSERTION( bmsg , "TbfDes::merge_ack(): bss messsage is null. \n");

            mac_option*  mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
            ASSERTION( mac_opt , "TbfDes::merge_ack(): mac option is null. \n");

            rlc_option*  rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
            ASSERTION( rlc_opt , "TbfDes::merge_ack(): rlc option is null. \n");

            if ( j == 0 ) {

                if ( mac_opt->burst_num != 0 ) {
                    printf("TbfDes::merge_ack(): the number of bursts does not match the boundary of a block. (1)\n");
                    exit(1);
                }

            }

            if ( (mac_opt->msg) &&
                (mac_opt->msg_type == NDMSG) &&
                ((rlc_opt->ctl_msg_type == PKT_UPLINK_ACKNACK) && (rlc_opt->direction == 1)) ) {

                if ( mac_opt->burst_num != 3 ) {
                    printf("TbfDes::merge_ack(): The control message is carried by an incorrect burst. \n");
                    exit(1);
                }

                NDistrMsg* nmsg = reinterpret_cast<NDistrMsg*> (mac_opt->msg);

                PUACK* puack = reinterpret_cast<PUACK*> (nmsg->get_nondistr_msg());

                AckNack* ack_nack_des = puack->get_acknack();
                //ack_nack_des->get_fai();

                /* Get SSN (starting sequence number of this RBB) */
                /* Read RBB (Received Block Bitmap) */
                uchar ssn    = ack_nack_des->get_ssn();

                for ( ulong k=j-3 ; k<=j ; ++k )
                    ssn_table[k] = ssn;

            }

            ptr_listelem = ptr_listelem->get_next();
            if ( ptr_listelem )
                ptr_elem = ptr_listelem->get_elem();
            else
                break;
        }

        ulong start_elem_id = 0;
        ulong the_same_ack_burst_num = 0;

        for ( ulong j=0 ; j<max_ack_num ; ++j ) {
            if ( ssn_table[start_elem_id] == ssn_table[j] ) {
                the_same_ack_burst_num ++;
            }
            else
                break;
        }


        /* RBB Merging Process */
        if ( the_same_ack_burst_num > 4 ) {

            ptr_listelem = pacch_sq[tn]->get_head();

            for ( ulong j=0 ; j<the_same_ack_burst_num ; ++j) {

                ptr_elem = ptr_listelem->get_elem();
                ASSERTION( ptr_elem , "TbfDes::merge_ack(): ptr_elem(): is null. (2) \n");

                bss_message* bmsg    = reinterpret_cast<bss_message*> (ptr_elem->DataInfo_);
                ASSERTION( bmsg , "TbfDes::merge_ack(): bss messsage is null. \n");
                mac_option* mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
                ASSERTION( mac_opt , "TbfDes::merge_ack(): mac option is null. \n");
                rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
                ASSERTION( rlc_opt , "TbfDes::merge_ack(): rlc option is null. \n");

                if ( j == 0 ) {

                    if ( mac_opt->burst_num != 0 ) {
                        printf("TbfDes::merge_ack(): the number of bursts does not match the boundary of a block. (2)\n");
                        exit(1);
                    }

                }

                if ( (mac_opt->msg) &&
                    (mac_opt->msg_type == NDMSG) &&
                    ((rlc_opt->ctl_msg_type == PKT_UPLINK_ACKNACK) && (rlc_opt->direction == 1)) ) {

                    if ( mac_opt->burst_num != 3 ) {
                        printf("TbfDes::merge_ack(): The control message is carried by an incorrect burst. \n");
                        exit(1);
                    }

                    NDistrMsg* nmsg = reinterpret_cast<NDistrMsg*> (mac_opt->msg);

                    PUACK* puack = reinterpret_cast<PUACK*> (nmsg->get_nondistr_msg());

                    AckNack* ack_nack_des = puack->get_acknack();

                    /* Read RBB (Received Block Bitmap) */
                    uchar* rbb   = ack_nack_des->get_rbb();
                    for ( ulong k=0 ; k<64 ;++k) {
                        merged_rbb[k] |= rbb[k];
                    }


                }

                ptr_listelem = ptr_listelem->get_next();
                if ( ptr_listelem )
                    ptr_elem = ptr_listelem->get_elem();
                else
                    break;

            }


            /* Merge Event Process */
            ulong delete_burst_num = (the_same_ack_burst_num-4);
            if ( (delete_burst_num%4) != 0 ) {
                printf("MergeAck(): the number of bursts to be deleted is not the multiple of four.\n");
                exit(1);
            }

            ptr_listelem = pacch_sq[tn]->get_head();

            for ( ulong j = 0; j<delete_burst_num ; ++j ) {

                ptr_elem = ptr_listelem->get_elem();
                ASSERTION( ptr_elem , "TbfDes::merge_ack(): ptr_elem(): is null. (3)\n");
                pacch_sq[tn]->remove_head();


                bss_message* bmsg    = reinterpret_cast<bss_message*> (ptr_elem->DataInfo_);
                ASSERTION( bmsg , "TbfDes::merge_ack(): bss messsage is null. \n");
                mac_option* mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
                ASSERTION( mac_opt , "TbfDes::merge_ack(): mac option is null. \n");
                rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
                ASSERTION( rlc_opt , "TbfDes::merge_ack(): rlc option is null. \n");

                NB* nb_p = reinterpret_cast<NB*> (bmsg->user_data);
                delete nb_p;
                bmsg->user_data = NULL;
                bmsg->user_data_len = 0;

                if ( mac_opt->msg ) {

                    shared_obj_dec_refcnt( mac_opt->msg_type , mac_opt->msg );
                    int res = shared_obj_release( mac_opt->msg_type , mac_opt->msg );
                    if ( res ) {
                        mac_opt->msg = 0;
                    }

                }

                FREE_BSS_EVENT( ptr_elem );
                ptr_elem = NULL;

                ptr_listelem = pacch_sq[tn]->get_head();
                if ( !ptr_listelem)
                    break;
            }

            if ( ((pacch_sq[tn]->get_list_num())%4) != 0  ) {
                printf("MergeAck(): the number of remaining ACK bursts is not the multiple of four. The number is %ld.\n",
                     (pacch_sq[tn]->get_list_num()) );
                exit(1);
            }

            ptr_listelem = pacch_sq[tn]->get_head();
            ptr_elem = ptr_listelem->get_elem();
            ASSERTION( ptr_elem , "TbfDes::merge_ack(): ptr_elem(): is null. (4)\n");

            bss_message* bmsg    = reinterpret_cast<bss_message*> (ptr_elem->DataInfo_);
            ASSERTION( bmsg , "TbfDes::merge_ack(): bss messsage is null. \n");
            mac_option*  mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
            ASSERTION( mac_opt , "TbfDes::merge_ack(): mac option is null. \n");
            rlc_option*  rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
            ASSERTION( rlc_opt , "TbfDes::merge_ack(): rlc option is null. \n");

            if ( mac_opt->burst_num != 0 ) {
                printf("TbfDes::merge_ack(): the number of bursts does not match the boundary of a block. (3)\n");
                exit(1);
            }


            for ( ulong j=0 ; j<3 ; ++j ) {

                if ( ptr_listelem)
                    ptr_listelem = ptr_listelem->get_next();
                else {
                    printf("TbfDes");
                    exit(1);
                }
            }

            ptr_elem = ptr_listelem->get_elem();
            ASSERTION( ptr_elem , "TbfDes::merge_ack(): ptr_elem(): is null. (5)\n");

            bmsg    = reinterpret_cast<bss_message*> (ptr_elem->DataInfo_);
            ASSERTION( bmsg , "TbfDes::merge_ack(): bss messsage is null. \n");
            mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
            ASSERTION( mac_opt , "TbfDes::merge_ack(): mac option is null. \n");
            rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
            ASSERTION( rlc_opt , "TbfDes::merge_ack(): rlc option is null. \n");


            if ( (mac_opt->msg) &&
                (mac_opt->msg_type == NDMSG) &&
                ((rlc_opt->ctl_msg_type == PKT_UPLINK_ACKNACK) && (rlc_opt->direction == 1)) ) {

                if ( mac_opt->burst_num != 3 ) {
                    printf("TbfDes::merge_ack(): The control message is carried by an incorrect burst. \n");
                    exit(1);
                }

                NDistrMsg* nmsg = reinterpret_cast<NDistrMsg*> (mac_opt->msg);

                PUACK* puack = reinterpret_cast<PUACK*> (nmsg->get_nondistr_msg());

                AckNack* ack_nack_des = puack->get_acknack();

                /* Read RBB (Received Block Bitmap) */
                uchar* rbb   = ack_nack_des->get_rbb();

                for ( ulong k=0 ; k<64 ;++k) {
                    rbb[k] = merged_rbb[k];
                }

            }


        }

        delete ssn_table;
        delete merged_rbb;


        return 1;
    }

    int TbfDescription::reset_for_roaming() {

        nid                     = 0;
        /* 0 stands for sender, 1 stands for receiver , 2 stands for single-block allocation */
        sr_bit                  = gprs::sender;

        state                   = CHANNEL_REQ_PHASE;

        tfi                     = UNASSIGNED;

        detach_flag             = false;
        llgmm_reset_flag        = false;
        uplink_freq             = -1;
        downlink_freq           = -1;

        for (int i=0; i<8; ++i )
            ts_alloc[i] = 0;

        ch_coding_scheme  = 1;
        tbf_starting_time = -1;

        ch_req_cnt      = 0;     /* channel request count */

        if ( stored_req )
            delete stored_req;

        stored_req = NULL;


        /* Uplink transmission parameters */
        if ( usf_granted_ind )
            usf_granted_ind->clear();

        memset( usf_tn , gprs::usf_unused, 8);

        if ( ta_ie )
            delete ta_ie ;

        ta_ie = NULL;

        /* PACCH description:
        * (1)PktResourceRequest (single block assignment)
        * (2)ControlACKs (indicated by RRBP)
        */
        /*pacch_blkn  = -1;
        pacch_tn    = -1;*/

        for ( int i=0 ; i<12 ;++i ) {

            for ( int j=0; j<8 ; ++j )
                pacch_blkn_bitmap[i][j] = 0;

        }

        /* Single Block allocation */
        if ( sbafreq )
            delete sbafreq;

        sbafreq             = NULL;
        sba_in_transmission = 0;
        tbf_starting_time_count_down_value = 0;

        /* Packet Data Traffic Channels */
        for ( int i=0; i<8 ;++i ) {

            if ( pdtch_sq[i] )
                pdtch_sq[i]->flush();

            if ( pacch_sq[i] )
                pacch_sq[i]->flush();
        }

        /* Dedicated control channels */

        if ( pacch_pending_event_sq )
            pacch_pending_event_sq->flush();;

        rrbp_cur = gprs::rrbp_unused;

        return 1;
    }

    long TbfControlBlock::compute_start_bsn(rlc_option* rlc_opt ) {

        ASSERTION( rlc_opt, "TCB: compute_start_bsn(): parameter 'rlc option' is null.\n");

        long tmp = rlc_opt->user_data_bsn - rlc_opt->blk_position;
        if ( tmp < 0 )
            tmp += GPRS_SNS;

        /* range test */
        if ( tmp >=128 ) {
            cout << "partial queue: start_bsn is larger than 128." << endl;
            exit(1);
        }

        return tmp;
    }

