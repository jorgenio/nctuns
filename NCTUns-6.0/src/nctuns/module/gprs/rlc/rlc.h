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

/* This file defines the RLC-related variables for error control and flow
 * control.
 *
 * CreateDate:  08/11/2003
 * Author:      Chih-che Lin
 * Email:       jclin@csie.nctu.edu.tw
 */

#ifndef __RLC_H
#define __RLC_H


#include <object.h>
#include <gprs/include/types.h>
//#include "tcb.h"
#include <gprs/include/bss_message.h>

//#define __BTS_RLC_LLC_DIRECT_CONNECT

class DTbfMap;
class UTbfMap;
typedef class UplinkTbfControlBlock UTCB;
typedef class DownlinkTbfControlBlock DTCB;

class Rlc : public NslObject {
    protected:
        /* internal variables */
        uchar           node_type; /* gprs::ms  or gprs::bts */
        uchar           state;
        ulong           window_size;
        int         retry_cnt;
        int         max_retry_num;
        int         user_specified_cs;
        int         command_mac(uchar cmd,ulong imsi, ulong tlli_old ,ulong tlli_cur ,uchar tfi, long freq, void* msg);
        int         send_ack(Event* eack_p);

        /* temp buffer for sending side */
        SList<Event>*   temp_frame_buf;

        /* convert the TbfDes to TCB */
        int transform_tcb_descriptions(void* tcb, void* tbfdes, bool utcb_or_dtcb);

    public:
        Rlc(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
        virtual ~Rlc();
        Event*  search_unsent_event_by_tlli(ulong tlli);
        int     trigger_send(UTCB* tcb);
        int     trigger_send(DTCB* tcb);
        virtual int send(Event* ep) {return NslObject::send(ep);}
        virtual int recv(Event* ep)=0;

        virtual int ori_send(Event* ep) {return NslObject::send(ep);}
        #ifndef __BTS_RLC_LLC_DIRECT_CONNECT
        virtual int ori_recv(Event* ep) {return NslObject::recv(ep);}
        #else
        virtual int ori_recv(Event* ep);
        #endif
};

class MsRlc : public Rlc {
    private:
        UTbfMap* utbfmap;
        DTbfMap* dtbfmap;
    public:
        MsRlc(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
        ~MsRlc();
        int send(Event* ep);
        int recv(Event* ep);
};

class BtsRlc : public Rlc {
    private:
        UTbfMap* utbfmap[125];
        DTbfMap* dtbfmap[125];
    public:
        BtsRlc(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
        ~BtsRlc();
        int send(Event* ep);
        int recv(Event* ep);
};



class UTbfMap {
    public:
        Array<UTCB,static_cast<ulong> (32)>* utcb_list;
        ulong imsi[32];
        ulong tlli[32];
        uchar selected[32];
        uchar ack_req[32];

        UTbfMap();
        ~UTbfMap() {if (utcb_list) delete utcb_list;}
        int   clear_selected() {bzero(selected,32);return 1;}
        UTCB* search(uchar tfi);
        UTCB* search_by_imsi(ulong imsi);
        UTCB* search_by_tlli(ulong tlli);
        UTCB* sel_tbf_to_send(uchar tn,uchar* ch_bitmap);
        UTCB* get(uchar tfi);
        int   insert(UTCB* utcb, ulong imsi1, ulong tlli1);
        int   remove_entry( ulong tlli1);
        int   remove_entry_by_tfi( uchar tfi);
        int   remove( ulong tlli1);
        int   update_tlli(UTCB* utcb, ulong newtlli);
};

class DTbfMap {
    public:
        Array<DTCB,static_cast<ulong> (32)>*    dtcb_list;
        ulong imsi[32];
        ulong tlli[32];
        uchar selected[32];
        uchar ack_req[32];

        DTbfMap();
        ~DTbfMap() {if (dtcb_list) delete dtcb_list;}
        int   clear_selected() {bzero(selected,32);return 1;}
        DTCB* search_by_imsi(ulong imsi);
        DTCB* search_by_tlli(ulong tlli);
        DTCB* search(uchar tfi);
        DTCB* get(uchar tfi);
        DTCB* sel_tbf_to_send(uchar tn,uchar* ch_bitmap);
        int   insert(DTCB* dtcb, ulong imsi1, ulong tlli1);
        int   remove_entry( ulong tlli1);
        int   remove_entry_by_tfi( uchar tfi);
        int   remove( ulong tlli1);
        int   update_tlli(DTCB* dtcb, ulong newtlli);
};



/* Note: RLC/MAC control blocks are always encoded with CS-1 */

typedef class RlcDownlinkDataBlockHeader {
    friend class GprsMsMac;
    friend class GprsBtsMac;
    friend class MsRlc;
    friend class BtsRlc;
    friend class TbfControlBlock;
    friend class DownlinkTbfControlBlock;
    friend class UplinkTbfControlBlock;
    private:
    /* page 116 in ts 44.060 */
    uchar power_reduction;
    uchar tfi;
    uchar fbi; /* final block identifier */
    uchar bsn; /* block sequence number */
    uchar length_indicator;
    uchar mbit;
    uchar ebit;

    public:
    RlcDownlinkDataBlockHeader() {bzero(this,sizeof(RlcDownlinkDataBlockHeader));}
    int   pack_header(uchar* ptr);
    int   unpack_header(uchar* ptr);
} RDDBH;

typedef class RlcUplinkDataBlockHeader {
    friend class GprsMsMac;
    friend class GprsBtsMac;
    friend class MsRlc;
    friend class BtsRlc;
    friend class TbfControlBlock;
    friend class UplinkTbfControlBlock;
    friend class DowlinkTbfControlBlock;
    private:
    /* page 117 in ts 44.060 */
    uchar pi;
    uchar tfi;
    uchar ti;
    uchar bsn;
    uchar mbit;
    uchar ebit;

    uchar length_indicator;
    ulong tlli;
    uchar pfi;
    public:
    RlcUplinkDataBlockHeader() {bzero(this,sizeof(RlcUplinkDataBlockHeader));}
    int   pack_header(uchar* ptr);
    int   unpack_header(uchar* ptr);
} RUDBH;

typedef class RlcDownlinkControlBlockHeader {
    friend class GprsMsMac;
    friend class GprsBtsMac;
    friend class MsRlc;
    friend class BtsRlc;
    friend class TbfControlBlock;
    friend class DowlinkTbfControlBlock;
    friend class UplinkTbfControlBlock;
    private:
    uchar rbsn;
    uchar rti;
    uchar fs;
    uchar ac;
    uchar pr;
    uchar tfi;
    uchar dbit;

    public:
    RlcDownlinkControlBlockHeader() {bzero(this,sizeof(RlcDownlinkControlBlockHeader));}
    int   pack_header( uchar* ptr );
    int   unpack_header( uchar* ptr );
} RDCBH;

uchar* channel_encoding(int scheme_num, uchar* data);
uchar* channel_decoding(int scheme_num, uchar* symbol);

typedef class PollIndicator {

    public:
        uchar  polling_ch_bitmap[250];
        uchar  polling_tn;
        uchar  rrbp_ind; /* the value of rrbp_ind is define in SPEC. The valid values
                          * for this field are 0,1,2,3. The value "gprs::rrbp_unused",defined
                          * gprs_nodetype.h, indicates this field is invalid.
                          */

        PollIndicator() {bzero(polling_ch_bitmap,250);polling_tn=0;rrbp_ind=gprs::rrbp_unused;}

} PollInd;

#endif
