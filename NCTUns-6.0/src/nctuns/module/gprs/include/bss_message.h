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

#ifndef __BSS_MSG_
#define __BSS_MSG_

#include <packet.h>
#include <gprs/include/types.h>
#include <gprs/include/gprs_nodetype.h>
#include <gprs/include/generic_list.h>

#define CREATE_BSS_EVENT(ep) do {           \
    CREATE_EVENT( (ep) );                       \
    bss_message* bssmsg = new bss_message;  \
    bzero(bssmsg,sizeof(bss_message));      \
    (ep)->DataInfo_ = bssmsg;                 \
} while(0)

#define FREE_BSS_EVENT(ep) do {                                                 \
    if ( (ep)->DataInfo_) {                                                     \
        free_bss_msg_elem(reinterpret_cast<bss_message*> (ep->DataInfo_) );     \
        delete (reinterpret_cast<bss_message*> (ep->DataInfo_));                \
        ep->DataInfo_ = NULL;                                                   \
    }                                                                           \
    FREE_EVENT(ep);                                                             \
} while(0)


#define FREE_BSS_EVENT_INCLUDING_USER_DATA(ep) do {                             \
    if ( (ep)->DataInfo_) {                                                     \
                                                                                \
        bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);   \
        if( bssmsg->user_data ) {                                               \
                                                                                \
                delete bssmsg->user_data;                                       \
                bssmsg->user_data      = NULL;                                  \
                bssmsg->user_data_len  = 0;                                     \
                                                                                \
        }                                                                       \
        free_bss_msg_elem( bssmsg );                                            \
        delete (bssmsg);                                                        \
        ep->DataInfo_ = NULL;                                                   \
    }                                                                           \
    FREE_EVENT(ep);                                                             \
} while(0)

#define bssall_len(x) (x->rlc_option_len + x->llc_option_len + x->sndcp_option_len+x->rlc_header_len+x->mac_header_len+x->llc_header_len+x->sndcp_header_len+x->bssgp_header_len+x->user_data_len)


typedef struct event Event;

typedef struct bss_message {

    struct  p_hdr p_hdr;
    struct  s_exthdr exthdr;
    short   flag; /*  PF_SEND , PF_RECV */
    uchar   imc_flag; /* Inter-Module Communication Flag */
    //uchar   rl_added_pkt_flag;

    char*   mac_option;
    char*   rlc_option;
    char*   llc_option;
    char*   sndcp_option;

    ulong   mac_option_len;
    ulong   rlc_option_len;
    ulong   llc_option_len;
    ulong   sndcp_option_len;

    ulong   mac_header_len;
    ulong   rlc_header_len;
    ulong   llc_header_len;
    ulong   sndcp_header_len;
    ulong   bssgp_header_len;
    ulong   user_data_len;

    char*       rlc_header;
    char*       mac_header;
    char*       llc_header;
    char*       sndcp_header;
    char*       bssgp_header;
    char*       user_data;

    char        guard_space[128];
    Packet*     packet;
    struct wphyInfo*   wphyinfo;

} bss_message;

long get_pkt_payload_length(Packet* pkt);
int free_bss_msg_elem           (bss_message* ptr);
int copy_bss_msg_flags          (bss_message* dst, bss_message* src);
int copy_bss_msg_options        (bss_message* dst, bss_message* src);
int copy_bss_msg_headers        (bss_message* dst, bss_message* src);
int copy_bss_msg_userdata       (bss_message* dst, bss_message* src);
int copy_bss_msg_packet_field   (bss_message* dst, bss_message* src);
int copy_bss_msg_ctlmsg         (bss_message* dst, bss_message* src);

Event* copy_gprs_pkt(Event* old_ep);
int    destroy_gprs_pkt(Event* ep);

int create_llc_option(bss_message* bssmsg);
int create_rlc_option(bss_message* bssmsg);
int create_mac_option(bss_message* bssmsg);
int insert_llc_header(bss_message* bssmsg, void* ptr,ulong len);
int insert_rlc_header(bss_message* bssmsg, void* ptr,ulong len);
int insert_mac_header(bss_message* bssmsg, void* ptr,ulong len);
int insert_user_data (bss_message* bssmsg, void* ptr,ulong len);


class llc_option {
    public:
    uchar  qos;
    ulong  tlli;
    ulong  oldtlli;
    ulong  command;
    long   req_bsic;
    long   req_rai;
    long   req_ch_no;
    ulong  addtional_info;
    void*  additional_params;
    llc_option() {bzero(this,sizeof(llc_option)); req_bsic = -1; req_rai = -1; req_ch_no=-1; }
    ~llc_option() { if (additional_params) delete (uchar*)additional_params; }
};

class rlc_option {
    public:
    void* upcall;
    u_int64_t   pkt_time_stamp;
    uchar       blk_cnt;      /* the total block count for a llc frame */
    uchar       blk_position; /* the position of this rlc block */
    uchar       direction;    /* 0: uplink, 1: downlink */
    short       ctl_msg_type;
    uchar       detach_flag;
    ulong       detach_tlli;
    uchar       cmd;          /* 0: user_data (Not command).
                               * 1: uplink_tbf_establish.    2: downlink_tbf_establish with user_data as imsi.
                               * 3: uplink_tbf_release.      4: downlink_tbf_release.
                               * 5: uplink_tbf_establihsed   6: downlink_tbf_estatblished
                               * 7: paging_request           8: paging_response
                               * 9: push a rlc data block
                               * messages with cmd == tbf_established shall carry TBF Descriptor in user_data field.
                               */
    char        tfi;
    long        chid;
    uchar       cs;
    uchar       rrbp;
    uchar       stalled;
    uchar       send_ack_ind;
    signed char user_data_bsn;

    /* upper layer header information */
    ulong sndcp_header_len;
    ulong llc_header_len;
    ulong user_data_len; /* total length for user data , without llc header and sndcp header */

    rlc_option() {bzero(this,sizeof(rlc_option));ctl_msg_type = -1;rrbp = gprs::rrbp_unused;}
};

typedef enum ObjType {

    DMSG            = 1,
    NDMSG           = 2,
    DUMMYBURST      = 3

} ObjType;


class mac_option {
   public:
   long         channel_id;
   uchar        btx_flag;
   u_int32_t    nid;
   long         fn;
   char         tn;
   long         bn;
   char         burst_num;  /* ranged from 0 to 3 */
   char         burst_type;
   ObjType      msg_type;
   void*        msg;
   mac_option() {bzero(this,sizeof(mac_option));}
   ~mac_option();
};

int free_pkt( Packet* pkt );
int insert_macopt_msg( mac_option* mac_opt , ObjType msg_type , void* msg );



typedef class EventRec {

    public:
        EventRec(Event* ep);
        Event*     ep_;
        u_int64_t  ts_;
        u_int32_t  nid_;

} EventRec;

typedef class EventTraceList : public SList<EventRec> {

    public:
    EventRec* search(Event* ep);
    int remove(Event* ep);

} ETList;

int rec_upper_layer_pkt(Packet* pkt);
int remove_upper_layer_pkt(Packet* pkt);
#endif
