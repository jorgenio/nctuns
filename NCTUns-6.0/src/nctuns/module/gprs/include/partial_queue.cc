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

#ifndef __PARTIAL_QUEUE_IMPL_CC
#define __PARTIAL_QUEUE_IMPL_CC
#include <stdlib.h>
#include <packet.h>
#include <timer.h>
#include <nctuns_api.h>
#include <gprs/include/partial_queue.h>
#include <gprs/include/burst_type.h>
#include <gprs/include/bss_message.h>
#include <gprs/include/generic_list_impl.h>
#include <gprs/include/GPRS_rlcmac_message.h>
#include <gprs/rlc/rlc_shared_def.h>

#ifdef  __DMALLOC_DEBUG__
#include <dmalloc.h>
#endif

#define __KEEP_QUIET
//#define __SHOW_HISTORY_LIST_STATUS

    using namespace std;

    /******** partially_assembled_llcframe_list_elem ********/

    int partially_assembled_llcframe_list_elem::dump() {

        printf("#----- partially_assembled_llcframe_list_elem Content Display -----#\n");
        //printf("Packet Addr = %ld \n", pkt_addr_ );
        printf("start bsn = %d \n " , (int)(start_bsn_) );
        printf("blk_cnt   = %d \n " , (int)(blk_cnt_) );
        printf("reassembled timestamp = %lld \n " , GetCurrentTime() );

        return 1;
    }

    HisLlcFrElem* partially_assembled_llcframe_list_elem::pkt_history_rec(HistoryAssembledLlcFrameElem* elem , Packet* pkt_addr ) {

        ASSERTION( elem , "pallcfr_list_elem::pkt_history_rec(): elem is null.\n");

        elem->write( (GetCurrentTime()) , start_bsn_ , blk_cnt_ , pkt_addr );

        return elem;

    }

    partially_assembled_llcframe_list_elem::partially_assembled_llcframe_list_elem
        ( uchar start_bsn , uchar blk_cnt , ulong sndcp_hl , ulong llc_hl , ulong user_dl , u_int64_t pkt_ts )
    {
        if ( !range_test(start_bsn) ) {
             printf("assertion: Fatal Error: partially assembled llcframe with illegal start bsn \n");
             return ;
        }
        else {
             start_bsn_ = start_bsn;

             if (!blk_cnt) {
                 printf("assertion: Fatal Error: partially assembled llcframe with blkcnt being 0 \n");
                 return ;
             }
             else {
                 blk_cnt_   = blk_cnt;
                 tmp_ring   = new uchar*[blk_cnt_];
             }
        }

        received_cnt_       = 0 ;
        sndcp_header_len_   = sndcp_hl;
        llc_header_len_     = llc_hl;
        user_data_len_      = user_dl;
        partial_event       = NULL;
        pkt_time_stamp      = pkt_ts;


        for (int i=0 ; i< blk_cnt_ ;++i)
            tmp_ring[i] = NULL ;
    }

    int partially_assembled_llcframe_list_elem::search(uchar bsn) {

        uchar position;

        if ( (start_bsn_ + blk_cnt_) < GPRS_SNS ) {
            position = bsn - start_bsn_;
            if (!tmp_ring[position])
                return -1;
            else
                return  position; /* return its position in a llc frame */
        }
        else {

            if ( bsn > start_bsn_ )
                position = bsn - start_bsn_;
            else
                position = (GPRS_SNS - start_bsn_) + bsn ;

            if (!tmp_ring[position])
                return -1;
            else
                return  position; /* return its position in a llc frame */
        }

    }

    int partially_assembled_llcframe_list_elem::release_partial_event() {

        if ( partial_event ) {

            bss_message* bss_msg = reinterpret_cast<bss_message*>(partial_event->DataInfo_);

            if ( bss_msg->packet ) {

                if ( bss_msg->packet) {
                    remove_upper_layer_pkt(bss_msg->packet);
                    free_pkt(bss_msg->packet);
                    bss_msg->packet     = NULL;
                }

            }

            FREE_BSS_EVENT(partial_event);
        }

        return 1;

    }

    int partially_assembled_llcframe_list_elem::insert(uchar bsn, uchar* decoded_data, u_int64_t cur_pkt_ts ) {


        if ( cur_pkt_ts != pkt_time_stamp ) {

            printf("partial_llcframe_list::insert(): pkt_time_stamps do not match.\n");

            return -10;
        }

        if ( (start_bsn_ + blk_cnt_) < GPRS_SNS ) {

            /* range test */
            if ( (bsn < start_bsn_) || (bsn >= (start_bsn_+blk_cnt_)) ) {
                printf("partially assembled llc frame list: Assertion failed: bsn is out of range. (case1) \n");
                exit(1);
            }

            uchar position;
            position = bsn - start_bsn_;
            if ( !tmp_ring[position] ) {
                tmp_ring[position] = decoded_data;
                ++received_cnt_;
                return 1;
            }
            else {
                #ifndef __KEEP_QUIET
                printf("partial_llcframe_list: [Warning] The received data with bsn = %d is received again (case1). \n", bsn );
                #endif
                delete decoded_data;
                return -1;
            }

        }

        else { /* In the case that (start_bsn_+blk_cnt_) >= GPRS_SNS */

            /* range test */
            if ( (bsn < start_bsn_) && (bsn >= (start_bsn_ + blk_cnt_)%GPRS_SNS) ) {
                printf("partially assembled llc frame list: Assertion failed: bsn is out of range. (case2) \n");
                exit(1);
            }

            uchar position;
            if ( bsn >= start_bsn_ )
                position = bsn - start_bsn_;
            else
                position = (GPRS_SNS - start_bsn_) + bsn ;

            if ( !tmp_ring[position] ) {
                tmp_ring[position] = decoded_data;
                ++received_cnt_;
                return 1;
            }
            else {
                #ifndef __KEEP_QUIET
                printf("partial_llcframe_list: [Warning] The received data with bsn = %d is received again (case2). \n", bsn );
                #endif
                return -1;
            }

        }

        return 1;
    }

    int partially_assembled_llcframe_list_elem::create_partial_event( Event* ep) {

        ASSERTION( ep , "PAAL elem::create_partial_event(): ep is null. \n");

        CREATE_BSS_EVENT( partial_event );

        bss_message* msg    = reinterpret_cast<bss_message*> (ep->DataInfo_);
        bss_message* msg1   = reinterpret_cast<bss_message*> (partial_event->DataInfo_);

        /* copy the out-of-band information to this */
        if ( !msg->llc_option)
        ASSERTION( msg->llc_option , "TCB fast_reassemble(): llc_option is NULL." );

        if ( msg->sndcp_option ) {
            msg1->sndcp_option       = new char[ msg->sndcp_option_len ];
            msg1->sndcp_option_len   = msg->sndcp_option_len ;
            memcpy ( msg1->sndcp_option , msg->sndcp_option , msg->sndcp_option_len );
        }

        if ( msg->llc_option ) {
            msg1->llc_option         = new char[msg->llc_option_len];
            msg1->llc_option_len     = msg->llc_option_len;
            memcpy ( msg1->llc_option , msg->llc_option , msg->llc_option_len );
        }

        if ( msg->packet )
            msg1->packet = msg->packet;

        return 1;
    }

    partially_assembled_llcframe_list_elem::~partially_assembled_llcframe_list_elem() {

        for (int i=0 ; i< blk_cnt_ ;++i) {
            if (tmp_ring[i]) {
                //delete tmp_ring[i] ;
                tmp_ring[i] = NULL;
            }
        }
        delete tmp_ring;
        /* partial event is created when the last block is received. Thus,
         * it is not necessary to release it.
         */
        partial_event = NULL;
        return ;
    }

    Event* partially_assembled_llcframe_list_elem::fast_assembling(ulong datasize) {

        if ( received_cnt_ == blk_cnt_ ) {

            /* create an event corresponding the llc frame */
            Event* eframe = partial_event;

            bss_message *msg    = reinterpret_cast<bss_message*> (eframe->DataInfo_);
            msg->flag           = PF_RECV;

            char *sndcp_header  = new char[sndcp_header_len_];
            char *llc_header    = new char[llc_header_len_];
            char *user_data     = new char[user_data_len_];
            char *buf           = new char[sndcp_header_len_+llc_header_len_+user_data_len_];

            ulong total_frame_len_ = (sndcp_header_len_ + llc_header_len_ + user_data_len_) ;
            bzero(sndcp_header  , sndcp_header_len_);
            bzero(llc_header    , llc_header_len_);
            bzero(user_data     , user_data_len_);
            bzero(buf           , total_frame_len_ );

            msg->sndcp_header_len   = sndcp_header_len_;
            msg->llc_header_len     = llc_header_len_;
            msg->user_data_len      = user_data_len_;


            for (int i=0 ; i<blk_cnt_ ; ++i ) {

                if ( i<(blk_cnt_-1))
                    memcpy(buf+i*datasize, tmp_ring[i] , datasize);
                else {
                    int tail_len = total_frame_len_ - (blk_cnt_-1)*datasize;
                    memcpy(buf+i*datasize, tmp_ring[i] , tail_len );
                }

                if ( tmp_ring[i]) {
                    free(tmp_ring[i]);
                    tmp_ring[i] = NULL;
                }


            }

            memcpy(sndcp_header , buf                                       , sndcp_header_len_ );
            memcpy(llc_header   , buf+ sndcp_header_len_                    , llc_header_len_ );
            memcpy(user_data    , buf+ sndcp_header_len_ + llc_header_len_  , user_data_len_ );

            msg->sndcp_header   = sndcp_header;
            msg->llc_header     = llc_header;
            msg->user_data      = reinterpret_cast<char*> (user_data);

            delete buf;

            partial_event = NULL;


            return eframe;
        }
        else if  ( received_cnt_ > blk_cnt_ ) {
            cout << "partial_queue_elem fast_assembling(): received_cnt_ > blk_cnt" << endl;
            exit(1);
        }
        else {
            return NULL;
        }
    }

    /***************************************************************
     *** The implementation of partially assembled llcframe list ***
     ***************************************************************/

    int partially_assembled_llcframe_list::his_pkt_insert( HisLlcFrElem* elem ) {

        return his_fr_list->insert(elem);

    }

    partially_assembled_llcframe_list::partially_assembled_llcframe_list() {

        his_fr_list = new HisFrList;
        int size    = GPRS_SNS;
        list_       = new partially_assembled_llcframe_list_elem*[size];

        for ( int i=0 ; i<size ; ++i ) {
            list_[i] = NULL;
        }
    }

    partially_assembled_llcframe_list::~partially_assembled_llcframe_list() {

        int size = GPRS_SNS;

        if (list_) {
            for ( int i=0 ; i<size ; ++i ) {
                if (list_[i])
                    delete list_[i];
            }
        }
        delete list_;
        delete his_fr_list;
        his_fr_list = NULL;
        list_ = NULL;
    }

    int partially_assembled_llcframe_list::create(uchar start_bsn ,
        uchar blk_cnt,ulong sndcp_hl , ulong llc_hl , ulong user_dl, u_int64_t pkt_ts ) {

        int size = GPRS_SNS;

        if ( blk_cnt > size ) {
            printf("partially assembled llcframe list: Assertion failed: too large frame with block count = %d \n", blk_cnt );
            exit(1);
        }

        if ( list_[start_bsn] ) {
            printf("partially assembled llcframe list: Assertion failed: the bucket %d is already used \n", start_bsn);
            exit(1);
        }

        list_[start_bsn] =
            new partially_assembled_llcframe_list_elem(start_bsn,blk_cnt,sndcp_hl,llc_hl,user_dl,pkt_ts);

        return 1;
    }

    int partially_assembled_llcframe_list::destroy(uchar start_bsn) {

        delete list_[start_bsn];
        list_[start_bsn] = NULL;
       return 1;
    }

    partially_assembled_llcframe_list_elem* partially_assembled_llcframe_list::get_list(uchar index) {

        if (index>= GPRS_SNS ) {
            cout << "partially_assembled_llcframe_list::get_list() index out of range. index = " << (int)index << endl;
            exit(1);
        }

        return list_[index];

    }

    ulong partially_assembled_llcframe_list::get_blk_cnt( uchar sbsn ) {

        return (list_[sbsn]->get_blk_cnt() );

    }






/* -------------------------------------------------------- */

    partially_assembled_rlcblk::partially_assembled_rlcblk()  {

        burst_cnt_      = 4;
        tmp_ring        = new Event*[burst_cnt_];
        received_cnt_   = 0 ;
        use_indicator   = false;
        for (int i=0 ; i< burst_cnt_ ;++i)
            tmp_ring[i] = NULL ;


    }

    int partially_assembled_rlcblk::insert(uchar burst_num, Event* ep ) {

        if ( burst_num >=burst_cnt_ ) {
            cout <<"Partial rlcblk::insert(): illegal burst_num=" << (int) (burst_num) << endl;
        }

        /* perform received burst sequence checking */
        uchar in_complete_flag = false;
        for ( int i=0 ; i<burst_num ;++i ) {
            if ( !tmp_ring[i] ) {
                in_complete_flag = true;
            }
        }

        if ( in_complete_flag ) {

            for ( int i=0 ; i<4 ;++i ) {

                if ( tmp_ring[i] ) {

                    /*bss_message* bmsg = reinterpret_cast<bss_message*> (tmp_ring[i]->DataInfo_);
                    mac_option* mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);

                    if ( (mac_opt->burst_type == NORMAL_BURST) || (mac_opt->burst_type == CTL_BURST) ) {

                        NB* nb_p = reinterpret_cast<NB*> (bmsg->user_data);
                        if ( nb_p )
                            delete nb_p;
                        bmsg->user_data     = NULL;
                        bmsg->user_data_len = 0;

                    }

                    if ( mac_opt->msg ) {

                        shared_obj_dec_refcnt( mac_opt->msg_type , mac_opt->msg );
                        int res = shared_obj_release( mac_opt->msg_type , mac_opt->msg );
                        if ( res ) {
                            mac_opt->msg = 0;
                        }

                    }*/

                    FREE_BSS_EVENT(tmp_ring[i]);
                    tmp_ring[i] = NULL;
                    received_cnt_ = 0;
                    use_indicator = false;
                }
            }

            printf("Partial rlcblk::insert(): drop this burst because we missed the preceding ones for assembling\n");

            bss_message* bmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
            mac_option* mac_opt = (mac_option*) (bmsg->mac_option);
            rlc_option* rlc_opt = (rlc_option*) (bmsg->rlc_option);

            if ( mac_opt->burst_type == DUMMY_BURST) {
                printf("Dropped burst is dummy burst. It's safe.\n");
            }
            else {
                printf("Dropped burst type = %d. \n", mac_opt->burst_type);
                if ( rlc_opt ) {
                    printf("rlc_opt ctl_msg_type=%d direction=%d \n" , rlc_opt->ctl_msg_type, rlc_opt->direction );
                }
            }

            /*if ( mac_opt->burst_type == NORMAL_BURST ) {

                NB* nb_p = reinterpret_cast<NB*> (bmsg->user_data);
                if ( nb_p )
                    delete nb_p;
                bmsg->user_data     = NULL;
                bmsg->user_data_len = 0;

            }

            if ( mac_opt->msg ) {

                shared_obj_dec_refcnt( mac_opt->msg_type , mac_opt->msg );
                int res = shared_obj_release( mac_opt->msg_type , mac_opt->msg );
                if ( res ) {
                    mac_opt->msg = 0;
                }

            }*/


            FREE_BSS_EVENT(ep);
            return 0;

        }

        if ( !tmp_ring[burst_num] ) {
            tmp_ring[burst_num] = ep;
            ++received_cnt_;
            use_indicator = true;
            return 1;
        }
        else {

            printf("partially assembled rlcblk::insert(): Assertion failed: the inserted slot is not empty\n");
            cout <<"duplicated burst_num = " << static_cast<int> (burst_num) << endl;
            bss_message* bmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
            rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bmsg->rlc_option);
            if (rlc_opt)
                cout << "burst tfi = " << static_cast<int> (rlc_opt->tfi) << " burst ctlmsg type=" << rlc_opt->ctl_msg_type << endl;

            mac_option* mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
            if (mac_opt) {

                cout << "burst type = "<< static_cast<int> (mac_opt->burst_type) << endl;
                if ( mac_opt->msg ) {

                    shared_obj_dec_refcnt( mac_opt->msg_type , mac_opt->msg );
                    int res = shared_obj_release( mac_opt->msg_type , mac_opt->msg );
                    if ( res ) {
                        mac_opt->msg = 0;
                    }

                }
            }


            if ( mac_opt->burst_type == NORMAL_BURST ) {
                NB* nb_p = reinterpret_cast<NB*> (bmsg->user_data);
                if ( nb_p )
                    delete nb_p;
                bmsg->user_data     = NULL;
                bmsg->user_data_len = 0;
            }
            FREE_BSS_EVENT(ep);

            return 0;
        }

    }

    partially_assembled_rlcblk::~partially_assembled_rlcblk() {
        for (int i=0 ; i< burst_cnt_ ;++i) {
            if (tmp_ring)
                delete tmp_ring[i] ;
                tmp_ring[i] = NULL;
        }
        delete tmp_ring;
        tmp_ring = NULL;
        return ;
    }

    int partially_assembled_rlcblk::flush() {

        for (int i=0 ; i< burst_cnt_ ;++i) {

            if (tmp_ring[i]) {

                //printf("Free ep addr = %x \n" , tmp_ring[i] );
                bss_message* bmsg = reinterpret_cast<bss_message*> (tmp_ring[i]->DataInfo_);
                if ( bmsg ) {

                    if ( bmsg->user_data) {
                        mac_option* mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
                        if ( (mac_opt->burst_type == NORMAL_BURST) || (mac_opt->burst_type == CTL_BURST) ) {
                            NB* nb_p = reinterpret_cast<NB*> (bmsg->user_data);
                            delete nb_p;
                            bmsg->user_data = NULL;
                            bmsg->user_data_len = 0;
                        }

                    }

                    mac_option* bs_mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
                    shared_obj_dec_refcnt( bs_mac_opt->msg_type , bs_mac_opt->msg );
                    int res = shared_obj_release( bs_mac_opt->msg_type , bs_mac_opt->msg );
                    if ( res ) {
                        bs_mac_opt->msg = 0;
                    }


                    if ( bmsg->packet ) {
                        remove_upper_layer_pkt(bmsg->packet);
                        free_pkt(bmsg->packet);
                        bmsg->packet     = NULL;
                    }
                }

                FREE_BSS_EVENT(tmp_ring[i]);
                tmp_ring[i] = NULL;
            }

        }

        received_cnt_ = 0;
        use_indicator = false;
        return 1;
    }

    Event* partially_assembled_rlcblk::fast_assembling() {

        if ( received_cnt_ == burst_cnt_ ) {

            for (int i=0 ; i<burst_cnt_ ;++i) {
                /* this assertion shall not occur since the the fast_assembling function
                 * is invoked when the burst_num >= 3.
                 */

                IS_NULL_STR(tmp_ring[i],"Partially burst queue() fast_assembling: Not receive all of four bursts.\n",NULL);
                IS_NULL_STR(tmp_ring[i]->DataInfo_,"Partially burst queue() fast_assembling: DataInfo_ is empty\n",NULL);
            }

            /* take the data carried by four bursts */
            bss_message* recv_bssmsg    = reinterpret_cast<bss_message*> (tmp_ring[3]->DataInfo_);
            bss_message* first_bssmsg   = reinterpret_cast<bss_message*> (tmp_ring[0]->DataInfo_);

            /* decode the data according to its burst_type */
            mac_option* mac_opt = reinterpret_cast<mac_option*> (recv_bssmsg->mac_option);

            Event* erlcblk;
            CREATE_BSS_EVENT(erlcblk);
            bss_message* bss_msg = reinterpret_cast<bss_message*> (erlcblk->DataInfo_);
            bss_msg->flag        = PF_RECV;

            copy_bss_msg_options  (bss_msg, recv_bssmsg );
            copy_bss_msg_headers  (bss_msg, recv_bssmsg );



            if ( mac_opt->burst_type == NORMAL_BURST ) {

                NB* nb = reinterpret_cast<NB*> (first_bssmsg->user_data);
                IS_NULL_STR(erlcblk,"Partially burst queue() fast_assembling: erlcblk is null\n",NULL);
                IS_NULL_STR(bss_msg,"Partially burst queue() fast_assembling: bss_msg is null\n",NULL);

                bss_msg->user_data  = reinterpret_cast<char*>(nb->get_data_ptr1());
                bss_msg->packet     = recv_bssmsg->packet;


            }
            else if ( mac_opt->burst_type == DUMMY_BURST ) {
                ;
            }

            else if ( mac_opt->burst_type == CTL_BURST) {
                bss_msg->user_data  = reinterpret_cast<char*> (mac_opt->msg);
                bss_msg->packet     = recv_bssmsg->packet;

            }
            else if ( mac_opt->burst_type == ACCESS_BURST ) {
                cout << "partial_rlcmac_blk_elem fast_assembling(): The processing for access bursts is not implemented so far. "
                    << endl;
                return NULL;
            }
            else {
                cout << "partial_rlcmac_blk_elem fast_assembling(): Unknown burst type. Fast-assembing failed." << endl;
                return NULL;
            }


            /* free the four bursts and make the tmp_ring pointers to be NULL.
             * Unless the tmp_ring slots are always shown busy.
             */
            for ( int i=0 ; i<burst_cnt_ ; ++i) {

                if ( tmp_ring[i] ) {
                    //printf("Free ep addr = %x \n" , tmp_ring[i] );
                    bss_message* bmsg = reinterpret_cast<bss_message*> (tmp_ring[i]->DataInfo_);
                    if ( bmsg ) {

                        if ( bmsg->user_data) {
                            mac_option* mac_opt = reinterpret_cast<mac_option*> (bmsg->mac_option);
                            if ( (mac_opt->burst_type == NORMAL_BURST) || (mac_opt->burst_type == CTL_BURST) ) {
                                NB* nb_p = reinterpret_cast<NB*> (bmsg->user_data);
                                delete nb_p;
                                bmsg->user_data = NULL;
                                bmsg->user_data_len = 0;
                            }
                        }

                    }

                    FREE_BSS_EVENT(tmp_ring[i]);
                    tmp_ring[i] = NULL;
                }

            }

            /* the user_data and packet fields shall not be released */

            received_cnt_ = 0 ;
            use_indicator = false;

            return erlcblk;
        }
        else if ( received_cnt_ > burst_cnt_ ) {

            printf ("partially burst queue fast assemble(): Abnormality: recv_burst_cnt > indicated_burst_cnt \n");

            for ( int i=0 ; i<received_cnt_ ; ++i) {

                if ( tmp_ring[i] ) {
                    //printf("Free ep addr = %x \n" , tmp_ring[i] );
                    bss_message* bmsg = reinterpret_cast<bss_message*> (tmp_ring[i]->DataInfo_);
                    if ( bmsg ) {

                        if ( bmsg->user_data) {
                            delete bmsg->user_data;
                            bmsg->user_data = NULL;
                            bmsg->user_data_len = 0;
                        }


                        if ( bmsg->packet ) {
                            remove_upper_layer_pkt(bmsg->packet);
                            free_pkt(bmsg->packet);
                            bmsg->packet     = NULL;
                        }

                    }


                    FREE_BSS_EVENT(tmp_ring[i]);
                    tmp_ring[i] = NULL;
                }

            }

            /* the user_data and packet fields shall not be released */

            received_cnt_ = 0 ;
            use_indicator = false;

            return NULL;

        }
        else {

            if ( received_cnt_ == 0 ) {
                ;//return NULL;
            }
            else
                printf ("partially burst queue fast assemble(): Abnormality: recv_burst_cnt < indicated_burst_cnt \n");

            for ( int i=0 ; i<burst_cnt_ ; ++i) {

                if ( tmp_ring[i] ) {
                    //printf("Free ep addr = %x \n" , tmp_ring[i] );
                    bss_message* bmsg = reinterpret_cast<bss_message*> (tmp_ring[i]->DataInfo_);
                    if ( bmsg ) {

                        if ( bmsg->user_data) {
                            delete bmsg->user_data;
                            bmsg->user_data = NULL;
                            bmsg->user_data_len = 0;
                        }

                        if ( bmsg->packet ) {
                            remove_upper_layer_pkt(bmsg->packet);
                            free_pkt(bmsg->packet);
                            bmsg->packet     = NULL;
                        }

                    }


                    FREE_BSS_EVENT(tmp_ring[i]);
                    tmp_ring[i] = NULL;
                }

            }

            /* the user_data and packet fields shall not be released */

            received_cnt_ = 0 ;
            use_indicator = false;

            return NULL;
        }


    }


/****************************************************************************************************/
    HistoryFrameList::HistoryFrameList()  {
        elem_cnt        = 0;
        release_timer   = NULL;
        release_period  = 2;
        expire_time     = 10;
        head            = NULL;
        tail            = NULL;
    }

    int HisLlcFrElem::write( u_int64_t ts, uchar start_bsn,uchar blk_cnt, Packet* pkt_addr) {

        ts_         =   ts;
        start_bsn_  =   start_bsn;
        blk_cnt_    =   blk_cnt;
        pkt_addr_   =   pkt_addr;
        next        =   0 ;

        return 1;
    }

    int HisLlcFrElem::dump() {

        printf("#----- HisLlcFrElem Content Display -----#\n");
        printf("Packet Addr = %p \n", pkt_addr_ );
        printf("start bsn = %d \n " , (int)(start_bsn_) );
        printf("blk_cnt   = %d \n " , (int)(blk_cnt_) );
        printf("recv timestamp = %lld \n " , ts_ );

        return 1;
    }

    HistoryFrameList::~HistoryFrameList() {

        HisLlcFrElem* ptr;
        for ( ptr = head ; ptr ; ptr=ptr->next ) {

            delete ptr;

        }

        if ( release_timer ) {

            delete release_timer;
            release_timer = NULL;

        }

    }

    int HistoryFrameList::release() {

        HisLlcFrElem*   ptr;
        u_int64_t       cur_ts = GetCurrentTime();

        for ( ptr = head ; ptr ;  ) {

            u_int64_t  diff         = (cur_ts - ptr->ts_ );
            double     diff_sec     = 0;
            TICK_TO_SEC( diff_sec , diff  );

            if ( diff_sec >= ((double)(expire_time))  ) {

                if ( ptr == head ) {

                    head = ptr->next;

                    if ( head ) {
                        head->prev = NULL;
                    }

                    HisLlcFrElem*   tmp = ptr;
                    ptr = ptr->next;
                    delete tmp;
                    --elem_cnt;


                }
                else {

                    ptr->prev->next = ptr->next;
                    HisLlcFrElem*   tmp = ptr;
                    ptr = ptr->next;
                    delete tmp;
                    --elem_cnt;

                }

                //printf("HFList::release(): release hit.\n");

            }
            else {

                ptr = ptr->next;

            }

        }

        printf("HFList::release(): The number of elements is %ld \n", elem_cnt );
        return 1;

    }

    int HistoryFrameList::insert(HisLlcFrElem* elem) {

        ASSERTION( elem , "HisFrList::insert(): elem is null.\n");

        if ( !release_timer ) {

            release_timer = new timerObj;
            release_timer->init();
            release_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),(int(NslObject::*)(Event*))&HistoryFrameList::release);
            u_int64_t period_tick;
            SEC_TO_TICK(period_tick,release_period );
            release_timer->start( period_tick, period_tick);

        }

        HisLlcFrElem* res = search(elem->pkt_addr_);

        if ( res ) {

            if ( (res->start_bsn_ == elem->start_bsn_) && (res->blk_cnt_ == elem->blk_cnt_) ) {

                #ifdef __SHOW_HISTORY_LIST_STATUS
                printf("HisFrList::insert(): Detect a packet that has been received in the past.\n");
                res->dump();
                #endif
                return -1;

            }
            else {

                #ifdef __SHOW_HISTORY_LIST_STATUS
                printf("HisFrList::insert(): Detect a packet with the same address as that received in the pass.\n");
                printf("HisFrList::insert(): Delete the old entry.\n");
                #endif
                del_entry( res );
                delete res;

            }
        }

        if ( head == NULL ) {

            head = elem;
            tail = head;
            head->prev = NULL;
            tail->next = NULL;
            ++elem_cnt;

        }
        else {

            tail->next = elem;
            elem->prev = tail;
            elem->next = NULL;
            tail = elem;
            ++elem_cnt;

        }

        return 1;

    }

    int HistoryFrameList::del_entry (HisLlcFrElem* elem ) {

        HisLlcFrElem* ptr;

        ASSERTION ( elem , "HisFrList::del(): given elem is null.\n");

        if ( elem == head ) {

            head = elem->next;

            if ( !head )
                tail = head;

            --elem_cnt;
            return 1;

        }

        for ( ptr = head ; ptr ; ptr=ptr->next ) {

            if ( ptr->next == elem ) {

                ptr->next = elem->next;

                --elem_cnt;
                return 1;

            }

        }

        return 0;

    }


    HisLlcFrElem*   HistoryFrameList::search(Packet* pkt_addr) {

        HisLlcFrElem* ptr;

        for ( ptr = head ; ptr ; ptr=ptr->next ) {

            if ( pkt_addr == ptr->pkt_addr_ ) {
                return ptr;
            }

        }

        return NULL;

    }


    int HistoryFrameList::dump_elem( HisLlcFrElem* elem ) {
        elem->dump();
        return 1;
    }


#endif
