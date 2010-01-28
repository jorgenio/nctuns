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

#ifndef __BSS_MSG_IMPL_H__
#define __BSS_MSG_IMPL_H__
#include <exception>
#include <iostream>
#include <event.h>
#include <nctuns_api.h>
#include <gprs/include/bss_message.h>
#include <gprs/include/burst_type.h>
#include <gprs/include/GPRS_rlcmac_message.h>
#include <gprs/include/generic_list_impl.h>
#include <packet.h>
#include <stdlib.h>
#include <wphyInfo.h>
#ifdef  __DMALLOC_DEBUG__
#include <dmalloc.h>
#endif

//#define __BSS_MESSAGE_ROUTE_DETAILED_DEBUG

using namespace std;

ETList* etl_p = NULL;

int rec_upper_layer_pkt(Packet* pkt);
int remove_upper_layer_pkt(Packet* pkt);

int rec_created_event(Event* ep) {

    if ( !etl_p )
        etl_p = new ETList;

    if ( ep->created_nid_ > 4096 ) {
        printf("rec_created_event(): nid %ul exceeds the maximum value.\n", ep->created_nid_);
        exit(1);
    }

    EventRec* er_p = new EventRec(ep);
    return etl_p->insert_tail(er_p);
}

int rev_deleted_event(Event* ep) {

    return etl_p->remove(ep);

}

EventRec::EventRec( Event* ep) {

    ep_  = ep;
    ts_  = GetCurrentTime();
    nid_ = ep->created_nid_;

}

EventRec* EventTraceList::search(Event* ep) {

    ListElem<EventRec>* ptr;
        for ( ptr=head ; ptr ; ptr=ptr->get_next() ) {

            EventRec* elem = ptr->get_elem();
            ASSERTION( elem , "EventTraceList search(): elem is null.\n" );
            if ( (elem->ts_ == ep->created_ts_) && (elem->nid_ == ep->created_nid_) ) {
                return elem;
            }
        }

        return NULL;
}

int EventTraceList::remove(Event* ep) {

    EventRec* ptr = search(ep);
    if ( ptr ) {
        remove_entry(ptr);
        delete ptr;
        return 1;
    }

    return 0;
}

int free_pkt( Packet* pkt ) {

    IS_NULL_STR(pkt, "free_pkt(): given argument, pkt, is null.\n" , -1 );

    //pkt->release();
    //free(pkt);
    delete pkt;

    return 1;
}

int create_llc_option(bss_message* bssmsg) {
    IS_NULL_STR(bssmsg,"create_llc_option(): bssmsg is null\n" , -1);

    llc_option* ptr = new llc_option;
    IS_NULL_STR(ptr,"create_llc_option(): llc_option is null\n" , -1);
    bzero(ptr,sizeof(llc_option));
    bssmsg->llc_option      = reinterpret_cast<char*> (ptr);
    bssmsg->llc_option_len  = sizeof(llc_option);

    return 1;

}

int create_rlc_option(bss_message* bssmsg) {
    IS_NULL_STR(bssmsg,"create_rlc_option(): bssmsg is null\n" , -1);

    rlc_option* ptr = new rlc_option;
    bzero(ptr,sizeof(rlc_option));
    IS_NULL_STR(ptr,"create_rlc_option(): rlc_option is null\n" , -1);

    ptr->tfi            = -1;
    ptr->ctl_msg_type   = -1;
    ptr->cmd            = 0;         /* 0: user_data
                                      * 1: uplink_tbf_establish.    2: downlink_tbf_establish with user_data as imsi.
                                      * 3: uplink_tbf_release.      4: downlink_tbf_release.
                                      * 5: uplink_tbf_establihsed   6: downlink_tbf_estatblished
                                      * 7: paging_request           8: paging_response
                                      * messages with cmd == tbf_established shall carry tbfdescriptor in user_data field.
                                      */
    ptr->tfi            = -1;
    ptr->cs             = 1;
    ptr->rrbp           = gprs::rrbp_unused;

    bssmsg->rlc_option      = reinterpret_cast<char*> (ptr);
    bssmsg->rlc_option_len  = sizeof(rlc_option);

    return 1;

}

int create_mac_option(bss_message* bssmsg) {
    IS_NULL_STR(bssmsg,"create_mac_option(): bssmsg is null\n" , -1);

    mac_option* ptr = new mac_option;
    bzero(ptr,sizeof(mac_option));
    IS_NULL_STR(ptr,"create_mac_option(): mac_option is null\n" , -1);

    ptr->burst_type = NORMAL_BURST;

    bssmsg->mac_option      = reinterpret_cast<char*> (ptr);
    bssmsg->mac_option_len  = sizeof(mac_option);

    return 1;

}

int insert_llc_header(bss_message* bssmsg, void* ptr,ulong len) {
    IS_NULL_STR(bssmsg,"create_llc_header(): bssmsg is null\n" , -1);
    IS_NULL_STR(ptr,"create_llc_header(): llc_header is null\n" , -1);

    bssmsg->llc_header      = reinterpret_cast<char*> (ptr);
    bssmsg->llc_header_len  = len;

    return 1;

}

int insert_rlc_header(bss_message* bssmsg, void* ptr,ulong len) {
    IS_NULL_STR(bssmsg,"create_rlc_header(): bssmsg is null\n" , -1);
    IS_NULL_STR(ptr,"create_rlc_header(): rlc_header is null\n" , -1);

    bssmsg->rlc_header      = reinterpret_cast<char*> (ptr);
    bssmsg->rlc_header_len  = len;

    return 1;

}


int insert_mac_header(bss_message* bssmsg, void* ptr,ulong len) {
    IS_NULL_STR(bssmsg,"create_mac_header(): bssmsg is null\n" , -1);
    IS_NULL_STR(ptr,"create_mac_header(): mac_header is null\n" , -1);

    bssmsg->mac_header      = reinterpret_cast<char*> (ptr);
    bssmsg->mac_header_len  = len;

    return 1;

}

int insert_user_data(bss_message* bssmsg, void* ptr,ulong len) {
    IS_NULL_STR(bssmsg,"create_user_data(): bssmsg is null\n" , -1);
    IS_NULL_STR(ptr,"create_user_data(): user_data pointer is null\n" , -1);

    bssmsg->user_data      = reinterpret_cast<char*> (ptr);
    bssmsg->user_data_len  = len;

    return 1;

}




int free_bss_msg_elem(bss_message* ptr) {
    if (ptr->mac_option) {

        mac_option* mac_opt = reinterpret_cast<mac_option*> (ptr->mac_option);
        bzero( ptr->mac_option , sizeof(mac_option));
        delete mac_opt;
        ptr->mac_option = NULL;

    }
    if (ptr->rlc_option) {
        bzero( ptr->rlc_option , sizeof(rlc_option));
        delete reinterpret_cast<rlc_option*> (ptr->rlc_option);
        ptr->rlc_option = NULL;
    }
    if (ptr->llc_option) {
        bzero( ptr->llc_option , sizeof(llc_option));
        delete reinterpret_cast<llc_option*> (ptr->llc_option);
        ptr->llc_option = NULL;
    }
    if (ptr->sndcp_option) {
        delete reinterpret_cast<char*> (ptr->sndcp_option);
        ptr->sndcp_option = NULL;
    }
    if (ptr->rlc_header) {
        delete ptr->rlc_header;
        ptr->rlc_header = NULL;
    }
    if (ptr->mac_header) {
        delete ptr->mac_header;
        ptr->mac_header = NULL;
    }
    if (ptr->llc_header) {
        delete ptr->llc_header;
        ptr->llc_header = NULL;
    }
    if (ptr->sndcp_header) {
        delete ptr->sndcp_header;
        ptr->sndcp_header = NULL;
    }

    if ( ptr->wphyinfo ) {
        free(ptr->wphyinfo);
        ptr->wphyinfo = NULL;
    }


    return 1;
}

int copy_bss_msg_flags  (bss_message* dst, bss_message* src) {

    assert(dst);
    assert(src);

    memcpy(&(dst->p_hdr),&(src->p_hdr), sizeof(struct p_hdr));
    memcpy(&(dst->exthdr),&(src->exthdr), sizeof(struct s_exthdr));

    dst->flag = src->flag; /*  PF_SEND , PF_RECV */
    dst->imc_flag = src->imc_flag; /* Inter-Module Communication Flag */

    return 1;
}

int copy_bss_msg_options  (bss_message* dst, bss_message* src) {

    if (!dst) {
        printf("copy_bss_msg_options(): dst pointer is null\n");
        return -1;
    }
    if (!src) {
        printf("copy_bss_msg_options(): src pointer is null\n");
        return -1;
    }
    try {

        if (src->mac_option) {

            mac_option* tmp_macopt = new mac_option;

            dst->mac_option = reinterpret_cast<char*> (tmp_macopt);

            bzero( dst->mac_option , sizeof(mac_option));
            memcpy(dst->mac_option,src->mac_option,src->mac_option_len);
            dst->mac_option_len = src->mac_option_len;
            /* the copy of shared obj should be explicitly stated. */
            tmp_macopt->msg = NULL;

        }
        else {
            dst->mac_option     = NULL;
            dst->mac_option_len = 0;
        }

        if (src->rlc_option) {
            dst->rlc_option   = (char*)malloc(sizeof(rlc_option));
            memcpy(dst->rlc_option,src->rlc_option,src->rlc_option_len);
            dst->rlc_option_len = src->rlc_option_len;
        }
        else {
            dst->rlc_option = NULL;
            dst->rlc_option_len = 0;
        }

        if (src->llc_option) {
            dst->llc_option   = (char*)malloc(sizeof(llc_option));
            memcpy(dst->llc_option,src->llc_option,src->llc_option_len);
            dst->llc_option_len = src->llc_option_len;
        }
        else {
            dst->llc_option = NULL;
            dst->llc_option_len = 0;
        }

        if (src->sndcp_option) {
            dst->sndcp_option = (char*)malloc( src->sndcp_option_len);
            memcpy(dst->sndcp_option,src->sndcp_option,src->sndcp_option_len);
            dst->sndcp_option_len = src->sndcp_option_len;
        }

        else{
            dst->sndcp_option = NULL;
            dst->sndcp_option_len = 0;
        }

        if ( src->wphyinfo ) {
            wphyInfo* dst_wphyinfo = (struct wphyInfo *)malloc(sizeof(struct wphyInfo));
            assert(dst_wphyinfo);
            bzero( dst_wphyinfo , sizeof(struct wphyInfo) );
            memcpy( dst_wphyinfo , src->wphyinfo , sizeof(struct wphyInfo) );
            dst->wphyinfo = dst_wphyinfo;
        }
    }
    catch(std::exception& e) {
        cout << "copy_bss_msg_options: space allocation for bss_msg options failed\n reason="<< e.what() << endl;
        return -1;
    }
    return 1;
}
int copy_bss_msg_headers  (bss_message* dst, bss_message* src) {
    if (!dst) {
        printf("copy_bss_msg_options(): dst pointer is null\n");
        return -1;
    }
    if (!src) {
        printf("copy_bss_msg_options(): src pointer is null\n");
        return -1;
    }
    try {
        if (src->mac_header) {
            dst->mac_header   = new char[dst->mac_header_len];
            memcpy(dst->mac_header,src->mac_header,src->mac_header_len);
                dst->mac_header_len = src->mac_header_len;
        }
        else {
            dst->mac_header     = NULL;
            dst->mac_header_len = 0;
        }

        if (src->rlc_header) {
            dst->rlc_header   = new char[dst->rlc_header_len];
            memcpy(dst->rlc_header,src->rlc_header,src->rlc_header_len);
            dst->rlc_header_len = src->rlc_header_len;
        }
        else {
            dst->rlc_header     = NULL;
            dst->rlc_header_len = 0;
        }

        if  (src->llc_header) {
            dst->llc_header   = new char[dst->llc_header_len];
            memcpy(dst->llc_header,src->llc_header,src->llc_header_len);
            dst->llc_header_len = src->llc_header_len;
        }
        else {
            dst->llc_header     = NULL;
            dst->llc_header_len = 0;
        }

        if (src->sndcp_header ) {
            dst->sndcp_header = new char[dst->sndcp_header_len];
            memcpy(dst->sndcp_header,src->sndcp_header,src->sndcp_header_len);
            dst->sndcp_header_len = src->sndcp_header_len;
        }
        else{
           dst->sndcp_header = NULL;
           dst->sndcp_header_len = 0;
            }
    }
    catch(std::exception& e) {
        cout << "copy_bss_msg_headers: space allocation for bss_msg headers failed\n reason="
             << e.what() << endl;
        return -1;
    }




    return 1;
}

int copy_bss_msg_ctlmsg (bss_message* dst, bss_message* src) {

    if (!dst) {
        printf("copy_bss_msg_ctlmsg(): userdata pointer is null\n");
        exit(1);
    }
    if (!src) {
        printf("copy_bss_msg_ctlmsg(): src pointer is null\n");
        exit(1);
    }

    mac_option* src_mac_opt = reinterpret_cast<mac_option*> (src->mac_option);

    if ( src_mac_opt ) {

        if ( src_mac_opt->msg ) {

            mac_option* dst_mac_opt = reinterpret_cast<mac_option*> (dst->mac_option);

            if ( dst_mac_opt ) {

                if ( !dst_mac_opt->msg ) {

                    dst_mac_opt->msg = src_mac_opt->msg;
                    return 1;
                
		}
                else
                    return 0;

            }
            else
                return 0;

        }
        else
            return 0;
    }
    else
        return 0;

}

int copy_bss_msg_userdata (bss_message* dst, bss_message* src) {
    if (!dst) {
        printf("copy_bss_msg_userdata(): userdata pointer is null\n");
        exit(1);
    }
    if (!src) {
        printf("copy_bss_msg_userdata(): src pointer is null\n");
        exit(1);
    }
    if (src->user_data_len <= 0) {
        cout << "copy bssmsg userdata(): Error: the user_data_len of src is less than zero." << endl;
        exit(1);
    }

    if (!dst->user_data)
        dst->user_data = new char[src->user_data_len];
    else {
        cout << "copy bssmsg userdata(): Error: the user_data field of destination is already used." << endl;
        exit(1);
    }

    if (src->user_data && src->user_data_len ) {
        memcpy(dst->user_data,src->user_data,src->user_data_len);
        dst->user_data_len = src->user_data_len;
        return 1;
    }
    else {

        delete dst->user_data;
        dst->user_data      = NULL;
        dst->user_data_len  = 0;
        cout << "copy bssmsg userdata(): Error: the user_data field of src is invalid." << endl;
        return 1;
    }
}

int copy_bss_msg_packet_field (bss_message* dst, bss_message* src) {
    if (!dst) {

        printf("copy_bss_msg_packet_field(): userdata pointer is null\n");
        exit(1);

    }

    if (!src) {

        printf("copy_bss_msg_packet_field(): src pointer is null\n");
        exit(1);

    }


    if (!src->packet) {

        #ifdef __BSS_MESSAGE_ROUTE_DETAILED_DEBUG
        cout << "Warning: copy_bss_msg_packet_field(): src->packet is NULL." << endl;
        cout << "It may occur when partition Control message or dummy message" << endl;
        #endif

        return 1;

    }



    dst->packet = src->packet->copy();

    #ifdef __DISABLED_COPY_PACKET_FIELD_TRACK

        if ( dst->packet ) {

            Packet* pkt = dst->packet;
            ulong   randomized_num = rand();
            pkt->pkt_addinfo("RNUM", (char*)&randomized_num , sizeof(ulong) );

            printf("copy_bss_msg_packet_field(): Add a randomized value %lu into pkt.\n", randomized_num );


            int rec_res = rec_upper_layer_pkt(pkt);

            if ( rec_res < 0 ) {
                printf("copy_bss_msg_packet_field(): record upper_layer_pkt failed.\n");
            }

            return 1;
        }

        else
            return 0;

    #else

        return 1;

    #endif

}

long get_pkt_payload_length(Packet* pkt) {
    IS_NULL(pkt,0);
    struct pbuf *pbuf = pkt->pkt_getpbuf();
    IS_NULL(pbuf,0);
    struct pbuf *spbuf = (struct pbuf *)pbuf->p_sptr;
    IS_NULL(spbuf,0);
    return spbuf->p_len;
}

mac_option::~mac_option() {

    ;/* The decrement for the reference count of a shared msg should be explicitly stated.*/

}

int insert_macopt_msg( mac_option* mac_opt , ObjType msg_type , void* msg ) {

    ASSERTION( mac_opt , "insert_macopt_msg():mac_opt is null.\n");
    ASSERTION( mac_opt , "insert_macopt_msg():msg is null.\n");

    if ( msg_type != DUMMYBURST && msg_type != NDMSG && msg_type != DMSG ) {
        printf("insert_macopt_msg(): Illegal msg_type %d. \n", msg_type);
        exit(1);
    }

    mac_opt->msg_type   = msg_type;
    mac_opt->msg        = msg;

    return 1;
}

Event* copy_gprs_pkt(Event* old_ep) {

    assert(old_ep);
    Event* new_ep = NULL;
    CREATE_BSS_EVENT(new_ep);
    
    bss_message* old_bmsg_p = reinterpret_cast<bss_message*> (old_ep->DataInfo_);
    assert(old_bmsg_p);
    bss_message* new_bmsg_p = reinterpret_cast<bss_message*> (new_ep->DataInfo_);
    assert(new_bmsg_p);

    copy_bss_msg_flags          (new_bmsg_p, old_bmsg_p);
    copy_bss_msg_options        (new_bmsg_p, old_bmsg_p);
    copy_bss_msg_headers        (new_bmsg_p, old_bmsg_p);
    copy_bss_msg_userdata       (new_bmsg_p, old_bmsg_p);
    copy_bss_msg_packet_field   (new_bmsg_p, old_bmsg_p);

    return new_ep;

}

int destroy_gprs_pkt(Event* ep) {

    assert(ep);

    bss_message* bmsg_p = reinterpret_cast<bss_message*> (ep->DataInfo_);
    assert(bmsg_p);

    if (bmsg_p->packet) {
        delete bmsg_p->packet;
	bmsg_p->packet = 0;
    }

    FREE_BSS_EVENT_INCLUDING_USER_DATA(ep);
    return 1;

}

#endif
