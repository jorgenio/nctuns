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

#ifndef __STATE_ARRAY_H__
#define __STATE_ARRAY_H__

#include <gprs/include/types.h>
#include "rlc_shared_def.h"

typedef struct event Event;

class AckStateArrayElem {
    public:
        uchar       state;
        long        bsn; /* ranged from 0 to (2^7 -1) */
        u_int64_t   last_sent_timestamp;
        Event*      upper_layer_pdu;
        AckStateArrayElem();
};

class StateArray {

    protected:
        uchar               state; /* 0 stands for NOT_FUNCTION , 1 stands for START_TO_FUNCTION */
        AckStateArrayElem*  array;
        ulong               head; /* head points to the first unused slot */
        ulong               tail; /* tail points to the expected slot number of the oldest sent block that the ACK should return.
                                   * Be equivalent to pending_ack_index
                                   */
        ulong               elem_num;
        ulong               window_size;
        int                 add_head();
        int                 add_tail();
        int                 range_test(ulong bsn);
        int                 within_window(ulong bsn);

    public:
        StateArray();
        virtual ~StateArray();
        long                get_head();
        long                get_state();
        long                get_tail();
        int                 is_empty();
        int                 is_full();
        int                 get_state(ulong bsn );
        int                 set_state(ulong bsn , uchar state);
        AckStateArrayElem*  get_blk(ulong bsn);

        virtual int         insert(Event* e_p, ulong bsn )  = 0;
        virtual int         recycle()                       = 0;

};

class SendBuf : public StateArray {
    private:
        ulong                vs; /* vs denotes the sequence number of the next in-sequence rlc block to be transmitted. */
        ulong                va; /* va denotes the oldest block that has not been ACKed. */
        ulong                cur_sent_pending_acked_bsn;
    public:
        SendBuf();
        virtual ~SendBuf();
        int                 is_frozen();
        int                 test_full(long blk_cnt);
        int                 insert(Event* ep, ulong bsn);
        int                 recycle();
        int                 recycle_transmitted_event(ulong sbsn,ulong blk_cnt);
        int                 del( ulong bsn);
        int                 forward_tail();
        //int                 forward_nack_index();
        int                 forward_va();
        Event*              get_transmitted_blk();
        int                 trigger_ack_scheduler();
        ulong               get_vs();
        ulong               get_va();
        ulong               get_cur_sent_pending_acked_bsn();
        int                 within_cur_window(ulong bsn);
};


class RecvBuf : public StateArray {
        long                ssn; /* ssn == vr, the value of which is one higher than the highest bsn so far received */
        long                vq;  /* the start of the receive window */
        int                 forward_head(long bsn); /* head stands for the beginning of unused slots */
        int                 forward_vq();
    public:
        RecvBuf();
        virtual ~RecvBuf();
        Event*              get_event_by_bsn(long bsn);
        int                 within_recv_window(ulong bsn);
        int                 generate_rbb(uchar* buf);
        int                 insert(Event* ep, ulong bsn);
        int                 recycle();
        int                 set_state (long bsn,long state);
        int                 recycle_current_event(ulong sbsn,ulong blk_cnt);
        long                get_ssn() {return ssn;}
        long                get_vq()  {return vq;}

};

#endif
