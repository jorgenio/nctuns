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

#ifndef __TCB__H
#define __TCB__H

#include <object.h>
#include <gprs/include/types.h>
#include <gprs/include/bss_message.h>

#define READY 1
#define NOT_READY 0

#define ACK_TIMER_PERIOD 1 /* unit: sec */

/* Internal State Definition */
#define CHANNEL_REQ_PHASE           10
#define RESOURCE_REQ_PHASE          11
#define CHANNEL_REQ_FOR_PAGE_RES    12
#define PAGING_RESPONSING           13
#define PAGING_RESPONSED            14
#define RETRY_PHASE                 15

class Rlc;
class timerObj;
class partially_assembled_llcframe_list;
class SendBuf;
class RecvBuf;
typedef class SingleBlkAllocFreqParam SbaFreq;
class TwoSlotUsfGrantedInd;
class PacketRequestReference;
class PacketTimingAdvance;
typedef class TbfDescription TbfDes;

typedef class TbfControlBlock : public NslObject {
    friend class Rlc;
    friend class MsRlc;
    friend class BtsRlc;
    friend class UTbfMap;
    friend class DTbfMap;
    protected:
        /* internal variables */
        uchar               state;
        ulong               nodetype;
        ulong               tlli; /* the tlli this tbf belongs to */
        uchar               tfi;
        uchar               detach_flag;
        uchar               llgmm_reset_flag;
        short               uplink_freq;
        short               downlink_freq;
        uchar               timeslot_allocation;
        uchar               channel_coding_scheme;
        ulong               datasize_;
        ulong               window_size;
        ulong               elem_num;
        bool                sr_bit; /* 0 stands for sender, 1 stands for receiver */

        /* variables used in sending side */
        SList<Event>*       send_list;
        //SList<Event>*       ack_req_queue;

        ulong               tx_cnt;
        ulong               ack_threshold;
        long                vs;
        long                va;
        bool                vcs;

        SendBuf*            vb;
        u_int64_t           prepare_ack_timestamp;
        virtual int         partition_upper_layer_pdu(Event* ep) = 0;
        Event*              scheduler();

        /* variables used in receiving side */
        Event*              ack_event;
        uchar               ack_req;
        uchar               rrbp_value;
        long                vr;
        long                vq;
        RecvBuf*            vn;
        partially_assembled_llcframe_list* partial_queue;
        ulong               set_datasize(int coding_scheme_num);
        uchar*              decoding(Event* e_p);
        uchar*              get_datablk_buf();
        Event*              fast_reassemble_upper_layer_pdu(uchar bsn,uchar* decoded_data);
        int                 send_ack(Event* ep);
        void*               nslobj;
        int                 (Rlc::*recv_func) (Event*);
        int                 (Rlc::*send_func) (Event*);
        long                compute_start_bsn(rlc_option* rlc_opt);

        uchar               dummy_data_array[15];
        /* Acknowledge timer */
        timerObj*           ack_timer;

        virtual int         process_ack(void*)  = 0 ;
        virtual int         create_ack()        = 0 ;

    public:
        TbfControlBlock(bool sr_bit, uchar tfi1, void* nslobj1, int (Rlc::*send_func1) (Event*), int (Rlc::*recv_func1) (Event*));

        virtual ~TbfControlBlock();

        int                 transform_tcb_descriptions(TbfDes* tbfdes);
        int                 get_datablk_buf_len();
        bool                tfi_range_test() {return (tfi<32)?true:false;}
        int                 send(Event* ep);
        int                 recv_data(Event* ep,uchar* decoded_data);
        int                 recv_ack(void* ack_info);
        int                 set_state(uchar state);
        int                 get_state() {return state;}
        uchar               get_tfi() {return tfi;}
        uchar               get_ta()  {return timeslot_allocation;}
        ulong               get_tx_cnt() {return tx_cnt;}
        ulong               get_tlli()  {return tlli;}
        int                 init_ack_timer();
        int                 start_ack_timer();
        int                 cancel_ack_timer();
        int                 restart_ack_timer();
        int                 prepare_ack();
} TCB;

typedef class DownlinkTbfControlBlock : public TbfControlBlock {
        int         partition_upper_layer_pdu(Event* ep);

    public:
        DownlinkTbfControlBlock(bool sr_bit, uchar tfi1, void* nslobj, int (Rlc::*send_func1) (Event*) , int (Rlc::*recv_func1) (Event*));
        virtual int process_ack(void* pda);
        virtual int create_ack();
} DTCB;

typedef class UplinkTbfControlBlock : public TbfControlBlock {
        int partition_upper_layer_pdu(Event* ep);
    public:
        UplinkTbfControlBlock(bool sr_bit, uchar tfi1, void* nslobj, int (Rlc::*send_func1) (Event*) , int (Rlc::*recv_func1) (Event*));
        virtual int process_ack(void* pua );
        virtual int create_ack();
} UTCB;

#define SENDER          0
#define RECEIVER        1
#define SBA_TYPE        2

typedef class TbfDescription {

    friend class GprsBtsMac;
    friend class GprsMsMac;
    friend class TaAllocation;
    friend class TbfDesList;
    friend class TbfControlBlock;
    friend class TlliHashCache;

    private:
    u_int32_t               nid;

    /* 0 stands for sender, 1 stands for receiver , 2 stands for single-block allocation */
    uchar                   sr_bit;

    uchar                   nodetype;
    uchar                   state;
    ulong                   tlli;
    uchar                   tfi;
    uchar                   detach_flag;
    uchar                   llgmm_reset_flag;
    short                   uplink_freq;
    short                   downlink_freq;
    uchar                   ts_alloc[8];
    uchar                   ch_coding_scheme;
    short                   tbf_starting_time;

    char                    ch_req_cnt;     /* channel request count */

    PacketRequestReference* stored_req;
    ulong                   imsi;           /* used for downlink only */

    /* Uplink transmission parameters */
    TwoSlotUsfGrantedInd*   usf_granted_ind;
    uchar                   usf_tn[8];
    PacketTimingAdvance*    ta_ie;

    /* PACCH description:
     * (1)PktResourceRequest (single block assignment)
     * (2)ControlACKs (indicated by RRBP)
     */
    uchar*                  pacch_blkn_bitmap[12];

    /* Single Block Allocation */
    SbaFreq*                sbafreq;
    char                    sba_in_transmission;
    long                    tbf_starting_time_count_down_value;

    /* Packet Data Traffic Channels */
    SList<Event>*           pdtch_sq[8];

    /* Dedicated Control Channels */
    SList<Event>*           pacch_sq[8];
    SList<Event>*           pacch_pending_event_sq;

    int                     merge_ack(ulong tn);
    uchar                   rrbp_cur;

    public:
    TbfDescription(uchar srbit, bool nodetype1 );
    ~TbfDescription();
    int     insert_sba(SbaFreq* sbafreqdes,ulong uplink_fn);
    int     reset_for_roaming();

    u_int32_t get_nid()      {return nid;}
    inline  uchar get_tfi()  {return tfi;}
    inline  ulong get_tlli() {return tlli;}
    inline  ulong get_imsi() {return imsi;}
} TbfDes;

#endif
