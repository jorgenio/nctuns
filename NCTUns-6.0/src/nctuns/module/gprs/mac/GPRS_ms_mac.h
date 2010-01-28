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

#ifndef __NCTUNS_GPRS_MS_MAC_H
#define __NCTUNS_GPRS_MS_MAC_H

#include <gprs/SndcpGmm/GPRS_mm_message.h>
#include <gprs/include/burst_type.h>
#include <gprs/include/GPRS_rlcmac_message.h>
#include "mac_header.h"
#include "mac_shared_def.h"
#include <gprs/rlc/tcb.h>
#include <gprs/radiolink/radiolink.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>


class timerObj;
class BcchInfo;
class BcchList;
class partially_assembled_rlcblk;

class GprsMsMac : public NslObject {
    private:
    uchar       state;
    uchar       roaming_flag;
    ulong       tlli;

    /* GPRS log */
    uchar       _ptrlog;
    uchar       log_flag;

    /* internal timers for synchronization */
    ulong       qn;         /* quarter bit number */
    ulong       bn;         /* bit number */
    ulong       tn;         /* time slot number */
    ulong       fn;         /* frame number */
    long        blkn;       /* block number: used for multi-frame structure multiplexing */
    long        uplink_tn;
    long        uplink_fn;
    long        uplink_blkn;
    uchar       trigger_sending_ind[8];
    uchar       triggered_tn;
    uchar       triggered_fn;
    char        triggered_blkn;

    int         update_bn();
    int         update_tn();
    int         update_fn(long fn);
    int         update_uplink_fn(long fn);
    int         update_blkn();
    int         update_counters(uchar tn,ulong fn);
    int         update_ts_map(bool d_bit,uchar timeslot_allocation,uchar tfi);
    ulong       blkn_to_starting_fn(long blkn1);
    long        fn_to_blkn(ulong fn1);

    inline int  is_blkn_boundary();
    inline int  test_is_blkn_boundary(ulong fn1);
    inline int  fn_to_burst_num(ulong fn1);

    int         set_trigger_sending_indicator();

    uchar       downlink_ta_map[8];
    uchar       uplink_ta_map[8];

    /* arbitration parameters */
    ushort up_down[8];

    uchar       dummy_data_array[15];
    /* operating mode */
    uchar       mac_mode;

    /* BCCH info list */
    bool        scanning_mode;
    BcchInfo*   cur_used_bss;
    long        cur_monitored_neighbor_list_index;
    BcchInfo*   get_cur_monitored_neighbor_cell();
    BcchList*   bcch_list;
    short       cur_ch_no;
    uchar       psi3_info_recv_flag;
    u_int32_t   incoming_event_nid;
    uchar       listen_chmap[250];
    int         clear_listening_channel() {bzero(listen_chmap,250);return 1;}
    int         mark_listening_channel(long channel_num);
    int         set_listening_channel();
    int         configure_listened_channels();
    int         is_idle_frame_for_ta_update();
    int         is_idle_frame_for_signal_measurement();
    int         is_idle_frame();
    int         test_is_idle_frame(ulong fn1);
    radiolink*  radiolink_obj;

    /* partially-established TCBs */
    long    neighbor_signal_rank[8];
    TbfDes* dtcb;
    TbfDes* utcb;
    uchar   dtcb_rrbp_in_trans_flag;

    /* Downlink transmission control parameters:
     * page_mode,
     */
    uchar page_mode;
    //SList<pagingresponse>* paging_list;

    /* Uplink transmission control parameters:
     * USF allocation, persistence_level
     */
    uchar persistence_level[4];

    /* TDMA scheduler */
    timerObj* sched_timer;

    /* timers */
    timerObj* second_timer;
    timerObj* random_timer;
    timerObj* chreq_timer;
    timerObj* pua_timer;


    int show_rssi();
    int gprs_scheduler();

    /* broadcast information acquisition: not implemented so far */
    int fully_acquisition()   {return 1;}
    int partial_acquisition() {return 1;}

    /* cell change related procedures: not implemented so far */
    int cell_reselection()    {return 1;}

    /* packet transfer related procedures */
    int establish_downlink_tbf(uchar cause, long chid, ulong tlli);
    int establish_uplink_tbf(uchar cause, long chid, ulong tlli);
    int randomized_utcb_channel_request();
    int randomized_dtcb_channel_request();
    int retry_establish_uplink_tbf();
    int retry_resource_request();
    int release_uplink_tbf() {return 1;}
    int channel_request(TbfDes* utcb, long chid, ulong cause );
    int resource_request(TbfDes* tbfdes,long chid);
    int send_ack_nack();
    int retransmission();

    /* packet sending partition unit */
    int block_partition(Event* ep, int tfi);   /* if ( tfi>=0 ), tfi references to pdtch_uplink_squeues.
                                                * else tfi references to control channel queues:
                                                * PACCH_SQUEUE, PRACH_SQUEUE */
    /* packet receiving dispatcher */
    int burst_demultiplexer(Event* ep);
    int ctrmsg_demultiplexer(uchar* msg, char message_type);

    /* send burst queues */

    SList<Event>*                       prach_squeue;

    partially_assembled_rlcblk*         downlink_rq[8];
    uchar                               drq_flush_ind[8];

    /* recv burst record : keep track of bursts received from which BSS */
    u_int64_t pre_recv_timestamp;
    RBRec   rb_rec[8];
    int     record_recv_burst(mac_option* mac_opt, double rssi_val);
    int     detect_collision(mac_option* mac_opt , uchar recv_tn);
    int     validate_mac_option(mac_option* mac_opt);


    int ack_rlc(ulong command,void* msg);
    int create_ctl_ack(uchar ctlack_value);
    int gmm_update();
    int gmm_roaming_procedure(long bcch_no);

    public:
    GprsMsMac(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
    int init();
    int recv(Event_ *event_p );
    int send(Event_ *event_p );
    int send_timestamp(Event* ep, long chid);
};

class BcchInfo {
    friend class GprsMsMac;
    private:

        u_int32_t   nid;
        uchar       rai;
        uchar       bsic;
        uchar       tsc;
        uchar       bcch_no;
        long        start_ch;
        long        end_ch;
        char        bcch_change_mark;

        /* signal strength */
        double      rssi;

        /* scanning mode parameters */
        ushort      received_burst_cnt;

        /* PBCCH info */
        char        pbcch_tn;
        char        pbcch_change_mark;
        char        psi2_change_mark;
        char        psi3_change_mark;
        char        psi1_repeat_period;

        GprsCellOptions         gprs_cell_opt;
        PrachControlParameters  prach_ctl_param;
        PccchOrgParam           pccch_org_param;
        CellId                  cell_id;
        uchar                   bss_tsc;

    public:
        BcchInfo();
        int         update(BcchInfo* new_entry );
        int         clear();
        int         copy_from_dbssdes( DBssDes* dbssdes );
        int         set_nid(u_int32_t nid1 ) { nid = nid1;return 1; }
        int         set_ch_range( long start_ch1 , long end_ch1 ) { start_ch = start_ch1; end_ch = end_ch1; return 1;}
        int         set_bcch_no( uchar bcch_no1)        {bcch_no    = bcch_no1; return 1;}
        int         set_bsic(uchar bsic1)               {bsic       = bsic1;    return 1;}
        int         set_rai( uchar rai1)                {rai        = rai1;     return 1;}
        int         set_tsc( uchar tsc1)                {tsc        = tsc1;     return 1;}
        u_int32_t   get_nid()                       {return nid;}
        double      get_rssi()                      {return rssi;}
        uchar       get_rai()                           {return rai;}
        uchar       get_tsc()                           {return tsc;}
        uchar       get_bcch_no()                       {return bcch_no;}
        uchar       get_bsic()                          {return bsic;}

};

class BcchList : public SList<BcchInfo> {
    public:
    int construct_from_psi3( Psi3* psi3msg );
    BcchInfo* search(ulong bcch_no);
    BcchInfo* search_by_bsic_rai(uchar bsic, uchar rai);
    int       show_rssi();
};

#endif
