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

#ifndef __STATE_ARRAY_CC
#define __STATE_ARRAY_CC


#include <stdlib.h>
#include <event.h>
#include <nctuns_api.h>
#include "state_array.h"
#include <gprs/include/bss_message.h>

//#define RECV_BUF_VN_DEBUG
//#define __TAIL_DEBUG
//#define __DELELE_PKT_DEBUG
    using namespace std;

    int remove_upper_layer_pkt(Packet* pkt);

    AckStateArrayElem::AckStateArrayElem() {
        state               = INVALID;
        bsn                 = -1;
        upper_layer_pdu     = NULL;
        last_sent_timestamp = 0;
    }

    AckStateArrayElem* StateArray::get_blk(ulong bsn) {
        return (range_test(bsn))? (&array[bsn]) : NULL;
    }


    StateArray::StateArray()    {
        elem_num    = GPRS_SNS;
        window_size = GPRS_WS;
        array       = new AckStateArrayElem[elem_num];
        head        = 0;
        tail        = 0;
        state       = 0;
    }
    StateArray::~StateArray()    {
    }

    int StateArray::add_head()               { head = (head+1)%elem_num; return 1;}

    int StateArray::add_tail()               { tail = (tail+1)%elem_num; return 1;}

    int StateArray::range_test(ulong bsn)    { return ( (bsn >=0) && (bsn < elem_num) )? 1: 0; }

    int StateArray::within_window(ulong bsn) {
        //return ( (bsn>=(tail%elem_num)) && (bsn<((tail+window_size)%elem_num)) ) ? 1: 0;

        int window_end = (tail+window_size)%elem_num;

        if ( (u_long)window_end > tail ) {

            if ( (bsn >= tail) && (bsn < (u_long)window_end))
                return 1;
            else
                return 0;
        }
        else if ( (u_long)window_end == tail) {
            /* impossible */
            printf("StateArray::within_window(): window_size == queue_size.\n");
            exit(1);
        }
        else {
            /* window_end < vq */
            if ( bsn < (u_long)window_end )
                bsn += elem_num;
            window_end += elem_num;

            if ( (bsn >= tail) && (bsn < (u_long)window_end))
                return 1;
            else
                return 0;

        }

    }

    int SendBuf::within_cur_window(ulong bsn) {


        ulong window_end = vs;

        if ( window_end > tail ) {

            if ( (bsn >= tail) && (bsn < window_end))
                return 1;
            else
                return 0;
        }
        else if ( window_end == tail) {
            if ( bsn == tail )
                return 1;
            else
                return 0;
        }
        else {
            /* window_end < vq */
            if ( bsn < window_end )
                bsn += elem_num;
            window_end += elem_num;

            if ( (bsn >= tail) && (bsn < window_end))
                return 1;
            else
                return 0;

        }

    }

    long StateArray::get_head()  { return head;}
    long StateArray::get_tail()  { return tail;}
    long StateArray::get_state() { return state;}
    int  StateArray::is_empty()  { return ((head%elem_num) == tail)? 1: 0;}
    int  StateArray::is_full()   { return ((head+1%elem_num) == tail)? 1: 0;}


    SendBuf::SendBuf() {
        vs  = 0;
        va  = 0;
        cur_sent_pending_acked_bsn = 0;
    }
    SendBuf::~SendBuf() {
    }

    ulong SendBuf::get_vs() {return vs;}
    ulong SendBuf::get_va() {return va;}
    ulong SendBuf::get_cur_sent_pending_acked_bsn() {return cur_sent_pending_acked_bsn;}

    int SendBuf::is_frozen() {

        if ( ((va+window_size) % elem_num) == vs )
            return true;

        else
            return false;
        /* always return false, because checking if send buffer should be
         * frozen is more complex and performed by TCB::scheduler.
         */
    }

    int  SendBuf::test_full(long blk_cnt) {

        if ( blk_cnt < 0 ) {
            cout <<"SendBuf test_full(): Warning: block count is less than or equal to zero." << endl;
            exit(1);
        }

        if ( (ulong)blk_cnt > elem_num ) {
            cout <<"SendBuf test_full(): Warning: block count is too many." << endl;
            exit(1);
        }

        if ( (head+1)%elem_num == tail )
            return true;

        if ( head >= tail ) {

            if ( head + blk_cnt < elem_num )
                return false;

            else {

                if ( (head + blk_cnt - elem_num) >= tail )
                    return true;
                else
                    return false;
            }
        }
        else {

            if ( ( head + blk_cnt ) >=tail )
                return true;
            else
                return false;
        }

    }


    int StateArray::get_state(ulong bsn ) {

        if ( range_test(bsn) )
            return array[bsn].state;

        else {
            printf("RLC StateArray: index is out of range , value=%lu \n",bsn);
            return -1;
        }
    }

    int StateArray::set_state(ulong bsn, uchar state) {
        if ( (range_test(bsn)) )
        {
            array[bsn].state = state;
            return 1;
        }
        else {
            printf("RLC StateArray: bsn is out of range or in unknown state , bsn = %lu state = %o\n",bsn,state);
            return -1;
        }
    }

    int SendBuf::insert(Event* e_p, ulong bsn) {

        if ( !state )
            state = 1;

        recycle();

        if ( range_test(bsn) ) {

            if ( is_full() ) {
                printf("Rlc SendBuf: Buffer is full\n");
                return -1;
            }
            if ( bsn != (head%elem_num) ) {
                printf("Rlc SendBuf: Assertion failed: the insertion of blocks to be sent is not sequential.\n");
                exit(1);
            }

            AckStateArrayElem *ptr = &array[head];

            if ( ptr->state == INVALID ) {
                ptr->upper_layer_pdu    = e_p;
                ptr->state              = NACK;
                ptr->bsn                = bsn;

                add_head();
                //forward_nack_index();

            }

            else {
                printf("Rlc SendBuf: abnormality: the state of the slot (bsn=%ld) to be updated is not invalid\n", bsn);

                #ifdef  __TAIL_DEBUG
                printf("\nSendbuf : tail = %lu head=%lu\n", tail,head);
                #endif
                exit(1);
            }

            return 1;
        }

        else {
            printf("RLC SendBuf insert(): bsn out of range!\n");
            exit(1);
        }
    }

    int SendBuf::del( ulong bsn) {

        if ( bsn > 127 ) {
            printf("SendBuf::del(): given bsn %ld is out of range.\n", bsn );
            exit(1);
        }

        if ( array[bsn].state == ACKED ) {

            if ( array[bsn].upper_layer_pdu ) {

                array[bsn].state  = INVALID;
                array[bsn].bsn    = -1;
                array[bsn].last_sent_timestamp = 0;

                if ( array[bsn].upper_layer_pdu ) {

                    Event* ep           = reinterpret_cast<Event*> (array[bsn].upper_layer_pdu);
                    bss_message* bmsg   = reinterpret_cast<bss_message*> (ep->DataInfo_);

                    if ( bmsg ) {

                        if ( bmsg->user_data ) {
                            delete bmsg->user_data;
                            bmsg->user_data     = NULL;
                            bmsg->user_data_len = 0;
                        }

                        if ( bmsg->packet ) {

                            Packet* pkt = bmsg->packet;
                            //remove_upper_layer_pkt(pkt);
                            free_pkt(pkt);
                            bmsg->packet = NULL;

                        }
                    }


                    FREE_BSS_EVENT( array[bsn].upper_layer_pdu );
                    array[bsn].upper_layer_pdu = NULL;
                }

            }

            #ifdef __DELELE_PKT_DEBUG
            printf("SendBuf::del(): del bsn %ld ts = %lld.\n", bsn , GetCurrentTime() );
            #endif
            return 1;

        }
        else {
            printf("SendBuf::del(): given bsn %ld is empty.\n", bsn );
            exit(1);
        }

    }

    int SendBuf::recycle() {

        for ( ulong cnt = 0 ; cnt < 20 ; ++cnt ) {

            ulong ind = (head + cnt )%elem_num;

            if ( (array[ind].state != ACKED) && (array[ind].state != INVALID) )
                break;

            #ifdef __DELELE_PKT_DEBUG
            printf("SendBuf::recycle(): del bsn %ld ts = %lld.\n", ind , GetCurrentTime() );
            #endif

            array[ind].state  = INVALID;
            array[ind].bsn    = -1;
            array[ind].last_sent_timestamp = 0;

            if ( array[ind].upper_layer_pdu ) {
                Event* ep           = reinterpret_cast<Event*> (array[ind].upper_layer_pdu);
                bss_message* bmsg   = reinterpret_cast<bss_message*> (ep->DataInfo_);
                if ( bmsg ) {

                    if ( bmsg->user_data ) {

                        delete bmsg->user_data;
                        bmsg->user_data     = NULL;
                        bmsg->user_data_len = 0;

                    }

                    if ( bmsg->packet ) {

                        Packet* pkt = bmsg->packet;
                        //remove_upper_layer_pkt(pkt);
                        free_pkt(pkt);
                        bmsg->packet = NULL;

                    }
                }
                FREE_BSS_EVENT( array[ind].upper_layer_pdu );
                array[ind].upper_layer_pdu = NULL;
            }

        }

        forward_tail();

        if ( (head+1)%elem_num == tail  ) {
            cout << "Send Buf: buffer is full. The used slots can not be freed." << endl;
            return -1;
        }

        return 1;
    }

    int SendBuf::recycle_transmitted_event(ulong sbsn,ulong blk_cnt) {

        for ( ulong cnt = 0 ; cnt < 20 ; ++cnt ) {

            ulong ind = (head + cnt )%elem_num;

            if ( (array[ind].state != ACKED) && (array[ind].state != INVALID) )
                break;

            #ifdef __DELELE_PKT_DEBUG
            printf("SendBuf::recycle_tx_blks(): del bsn %ld ts= %lld.\n", ind , GetCurrentTime() );
            #endif

            array[ind].state  = INVALID;
            array[ind].bsn    = -1;
            array[ind].last_sent_timestamp = 0;

            if ( array[ind].upper_layer_pdu ) {
                Event* ep           = reinterpret_cast<Event*> (array[ind].upper_layer_pdu);
                bss_message* bmsg   = reinterpret_cast<bss_message*> (ep->DataInfo_);
                if ( bmsg ) {

                    if ( bmsg->user_data ) {

                        delete bmsg->user_data;
                        bmsg->user_data     = NULL;
                        bmsg->user_data_len = 0;

                    }

                    if ( bmsg->packet ) {

                        Packet* pkt = bmsg->packet;
                        //remove_upper_layer_pkt(pkt);
                        free_pkt(pkt);
                        bmsg->packet = NULL;

                    }
                }
                FREE_BSS_EVENT( array[ind].upper_layer_pdu );
                array[ind].upper_layer_pdu = NULL;
            }

        }

        forward_tail();

        if ( (head+1)%elem_num == tail  ) {
            cout << "Send Buf: buffer is full. The used slots can not be freed." << endl;
            return -1;
        }

        return 1;
    }


    Event* SendBuf::get_transmitted_blk() {

        int send_nack_or_pending_ack = 0;

        if ( ((va+window_size) > (ulong)(vs) ) && (vs >= va )  ) {
            send_nack_or_pending_ack = 1; /* nack */
        }
        else if ( (va >= vs) && ((va+window_size)%elem_num > (ulong)(vs) )  ) {
            send_nack_or_pending_ack = 1; /* nack */
        }
        else if ( ((va+window_size) == (ulong)(vs) ) && (vs > va ) ) {
            send_nack_or_pending_ack = 2; /* pending ack */
        }
        else if ( (va > vs ) && ((va+window_size)%elem_num == (ulong)(vs) ) ) {
            send_nack_or_pending_ack = 2; /* pending ack */
        }
        else
            send_nack_or_pending_ack = 0;
            /* window is frozen: This condition doesn't occur except that
             * the command 'frozen' is explicitly indicated.
             */


        if ( send_nack_or_pending_ack == 1 ) {

            if ( array[vs].state == NACK ) {

                int tmp_index = vs;
                vs = (++vs)%elem_num;
                array[tmp_index].last_sent_timestamp = GetCurrentTime();
                array[tmp_index].state = PENDING_ACK;
                return array[tmp_index].upper_layer_pdu;

            }
            else {
                send_nack_or_pending_ack = 2;
            }
        }


        if ( send_nack_or_pending_ack == 2 ) {

            /* update the value of cur_sent_pending_acked_bsn before it is used */

            if ( ! (within_cur_window(cur_sent_pending_acked_bsn)) ) {
                cur_sent_pending_acked_bsn = va;
            }

            if ( array[cur_sent_pending_acked_bsn].state == NACK ||
                    array[cur_sent_pending_acked_bsn].state == PENDING_ACK ) {

                ulong tmp_index = cur_sent_pending_acked_bsn;
                cur_sent_pending_acked_bsn = (++cur_sent_pending_acked_bsn)%elem_num;
                if ( cur_sent_pending_acked_bsn == (vs+1)%elem_num )
                    cur_sent_pending_acked_bsn = va;

                array[tmp_index].last_sent_timestamp = GetCurrentTime();
                array[tmp_index].state = PENDING_ACK;
                return array[tmp_index].upper_layer_pdu;

            }
            else {

                for ( ulong i=0 ; i<window_size ; ++i) {

                    ulong index = (cur_sent_pending_acked_bsn+i)%elem_num;

                    if ( index == (vs+1)%elem_num ) {
                        cur_sent_pending_acked_bsn = va;
                        break;
                    }

                    if ( array[index].state == PENDING_ACK ) {
                        cur_sent_pending_acked_bsn = index;
                        break;
                    }

                }

                if ( array[cur_sent_pending_acked_bsn].state == PENDING_ACK ) {

                    int tmp_index = cur_sent_pending_acked_bsn;

                    cur_sent_pending_acked_bsn = (++cur_sent_pending_acked_bsn)%elem_num;

                    if ( cur_sent_pending_acked_bsn == (vs+1)%elem_num )
                        cur_sent_pending_acked_bsn = va;

                    array[tmp_index].last_sent_timestamp = GetCurrentTime();
                    array[tmp_index].state = PENDING_ACK;
                    return array[tmp_index].upper_layer_pdu;
                }
                else {
                    va = vs;
                    cur_sent_pending_acked_bsn = va;
                }

            }
        }

        return NULL;

    }

    int SendBuf::trigger_ack_scheduler() {

        int nacked_cnt      = 0;
        int pending_ack_cnt = 0;
        int acked_cnt       = 0;
        int invalid_cnt     = 0;

        for ( ulong i=0 ; i<window_size ; ++i) {

            int index = (va + i ) % elem_num;

            if ( array[index].state == NACK )
                ++nacked_cnt;

            if ( array[index].state == PENDING_ACK )
                ++pending_ack_cnt;

            if ( array[index].state == ACKED )
                ++acked_cnt;

            if ( array[index].state == INVALID )
                ++invalid_cnt;

        }

        ulong all_cnt = 0;
        if ( (all_cnt=(nacked_cnt + pending_ack_cnt + acked_cnt + invalid_cnt)) != window_size ) {

            printf("SendBuf::trigger_ack_scheduler(): counting_number %ld != window_size \n", all_cnt );
            exit(1);

        }


        if ( nacked_cnt < 10 ) {

            if ( pending_ack_cnt > 0 ) {
                //printf("BTS::schedule an ACK at tick = %lld \n", GetCurrentTime() );
                return 1;
            }

        }

        return 0;

    }


    int SendBuf::forward_tail() {

        int meet_vs_flag = false;

        if ( tail < head ) {
            for ( ulong i=tail ; (i)<=head; ++i) {
                tail = i;

                if ( tail == vs )
                    meet_vs_flag = true;

                if (array[i].state == PENDING_ACK || array[i].state == NACK ) {
                    break;
                }
            }
        }
        else if ( tail > head ) {

            int flag = false;
            for ( ulong i=tail ; (i)<elem_num ; ++i) {

                tail = i;

                if ( tail == vs )
                    meet_vs_flag = true;

                if (array[i].state == PENDING_ACK || array[i].state == NACK ) {
                    flag = true;
                    break;
                }
            }

            for ( ulong i=0 ; (i<=head) && (!flag) ; ++i) {

                tail = i;
                if ( tail == vs )
                    meet_vs_flag = true;

                if (array[i].state == PENDING_ACK || array[i].state == NACK ) {
                    flag = true;
                    break;
                }
            }

        }
        else {
            ; /* SendBuf is empty */
        }


        #ifdef  __TAIL_DEBUG
        printf("\nSendbuf : tail = %lu head=%lu\n", tail, head);
        #endif

        forward_va();
        if ( meet_vs_flag ) {

            ulong flag = false;
            for ( ulong i=0 ; i<window_size ; ++i) {

                ulong ind = (tail + i)%elem_num;

                if ( array[ind].state == NACK ) {

                    vs = ind;
                    flag = true;
                    break;

                }

            }

            if ( !flag ) {

                vs = va;

            }

        }

        return tail;
    }


    int SendBuf::forward_va() {

        long old_va = va;
        va = tail;

        if ( (u_long)old_va != va ) {
            ;//cur_sent_pending_acked_bsn = va;
        }

        return 1;
    }

    int RecvBuf::forward_vq() {


        int found_flag = false;
        if ( vq >= 0 ) {
            if ( array[vq].state == RECEIVED ) {
                for ( ulong i=vq ; (i%elem_num)<head; ++i) {
                    if (array[i].state != RECEIVED ) {
                        vq = i;
                        found_flag = true;
                        break;
                    }
                }
            }
        }

        if ( !found_flag )
            vq = ssn;

        tail = vq;
        return 1;
    }

    int RecvBuf::forward_head(long bsn) {
        if ( head<=(bsn%elem_num))
            head = (bsn+1)%elem_num;

        return 1;
    }

    int RecvBuf::within_recv_window(ulong bsn) {

        int window_end = (vq+window_size)%elem_num;

        if ( window_end > vq ) {

            if ( (bsn >= (u_long)vq) && (bsn < (u_long)window_end))
                return 1;
            else
                return 0;
        }
        else if ( window_end == vq) {
            /* impossible */
            printf("RecvBuf::within_recv_window(): window_size == queue_size.\n");
            exit(1);
        }
        else {
            /* window_end < vq */
            if ( bsn < (u_long)window_end )
                bsn += elem_num;
            window_end += elem_num;

            if ( (bsn >= (u_long)vq) && (bsn < (u_long)window_end))
                return 1;
            else
                return 0;

        }

    }

    int RecvBuf::insert(Event* ep, ulong bsn) {

        if ( !state )
            state = 1;

        if ( range_test(bsn) ) {

            if ( within_recv_window(bsn) < 0 ) {
                return -1;
            }

            recycle();

            #ifdef RECV_BUF_VN_DEBUG
            {
                bss_message* bmsg = (bss_message*)ep->DataInfo_;
                rlc_option* rlc_opt = (rlc_option*)bmsg->rlc_option;

                printf("bsn= %d, vq=%d vr=%d ssn=%d pkt_ts=%lld ctl_msg_type=%d\n", bsn , vq , ssn , ssn, rlc_opt->pkt_time_stamp, rlc_opt->ctl_msg_type);
            }
            #endif

            if ( array[ (head+1)%elem_num ].upper_layer_pdu != NULL  ) {
                printf("RLC RecvBuf: Buffer is full\n");
                return -1;
            }

            AckStateArrayElem *ptr = &array[bsn];

            if ( (ptr->state == INVALID) ||
                 ( (ptr->state == RECEIVED) && (ptr->bsn < 0) && (!ptr->upper_layer_pdu) ) ) {

                ptr->upper_layer_pdu    = ep;
                ptr->state              = RECEIVED;
                ptr->bsn                = bsn;

                forward_head(bsn);
                ssn     = (bsn+1)%GPRS_SNS;
                if ( static_cast<long> (bsn) == vq ) {
                    forward_vq();
                }

                //printf("RLC RecvBuf: insert new block with BSN = %d\n", bsn );
                return 1;
            }
            else {

                Event* ep_ptr = ptr->upper_layer_pdu;
                bss_message* bmsg_p = (bss_message*)( ep_ptr->DataInfo_);
                rlc_option* rlc_opt1 = (rlc_option*)bmsg_p->rlc_option;

                /* take the most recently received block */
                if ( ep != ep_ptr ) {
                    bmsg_p->user_data = NULL;
                    bmsg_p->user_data_len = 0;

                    printf("RLC RecvBuf: insert duplicated block with BSN = %lu pkt_ts =%llu \n", bsn ,
                            rlc_opt1->pkt_time_stamp );

                    /*if ( bmsg_p->packet) {
                        remove_upper_layer_pkt(bmsg_p->packet);
                        free_pkt(bmsg_p->packet);
                        bmsg_p->packet = NULL;
                    }*/


                    FREE_BSS_EVENT_INCLUDING_USER_DATA(ep_ptr);
                }

                //forward_head(bsn);

                ptr->upper_layer_pdu    = ep;
                ssn = (bsn+1)%GPRS_SNS;
                ptr->state              = RECEIVED;
                ptr->bsn                = bsn;

                if ( static_cast<long> (bsn) == vq )
                    forward_vq();


                return 1;
            }
        }
        else {
            printf("RLC RecvBuf insert(): BSN out of range!\n");
            exit(1);
        }

    }

    int RecvBuf::recycle_current_event(ulong sbsn,ulong blk_cnt) {

        if ( sbsn >= 128 ) {

            printf("RecvBuf::recycle_cur_event(): sbsn is out of range.\n");
            exit(1);

        }

        if ( blk_cnt >=128 ) {

            printf("RecvBuf::recycle_cur_event(): sbsn is out of range.\n");
            exit(1);

        }


        for ( ulong cnt = 0 ; cnt < blk_cnt ; ++cnt ) {

            ulong ind = ( sbsn + cnt )%elem_num;

            if ( (array[ind].state != RECEIVED) ) {

                printf("RecvBuf::recycle_cur_event(): the state of array[%lu] is not received.\n", ind );

                for ( ulong i=0 ; i<blk_cnt ;++i) {
                    int index = (i+ind)%elem_num;
                    printf("array[%d].state = %d bsn = %ld pdu=%p .\n", index , array[index].state , array[index].bsn ,
                            array[index].upper_layer_pdu );
                }
                //exit(1);
                return 1;

            }

            if ( array[ind].upper_layer_pdu ) {

                array[ind].state = RECEIVED;
                array[ind].bsn   = -1;

                Event* ep           = reinterpret_cast<Event*> (array[ind].upper_layer_pdu);
                bss_message* bmsg   = reinterpret_cast<bss_message*> (ep->DataInfo_);
                if ( bmsg ) {
                    if ( bmsg->user_data ) {
                        delete bmsg->user_data;
                        bmsg->user_data     = NULL;
                        bmsg->user_data_len = 0;
                    }
                }

                FREE_BSS_EVENT( array[ind].upper_layer_pdu );
                array[ind].upper_layer_pdu = NULL;
                //printf("free event with bsn = %ld \n" , ind );
            }

        }

        return 1;

    }

    int RecvBuf::recycle() {

        forward_vq();

        ulong ind = tail;
        for ( ulong cnt = 0 ; cnt < 10 ; ++cnt ) {

            ind = ( head + cnt )%elem_num;

            if ( ind == tail )
                continue;

            if ( (array[ind].state != RECEIVED) && ((array[ind].state != INVALID)) )
                break;

            array[ind].state    = INVALID;
            array[ind].bsn      = -1;

            if ( array[ind].upper_layer_pdu ) {

                Event* ep           = reinterpret_cast<Event*> (array[ind].upper_layer_pdu);
                bss_message* bmsg   = reinterpret_cast<bss_message*> (ep->DataInfo_);

                if ( bmsg ) {
                    if ( bmsg->user_data ) {

                            /* Only data blocks should release user_data fields */
                            delete bmsg->user_data;
                            bmsg->user_data     = NULL;
                            bmsg->user_data_len = 0;

                    }
                }


                FREE_BSS_EVENT( array[ind].upper_layer_pdu );
                array[ind].upper_layer_pdu = NULL;

            }

        }

        //if ( (head+1)%elem_num == tail  ) {
        if ( array[ (head+1)%elem_num ].upper_layer_pdu != NULL  ) {
            cout << "Recv Buf: buffer is full. The used slots can not be freed." << endl;
            cout << "head = " << head << " tail = "  << tail << endl;
            return -1;
        }

        return 1;
    }

    int RecvBuf::generate_rbb(uchar* buf) {
        if (!buf) {
            printf("Rlc RecvBuf(): Assertion failed: buf points to zero\n");
            return -1;
        }
        bzero(buf,64);
        for (ulong i=0; i<window_size; ++i) {

            long ind = ssn - i;
            if ( ind < 0 )
                ind += elem_num;

            buf[i] = array[ind].state;
        }
        return 1;
    }

    int RecvBuf::set_state(long bsn,long state) {

        if ( !range_test(bsn))
            return -1;

        array[bsn].state = state;
        return 1;

    }

    RecvBuf::RecvBuf() {
        ssn = -1;
        vq  = 0;
    }
    RecvBuf::~RecvBuf() {
    }

    Event*  RecvBuf::get_event_by_bsn(long bsn) {
        if (range_test(bsn))
            return ( array[bsn].upper_layer_pdu )? array[bsn].upper_layer_pdu: NULL;
        else
            return NULL;
    }

#endif
