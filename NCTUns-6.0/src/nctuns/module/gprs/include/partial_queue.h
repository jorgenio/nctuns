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

#ifndef __PARTIAL_QUEUE_H
#define __PARTIAL_QUEUE_H
#include <gprs/include/types.h>

class HistoryFrameList;
class Packet;
class timerObj;

typedef class HistoryAssembledLlcFrameElem {
    friend class HistoryFrameList;
    private:

    u_int64_t                       ts_;
    uchar                           start_bsn_;
    uchar                           blk_cnt_;
    Packet*                         pkt_addr_;
    HistoryAssembledLlcFrameElem*   prev;
    HistoryAssembledLlcFrameElem*   next;

    public:
    HistoryAssembledLlcFrameElem() { ts_=0; start_bsn_ = 0; blk_cnt_ = 0; pkt_addr_ = (Packet*)(0xffffffff); prev=NULL; next=NULL; }
    int write( u_int64_t ts, uchar start_bsn,uchar blk_cnt, Packet* pkt_addr);
    int dump();
    ~HistoryAssembledLlcFrameElem() {;}

} HisLlcFrElem ;

typedef class HistoryFrameList {

    private:
    ulong           elem_cnt;
    ulong           release_period;
    ulong           expire_time;
    timerObj*       release_timer;
    HisLlcFrElem*   head;
    HisLlcFrElem*   tail;

    public:
    HistoryFrameList();
    ~HistoryFrameList();
    int             insert(HisLlcFrElem* elem);
    HisLlcFrElem*   search(Packet* pkt_addr);
    int             dump_elem( HisLlcFrElem* elem);
    int             del_entry (HisLlcFrElem* elem );
    int             release();


} HisFrList;

class partially_assembled_llcframe_list_elem {
     private:
         uchar      start_bsn_;
         uchar      blk_cnt_;
         uchar      received_cnt_;
         ulong      user_data_len_;
         ulong      llc_header_len_;
         ulong      sndcp_header_len_;
         uchar*     *tmp_ring;
         Event*     partial_event;
         u_int64_t  pkt_time_stamp;
         bool       range_test(uchar bsn) { return (bsn<=127)?true:false; }
     public:

         partially_assembled_llcframe_list_elem
            (uchar start_bsn , uchar blk_cnt , ulong sndcp_hl , ulong llc_hl , ulong user_dl , u_int64_t pkt_ts );
         ~partially_assembled_llcframe_list_elem();

         int            search(uchar bsn);
         int            dump();
         int            insert(uchar bsn,uchar* decoded_data, u_int64_t cur_pkt_ts );
         int            create_partial_event( Event* ep);
         int            release_partial_event();
         uchar          get_start_bsn()         {return start_bsn_;}
         uchar          get_blk_cnt()           {return blk_cnt_;}
         uchar          get_received_cnt()      {return received_cnt_;}
         ulong          get_total_frame_len()   {return (sndcp_header_len_ + llc_header_len_ + user_data_len_) ;}
         Event*         fast_assembling(ulong datasize);
         HisLlcFrElem*  pkt_history_rec(HistoryAssembledLlcFrameElem* elem , Packet* pkt_addr );
         /* remove() is not necessary because partially stored data are cleaned once */

};


class partially_assembled_llcframe_list {
    private:
        partially_assembled_llcframe_list_elem*     *list_;
        HisFrList*                                  his_fr_list;

    public:

        partially_assembled_llcframe_list();
        ~partially_assembled_llcframe_list();


        partially_assembled_llcframe_list_elem* get_list(uchar index);
        int create( uchar start_bsn , uchar blk_cnt , ulong sndcp_hl , ulong llc_hl , ulong user_dl , u_int64_t pkt_ts );
                                                            /* create a bucket for a partially assembled list */

        int destroy(uchar start_bsn);                       /* destroy a bucket with bsn being start_bsn */

        int  his_pkt_insert( HisLlcFrElem* elem );

        ulong  get_blk_cnt( uchar sbsn );
        Event* fast_assembling(uchar start_bsn,ulong datasize);
};

class partially_assembled_rlcblk {
     private:

         bool   use_indicator;
         uchar  burst_cnt_;
         uchar  received_cnt_;
         Event* *tmp_ring;

     public:
         partially_assembled_rlcblk();
         ~partially_assembled_rlcblk();

         int    insert(uchar burst_num, Event* ep); /* insert a decoded data to the corresponding bucket */
         uchar  get_burst_cnt()         {return burst_cnt_;}
         uchar  get_received_cnt()      {return received_cnt_;}
         Event* fast_assembling();      /* flush all received bursts */
         int    flush();

};

#endif
