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

/* This file defines LLC frame formats, LLE commands/responses,
 * service primitives provided by LLC layer. Also it contains
 * the definition of LLC module consisting of LLME and several
 * LLEs. Basically, LLC module in NCTUns conforms to 3GPP TS 44.064,
 * so far this module only supports unconfirmed data transfer (ADM).
 *
 * CreateDate: 07/30/2003
 * Author:     Chih-che Lin
 * Email:      jclin@csie.nctu.edu.tw
 */

#ifndef __LLC_H
#define __LLC_H

#include <string.h>
#include <sys/types.h>
#include <timer.h>
#include <module/gprs/include/types.h>
#include <module/gprs/include/bss_message.h>
#include <module/gprs/include/gprs_nodetype.h>
#include <module/gprs/include/generic_list.h>
#include <module/gprs/include/generic_list_impl.h>
#include <gprs/SndcpGmm/GPRS_mm_message.h>
#include <object.h>

/* Event queue size setting */
#define __GPRS_LL_EVENT_QUEUE_LIMITED_SIZE__ 1

/* control field type */
#define I_PLUS_S_FORMAT 0
#define S_FORMAT        1
#define UI_FORMAT       2
#define U_FORMAT        3

/* c/r bit definition in addr field */
#define MS_COMMAND      0
#define SGSN_RESPONSE   0
#define MS_RESPONSE     1
#define SGSN_COMMAND    1

/* sapi number definition */
#define LLGMM           0x1
#define TOM2            0x2
#define LL3             0x3
#define LL5             0x5
#define LLSMS           0x7
#define TOM8            0x8
#define LL9             0x9
#define LL11            0xb

/* service primitive number */
#define LLGMM_ASSIGN            1
#define LLGMM_TRIGGER           2
#define LLGMM_RESET             3
//#define PAGING_RESPONSE         4 define in bss_message.h
#define LLGMM_DETACH            5   /* non-standard command for the consistency of data structures among layers. */
#define LL_UNITDATA_REQ_MS      101
#define LL_UNITDATA_IND_MS      102
#define LL_UNITDATA_REQ_SGSN    103
#define LL_UNITDATA_IND_SGSN    104
#define LL_XID_REQ              101
#define LL_XID_IND              102
#define LL_XID_RES              103
#define LL_XID_CNF              104
#define GRR_DATA_REQ            201
#define GRR_DATA_IND            202
#define GRR_UNITDATA_REQ        203
#define GRR_UNITDATA_IND        204
#define BSSGP_DL_UNITDATA_REQ   211
#define BSSGP_UL_UNITDATA_IND   212

/* states of llme and lle */
#define TLLI_ASSIGNED                           0x0
#define TLLI_CHANGED_AND_TLLI_OLD_STILL_VALID   0x1
#define TLLI_UNASSIGNED                         0x2
#define EXCEPTION_BUT_RECOVERED                 0x3
#define EXCEPTION                               0x4

/* llc frame parameters */
#define FCS_SIZE            3
#define UI_CTL_FIELD_SIZE   2



/* This control message format conforms to struct control_message
 * in SNDCP layer. The consistency between this two structures should
 * be taken care.
 */

class SndcpGmmControlMsg {
    public:
                uchar   nsapi;
                uchar   control;
                ushort  qos_class;
};

class EventElem {
    public:
        Event*          ep;
        EventElem*      next;
};
class EventQueue {
    private:
        EventElem   *head;
        EventElem   *tail;
        ulong       num_of_elem;
    public:
        EventQueue();
        ~EventQueue();
        int     enqueue(Event* ep);
        Event*  dequeue();
        bool    is_full();
        bool    is_empty();
};


class LlcFrame {

    private:
        uchar   addr_field;
        bool    pd_;
        bool    cr_;
        uchar   sapi_;

                uchar   format_type;    /* I_PLUS_S_FORMAT , S_FORMAT , UI_FORMAT , U_FORMAT */
                uchar*  control_field;  /* various lengths according to format_type. At present, we support
                                 * UI format (for Data) and U format(for control messages).
                                 */
                bool    encryption_;
                bool    protected_mode_;
                short   unconfirmed_seqno_;

                uchar*  information_field;
                uchar   fcs[3];

    public:
        LlcFrame() {bzero(this,sizeof(LlcFrame));}
        LlcFrame(uchar frame_type);
        ~LlcFrame() { if (control_field) delete control_field;}
        /* manipulate addresss field */
                int     insert_addr_field(bool pd, bool cr, uchar sapi);
                bool    get_pd();
                bool    get_cr();
                uchar   get_sapi();
                uchar   get_addr_field();

                /* manipulate control field */
                int     insert_ui_format_control_field( bool encryption, bool protected_mode , short unconfirmed_seqno );
                bool    get_e_bit();
                bool    get_pm_bit();
                short   get_unconfirmed_seqno();

                /* manipulate information field */
                int     insert_info_field(void* info); /* Notice: this function only makes info field point to pkt->p_data */
                uchar*  get_info_field();

                /* manipulate FCS field */
                int             insert_fcs(uchar *given_fcs=NULL);
                uchar*      get_fcs()           {return fcs;};

                /* pack all fields into NCTUns packet structure */
                int     pack(Event* p);
                int             unpack(Event *p,int frame_type);

};
class Lle;
class Llme {
    private:
                uchar   state; /* TLLI_ASSIGNED, TLLI_UNASSIGNED */
        ulong   lle_num;
                ulong   _tlli_cur;
                ulong   _tlli_old;
                Lle**   lle_array;
    public:
                Llme( Lle** lle , ulong tlli);
                uchar   get_state()     {return state;};
                ulong   get_tlli()      {return _tlli_cur;};
                ulong   get_tlli_old()  {return _tlli_old;};
        int     set_tlli(ulong tlli_new, ulong tlli_old);
                int     process_control_primitive(Event* pkt);
};

class Lle {
    private:
                uchar   state; /* the states of LLE includes EXCEPTION , EXCEPTION_BUT_RECOVERED,
                        * TLLI_UNASSIGNED, TLLI_ASSIGNED, TLLI_CHANGED_AND_TLLI_OLD_STILL_VALID,
                        * SUSPEND */
                int     sapi;  /* the sapi associated to this LL entity */
                ulong   node_type;
                uchar*  req_slot;
                EventQueue  sendqueue;
                /* ABM mode related internal variables : not implement at this stage */

                /* ADM mode related internal variables : ranged from 0 to 511 (9-bit counter) */
                ushort  unconfirmed_send_state_variable;        /* V(U)  */
                ushort  unconfirmed_receive_state_variable;     /* V(UR) */
                ushort  unconfirmed_sequence_number;            /* N(U)  */
                ushort  n201_u_userdata;                        /* maximum number of octets of an information field */
                ushort  n201_u_gmm;                             /* maximum number of octets of an information field */
                ushort  n201_u_tom;                             /* maximum number of octets of an information field */
                ushort  n201_u_sms;                             /* maximum number of octets of an information field */

                LlcFrame*   framelist;
                EventQueue* sendtarget;
                EventQueue* recvtarget;
                Llme*       llme;
    public:
    friend class Llme;
                Lle (int specified_sapi, uchar& reqline , EventQueue* sendqueue , EventQueue* recvqueue, ulong nodetype);
                ~Lle() {  if (framelist) delete framelist; }

                int set_llme(Llme* llme1) {llme = llme1;return 1;}
                int process_l3pdu(Event *event_p );
                int process_incoming_frame(Event* e_p);

                /* ADM mode related actions */
                int             send_command(int command_type);
                int             send_uidata (int num_pkt);
                int             recv_command();
                int             recv_uidata();

};

class LlcIe {
    private:
                ulong       node_type;
                uchar       send_req[17]; /* start from 1, 0 is reserved for future use */

                Lle         *lle[17]; /* start from 1, 0 is unused */
                Llme        *llme;
                int         qos_classifier(llc_option* msg);
                int         dispatch(Event* event_p);
                int         multiplexer();
                int         demultiplexer(Event* event_p);

    public:
                LlcIe(EventQueue* sendqueue,EventQueue* recvqueue, ulong nodetype, ulong tlli);
                ~LlcIe();
                int     init();
                int     recv(Event_ *event_p );
                int     send(Event_ *event_p );
        int     set_tlli(ulong tlli_new, ulong tlli_old) {return llme->set_tlli(tlli_new,tlli_old);}
                ulong   get_tlli() {return llme->get_tlli();}
};
class LlcIeList;
class Llc : public NslObject {
    private:
        uchar       freeze_flag;
        ulong       nodetype;
        LlcIeList*  llcie_list;
        EventQueue* sendqueue;
        EventQueue* recvqueue;
        NslObject*  peer_llc; /* only for debugging */

        /* roaming testing timer */
        timerObj*   gmm_update_timer;
        int         gmm_update();

                bool is_random_tlli(ulong tlli)     {return (tlli&0x0000001e)?true:false;}
                bool is_local_tlli(ulong tlli)      {return (tlli&0x00000003)?true:false;}
                bool is_foreign_tlli(ulong tlli)    {return (tlli&0x00000001)?true:false;}
                bool is_auxiliary_tlli(ulong tlli)  {return (tlli&0x0000000e)?true:false;}
    public:
            Llc(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
        ~Llc();
        int init();
        int send(Event* ep);
        int recv(Event* ep);
        int continue_send();
        int continue_recv();
};
class LlcIeList : public SList<LlcIe> {
    public:
      LlcIe* search(ulong tlli);
};

int rec_upper_layer_pkt(Packet* pkt);
int remove_upper_layer_pkt(Packet* pkt);

typedef class PacketRec {

    public:
        PacketRec(Packet* pkt);
        Packet*    pkt_;
        u_int64_t  ts_;

} PacketRec;

typedef class PacketTraceList : public SList<PacketRec> {

    private:
    u_int32_t release_period_;
    u_int32_t expiration_time_;

    timerObj* release_timer;
    int periodical_release_expired_packet();

    public:
    PacketTraceList();
    PacketRec* search(Packet* pkt);
    int remove(Packet* ep);

    /* get_nid() function is mandortory for an object that does not belong to
     * any node. Returning zero is used to notify S.E. that this object is a global
     * object.
     */

    u_int32_t get_nid() {return 0;}

} PTList;


#endif
