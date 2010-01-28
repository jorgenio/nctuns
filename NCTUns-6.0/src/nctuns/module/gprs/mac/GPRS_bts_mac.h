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

#ifndef __NCTUNS_GPRS_BTS_MAC_H
#define __NCTUNS_GPRS_BTS_MAC_H

#include <object.h>
#include <gprs/include/types.h>
#include <gprs/include/bss_message.h>
#include <gprs/include/GPRS_rlcmac_message.h>

class timerObj;
class BssDescription;
class BssDesList;
class SbaFreqList;
class TaAllocation;
class TbfDesList;
class TlliHashCache;
class radiolink;
class partially_assembled_rlcblk;
typedef class TbfDescription TbfDes;

class GprsBtsMac : public NslObject{
    private:
    uchar state; /* 0 indicates un-ready mode, 1 indicates ready mode. */

    /* internal timers for synchronization */

    ulong   qn;             /* quarter bit number */
    ulong   bn;             /* bit number */
    ulong   tn;             /* time slot number */
    ulong   fn;             /* frame number */
    long    blkn;           /* block number: used for multi-frame structure multiplexing */
    long    uplink_tn;
    long    uplink_fn;
    long    uplink_blkn;
    int     update_internal_counters();
    int     update_blkn();
    int     blkn_to_starting_fn(long blkn1);
    long    fn_to_blkn(ulong fn1);
    inline int is_blkn_boundary();
    inline int is_uplink_blkn_boundary();
    inline int fn_to_burst_num(ulong fn1);


    /* user-specified parameters */
    int bsic;
    int rai;

    int sfreq;
    int efreq;

    int downlink_tsnum;
    int uplink_tsnum;

    /* arbitration parameters */
    ushort up_down[125][8];

    /* BCCH info list */
    BssDescription* bssdes;
    BssDesList*     neighbor_cell_list;
    int is_pbcch();
    int is_pccch();
    int is_idle_frame();

    uchar       listen_chmap[250];
    radiolink*  radiolink_obj;
    int         clear_listening_channel() {bzero(listen_chmap,250);return 1;}
    int         mark_listening_channel(long channel_num);
    int         dump_listening_channel();
    int         set_listening_channel();

    /* partially established TCBs */
    SbaFreqList* sbalist;

    /* TDMA scheduler */
    timerObj* sched_timer;
    int gprs_scheduler(bool tn_boundary);

    /* packet transfer related procedures */
    PacketUplinkAssignment*         single_blk_allocation(bss_message* bssmsg);
    PacketUplinkAssignment*         uplink_tbf_allocation(bss_message* bssmsg,ulong ms_tlli, uchar num_of_allocated_blk);
    PacketDownlinkAssignment*       downlink_tbf_allocation(TbfDes* utbfdes, uchar num_of_allocated_blk);
    int send_ack_nack();
    int retransmission();

    /* packet sending partition unit */
    int block_partition(Event* ep, int tfi);   /* if ( tfi>=0 ), tfi references to pdtch_uplink_squeues.
                                                * else tfi references to control channel queues:
                                                * PACCH_SQUEUE, PRACH_SQUEUE
                                                */
    /* packet receiving dispatcher */
    int     burst_demultiplexer(Event* ep);
    int     ctrmsg_demultiplexer(uchar* msg,uchar message_type);
    Event*  create_pkt_access_reject();
    Event*  create_paging_request(ulong imsi);

    uchar   trigger_sending_ind[8];

    /**** send burst queues *****/
    SList<Event>*  dummy_burst_squeue[8];

    uchar  dummy_data_array[15];
    /* PBCCH send queue */
    SList<Event>*  psi1_squeue;
    SList<Event>*  pbcch_squeue;
    /* PCCCH group send queues */
    SList<Event>*  ppch_squeue;
    SList<Event>*  pnch_squeue;
    SList<Event>*  pagch_squeue;

    /**** recv burst queues ****/
    partially_assembled_rlcblk*  uplink_rq[8];


    Event*  create_si13_msg();
    Event*  create_psi1_msg();
    Event*  create_psi2_msg();
    Event*  create_psi3_msg();
    Event*  create_pbcch_msg();
    int     create_dummy_burst_msg(long channel_id);
    Event*  create_measured_dummy_burst_msg(long channel_id);
    int     send_timestamp(Event* ep , ulong chid , u_int32_t dst_nid , uchar btx_flag );

    /* inter-module communication */
    int ack_rlc(int command, void* msg);
    /* add mac header */
    int add_mac_header(Event* ep,uchar blktype, uchar usf);

    TlliHashCache*  tlli_cache;
    TbfDesList*     pedl; /* partially_established_dtbf_list */
    char*           bss_cfg_filename;

    /* internal flag indicating log option being on-off : log flag */
    uchar  log_flag;
    /*GPRS log*/
    uchar  _ptrlog;


    public:
    GprsBtsMac(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
    ~GprsBtsMac();
    BssDesList* bsscfg_parser( u_int32_t nid , const char* filename );
    int init();
    int recv(Event_ *event_p );
    int send(Event_ *event_p );
};

typedef class BssDescription {
    friend class GprsBtsMac;
    friend class GprsMsMac;
    friend class BssDesList;

    private:
    u_int32_t       node_id;
    char            pdtch_downlink_multi_seq[12];
    long            start_ch;
    long            end_ch;
    TaAllocation*   ch_map[250]; /* 0 ~ 124 are uplink channels. 125 ~ 249 are downlink channels */
    long            bcch_no;
    uchar           pbcch_tn;
    uchar           tsc;
    uchar           rai;
    uchar           bsic;

    /* pccch organization parameters */
    uchar           bs_pbcch_blks;
    uchar           bs_pag_blks_res;
    uchar           bs_prach_blks;

    /* message change marks */
    char            bcch_change_mark;

    char            pbcch_change_mark;
    char            psi2_change_mark;
    char            psi3_change_mark;

    uchar           psi1_repeat_period;

    /* mobility management parameters */
    uchar           lai;                 /* location area identifier */
    uchar           rac;                 /* routing area code */
    ushort          cell_identity;       /* cell identity */

    public:

    BssDescription();
    uchar           channel_match(ushort chid);

} BssDes;

class BssDesList : public SList<BssDes> {
    public:
    BssDes*     search(ulong bsic);
    int         create_d_bssdes_list( DBssDes* area);
};

class TbfDesList : public SList<TbfDes> {
    public:
    //TbfDes* search(uchar pacch_tn);
    TbfDes* search_by_tn_usf(uchar tn1, uchar usf);
    TbfDes* search_sba(uchar tn1);
    TbfDes* search_by_tfi(uchar tfi);
    TbfDes* search_by_tlli(ulong tlli);
};

class  SbaFreqList: public SList<SbaFreq> {
    public:
    SbaFreq* search(ulong ch_no, uchar tn);
};

class TaAllocation {
    friend class GprsMsMac;
    friend class GprsBtsMac;
    private:
        uchar               tfi_cnt;
        TbfDes*             tbfdes[32];                 /* indexed by tfi */
        TbfDesList*         tn[8];
        uchar*              usf_granted_ind_in_tn[8];   /* for multiplexing of uplink */
        uchar               tfi_granted_tn[8];          /* for multiplexing of downlink, including UTBF PACCH and DTBF */
    public:
        TaAllocation();
        ~TaAllocation();
        int             set_tfi_granted_on_tn(uchar tn1,uchar tfi)  {tfi_granted_tn[tn1] = tfi; return 1;}
        uchar           get_tfi_granted_on_tn(uchar tn1)            {return tfi_granted_tn[tn1];}
        int             unset_tbfdes(ulong tfi);
        int             get_an_empty_tfi();
        int             set_tbfdes(ulong tfi, TbfDes* tbfdes1);
        int             remove_tbfdes_entry( TbfDes* tbfdes1);
        TbfDes*         get_sba_tbfdes(uchar tn1);
        uchar           get_tfi_cnt() {return tfi_cnt;}
        uchar           get_usf_in_tn (uchar tn_num,uchar tfi,bool is_pbcch_carrier );
        uchar           inquiry_usf_in_tn(uchar tn_num,uchar tfi);
        uchar           choose_usf_in_tn(uchar tn_num);
        TbfDes*         get_tbfdes_by_tn_usf(uchar tn, uchar usf);
        TbfDes*         get_tbfdes_by_tfi(uchar tfi);
        TbfDes*         get_tbfdes_by_tlli(ulong tlli);
        TbfDes*         get_granted_tbfdes(uchar tn1);
        int             merge_ack(ulong tn);
};

class TlliCacheElem {
    public:
        TlliCacheElem() {tlli = 0 ; utcb = 0; dtcb=0; };
        TlliCacheElem(TlliCacheElem* tce);
        ulong   tlli;
        TbfDes* utcb;
        TbfDes* dtcb;
};

class TlliCacheList : public SList<TlliCacheElem> {

    public:
        TlliCacheElem*  search_by_tlli(ulong tlli);
        int             remove_cache_entry( ulong tlli1);
};

class TlliHashCache {
        ulong           hash_size;
        TlliCacheList** hash_table;
    public:
        TlliHashCache();
        ~TlliHashCache();
        int insert( TbfDes* tbfdes);
        int remove( ulong tlli1);
        TlliCacheElem* search_by_tlli(ulong tlli);
        int rehash(ulong old_tlli, ulong new_tlli );
};

#endif
