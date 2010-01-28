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

/* This file implements class Llc, which corresponds to
 * LLC layer. In this version of implementation, only
 * unacknowledged mode is supported.
 *
 *
 * CreateDate:  07/31/2003
 * Author:      Chih-che Lin
 * Email:       jclin@csie.nctu.edu.tw
 */
#include <iostream>
#include "llc.h"
#include <ip.h>
#include <tcp.h>
#include <udp.h>

#include <gprs/include/bss_message.h>
#include <gprs/SndcpGmm/GPRS_mm_message.h>
#include <gprs/rlc/rlc_shared_def.h>
#include <nctuns_api.h>
#include <nodetype.h>
#include <maptable.h>
#include <stdlib.h>
#include <mbinder.h>

using namespace std;
#define __USE_LLC_CMD_STRUCTURE_SPECIFIED_BY_GMM
//#define __LLC_DIRECT_CONNECT_DEBUG
//#define __BTS_RLC_LLC_DIRECT_CONNECT
//#define __DATA_INSPECTION_DEBUG
//#define __LLCIE_QOS_DEBUG

extern typeTable* typeTable_;

MODULE_GENERATOR(Llc);

/* rec_upper_layer_pkt() records packets that are sent from upper layer modules, i.e.,
 * packets that usually carry user data. This function also associates the packets with
 * a randomly-generated number to differentiate packets with the same memory addresses.
 * remove_upper_layer_pkt() removes packets that are transmitted successfully by underlying
 * protocol modules,i.e., packets that are successfully received by Llc::recv(). Packets that
 * are not removed by this function will be released by a recycle_timer in a specified period
 * because it is very likely that these packets are dropped in their way to their destination.
 */

PTList* pt_list = NULL;

int rec_upper_layer_pkt(Packet* pkt) {

    #ifndef __REC_UPPER_LAYER_PKT

        return 1;

    #else

        ASSERTION( pkt , "rec_upper_layer_pkt(): pkt is NULL.\n");

        if ( !pt_list ) {
            pt_list = new PTList;
            printf("rec_upper_layer_pkt(): Create a global Packet Trace List.\n");
        }

        ASSERTION( pt_list , "rec_upper_layer_pkt(): pt_list is NULL.\n");

        PacketRec* pkt_rec_p = new PacketRec(pkt);

        pt_list->insert_tail(pkt_rec_p);

        return 1;

    #endif
}

int remove_upper_layer_pkt(Packet* pkt) {

    #ifndef __REC_UPPER_LAYER_PKT

        return 1;

    #else

        ASSERTION( pt_list , "remove_upper_layer_pkt(): pt_list is NULL.\n");

        ASSERTION( pkt , "remove_upper_layer_pkt(): pkt is NULL.\n");


        int res = pt_list->remove(pkt);

        return res;

    #endif
}

PacketRec::PacketRec( Packet* pkt) {

    pkt_  = pkt;
    ts_  = GetCurrentTime();

}

int PacketTraceList::periodical_release_expired_packet() {

    u_int64_t cur_ts  = GetCurrentTime();

    double    cur_sec  = 0;

    TICK_TO_SEC( cur_sec , cur_ts );

    if ( cur_sec <=expiration_time_ )
        return 0;

    ListElem<PacketRec>* ptr;

    for ( ptr=head ; ptr ; ptr=ptr->get_next() ) {

        PacketRec* elem = ptr->get_elem();
        ASSERTION( elem , "PacketTraceList search(): elem is null.\n" );


        u_int64_t diff_tick = cur_ts - elem->ts_;

        double    diff_sec  = 0;

        TICK_TO_SEC( diff_sec , diff_tick  );

        if ( diff_sec >= static_cast<double> (expiration_time_) ) {

            printf("PTList::periodical_release(): diff_sec = %g \n", diff_sec );

            Packet* pkt_p = elem->pkt_;

            if ( pkt_p ) {

                delete pkt_p;
                elem->pkt_ = NULL;

            }

            remove_entry(elem);
            delete elem;

        }
    }


    return 1;
}

PacketTraceList::PacketTraceList() {

    release_period_  = 10;
    expiration_time_ = 30;
    release_timer    = new timerObj;
    release_timer->init();
    release_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),
        (int(NslObject::*)(Event*))&PacketTraceList::periodical_release_expired_packet);

    u_int64_t period_tick;
    SEC_TO_TICK(period_tick,release_period_ );
    //turn off temporarily. release_timer->start( period_tick, period_tick);

}

PacketRec* PacketTraceList::search(Packet* pkt) {

    ListElem<PacketRec>* ptr;

    ASSERTION( pkt , "PTList(): given pkt is NULL.\n");

    ulong* pkt_rnum = reinterpret_cast<ulong*> (pkt->pkt_getinfo("RNUM"));

    if ( !pkt_rnum )
        return NULL;

    for ( ptr=head ; ptr ; ptr=ptr->get_next() ) {

        PacketRec* elem = ptr->get_elem();
        ASSERTION( elem , "PacketTraceList search(): elem is null.\n" );

        Packet* pkt_p = elem->pkt_;

        if ( pkt_p ) {

            ulong* pkt_p_rnum = reinterpret_cast<ulong*> (pkt_p->pkt_getinfo("RNUM"));

            if ( pkt_p_rnum ) {

                if ( (*pkt_rnum) == (*pkt_p_rnum) ) {
                    return elem;
                }

            }

        }
    }

    return NULL;
}

int PacketTraceList::remove(Packet* pkt) {

    PacketRec* ptr = search(pkt);
    if ( ptr ) {
        remove_entry(ptr);
        delete ptr;
        return 1;
    }

    return 0;
}

/**** LLE implementations ****/
    Lle::Lle (int specified_sapi, uchar& reqline, EventQueue* sendqueue , EventQueue* recvqueue, ulong nodetype ) {

        if ( specified_sapi<=0 || specified_sapi>16 ) {
            sapi = 11; /* if given sapi is invalid, use default sapi(the lowest class for sndcp) */
            state = EXCEPTION_BUT_RECOVERED;
        }
        else
            sapi = specified_sapi;

        if ( !(&reqline) ) {
            printf("Caution: In initialization, LLE%d was given an invalid req_slot\n", sapi );
            state = EXCEPTION;
        }
        else
            req_slot = &reqline;

        state                               = TLLI_UNASSIGNED;
        node_type                           = nodetype;
        sendtarget                          = sendqueue;
        recvtarget                          = recvqueue;
        unconfirmed_send_state_variable     = 0;    /* V(U)  */
        unconfirmed_receive_state_variable  = 0;    /* V(UR) */
        unconfirmed_sequence_number         = 0;    /* N(U)  */
        n201_u_userdata                     = 500 ; /* maximum number of octets of an information field */
        n201_u_gmm                          = 400 ; /* maximum number of octets of an information field */
        n201_u_tom                          = 270 ; /* maximum number of octets of an information field */
        n201_u_sms                          = 270 ; /* maximum number of octets of an information field */
        framelist                           = NULL; /* ADM mode need not keep the status of frames sent */
    }


    /* process_l3pdu:
     * process_l3pdu() identifies what kind of this PDU from upper layer is,e.g. LL_UNITDATA_REQ, LL_XID_REQ
     */
    int Lle::process_l3pdu(Event* event_p ) {

        if (this->sendqueue.is_full()) {

	    printf("[%llu]: Lle::process_l3pdu(): the send queue of the Lle is full.\n", GetCurrentTime());
	    destroy_gprs_pkt(event_p);
	    return 0;

	}


        /* XID req/res/ind/cnf are assumed not to appear
         * since SNDCP module omits XID procedures so far.
         */

         /* Create Corresponding LLC frame */
         LlcFrame  fr;
         bool encryption        = false;
         bool protected_mode    = false;
         /* ADM: if the event is LL_UNITDATA_REQ */
         if ( node_type == gprs::ms )
             fr.insert_addr_field(0 , MS_COMMAND , this->sapi );
         else
             fr.insert_addr_field(0 , SGSN_COMMAND , this->sapi );

         fr.insert_ui_format_control_field( encryption, protected_mode , this->unconfirmed_sequence_number );

         /* If the number of data is larger than n201_u_userdata, the upper layer pdu should
          * be framented as multiple small LLC frame of which size is n201_u_userdata.
          * Warnging: Not implemented!
          */

         bss_message* bssmsg  = reinterpret_cast<bss_message*> (event_p->DataInfo_);
         llc_option*  llc_opt = reinterpret_cast<llc_option*> (bssmsg->llc_option);

         if (!llc_opt) {
         
	    llc_opt = new llc_option;
            bssmsg->llc_option = (char*)llc_opt;
         
	 }

         if ( node_type == gprs::ms )
            llc_opt->command = GRR_UNITDATA_REQ;
         else
            llc_opt->command = BSSGP_DL_UNITDATA_REQ;

         llc_opt->tlli = llme->get_tlli();

         char* data = NULL;

         if ( bssmsg->packet ) {

            data = bssmsg->packet->pkt_get();

            if (!data) {
                FREE_BSS_EVENT(event_p);
                return 0;
            }

         }

         #ifdef __LLGMM_COMMANDS_ARE_TRANSMITTED_SEPARATELY__

            IS_NULL_STR(bssmsg->packet, "LlcFrame pack(): pkt is null\n" , 0);

            long datalen = get_pkt_payload_length(bssmsg->packet);
            bssmsg->user_data      = new char[datalen];
            bssmsg->user_data_len  = datalen;
            memcpy(bssmsg->user_data, bssmsg->packet->pkt_get() , datalen );

        #else
            if ( bssmsg->packet) {
                long datalen = get_pkt_payload_length(bssmsg->packet);
                bssmsg->user_data      = new char[datalen];
                bssmsg->user_data_len  = datalen;
                memcpy(bssmsg->user_data, bssmsg->packet->pkt_sget() , datalen );

            }
            else {
                IS_NULL_STR( bssmsg->user_data , "Llc: bssmsg->user_data is NULL\n" , 0 );
            }

        #endif

         //fr.insert_info_field(data);
         fr.insert_fcs();
         fr.pack(event_p); /* pack() should generate LLC header and insert it to bss_message->llc_option */

         this->sendqueue.enqueue(event_p);
         *req_slot = 1;
         return 1;
    }

    /* send_uidata: */
    int Lle::send_uidata(int num_of_pkts ) {
        /* for ADM, there is not a queue to store uidata. Therfore,
         * send a packet at a time.
         */
        Event *ep = sendqueue.dequeue();
        if (ep)
            sendtarget->enqueue(ep);
        return 1;
    }

    /* process_incoming_frame:
     */
    int Lle::process_incoming_frame(Event* ep) {
        /* IS_NULL(ep,0);
         * Assertion is performed in the invoker function.
         */
        /* add LL_UNITDATA_IND into bssmsg->llc_option */
        bss_message*    bssmsg  = reinterpret_cast<bss_message*> (ep->DataInfo_);
        llc_option*     llc_opt = reinterpret_cast<llc_option*> (bssmsg->llc_option);

        if (llc_opt) {
            /* primitive checking is reduced */
            if ( llc_opt->command == GRR_UNITDATA_IND )
                ;
            if ( llc_opt->command == BSSGP_UL_UNITDATA_IND)
                ;
        }
        else {

            llc_opt = new llc_option;
                bssmsg->llc_option = (char*)llc_opt;
            }

        if (node_type == gprs::ms )
            llc_opt->command = LL_UNITDATA_IND_MS;
        else
            llc_opt->command = LL_UNITDATA_IND_SGSN;

        /* deliver this frame to the recvqueue of LLC module */
        recvtarget->enqueue(ep);
        return 1;
    }

    /**** LLME implementations ****/
    Llme::Llme(Lle** lle , ulong tlli) {
        state       = TLLI_UNASSIGNED;
        _tlli_cur   = tlli;
        _tlli_old   = tlli;

        lle_num     = 17;
        lle_array = new Lle*[lle_num];

        if (!lle) {
            cout <<"Llme Llme(): lle entities are null" <<endl;
        }

        for ( ulong i=0 ; i<lle_num ; ++i ) {
            if ( lle[i] )
                lle_array[i] = lle[i];
            else
                lle_array[i] = NULL;
        }
    }

    int Llme::set_tlli(ulong tlli_new, ulong tlli_old) {

        if ( (tlli_old == 0xffffffff) && (tlli_new != 0xffffffff) ) {
            /* tlli_new shall be assigned */
            _tlli_cur   = tlli_new;
            _tlli_old   = 0;
            state       = TLLI_ASSIGNED;
        }
        else if ( (tlli_new == 0xffffffff) && (tlli_old != 0xffffffff) ) {
            /* tlli_old shall be unassigned */
            _tlli_cur   = 0;
            _tlli_old   = 0;
            state       = TLLI_UNASSIGNED;
        }
        else {
            /* both tlli_old and tlli_new are assigned. It means that for
            * a received LLC frame, both tlli_old and tlli_new shall be accepted;
            * for a transmitted frame, tlli_new shall be used.
            */
            _tlli_cur  = tlli_new;
            _tlli_old  = tlli_old;
            state      = TLLI_CHANGED_AND_TLLI_OLD_STILL_VALID;
        }


        for ( int i=0 ;i<17 ; ++i ) {
            if ( lle_array[i] )
                lle_array[i]->state = state;
        }

        return 1;
    }

    int Llme::process_control_primitive(Event* pkt) {
        bss_message* bssmsg  = reinterpret_cast<bss_message*> (pkt->DataInfo_);
        llc_option*  llc_opt = reinterpret_cast<llc_option*> (bssmsg->llc_option);

        if ( !llc_opt) {
            cout <<"Llme with tlli="<< this->_tlli_cur <<"process_control_primitives(): llc_opt is null" << endl;
            FREE_BSS_EVENT(pkt);
            return 0;
        }

        if ( llc_opt->command == LLGMM_ASSIGN ) {
            ulong tlli_old, tlli_new, kc, cipher_algo_no;
            kc = 0;
            cipher_algo_no = 0 ;
            /* check necessary parameters */
            tlli_old = llc_opt->oldtlli;
            tlli_new = llc_opt->tlli;

            if ( (tlli_old == 0xffffffff) && (tlli_new != 0xffffffff) ) {
                /* tlli_new shall be assigned */
                _tlli_cur   = tlli_new;
                _tlli_old   = 0;
                state       = TLLI_ASSIGNED;
            }
            else if ( (tlli_new == 0xffffffff) && (tlli_old != 0xffffffff) ) {
                /* tlli_old shall be unassigned */
                _tlli_cur   = 0;
                _tlli_old   = 0;
                state       = TLLI_UNASSIGNED;
            }
            else {
                /* both tlli_old and tlli_new are assigned. It means that for
                 * a received LLC frame, both tlli_old and tlli_new shall be accepted;
                 * for a transmitted frame, tlli_new shall be used.
                 */
                 _tlli_cur  = tlli_new;
                 _tlli_old  = tlli_old;
                 state      = TLLI_CHANGED_AND_TLLI_OLD_STILL_VALID;
            }


                for ( int i=0 ;i<17 ; ++i ) {
                if ( lle_array[i] )
                    lle_array[i]->state = state;
            }

        }

        #ifdef __LLGMM_COMMANDS_ARE_TRANSMITTED_SEPARATELY__
            FREE_BSS_EVENT(pkt);
        #endif

        return 1;
    }

/**** LlcIe implementations ****/

    LlcIe::LlcIe(EventQueue* sendqueue,EventQueue* recvqueue, ulong nodetype , ulong tlli ) {

        bzero(send_req,17);
        uchar   &req1  = send_req[1];
        uchar   &req3  = send_req[3];
        uchar   &req5  = send_req[5];
        uchar   &req9  = send_req[9];
        uchar   &req11 = send_req[11];

        for (int i=0 ; i<17 ; ++i )
            lle[i] = NULL;

        lle[1]      = new Lle(LLGMM,req1 , sendqueue , recvqueue , nodetype);
        lle[3]      = new Lle(LL3  ,req3 , sendqueue , recvqueue , nodetype);
        lle[5]      = new Lle(LL5  ,req5 , sendqueue , recvqueue , nodetype);
        lle[9]      = new Lle(LL9  ,req9 , sendqueue , recvqueue , nodetype);
        lle[11]     = new Lle(LL11 ,req11, sendqueue , recvqueue , nodetype);

        llme        = new Llme(lle , tlli);
        lle[1]->set_llme(llme);
        lle[3]->set_llme(llme);
        lle[5]->set_llme(llme);
        lle[9]->set_llme(llme);
        lle[11]->set_llme(llme);

        node_type   = nodetype;
    }

    LlcIe::~LlcIe() {
        for (int i=0 ; i<17; ++i)
            if (lle[i])  delete lle[i];

        if (llme) delete llme;
    }

    /* multiplexer: *********************************************************
     * multiplexer() is a function that multiplex LLEs with different sapis.
     * It controls which entity can send first and how many packets the
     * entity can send at a time. Any algorithm can be adopted as the arbitration
     * strategy. However, at present we use prioritized queues for each sapi. Each
     * queue conforms to FIFO rule. The fewer sapi value, the higher priority.
     */
    int LlcIe::multiplexer() {

        for (int i=1; i<=16 ; ++i ) {
            /* lle[i] has data waiting to be sent */
            if ( send_req[i] >0 ) {
                lle[i]->send_uidata(send_req[i]);
                send_req[i] = 0;
            }
        }
        return 1;
    }


    /* dispatch: ***************************************************************
     * dispatch() examines the types of service primitives that LLC layer provides
     * for upper layers. The type indicators are stored in PT_INFO field.
     */
    int LlcIe::dispatch(Event* ep) {

        bss_message*    bssmsg             = reinterpret_cast<bss_message*> (ep->DataInfo_);
        llc_option*     llc_opt            = reinterpret_cast<llc_option*>( bssmsg->llc_option);

        /* identify the type of the primitive received */
        int res,pended_sapi;
        if ( llc_opt->command == LLGMM_ASSIGN ) {

            llme->process_control_primitive(ep);

            /* since the data of GMM are carried in the control primitive as well,
             * LLC shall send this event down to RLC layer.
             */

            pended_sapi = LLGMM;
            //printf("LlcIe::send(): assign qos_level = %d \n", LLGMM);

            if ( (pended_sapi >=1) && (pended_sapi <=16) ) {

                if ((res=lle[pended_sapi]->process_l3pdu(ep)) <0 ) {
                    cout << "LlcIe dispatcher(): Exception occurs in lle" << pended_sapi
                        << "->process_l3pdu()"<< endl;
                    return res;
                }
            }

        }

        else if ( llc_opt->command == LL_UNITDATA_REQ_MS ) {

            pended_sapi = qos_classifier(llc_opt);
            if ( (pended_sapi >=1) && (pended_sapi <=16) ) {

                if ((res=lle[pended_sapi]->process_l3pdu(ep)) <0 ) {
                    cout << "LlcIe dispatcher(): Exception occurs in lle" << pended_sapi
                         << "->process_l3pdu()"<< endl;
                    return res;
                }
            }
        }

        else if ( llc_opt->command == LLGMM_TRIGGER ) {

            pended_sapi = qos_classifier(llc_opt);
            if ( (pended_sapi >=1) && (pended_sapi <=16) ) {

                if ((res=lle[pended_sapi]->process_l3pdu(ep)) <0 ) {
                    cout << "LlcIe dispatcher(): Exception occurs in lle" << pended_sapi
                         << "->process_l3pdu()"<< endl;
                    return res;
                }
            }
        }

        else if ( llc_opt->command == LL_UNITDATA_REQ_SGSN ) {
            pended_sapi = qos_classifier(llc_opt);
            if ( (pended_sapi >=1) && (pended_sapi <=16) ) {

                if ((res=lle[pended_sapi]->process_l3pdu(ep)) <0 ) {
                    cout << "LlcIe dispatcher(): Exception occurs in lle" << pended_sapi
                         << "->process_l3pdu()"<< endl;
                    return res;
                }
            }
        }
        else
            ; /* other types for ABM mode will be used in the future. */

        return 1;
    }


    /* qos_classifier: *******************************************************************
     * qos_classifier() returns correponding sapi value according to qos_profile and nsapi
     * sent by upper layer. If return 0, it means that this packet is sent to LLME sublayer.
     */

    int LlcIe::qos_classifier( llc_option* msg) {


        #ifdef  __LLCIE_QOS_DEBUG

        printf("Qos_classifier(): qos = %d \n" , msg->qos);

        #endif

        if ( msg->qos == 1 )
            return LLGMM;
        else
            return LL3;
    }

    /* LlcIe send():  ****************************************************************************/
    int LlcIe::send(Event *event_p ) {

        if ( dispatch(event_p) < 0 )
            return 0; /* negative value indicates unknown type of this event */

        multiplexer();

        return 1;
    }

    /* LlcIe demultiplexer(): ********************************************************************
     * demultiplexer() analyzes the incoming frame type and deliver the frame
     * to the appropriate LLE to strip off the LLC header and deliver the frame to L3
     */
    int LlcIe::demultiplexer(Event* e_p) {
        bss_message *bss_msg;
        bool    pd,cr;
        uchar   sapi;

        bss_msg = reinterpret_cast<bss_message*> (e_p->DataInfo_);
        {
           uchar* llc_header = reinterpret_cast<uchar*> (bss_msg->llc_header);
           uchar addr_field = llc_header[0];
           uchar mask = 0x80;
           pd = addr_field & mask;
           mask = 0x40;
           cr = addr_field & mask;
           mask = 0x0f;
           sapi = addr_field & mask;
        }

        if ( pd!=0 ) {
            cout << "Exception in LlcIe::demultiplexer(): receive a frame not with LLC type" << endl;
            return 0;
        }

        if ( !lle[sapi] ) {
            cout << "Exception in LlcIe::demultiplexer(): unsupported sapi = " << (int)(sapi) <<endl;
            return 0;
        }
        else {
            lle[sapi]->process_incoming_frame(e_p);
        }
        return 1;
    }

    /* recv: *****************************************************************************/
    int LlcIe::recv(Event *ep ) {
        IS_NULL_STR(ep, "LlcIe recv(): ep is null" , 0);
        IS_NULL_STR(ep->DataInfo_, "LlcIe recv(): ep->DataInfo_ is null" , 0);
        return demultiplexer(ep);
    }


    /******* Llc module **********************/
        Llc::Llc(u_int32_t type, u_int32_t id, struct plist* pl, const char *name) : NslObject(type, id, pl, name) {
        llcie_list  = new LlcIeList;
        sendqueue   = new EventQueue;
        recvqueue   = new EventQueue;

        const char* nodename = typeTable_->toName(get_type());
        if (!nodename) {
            cout << "Llc Llc(): Initialization failed: nodetype cannot found!" << endl;
            return;
        }
        if ( !strcmp("PHONE",nodename) )
            nodetype = gprs::ms;
        else if (!strcmp("SGSN",nodename))
            nodetype = gprs::sgsn;
        else {
            cout << "Llc Llc(): Initialization failed: nodetype is incorrect!" << endl;
            cout << "Llc Llc(): received nodetype =" << nodename << endl;
        }

        freeze_flag = false;
        gmm_update_timer = new timerObj;

    }

    Llc::~Llc() {
        if (llcie_list)
            delete llcie_list;
        if (sendqueue)
            delete sendqueue;
        if (recvqueue)
            delete recvqueue;

        if ( gmm_update_timer )
            delete gmm_update_timer;
    }

    int Llc::gmm_update() {
        ulong rai = 13;
        static char  bsic_array[7] = { 1 , 2, 0 , 3 , 4, 5, 6};
        static int   index = 0;

        struct preferred_list* mm_upd_list = (struct preferred_list*) malloc(sizeof(struct preferred_list));
        bzero(mm_upd_list,sizeof(struct preferred_list));
        bss_info* ptr  = mm_upd_list->list_head_p;

        for ( int i = 0 ; i<4 ; ++i ) {
            int ind = ( index + i ) % 7;
            bss_info* elem  = new bss_info;
            elem->bsic  = bsic_array[ind];
            elem->rai   = rai;
            elem->next  = NULL;

            if ( i == 0 ) {

                ptr  = elem;
                mm_upd_list->list_head_p = ptr;


            }
            else {
                ptr->next = elem;
                ptr = ptr->next;
            }
            ++mm_upd_list->num_of_list;

        }

        Event* ep;
        CREATE_BSS_EVENT(ep);
        bss_message* bmsg       = reinterpret_cast<bss_message*> (ep->DataInfo_);
        bmsg->flag              = PF_RECV;
        bmsg->sndcp_option      = reinterpret_cast<char*> (mm_upd_list);
        bmsg->sndcp_option_len  = sizeof(struct preferred_list);


        char *buf = new char[30];
        memcpy ( buf , "route_area_update_rq" , 21 );

        bmsg->user_data     = buf;
        bmsg->user_data_len = 30;

        return NslObject::recv(ep);

    }

    int Llc::init() {
        int (NslObject::*upcall)(MBinder *);
        upcall = (int (NslObject::*)(MBinder *))&Llc::continue_send;
        sendtarget_->set_upcall(this, upcall);
        cout <<"LLC: set up an upcall in the send_target_ to trigger continuous_send()." << endl;

        #ifdef __LLC_DIRECT_CONNECT_DEBUG

        if (nodetype == gprs::ms )
            peer_llc = InstanceLookup(3,1, "LLC" );
        else
            peer_llc = InstanceLookup(1,1, "LLC" );

        if (nodetype == gprs::ms ) {
            gmm_update_timer->init();
            gmm_update_timer->setCallOutObj(this, (int (NslObject::*)(Event*)) &Llc::gmm_update );
            u_int64_t tick;
            SEC_TO_TICK(tick, 10);
            gmm_update_timer->start(tick,tick);
        }

        /*#elsif __BTS_RLC_LLC_DIRECT_CONNECT

            if ( nodetype != gprs::ms ) {
                peer_llc = InstanceLookup( 2 , 1 , "BTSMAC" );
            }
        */
        #else

            peer_llc = NULL;

        #endif

        return NslObject::init();

        /* recv() doesn't set an upcall so far since the congestion in recvtarget_ shall not occur */
    }

    int Llc::send(Event* ep) {

        IS_NULL_STR(ep,"Llc send():ep is null\n",0);
        IS_NULL_STR(ep->DataInfo_,"Llc send() ep->DataInfo_ is null\n" , 0);

        bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        IS_NULL_STR(bssmsg->llc_option, "Llc send(): Assertion failed: Received an event without llc_option\n" , 0);


        /* Added by jclin on 02/02/2005:
         * Add a new info field to record whether this packet is transmitted
         * successfully.
         * Disabled_since 02/17/2005

        if ( bssmsg->packet ) {

            Packet* pkt = bssmsg->packet;

            ulong   randomized_num = rand();
            pkt->pkt_addinfo("RNUM", (char*)&randomized_num , sizeof(ulong) );
            printf("LLC::send(): Add a randomized value %lu into pkt.\n", randomized_num );


            int rec_res = rec_upper_layer_pkt(pkt);

            if ( rec_res < 0 ) {
                printf("LLC::send(): record upper_layer_pkt failed.\n");
            }

        }

        */

        #ifdef __DATA_INSPECTION_DEBUG


        cout << endl << "Node " << get_nid() << " Port " << get_port()
                << ((nodetype==gprs::ms)?" MS":" BTS") << "LLC: send pkt : ";

        if ( bssmsg->user_data)
            cout << (char*)(bssmsg->user_data) << endl;
        else
            cout << "NULL data or inter-module control event." << endl;

        printf("pkt addr = %ld \n", (ulong)(bssmsg->packet) );

        #endif



        #ifdef __USE_LLC_CMD_STRUCTURE_SPECIFIED_BY_GMM

            if ( bssmsg->user_data != NULL ) {

                if ( !strcmp(bssmsg->user_data, "route_area_update_rq") ||
                     !strcmp(bssmsg->user_data,"in_standby_state") ) {

                    printf("Llc::send(): ignore this control packet. cmd=%s \n", bssmsg->user_data );
                    FREE_BSS_EVENT_INCLUDING_USER_DATA( ep );
                    //FREE_BSS_EVENT( ep );
                    return 1;

                }

            }

            LLCOption* gmm_opt      = reinterpret_cast<LLCOption*> (bssmsg->llc_option);

            llc_option* llc_opt     = new llc_option;

            /* convert the command string of the upper layer into predefined command code */
            llc_opt->tlli       = gmm_opt->tlli;
            llc_opt->oldtlli    = gmm_opt->oldtlli;

            if ( nodetype == gprs::ms ) {
                llc_opt->qos        = gmm_opt->qos;
                llc_opt->req_ch_no  = gmm_opt->req_ch_no;
                llc_opt->req_bsic   = gmm_opt->req_bsic;
                llc_opt->req_rai    = gmm_opt->req_rai;
            }
            else {
                llc_opt->qos        = 3;
                llc_opt->req_ch_no  = -1;
                llc_opt->req_bsic   = -1;
                llc_opt->req_rai    = -1;
            }

            if ( llc_opt->req_ch_no >=0 ) {

                /* If a change cell request received from the upper layer */

                bssmsg->llc_option      = reinterpret_cast<char*> (llc_opt);
                bssmsg->llc_option_len  = sizeof(llc_option);

		/* C.C. Lin:
		 * Mark this packet as an inter-module communication pkt.
		 * This can notify the GPRS_FIFO module that it cannot
		 * drop this important control message.
		 */

		bssmsg->imc_flag = true;

                freeze_flag = 1;
                return NslObject::send(ep);

            }

            if ( bssmsg->user_data ) {
                if ( !strncasecmp ( reinterpret_cast<char*> (bssmsg->user_data) , "detach_accept" , 13 ) ) {
                    llc_opt->addtional_info = LLGMM_DETACH;
                }
            }

            if ( !strcmp(gmm_opt->control, "LLGMM_ASSIGN" ) ) {
                llc_opt->command = LLGMM_ASSIGN;
            }
            else if ( !strcmp( gmm_opt->control , "LLGMM_TRIGGER")) {
                llc_opt->command = LLGMM_TRIGGER;
            }
            else if ( !strcmp( gmm_opt->control , "LLGMM_RESET")) {

                llc_opt->addtional_info = LLGMM_RESET;

                cout << "LLC: received LLGMM_RESET: tlli = " << llc_opt->tlli
                        << " oldtlli= " << llc_opt->oldtlli << endl;

                bssmsg->llc_option      = reinterpret_cast<char*> (llc_opt);
                bssmsg->llc_option_len  = sizeof(llc_option);


		/* C.C. Lin:
		 * Mark this packet as an inter-module communication pkt.
		 * This can notify the GPRS_FIFO module that it cannot
		 * drop this important control message.
		 */

		bssmsg->imc_flag = true;

                NslObject::send(ep);

                /* remove the structure of llcie with this tlli */
                LlcIe* tmp_llcie = llcie_list->search(llc_opt->tlli);
                
		if ( tmp_llcie ) {

                    llcie_list->remove_entry( tmp_llcie );
                    delete tmp_llcie;
                
		}

                return 1;

            }
            else if ( !strcmp( gmm_opt->control , "PAGING_RESPONSE")) {
                llc_opt->command = PAGING_RESPONSE;
            }
            else {
                printf ("Llc send(): Assertion failed: unknown GMM command = %s \n", gmm_opt->control);
                assert(0);
	    }

            bssmsg->llc_option      = reinterpret_cast<char*> (llc_opt);
            bssmsg->llc_option_len  = sizeof(llc_option);

        #else

            llc_option* llc_opt     = reinterpret_cast<llc_option*> (bssmsg->llc_option);
            bssmsg->llc_option_len  = sizeof(llc_option);

        #endif

        free(gmm_opt);
        LlcIe* llcie = llcie_list->search(llc_opt->tlli);

        if (!llcie)
            llcie = llcie_list->search(llc_opt->oldtlli);

        if (!llcie) {

            if ( nodetype == gprs::ms ) {

                /* For MS, before sending any layer-3 PDU GMM should assign a TLLI to
                 * LLC layer to conduct the data transfer. Otherwise, the data to be
                 * transferred will be discarded by LLC layer undoutedly.
                 *
                 *
                 * TLLI type: (reference page. 261 in "GPRS General Packet Radio Service", Bates. )
                 *
                 * random TLLI: generated by GMM of MS with a random number and
                 *              used for sending attach request and RA update request.
                 *
                 * local  TLLI: generated by GMM of SGSN to permit a MS to transfer data
                 */

                /* uplink action */
                if (llc_opt->command == LLGMM_ASSIGN) {
                    llcie = new LlcIe( sendqueue, recvqueue, nodetype , llc_opt->tlli );
                    llcie_list->insert_tail(llcie);
                    llcie->send(ep);
                    return continue_send();
                }

                /* response for downlink action */
                else if ( llc_opt->command == PAGING_RESPONSE ) {

                    rlc_option* rlc_opt;

                    if (!bssmsg->rlc_option) {
                        rlc_opt = new rlc_option;
                        bssmsg->rlc_option = reinterpret_cast<char*> (rlc_opt);
                    }
                    else
                        rlc_opt = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

                    rlc_opt->cmd = PAGING_RESPONSE;

                    /* the paging response generated by GMM shall be carried by bssmsg->userdata */
                    return NslObject::send(ep);
                }

                else /* without TLLI indicated, data should be discarded */
                    return 0;
            }
            else {
                /* For SGSN, if a downlink layer-3 PDU is to be transferred without corresponding TLLI,
                 * it means that the destination MS has already moved out this routing area.
                 */
                return 0;
            }
        }
        else {

	    if (freeze_flag) {

                bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
		bssmsg->user_data     = 0;
		bssmsg->user_data_len = 0;
		destroy_gprs_pkt(ep);
		return 1;

	    }

            llcie->send(ep);
            return continue_send();
	    //int res = continue_send();
	    //return 1;
        }

        return 1;
    }

    int Llc::recv(Event* ep) {

        IS_NULL_STR(ep,"Llc recv():ep is null\n",0);
        IS_NULL_STR(ep->DataInfo_,"Llc recv() ep->DataInfo_ is null\n" , 0);
        bss_message* bssmsg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        IS_NULL_STR(bssmsg->llc_option, "Llc recv(): Assertion failed: Received an event without llc_option\n" , 0);
        llc_option* llc_opt = reinterpret_cast<llc_option*> (bssmsg->llc_option);

        #ifdef __DATA_INSPECTION_DEBUG

        cout << endl << "Node " << get_nid() << " Port " << get_port()
                << ((nodetype==gprs::ms)?" MS":" BTS") << "LLC: recv pkt : ";

        if ( bssmsg->user_data )
            cout << (char*)(bssmsg->user_data) << endl;
        else
            cout << "NULL data field or inter-module contron packet.\n" << endl;

        printf("pkt addr = %ld \n", (ulong)(bssmsg->packet) );

        #endif

        if ( bssmsg->packet ) {

            Packet* pkt = bssmsg->packet;

            const char *sep="+";
            char *brkt, *str, tok[4][50];
            char *buf = (char*) (pkt->pkt_sget());

            int header_len = sizeof(struct ip) + sizeof(struct udphdr);

            buf = buf + header_len;
            int i = 0;

            for( str=strtok_r(buf, sep, &brkt); str; str=strtok_r(NULL, sep, &brkt))
            {
                strcpy( tok[i], str);
                i++;
            }

            int pid = (int)atoi( tok[0] );

            printf("node [%u]: Llc recv pkt no. %d packet \n", get_nid(), pid );

            remove_upper_layer_pkt(pkt);

        }

        rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
        if ( rlc_opt ) {

            if ( rlc_opt->cmd == GMM_UPDATE ) 
                return NslObject::recv( ep );

            else if ( rlc_opt->cmd == ROAMING_COMPLETED ) {

                freeze_flag = 0;
                FREE_BSS_EVENT_INCLUDING_USER_DATA( ep );
                return 1;
            
	    }
            else
                ;
        }

        /* examine whether this packet is RLC indication event */
        if (llc_opt->command == SEND_PAGING_REQUEST ) {
            /* passing to GMM layer , and destroy this event if it is no longer needed */

        }
        else
            ;

        LlcIe* llcie = llcie_list->search(llc_opt->tlli);

        if (!llcie) {

            if ( nodetype == gprs::ms )  {
                llcie = llcie_list->get_head()->get_elem();
                if ( llcie ) {
                    if ( is_random_tlli(llcie->get_tlli()) ) {

                        llcie->set_tlli( llc_opt->tlli, llc_opt->tlli );
                        llcie->recv(ep);

                    }
                }
                else {
                    /* the MS is not attached to GPRS network or not in ready state */
                    return 0;
                }

            }
            else {

                if (is_random_tlli(llc_opt->tlli)) {
                    /* this is a GMM message with Attach Req. or RA Update:
                     * LLC creates a LLC IE for this LLC pdu. GMM in SGSN shall change the TLLI
                     * of this LLC IE with the TLLI it generated after it accepts the attach request.
                     */
                    llcie = new LlcIe( sendqueue, recvqueue, nodetype,llc_opt->tlli);
                    llcie_list->insert_tail(llcie);
                    llcie->recv(ep);
                }
                else /* illegal tlli */
                    return 0;
            }
        }
        else {
            llcie->recv(ep);
        }

        return continue_recv();
    }

    int Llc::continue_send() {

        if ( freeze_flag )
            return 1;

        #ifdef __LLC_DIRECT_CONNECT_DEBUG

        if (!sendqueue->is_empty()) {
            Event* epkt         = sendqueue->dequeue();
            bss_message* bmsg   = reinterpret_cast<bss_message*> (epkt->DataInfo_);
            bmsg->flag          = PF_RECV;
            return peer_llc->recv(epkt);
        }
        else
            return 1;

        /*#elsif __BTS_RLC_LLC_DIRECT_CONNECT

        if ( peer_llc->sendtarget_->qfull() )
            return 0;
        if (!sendqueue->is_empty()) {
            Event* epkt = sendqueue->dequeue();
            return put(epkt, peer_llc->sendtarget_);
        }
        else
            return 1;
        */
        #else

        if ( sendtarget_->qfull() )
            return 0;
        
	if (!sendqueue->is_empty()) {

            Event* ep = sendqueue->dequeue();
            
	    int res = put(ep, sendtarget_);
	    return res;

        }
        else
            return 1;

        #endif
    }

    int Llc::continue_recv() {
        if ( recvtarget_->qfull() )
            return 0;
        if (!recvqueue->is_empty()) {
            Event* epkt = recvqueue->dequeue();
            if ( epkt )
                return put(epkt, recvtarget_);
            else
                return 1;
        }
        else
            return 1;
    }

    /********* LlcIeList imple. *****************/
    LlcIe* LlcIeList::search(ulong tlli) {
        ListElem<LlcIe>* ptr;
        for (ptr = head ; ptr ; ptr=ptr->get_next()) {
            if ((ptr->get_elem())->get_tlli() == tlli) {
                return ptr->get_elem();
            }
        }
        return NULL;
    }
