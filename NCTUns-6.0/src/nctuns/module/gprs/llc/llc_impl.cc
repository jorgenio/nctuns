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

/* This file implements classes defined in llc.h
 *
 * CreateDate: 	07/30/2003
 * Author:	Chih-che Lin
 * Email:	jclin@csie.nctu.edu.tw
 */

#ifndef __LLC_IMPL_CC
#define __LLC_IMPL_CC

#include <stdio.h>
#include <string.h>
#include <gprs/include/generic_list.h>
#include <packet.h>
#include "llc.h"
using namespace std;

    /*************** EventQueue implementations *******************/

    EventQueue::EventQueue() {
        head = tail = NULL;
        num_of_elem = 0;
    }

    EventQueue::~EventQueue() {
        while(head) {
           EventElem* p = head->next;
           delete head;
           head = p;
        }
    }
    int EventQueue::enqueue(Event* ep) {

        IS_NULL_STR(ep,"EventQueue enqueue(): ep is null\n",-1);
        EventElem *elem = new EventElem;
        IS_NULL_STR(elem,"EventQueue enqueue(): elem is null\n",-1);

        elem->ep    = ep;
        elem->next  = NULL;

        if ( is_empty() ) {
            head = tail = elem;
            tail->next  = NULL;
        }
        else {
            tail->next  = elem;
            tail        = tail->next;
            tail->next  = NULL;
        }
        ++num_of_elem;
        return 1;
    }


    Event* EventQueue::dequeue() {
        if ( is_empty() )
            return NULL;

        else {
            Event* tmp      = head->ep;
            EventElem* tmp2 = head->next;

            if ( head == tail )
                tail = NULL;
            delete head;
            head = tmp2;

            --num_of_elem;
            return tmp;
        }
    }

    bool EventQueue::is_full() {
	
#define __GPRS_LL_EVENT_QUEUE_LIMITED_SIZE__ 1
#if __GPRS_LL_EVENT_QUEUE_LIMITED_SIZE__
	if (num_of_elem >= 50 ) {
	    printf("eventqueue: drop pkt\n");
	    return true;
	}
        else
	    return false;
#else
    	//return false; /* unlimited queue length */
#endif
    }

    bool EventQueue::is_empty() {
    	if ( !head )
    	     return true;
    	else
    	     return false;
    }



    /********************** LlcFrame implementation *************************/
    /* constructor */
    LlcFrame::LlcFrame (uchar frame_type) {
        /* format types: I format, S format , UI format , U format */
        /* various lengths according to format_type. At present, we support
         * UI format (for Data) and U format(for control messages).
         */
        bzero(this,sizeof(LlcFrame));
        format_type = frame_type;
    }

    /* manipulate address field */
    int LlcFrame::insert_addr_field(bool pd, bool cr, uchar sapi) {
        addr_field = (( static_cast<uchar>(pd) <<7) | ( static_cast<uchar>(cr)<<6) | sapi ) ;

        pd_ 	= pd;
        cr_ 	= cr;
        sapi_ 	= sapi;
        return 1;
    }

    bool LlcFrame::get_pd() {
        return pd_;
    }

    bool LlcFrame::get_cr() {
        return cr_;
    }
    uchar LlcFrame::get_sapi() {
        return sapi_;
    }

    uchar LlcFrame::get_addr_field() {
        return addr_field;
    }

    /* manipulate control field:
     * The detailed format is defined on p.22 in TS 44064. 
     */
    int LlcFrame::insert_ui_format_control_field( bool encryption, bool protected_mode , short unconfirmed_seqno ) {

        if (!control_field)
            control_field = new uchar[2];

        bzero( control_field , 2 );

        control_field[1] = (static_cast<uchar> (encryption)<<1) | static_cast<uchar>(protected_mode) ;

        /* For unconfirmed seq_no: 
         * the least 6 significant bits are stored in control_field[1], and 
         * the most 3 signicant bits are stored in the control_field[0].
         */
        {
            short seq_no_mask = 0x01ff;
            uchar temp = 0x0;
            unconfirmed_seqno &= seq_no_mask;
            seq_no_mask = 0x003f;
            temp = static_cast<uchar> (seq_no_mask & unconfirmed_seqno);
            control_field[1] |= (temp<<2);

            temp = static_cast<uchar> (unconfirmed_seqno>>6);
            control_field[0] = (0xc0 | temp) ;
        }

        encryption_ 		= encryption;
     	protected_mode_ 	= protected_mode;
    	unconfirmed_seqno_ 	= unconfirmed_seqno;
        format_type		= UI_FORMAT;
        return 1;
    }

    bool LlcFrame::get_e_bit() {
        return encryption_;
    }
    bool LlcFrame::get_pm_bit() {
        return protected_mode_;
    }
    short LlcFrame::get_unconfirmed_seqno() {
        return unconfirmed_seqno_;
    }

    /* manipulate information field */
    int LlcFrame::insert_info_field(void* info) {
        IS_NULL_STR(info, "LlcFrame insert_info_field(): info is null" , -1);
        /* Notice: info field points to pkt->p_data */
        information_field = static_cast<uchar*> (info);
        return 1;
    }

    uchar* LlcFrame::get_info_field() {
        return information_field;
    }

    /* manipulate FCS field */
    int	LlcFrame::insert_fcs(uchar* given_fcs) {
        bzero(fcs,3);
    	return 1;
    }

    /* LlcFrame pack(): 
     * packing all fields of UI frame header into 
     * a buffer,stored by bssmsg->llc_option. 
     */
     
    int LlcFrame::pack(Event* ep) {
    
        int length , control_field_len;
        
        /* compute the required size of header */
        {
            if (format_type == UI_FORMAT)
            {
               control_field_len = UI_CTL_FIELD_SIZE;
               length = sizeof(addr_field) + control_field_len + FCS_SIZE;
            }
            else
               length = 0; /* currently types other UI_FORMAT are unsupported. */
        }

        uchar *buf=NULL;
        if (length)
            buf = new uchar[length];

        if (!buf) {
           cout << "LlcFrame::pack(): there is no available space or receive unsupported format" <<endl;
           return -1;
        }

        uchar *ptr = (buf);
        memcpy( reinterpret_cast<void*> (ptr) , reinterpret_cast<void*> (&addr_field), sizeof(addr_field) );
        ptr += sizeof(addr_field);
        memcpy( ptr, control_field , control_field_len );
        ptr += sizeof( control_field_len );
        memcpy( ptr , fcs , FCS_SIZE );

        bss_message* bss_msg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        
        IS_NULL_STR(bss_msg,"LlcFrame pack(): Assertion failed since bss_msg is null\n",-1);
        
        bss_msg->llc_header     = reinterpret_cast<char*> (buf);
        bss_msg->llc_header_len = length;
        
        return 1;
    }

    /* The event transmitted shall remain each available header fields.
     * The content of userdata field may be changed when the event passes 
     * through layers. Because it shall reflect processes of layers
     */
     
    int LlcFrame::unpack(Event *ep,int frame_type) {
        
        bss_message *bss_msg = reinterpret_cast<bss_message*> (ep->DataInfo_);
        IS_NULL_STR(bss_msg, "LlcFrame unpack(): bss_msg is NULL\n", -1 );

        char* ptr = bss_msg->user_data;
        long  length;
        IS_NULL_STR(ptr, "LlcFrame::unpack(): no available data in pt_data", -1);
        
        bcopy ( reinterpret_cast<void*> (ptr) , reinterpret_cast<void*> (&addr_field) , sizeof(addr_field) );
        ptr += sizeof(addr_field);

        /* decode address field */
        sapi_   = (addr_field & 0x1f );
        cr_     = ((addr_field >> 5 ) & 0x01);
        pd_     = ((addr_field >> 6 ) & 0x01); 
                
        if ( frame_type == UI_FORMAT) {
            /* UI_FORMAT */
            length = sizeof(addr_field) + UI_CTL_FIELD_SIZE + FCS_SIZE;
            if (!control_field)
                control_field = new uchar[UI_CTL_FIELD_SIZE];

            bcopy ( ptr , control_field , UI_CTL_FIELD_SIZE );
            ptr +=UI_CTL_FIELD_SIZE;
            bcopy ( ptr , fcs , FCS_SIZE );

            //delete reinterpret_cast<char*> (bss_msg->llc_header);
            //bss_msg->llc_header = NULL;
            return 1;    
        }
        else {        
            cout << "LlcFrame unpack(): Exception: unsupported frame type = " << frame_type << endl;
            return -1;
        }
    }

#endif
