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
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include "GPRS_rlcmac_message.h"
#include <gprs/include/bss_message.h>
#include <gprs/include/burst_type.h>
#include <gprs/rlc/rlc_shared_def.h>
#include <timer.h>
#include <nctuns_api.h>
//#include <dmalloc.h>

using namespace std;

    SMsgList* shared_msg_list = NULL;

    SharedMsgList::SharedMsgList( ulong release_period , ulong expiry_time ) {
        release_period_ = release_period;
        expiry_time_    = expiry_time;
        release_timer   = NULL;

        dmsg_cnt= 0;
        ndmsg_cnt= 0;
        d_burst_cnt= 0;
        n_burst_cnt= 0;
    }

    /*SharedMsgList::~SharedMsgList();
    {

        if ( !msg )
            return 0;

        if ( msg_type == DMSG ) {

            DistributedMsg* dmsg = reinterpret_cast<DistributedMsg*> (msg);

            if ( !dmsg->get_refcnt() ) {
                delete dmsg;
                return 1;
            }
            else
                return 0;

        }
        else if ( msg_type == NDMSG ) {

            NonDistributedMsg* nmsg = reinterpret_cast<NonDistributedMsg*> (msg);

            if ( !nmsg->get_refcnt() ) {
                delete nmsg;
                return 1;
            }
            else
                return 0;

        }
        else if ( msg_type == DUMMYBURST ) {

            DB* db = reinterpret_cast<DB*> (msg);

            if ( !db->get_refcnt() ) {
                delete db;
                return 1;
            }
            else
                return 0;

        }
        else {
            printf("shared_obj_dec_refcnt(): unknown object type %ld. \n", msg_type);
            exit(1);
        }

    }*/

    int SharedMsgList::inc_dmsg_cnt() {
        ++dmsg_cnt;
        return 1;
    }
    int SharedMsgList::inc_ndmsg_cnt() {
        ++ndmsg_cnt;
        return 1;
    }
    int SharedMsgList::inc_d_burst_cnt() {
        ++d_burst_cnt;
        return 1;
    }
    int SharedMsgList::inc_n_burst_cnt() {
        ++n_burst_cnt;
        return 1;
    }
    int SharedMsgList::dec_dmsg_cnt() {
        --dmsg_cnt;
        return 1;
    }
    int SharedMsgList::dec_ndmsg_cnt() {
        --ndmsg_cnt;
        return 1;
    }
    int SharedMsgList::dec_d_burst_cnt() {
        --d_burst_cnt;
        return 1;
    }
    int SharedMsgList::dec_n_burst_cnt() {
        --n_burst_cnt;
        return 1;
    }

    int SharedMsgList::activate_timer() {

        if ( !release_timer ) {

            release_timer = new timerObj;
            release_timer->init();
            release_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),
                (int(NslObject::*)(Event*))&SharedMsgList::release);

            u_int64_t period_tick;
            SEC_TO_TICK(period_tick,release_period_ );
            release_timer->start( period_tick, period_tick);

        }


        return 1;
    }

    int SharedMsgList::release() {

        ListElem<SharedMsgElem>* ptr1;
        u_int64_t cur_ts = GetCurrentTime();
        double cur_sec;

        TICK_TO_SEC( cur_sec , cur_ts  );

        if ( cur_sec <= expiry_time_ ) {
            return 0;
        }

        for ( ptr1=head ; ptr1 ;  ) {

            SharedMsgElem* elem = ptr1->get_elem();
            if (elem) {

                u_int64_t diff_tick = cur_ts - (elem->get_ts());
                double    diff_sec  = 0;

                TICK_TO_SEC( diff_sec , diff_tick  );

                if ( ((elem->get_obj_type()) == DUMMYBURST) && ((elem->get_obj_subtype()) == DUMMY_BURST)
                        && (diff_sec >= ((double)(expiry_time_))) ) {

                    ListElem<SharedMsgElem>* tmp = ptr1;
                    ptr1=(ptr1->get_next());

                    remove_entry(tmp);
                    delete tmp;
                    shared_msg_list->dec_d_burst_cnt();

                    printf("SharedMsgList::release(): release an elem: remain_number = %ld.\n" , get_list_num() );

                }
                else {

                    /* There is no need to traverse all of the list because timestamps of elements
                     * in this list is in incremental order.
                     */

                    break;

                    //ptr1=(ptr1->get_next());
                }

            }
            else {
                printf("SharedMsgList::release(): the elem is null.\n");
                exit(1);
            }

        }

        printf("SharedMsgList::release(): Time: %lld sec. remain_number = %ld. d_burst_cnt=%lu n_burst_cnt=%lu dmsg_cnt=%lu ndmsg_cnt=%lu\n",
             (GetCurrentTime())/10000000, get_list_num(), d_burst_cnt, n_burst_cnt, dmsg_cnt, ndmsg_cnt );
        return 1;
    }

    int shared_obj_insert(SharedMsgElem* elem) {
        ASSERTION( elem , "shared_obj_insert(): Given object does not exist.\n" );

        if ( !shared_msg_list )
            shared_msg_list = new SMsgList(2,30);

        ASSERTION( shared_msg_list , "shared_obj_insert(): Shared object list does not exist.\n" );

        if ( !(shared_msg_list->is_rtimer_activated()) )
            shared_msg_list->activate_timer();

        /* statistics for analyzing memory leakage problem */
        if ( ((elem->get_obj_type()) == DUMMYBURST)&&((elem->get_obj_subtype()) == DUMMY_BURST))
            shared_msg_list->inc_d_burst_cnt();

        else if ( ((elem->get_obj_type()) == DUMMYBURST)&&((elem->get_obj_subtype()) == NORMAL_BURST))
            shared_msg_list->inc_n_burst_cnt();

        else if ( ((elem->get_obj_type()) == DMSG) )
            shared_msg_list->inc_dmsg_cnt();

        else if ( ((elem->get_obj_type()) == NDMSG) )
            shared_msg_list->inc_ndmsg_cnt();

        else
            ;

        return shared_msg_list->insert_tail( elem );
    }

    int shared_obj_remove_entry(SharedMsgElem* elem) {

        ASSERTION( elem , "shared_obj_remove_entry(): Given object does not exist.\n");
        ASSERTION( shared_msg_list , "shared_obj_remove_entry(): Shared object list does not exist.\n" );

        /* statistics for analyzing memory leakage problem */
        if ( ((elem->get_obj_type()) == DUMMYBURST)&&((elem->get_obj_subtype()) == DUMMY_BURST))
            shared_msg_list->dec_d_burst_cnt();

        else if ( ((elem->get_obj_type()) == DUMMYBURST)&&((elem->get_obj_subtype()) == NORMAL_BURST))
            shared_msg_list->dec_n_burst_cnt();

        else if ( ((elem->get_obj_type()) == DMSG) )
            shared_msg_list->dec_dmsg_cnt();

        else if ( ((elem->get_obj_type()) == NDMSG) )
            shared_msg_list->dec_ndmsg_cnt();

        else
            ;

        return shared_msg_list->remove_entry( elem );
    }

    DistributedContent::~DistributedContent() {
        return ;
        }


    int DistributedMsg::set_distr_msg(uchar msg_type, Direction dir, void* msg) {
        if ( msg_type <32 ) {
            cout << "msg_type is nondistributed!\n";
            return -1;
        }

        IS_NULL_STR(distr_content,"DistibutedMsg set_distr_msg(): distr_content is null.\n" , -1);
        IS_NULL_STR(msg,"DistibutedMsg set_distr_msg(): msg is null.\n" , -1);

        message_type    = msg_type;
        direction       = dir;
        distr_content->set_msg(msg);

        return 1;
    }

    int DistributedMsg::set_pgmode(uchar pgmode) {

        if (!distr_content)
            distr_content = new DistributedContent(pgmode);
        else
            distr_content->set_page_mode(pgmode);
        return 1;
    }

    uchar* DistributedMsg::pack() {
        IS_NULL_STR(distr_content,"DistrMsg pack(): the content field is NULL.",NULL);
        IS_NULL_STR(distr_content->msg,"DistrMsg pack(): 'content->msg' field is NULL.",NULL);
        uchar* tmp = NULL;
        try {
            if ( (message_type == PKT_ACCESS_REJECT ) ) {

            }
            else if ( (message_type == PKT_PAGING_REQUEST ) ) {

            }
            else if ( (message_type == PKT_PDCH_RELEASE) ) {

            }
            else if ( (message_type == PKT_PRACH_PARAM) ) {
            }
            else if ( (message_type == PKT_DOWNLINK_DUMMY_CONTROL_BLK) ) {
            }
            else if ( (message_type == PSI1) ) {
                Psi1* psi1msg = reinterpret_cast<Psi1*> (distr_content->msg);
                tmp = new uchar[18];
                bzero(tmp,18);
                tmp[0] = (distr_content->page_mode << 6 ) | (0x01<<5) | message_type;
                tmp[1] = (psi1msg->psi_repeat_period<<4) | psi1msg->psi_change_field;
                tmp[2] = (0x01<<7) | (psi1msg->psi_count_hr<<3) | psi1msg->pbcch_change_mark;
                tmp[3] = (psi1msg->measurement_order)<<7 | (psi1msg->psi_status_ind)<<6 | (psi1msg->psi_count_lr);

                /* global_powercontrol_parameters */

                tmp[4] = (0x0f  & psi1msg->global_power_ctl_param.pb)<<4 | (0x0f & psi1msg->global_power_ctl_param.alpha);
                tmp[5] = ((0x07 & psi1msg->global_power_ctl_param.t_avg_t)<<5) | (0x1f & psi1msg->global_power_ctl_param.t_avg_w);
                tmp[6] = ((0x0f & psi1msg->global_power_ctl_param.n_avg_l)<<4) |
                         ((0x01 & psi1msg->global_power_ctl_param.pc_meas_chan)<<3) |
                         ((0x01 & psi1msg->global_power_ctl_param.int_meas_channel_list_avail)<<2) |
                          (0x03 & psi1msg->global_power_ctl_param.t_avg_t);

                /* prach control param */
                memcpy( &tmp[7] , &psi1msg->prach_control_parameters.acc_ctl_class , 2 );

                for (int i=0 ; i<4 ; ++i) {
                    tmp[9] |= ((0x03 & psi1msg->prach_control_parameters.max_retrans[i])<< (i*2) );
                }
                tmp[10] = psi1msg->prach_control_parameters.tx_int | psi1msg->prach_control_parameters.s ;

                for  (int i=0 ; i<2 ; ++i) {
                    tmp[11+i] = ((0x0f & psi1msg->prach_control_parameters.persistence_level[i+1])<<4) |
                                (0x0f & psi1msg->prach_control_parameters.persistence_level[i]);
                }

                /* pccch org param and 1 bit of prach ctr param */
                tmp[13] = (0x01 << 7) |  /* this bit compensates for the persistence_level indicator in PrachControlParam */
                          ((0x01 & psi1msg->pccch_organization_parameters.bs_pcc_rel)<<6) |
                          ((0x03 & psi1msg->pccch_organization_parameters.bs_pbcch_blks)<<4) |
                          ((0x0f & psi1msg->pccch_organization_parameters.bs_pag_blks_res));

                tmp[14] = ((0x0f&psi1msg->pccch_organization_parameters.bs_prach_blks)<<4) |
                          ((0x03&psi1msg->gprs_cell_options.nmo)<<2) |
                          ((0x01&psi1msg->gprs_cell_options.access_burst_type)<<1) |
                            (0x01&psi1msg->gprs_cell_options.control_ack_type);

                tmp[15] = ((0x03&psi1msg->gprs_cell_options.drx_timer_max)<<6) |
                          ((0x07&psi1msg->gprs_cell_options.timer3192)<<3) |
                           (0x07&psi1msg->gprs_cell_options.timer3168) ;

                tmp[16] = ((0x07&psi1msg->gprs_cell_options.pan_dec)<<5)   |
                          ((0x0f&psi1msg->gprs_cell_options.bs_cv_max)<<1) |
                          (0x04&psi1msg->gprs_cell_options.drx_timer_max);

                tmp[17] = ((0x07&psi1msg->gprs_cell_options.pan_max)<<4) |
                          ((0x07&psi1msg->gprs_cell_options.pan_inc)<<1) |
                          0x01;
            }
            else if ( (message_type == PSI2) ) {
                Psi2* psi2msg = reinterpret_cast<Psi2*> (distr_content->msg);
                tmp = new uchar[9];
                bzero(tmp,9);
                tmp[0] = (distr_content->page_mode << 6 ) | (0x01<<5) | message_type;
                tmp[1] = ((0x07&psi2msg->psi2_count)<<5) |
                         ((0x07&psi2msg->psi2_index)<<2) |
                          (0x03&psi2msg->psi2_change_mark);
                tmp[2] = ((0x03&psi2msg->cell_identification.rac)<<6) |
                         ((0x1f&psi2msg->cell_identification.lai)<<1) |
                           0x01;
                tmp[3] = (0x03& static_cast<uchar> (psi2msg->cell_identification.cell_identity)) |
                         ((0xfc&psi2msg->cell_identification.rac)>>2);
                tmp[4] = (
                            (0x03 & static_cast<uchar>( (0xff00 & (psi2msg->cell_identification.cell_identity) ) >>8 ) )<<6) |
                            (0xfc & static_cast<uchar> (psi2msg->cell_identification.cell_identity)>>2) ;
                tmp[5] = 0x00 | (0x01 << 6) | (0xfc&(uchar)((0xff00&psi2msg->cell_identification.cell_identity)>>8));
                tmp[6] = ((uchar)(0x000f&psi2msg->arfcn)<<4) | (0x00 << 3) | (0x07&psi2msg->tsc);
                tmp[7] = ((uchar)(0x03f0&psi2msg->arfcn)>>4);
                tmp[8] = psi2msg->time_slot_allocation;
            }
            else if ( (message_type == PSI3) ) {

            /*
                Psi3* psi3msg = reinterpret_cast<Psi3*> (distr_content->msg);
                tmp = new uchar[9];
                bzero(tmp,9);

                tmp[0] = (distr_content->page_mode << 6 ) | (0x01<<5) | message_type;

                tmp[1] = ((0x01&psi3msg->serving_cell_params.exc_acc)<<7) |
                            ((0x01&psi3msg->serving_cell_params.cell_bar_access)<<6) |
                            ((0x0f&psi3msg->psi_bis_count)<<2) |
                            (0x03&psi3msg->psi3_change_mark);

                tmp[2] = ((0x03&psi3msg->serving_cell_params.gprs_ms_txpower_max_cch)<<6)|
                        (0x3f&psi3msg->serving_cell_params.gprs_rxlevel_access_min);

                tmp[3] = ((0x01&psi3msg->gen_cell_sel.c32_qual)<<7) |
                         ((0x01&psi3msg->gen_cell_sel.c31_hyst)<<6) |
                         ((0x03&psi3msg->serving_cell_params.multiband_reporting)<<4) |
                         0x00 <<2 |
                         ((0x1c&psi3msg->serving_cell_params.gprs_ms_txpower_max_cch)>>2);

                tmp[4] = 0x00 | ((0x07&psi3msg->gen_cell_sel.t_resel)<<4) |
                        ((0x01&psi3msg->gen_cell_sel.random_access_retry)<<3) |
                         ((0x07&psi3msg->gen_cell_sel.gprs_cell_reselect_hysteresis));
                */

            }
            else if ( (message_type == PSI3_BIS) ) {
            }
            else if ( (message_type == PSI4) ) {
            }
            else if ( (message_type == PSI5) ) {
            }
            else if ( (message_type == PSI6) ) {
            }
            else if ( (message_type == PSI7) ) {
            }
            else if ( (message_type == PSI8) ) {
            }
            else if ( (message_type == PSI13) ) {
            }
            else if ( (message_type == PSI14) ) {
            }
            else if ( (message_type == PSI3_TER) ) {
            }
            else if ( (message_type == PSI3_QUATER) ) {
            }
            else if ( (message_type == PSI15) ) {
            }
            else if ( (message_type == PSI16) ) {
            }
            else {
                cout << "DistributedMsg pack(): Unknown message type" << endl;
            }
            //distr_content->set_msg();
        }
        catch (std::exception& e) {
            cout << "Assertion failed DistMsg pack():" << e.what() << endl;
        }
        return tmp;
    }

    int DistributedMsg::unpack(uchar* ptr) {
        IS_NULL_STR(ptr,"DistributedMsg unpack(): ptr is null\n" ,-1);
        return 1;
    }

    int NonDistributedMsg::pack(uchar* ptr) {
        return 1;
    }

    int NonDistributedMsg::unpack(uchar* ptr) {
        return 1;
    }

    int NonDistributedMsg::set_pgmode(uchar pgmode) {

        if (!distr_content)
            distr_content = new DistributedContent(pgmode);
        else
            distr_content->set_page_mode(pgmode);
        return 1;
    }

    int NonDistributedMsg::set_distr_msg(void* msg) {
        if (!distr_content)
            return -1;
        else
            distr_content->set_msg(msg);
        return 1;
    }

    int NonDistributedMsg::set_nondistr_msg(uchar msg_type, Direction dir , void* msg1) {
        if ( msg_type >=32 ) {
            cout << "NDistrMsg set_nondistr_msg(): msg_type is distributed!\n";
            return -1;
        }

        IS_NULL_STR(msg1,"NonDistibutedMsg set_distr_msg(): msg1 is null.\n" , -1);

        message_type = msg_type;
        direction    = dir;
        nondistr_msg = msg1;
        return 1;
    }

    int NonDistributedMsg::set_addr_info(uchar field, GlobalTfi tfi_ie) {
        if (field!=GLOBAL_TFI_INDICATOR) {
            cout <<"NonDistributed Msg set_addr_info(): Error: the types of address and structure do not match. "
                 << "field =" << static_cast<int> (field) << endl;
            return -1;
        }
        addr_info = new AddressInfo(field,tfi_ie);
        IS_NULL_STR(addr_info, "NonDistrMsg::set_addr_info() : initialize addr_info_failed\n" , -1 );
        return 1;
    }

    int NonDistributedMsg::set_addr_info(uchar field, ushort value) {
        if (field != TQI_INDICATOR ) {
            cout <<"NonDistributed Msg set_addr_info(): Error: the types of address and structure do not match. "
                 << "field =" << static_cast<int> (field) << endl;
            return -1;
        }
        addr_info = new AddressInfo(field,value);
        IS_NULL_STR(addr_info, "NonDistrMsg::set_addr_info() : initialize addr_info_failed\n" , -1 );
        return 1;
    }

    int NonDistributedMsg::set_addr_info(uchar field, ulong value) {
        if (field!=TLLI_INDICATOR) {
            cout <<"NonDistributed Msg set_addr_info(): Error: the types of address and structure do not match. "
                 << "field =" << static_cast<int> (field) << endl;
            return -1;
        }
        addr_info = new AddressInfo(field,value);
        IS_NULL_STR(addr_info, "NonDistrMsg::set_addr_info() : initialize addr_info_failed\n" , -1 );
        return 1;
    }
    int NonDistributedMsg::set_addr_info(uchar field, PacketRequestReference req) {
        if (field!=PKT_REQ_REF_INDICATOR) {
            cout <<"NonDistributed Msg set_addr_info(): Error: the types of address and structure do not match. "
                 << "field =" << static_cast<int> (field) << endl;
            return -1;
        }
        addr_info = new AddressInfo(field,req);
        IS_NULL_STR(addr_info, "NonDistrMsg::set_addr_info() : initialize addr_info_failed\n" , -1 );
        return 1;
    }

    int NonDistributedMsg::get_addr_info(GlobalTfi* addr) {
        IS_NULL(addr,-1);
        if (addr_info->field_used == GLOBAL_TFI_INDICATOR ) {
            addr->direction     = addr_info->global_tfi_ie.direction;
             addr->tfi          = addr_info->global_tfi_ie.tfi;
             return 1;
        }
        return -1;
    }
    int NonDistributedMsg::get_addr_info(ulong* addr) {
        IS_NULL(addr,-1);
        if (addr_info->field_used == TLLI_INDICATOR ) {
             *addr = addr_info->tlli;
             return 1;
        }
        return -1;
    }
    int NonDistributedMsg::get_addr_info(ushort* addr) {
        IS_NULL(addr,-1);
        if (addr_info->field_used == TQI_INDICATOR ) {
             *addr = addr_info->tqi;
             return 1;
        }
        return -1;
    }
    int NonDistributedMsg::get_addr_info(PacketRequestReference* addr) {
        IS_NULL(addr,-1);
        if (addr_info->field_used == PKT_REQ_REF_INDICATOR ) {
             addr->ra_info      = addr_info->prr_ie.ra_info;
             addr->fn           = addr_info->prr_ie.fn;
             return 1;
        }
        return -1;
    }

    AckNack::AckNack() {
         final_ack_indication = 0;
         starting_sequence_number = 0;
         received_block_bitmap = NULL;
    }

    int AckNack::pack(uchar* buf) {
        if ( !received_block_bitmap ) {
            printf("ACKNACK pack(): received block bitmap is not initialized\n");
            return -1;
        }
        if ( !buf ) {
            printf("ACKNACK pack(): packing buffer is null\n" );
            return -1;
        }
        /* buf should have length with 9 bytes */
        //uchar* buf = new uchar[9];
        bzero(buf,9);
        buf[0] = ((final_ack_indication<<7) | (starting_sequence_number));
        uchar* ptr = &buf[1];
        for (int i=0 ; i<GPRS_SNS ; ++i) {
            ptr[i/8] |= (received_block_bitmap[i]<< (i%8) );
        }
        return 1;
    }

    int AckNack::unpack(uchar *buf) {
        if ( !buf ) {
            printf("ACKNACK pack(): packing buffer is null\n" );
            return -1;
        }
        final_ack_indication  = (buf[0] >>7);
        starting_sequence_number = (buf[0] & 0x80);
        for (int i=0 ; i<GPRS_SNS ; ++i) {
            received_block_bitmap[i] = ((buf[i/8] >> (i%8)) & 0x01 );
        }
        return 1;
    }

    PacketDownlinkAckNack::PacketDownlinkAckNack() {
        pfi     = 0;
        rb_id   = 0;
        ack_nack_description            = new AckNack;
        channel_request_description     = new ChannelRequestDescription;
        cqr                             = new ChannelQualityReport;
    }

    PacketDownlinkAckNack::~PacketDownlinkAckNack() {

        if ( ack_nack_description )
            delete ack_nack_description;
        if ( channel_request_description )
            delete channel_request_description;
        if ( cqr )
            delete cqr;

    }

    int PacketDownlinkAckNack::pack(uchar* buf) {
        return 1;
    }

    PacketUplinkAckNack::PacketUplinkAckNack() {
        channel_coding_command      = 1;
        contention_resolution_tlli  = 0;
        ack_nack_description    = new AckNack;
        packet_timing_advance   = new PacketTimingAdvance;
        power_control_params    = new PowerControlParameters;
    }

    PacketUplinkAckNack::~PacketUplinkAckNack(){

        if (ack_nack_description)
            delete ack_nack_description;
        if (packet_timing_advance)
            delete packet_timing_advance;
        if (power_control_params)
            delete power_control_params;

    }

    ushort PacketChannelRequest::pack() {
        ushort res;
        if (format_type == ONEPHASE_ACCESS) {
            res = 0;
            cout << "PktChReq pack(): one-phase access is not supported so far." << endl;
        }
        else if ( format_type == TWOPHASE_ACCESS ) {
            res = ( 0x00 | (0x30 << 5) | (priority<<3) | (0x03 & random_bits) );
        }
        else if ( format_type == FOR_PAGING_RESPONSE ) {
            res = 0x0 | (0x31<<5) | (0x1f & random_bits);
        }
        else {
            res = 0;
            cout << "PktChReq pack(): unsupported type = " << format_type << endl;
        }

        return res;
    }

    int PacketChannelRequest::unpack(ushort rainfo) {

        if (rainfo == 0) {
            format_type         = ONEPHASE_ACCESS;
            multislot_class     = (0x07e0 & rainfo)>>5;
            priority            = (0x0018 & rainfo)>>3;
            number_of_blocks    = 0; /* open-ended TBF */
            random_bits         = (0x0007 & rainfo);
            return -1;
        }
        else if ( (rainfo >>5) == 0x30 ) {
            format_type         = TWOPHASE_ACCESS;
            priority            = (0x0018 & rainfo)>>3;
            number_of_blocks    = 0; /* open-ended TBF */
            random_bits         = 0x0007 & rainfo;
        }
        else if ( (rainfo>>5) == 0x31) {
            format_type         = FOR_PAGING_RESPONSE;
            random_bits         = 0x001f & rainfo;
        }
        else
            ;
        length_type = ELEVEN_BITS;

        return 1;
    }

    PacketUplinkAssignment::~PacketUplinkAssignment(){
        if (frequency_params)   delete frequency_params;

        if (dynamic_allocation) delete dynamic_allocation;

        if (single_block_allocation) delete single_block_allocation;
    }

    PacketDownlinkAssignment::PacketDownlinkAssignment() {
        bzero(this,sizeof(PacketDownlinkAssignment));
        pkt_timing_advance  = new PacketTimingAdvance;
        power_ctl_param     = new PowerControlParameter;
        freq_param          = new FrequencyParameter;
    }

    PacketDownlinkAssignment::~PacketDownlinkAssignment() {
        if ( pkt_timing_advance)
            delete pkt_timing_advance;
        if (power_ctl_param)
            delete power_ctl_param;
        if (freq_param)
            delete freq_param;
    }
    DirectEncoding1Struct::DirectEncoding1Struct() {
        gprs_ma = new GprsMobileAllocation;
        maio    = 0;
    }

    DirectEncoding1Struct::~DirectEncoding1Struct() {
        if (gprs_ma)
            delete gprs_ma;
    }

    DirectEncoding2Struct::DirectEncoding2Struct() {
        bzero(this,sizeof(DirectEncoding2Struct));
        ma_freq_list = new MaFreqList;
    }

    DirectEncoding2Struct::~DirectEncoding2Struct() {
        if ( ma_freq_list )
            delete ma_freq_list;
    }

    FrequencyParameter::FrequencyParameter(uchar format1) {
        bzero(this,sizeof(FrequencyParameter));
        this->format = format1;
        if ( format == 0 ) {
            ;
        }
        else if ( format == 1) {
            indirect_encoding = new IndirectEncodingStruct;
        }
        else if ( format == 2) {
            direct_encoding1 = new DirectEncoding1Struct;
        }
        else if ( format == 3) {
            direct_encoding2 = new DirectEncoding2Struct;
        }
        else
            ;
    }

    FrequencyParameter::~FrequencyParameter() {
        if (indirect_encoding)
            delete indirect_encoding;
        if (direct_encoding1)
            delete direct_encoding1;
        if (direct_encoding2)
            delete direct_encoding2;
    }

/********************************************************************/
DistributedMsg::DistributedMsg(){

    message_type=255;
    distr_content=NULL;

    ref_cnt = 0;
    rec_entry_ptr = 0;
    rec_entry_ptr = new SharedMsgElem( (GetCurrentTime()) , DMSG , message_type , this );
    shared_obj_insert(rec_entry_ptr);

}

DistributedMsg::~DistributedMsg() {

    if (distr_content) {
        distr_content->free_msg( message_type , direction );
        delete distr_content;
    }
    distr_content = NULL;
    shared_obj_remove_entry( rec_entry_ptr );
    delete rec_entry_ptr;
    rec_entry_ptr = NULL;

}

/***********************************************************************/
NonDistributedMsg::NonDistributedMsg() {

    message_type    = 255;
    distr_content   = NULL;
    addr_info       = NULL;
    nondistr_msg    = NULL;

    ref_cnt         = 0;
    rec_entry_ptr   = 0;
    rec_entry_ptr   = new SharedMsgElem( (GetCurrentTime()) , NDMSG , message_type , this );
    shared_obj_insert(rec_entry_ptr);

}

NonDistributedMsg::~NonDistributedMsg() {

    if ( !ref_cnt ) {

        //printf("NDistMsg destructor is invoked.\n");
        free_ndmsg();

        if (distr_content) {
            distr_content->free_msg( message_type , direction );
            delete distr_content;
        }
        if (addr_info)
            delete addr_info;

        shared_obj_remove_entry( rec_entry_ptr );
        delete rec_entry_ptr;
        rec_entry_ptr = NULL;
    }

}

/**************************************************************************/

int NonDistributedMsg::free_ndmsg() {

    if ( !nondistr_msg )
        return 1;

    if ( direction == downlink ) {

        if ( message_type == PKT_ACCESS_REJECT ) {
            PAR* tmp = reinterpret_cast<PAR*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_CELL_CHANGE_ORDER ) {
            PacketCellChangeOrder* tmp = reinterpret_cast<PacketCellChangeOrder*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_DOWNLINK_ASSIGNMENT ) {
            PDA* tmp = reinterpret_cast<PDA*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_MEASUREMENT_ORDER ) {
            PacketMeasurementOrder* tmp = reinterpret_cast<PacketMeasurementOrder*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_PAGING_REQUEST ) {
            PacketPagingRequest* tmp = reinterpret_cast<PacketPagingRequest*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_PDCH_RELEASE ) {
            PacketPdchRelease* tmp = reinterpret_cast<PacketPdchRelease*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_POLLING_REQUEST ) {
            PacketPollingRequest* tmp = reinterpret_cast<PacketPollingRequest*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_POWER_CTL_TIMING_ADVANCE ) {
            PPCTA* tmp = reinterpret_cast<PPCTA*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_PRACH_PARAM ) {
            PacketPrachParameter* tmp = reinterpret_cast<PacketPrachParameter*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_QUEUEING_NOTIFICATION ) {
            PacketQueuingNotification* tmp = reinterpret_cast<PacketQueuingNotification*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_TIMESLOT_RECONFIGURATION ) {
            ;
        }

        else if ( message_type == PKT_TBF_RELEASE ) {
            PacketTbfRelease* tmp = reinterpret_cast<PacketTbfRelease*> (nondistr_msg);;
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_UPLINK_ACKNACK ) {
            PUACK* tmp = reinterpret_cast<PUACK*> (nondistr_msg);;
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_UPLINK_ASSIGNMENT ) {
            PUA* tmp = reinterpret_cast<PUA*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_CELL_CHANGE_CONTINUE ) {
            PacketCellChangeContinue* tmp = reinterpret_cast<PacketCellChangeContinue*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_NEIGHBOR_CELL_DATA ) {
            ;
        }

        else if ( message_type == PKT_SERVING_CELL_DATA ) {
            PacketServingCellData* tmp = reinterpret_cast<PacketServingCellData*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_DBPSCH_ASSIGNMENT ) {
            ;
        }

        else if ( message_type == MULTIPLE_TBF_DOWNLINK_ASSIGNMENT ) {
            ;
        }

        else if ( message_type == MULTIPLE_TBF_UPLINK_ASSIGNMENT ) {
            ;
        }

        else if ( message_type == MULTIPLE_TBF_TIMESLOT_RECONFIG ) {
            ;
        }

        else if ( message_type == PKT_DOWNLINK_DUMMY_CONTROL_BLK ) {
            ;
        }

        else if ( message_type == PSI1 ) {
            Psi1* tmp = reinterpret_cast<Psi1*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PSI2 ) {
            Psi2* tmp = reinterpret_cast<Psi2*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PSI3 ) {
            Psi3* tmp = reinterpret_cast<Psi3*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PSI3_BIS ) {
            ;
        }

        else if ( message_type == PSI3_TER ) {
            ;
        }

        else if ( message_type == PSI3_QUATER ) {
            ;
        }

        else if ( message_type == PSI4 ) {
            ;
        }

        else if ( message_type == PSI5 ) {
            ;
        }

        else if ( message_type == PSI6 ) {
            ;
        }

        else if ( message_type == PSI7 ) {
            ;
        }

        else if ( message_type == PSI8 ) {
            ;
        }

        else if ( message_type == PSI13 ) {
            ;
        }

        else if ( message_type == PSI14 ) {
            ;
        }

        else if ( message_type == PSI15 ) {
            ;
        }

        else if ( message_type == PSI16 ) {
            ;
        }
        else
            ;

    }

    else if ( direction == uplink ) {

        if ( message_type == PKT_CELL_CHANGE_FAILURE ) {
            PacketCellChangeFailure* tmp = reinterpret_cast<PacketCellChangeFailure*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_CONTROL_ACK ) {
            PktCtlAck* tmp = reinterpret_cast<PktCtlAck*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_DOWNLINK_ACKNACK ) {
            PDACK* tmp = reinterpret_cast<PDACK*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_UPLINK_DUMMY_CONTROL_BLK ) {
            ;
        }

        else if ( message_type == PKT_MEASUREMENT_REPORT ) {
            PacketMeasurementReport* tmp = reinterpret_cast<PacketMeasurementReport*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_ENHANCED_MEASUREMENT_REPORT ) {
            ;
        }

        else if ( message_type == PKT_MOBILE_TBF_STATUS ) {
            ;
        }

        else if ( message_type == PKT_PSI_STATUS ) {
            ;
        }

        else if ( message_type == EGPRS_PKT_DOWNLINK_ACKNACK ) {
            ;
        }

        else if ( message_type == PKT_PAUSE ) {
            ;
        }

        else if ( message_type == PKT_SI_STATUS ) {
            ;
        }

        else if ( message_type == PKT_RESOURCE_REQUEST ) {
            PacketResourceRequest* tmp = reinterpret_cast<PacketResourceRequest*> (nondistr_msg);
            if ( tmp ) delete tmp;
        }

        else
            ;

    }

    else {

        printf("Nondistr_msg free_nondistr_msg(): unknown direction.\n");
        exit(1);

    }

    nondistr_msg = NULL;
    return 1;

}

int DistributedContent::free_msg(uchar message_type , Direction direction ) {

    if ( !msg)
        return 1;

    if ( direction == downlink ) {

        if ( message_type == PKT_ACCESS_REJECT ) {
            PAR* tmp = reinterpret_cast<PAR*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_CELL_CHANGE_ORDER ) {
            PacketCellChangeOrder* tmp = reinterpret_cast<PacketCellChangeOrder*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_DOWNLINK_ASSIGNMENT ) {
            PDA* tmp = reinterpret_cast<PDA*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_MEASUREMENT_ORDER ) {
            PacketMeasurementOrder* tmp = reinterpret_cast<PacketMeasurementOrder*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_PAGING_REQUEST ) {
            PacketPagingRequest* tmp = reinterpret_cast<PacketPagingRequest*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_PDCH_RELEASE ) {
            PacketPdchRelease* tmp = reinterpret_cast<PacketPdchRelease*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_POLLING_REQUEST ) {
            PacketPollingRequest* tmp = reinterpret_cast<PacketPollingRequest*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_POWER_CTL_TIMING_ADVANCE ) {
            PPCTA* tmp = reinterpret_cast<PPCTA*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_PRACH_PARAM ) {
            PacketPrachParameter* tmp = reinterpret_cast<PacketPrachParameter*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_QUEUEING_NOTIFICATION ) {
            PacketQueuingNotification* tmp = reinterpret_cast<PacketQueuingNotification*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_TIMESLOT_RECONFIGURATION ) {
            ;
        }

        else if ( message_type == PKT_TBF_RELEASE ) {
            PacketTbfRelease* tmp = reinterpret_cast<PacketTbfRelease*> (msg);;
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_UPLINK_ACKNACK ) {
            PUACK* tmp = reinterpret_cast<PUACK*> (msg);;
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_UPLINK_ASSIGNMENT ) {
            PUA* tmp = reinterpret_cast<PUA*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_CELL_CHANGE_CONTINUE ) {
            PacketCellChangeContinue* tmp = reinterpret_cast<PacketCellChangeContinue*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_NEIGHBOR_CELL_DATA ) {
            ;
        }

        else if ( message_type == PKT_SERVING_CELL_DATA ) {
            PacketServingCellData* tmp = reinterpret_cast<PacketServingCellData*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_DBPSCH_ASSIGNMENT ) {
            ;
        }

        else if ( message_type == MULTIPLE_TBF_DOWNLINK_ASSIGNMENT ) {
            ;
        }

        else if ( message_type == MULTIPLE_TBF_UPLINK_ASSIGNMENT ) {
            ;
        }

        else if ( message_type == MULTIPLE_TBF_TIMESLOT_RECONFIG ) {
            ;
        }

        else if ( message_type == PKT_DOWNLINK_DUMMY_CONTROL_BLK ) {
            ;
        }

        else if ( message_type == PSI1 ) {
            Psi1* tmp = reinterpret_cast<Psi1*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PSI2 ) {
            Psi2* tmp = reinterpret_cast<Psi2*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PSI3 ) {
            Psi3* tmp = reinterpret_cast<Psi3*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PSI3_BIS ) {
            ;
        }

        else if ( message_type == PSI3_TER ) {
            ;
        }

        else if ( message_type == PSI3_QUATER ) {
            ;
        }

        else if ( message_type == PSI4 ) {
            ;
        }

        else if ( message_type == PSI5 ) {
            ;
        }

        else if ( message_type == PSI6 ) {
            ;
        }

        else if ( message_type == PSI7 ) {
            ;
        }

        else if ( message_type == PSI8 ) {
            ;
        }

        else if ( message_type == PSI13 ) {
            ;
        }

        else if ( message_type == PSI14 ) {
            ;
        }

        else if ( message_type == PSI15 ) {
            ;
        }

        else if ( message_type == PSI16 ) {
            ;
        }
        else
            ;

    }

    else if ( direction == uplink ) {

        if ( message_type == PKT_CELL_CHANGE_FAILURE ) {
            PacketCellChangeFailure* tmp = reinterpret_cast<PacketCellChangeFailure*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_CONTROL_ACK ) {
            PktCtlAck* tmp = reinterpret_cast<PktCtlAck*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_DOWNLINK_ACKNACK ) {
            PDACK* tmp = reinterpret_cast<PDACK*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_UPLINK_DUMMY_CONTROL_BLK ) {
            ;
        }

        else if ( message_type == PKT_MEASUREMENT_REPORT ) {
            PacketMeasurementReport* tmp = reinterpret_cast<PacketMeasurementReport*> (msg);
            if ( tmp ) delete tmp;
        }

        else if ( message_type == PKT_ENHANCED_MEASUREMENT_REPORT ) {
            ;
        }

        else if ( message_type == PKT_MOBILE_TBF_STATUS ) {
            ;
        }

        else if ( message_type == PKT_PSI_STATUS ) {
            ;
        }

        else if ( message_type == EGPRS_PKT_DOWNLINK_ACKNACK ) {
            ;
        }

        else if ( message_type == PKT_PAUSE ) {
            ;
        }

        else if ( message_type == PKT_SI_STATUS ) {
            ;
        }

        else if ( message_type == PKT_RESOURCE_REQUEST ) {
            PacketResourceRequest* tmp = reinterpret_cast<PacketResourceRequest*> (msg);
            if ( tmp ) delete tmp;
        }

        else
            ;

    }

    else {

        printf("DMSG free_msg(): unknown direction.\n");
        exit(1);

    }

    msg = NULL;
    return 1;

}

/*int DistributedMsg::free_dmsg() {

    if ( distr_content )
        distr_content->free_msg(message_type,direction);

    return 1;
}*/

void* DistributedMsg::get_distr_msg() {
    return distr_content->msg;
}

void* NonDistributedMsg::get_nondistr_msg() {
    return nondistr_msg;
}


/**************************************************************************/
SharedMsgElem::SharedMsgElem( u_int64_t ts1, ObjType obj_type1, ulong obj_subtype1, void* obj_ptr1 ) {

    ASSERTION( ts1      , "SMsgElem::SMsgElem(): Timestamp cannot be 0.\n");
    /*if ( !obj_ptr1) {

        //ASSERTION( obj_ptr1 , "SMsgElem::SMsgElem(): The address of shared object cannot be zero.\n");
        printf("1\n");
    }*/

    ts          = ts1;
    obj_type    = obj_type1;
    obj_subtype = obj_subtype1;
    obj_ptr     = obj_ptr1;


}

int SharedMsgElem::update_subtype( ulong subtype ) {

    obj_subtype = subtype;
    return 1;

}

int SharedMsgElem::update_obj_ptr( void* obj_ptr1 ) {

    ASSERTION( obj_ptr1 , "SMsgElem::update_obj_ptr(): The address of shared object cannot be zero.\n");

    obj_ptr = obj_ptr1;
    return 1;
}

/**************************************************************************/

int shared_obj_inc_refcnt( ObjType msg_type, void* msg) {

    if ( !msg )
        return 0;

    if ( msg_type == DMSG ) {

        DistributedMsg* dmsg = reinterpret_cast<DistributedMsg*> (msg);
        dmsg->inc_refcnt();
        return 1;

    }
    else if ( msg_type == NDMSG ) {

        NonDistributedMsg* nmsg = reinterpret_cast<NonDistributedMsg*> (msg);
        nmsg->inc_refcnt();
        return 1;

    }
    else if ( msg_type == DUMMYBURST ) {

        DB* db = reinterpret_cast<DB*> (msg);
        db->inc_refcnt();
        return 1;

    }
    else {
        printf("shared_obj_inc_refcnt(): unknown object type %d. \n", msg_type);
        exit(1);
    }

}

int shared_obj_dec_refcnt( ObjType msg_type, void* msg) {

    if ( !msg )
        return 0;

    if ( msg_type == DMSG ) {

        DistributedMsg* dmsg = reinterpret_cast<DistributedMsg*> (msg);
        dmsg->dec_refcnt();
        return 1;

    }
    else if ( msg_type == NDMSG ) {

        NonDistributedMsg* nmsg = reinterpret_cast<NonDistributedMsg*> (msg);
        nmsg->dec_refcnt();
        return 1;

    }
    else if ( msg_type == DUMMYBURST ) {

        DB* db = reinterpret_cast<DB*> (msg);
        db->dec_refcnt();
        return 1;

    }
    else {
        printf("shared_obj_dec_refcnt(): unknown object type %d. \n", msg_type);
        exit(1);
    }

}

int shared_obj_release( ObjType msg_type, void* msg) {

    if ( !msg )
        return 0;

    if ( msg_type == DMSG ) {

        DistributedMsg* dmsg = reinterpret_cast<DistributedMsg*> (msg);

        if ( !dmsg->get_refcnt() ) {
            delete dmsg;
            return 1;
        }
        else
            return 0;

    }
    else if ( msg_type == NDMSG ) {

        NonDistributedMsg* nmsg = reinterpret_cast<NonDistributedMsg*> (msg);

        if ( !nmsg->get_refcnt() ) {
            delete nmsg;
            return 1;
        }
        else
            return 0;

    }
    else if ( msg_type == DUMMYBURST ) {

        DB* db = reinterpret_cast<DB*> (msg);

        if ( !db->get_refcnt() ) {
            delete db;
            return 1;
        }
        else
            return 0;

    }
    else {
        printf("shared_obj_release(): unknown object type %d. \n", msg_type);
        exit(1);
    }

}

