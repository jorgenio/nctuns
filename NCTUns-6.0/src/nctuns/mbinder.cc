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

#include <assert.h>
#include <mbinder.h>
#include <nctuns_api.h>
#include <object.h>


MBinder::MBinder(NslObject *fr) {

	/* initialize Module-Binder Queue */
	mbq.mbq_head = mbq.mbq_tail = 0;
	mbq.mbq_maxlen = 1; /* by default */
	mbq.mbq_len = mbq.mbq_drops = 0;
	mbq.co_type = 0;
	mbq.coObj = 0;
	mbq.m_upcall = 0;

	next_mb = 0;
	frmObj = fr; toObj = 0;
}

MBinder::MBinder(NslObject *fr, int qlen) {

        /* initialize Module-Binder Queue */
        mbq.mbq_head = mbq.mbq_tail = 0;
        mbq.mbq_maxlen = qlen; 
        mbq.mbq_len = mbq.mbq_drops = 0;
        mbq.co_type = 0;
        mbq.coObj = 0;
        mbq.m_upcall = 0;

	next_mb = 0;
	frmObj = fr; toObj = 0;
}

MBinder::~MBinder() {

}

int MBinder::serve() {

	ePacket_	*pkt;


	MB_DEQUEUE(&mbq, pkt);
	if (pkt != NULL) {
		/*
		 * Send this packet to next module, if return 1,
		 * the packet is successfully sent to next module;
		 * otherwise the next module is busy and return -1.
		 */
		assert(toObj);
		if (toObj->get(pkt, this) > 0) {
			/*
			 * If successfully sent to next module,
			 * we still have to do an upcall. After
			 * upcall, should return 1 to indicate
			 * MBP(Module-Binder Poller).
			 */
			try_upcall();
			return(1);
		} else {
			/*
			 * Otherwise, packet can not be sent to
			 * next module, so we should prepend this
			 * dequeued packet into queue.
			 */
			MB_PREPEND(&mbq, pkt);
			return(-1);
		}
	}
	return(-1);
}


ePacket_ * MBinder::enqueue(ePacket_ *pkt) {

	ePacket_		*dq_pkt, *ptr, *ptr1;
	ePacket_		*p, *p1;


	dq_pkt = NULL; pkt->next_ep = 0;
	p = p1 = 0;
	
	/*
	 * If MBQ is full, then we should compare the
	 * incoming packet priority with those in the
	 * MBQ. If the priority of incoming packet is 
	 * higher than those in MBQ, then we should
	 * dequeue a packet with the lowest priority
	 * and enqueue that higher priority packet
	 * to MBQ.
	 */
	if (MB_QFULL(&mbq)) {
		MB_DROP(&mbq);
		p = mbq.mbq_head;
		for(ptr=mbq.mbq_head, ptr1=0; ptr; ptr1=ptr, ptr=ptr->next_ep) {
			if (GET_MLEVEL(ptr) <= GET_MLEVEL(p)) {
				p1 = ptr1;
				p = ptr;
			}
		}

		/*
                 * The one we found may have chance to be swapped out.
                 * So we still have to compare the found packet with 
                 * inserting one.
                 */
		if (GET_MLEVEL(pkt) <= GET_MLEVEL(p)) {
			/* The packets in queue all have higher priority
                         * than the that of 'pkt'. So we should swap
                         * out the 'pkt'.
                         */
                        return(pkt);
                }

		/* Swap the one we found. */
		if (p1 != NULL) {
                        /* swap out the low priority packet */
                        dq_pkt = p;
                        p1->next_ep = p->next_ep;
                        p->next_ep = 0;
                } else {
			dq_pkt = p;
                        if (mbq.mbq_head == mbq.mbq_tail) {
                                mbq.mbq_head = mbq.mbq_head->next_ep;
                                mbq.mbq_tail = mbq.mbq_head;
                        } else  mbq.mbq_head = mbq.mbq_head->next_ep;
                }
                mbq.mbq_len --;
	}

	/*
         * Otherwise, enqueue the packet
         * into queue. After queuing the packet,
         * we should register a polling request to
         * tell Module Binder-Buffer Poller to polling 
         * periodically.
         */
	if (mbq.mbq_tail == NULL) {
		/* add to head */
		mbq.mbq_head = pkt;
		mbq.mbq_tail = pkt;
	} else { 
		/*
		 * We enqueue packet and reorder the priority
		 * order in descending order in MBQ.
		 */
		for(ptr=mbq.mbq_head, ptr1=0; ptr; ptr1=ptr, ptr=ptr->next_ep) {
			if (GET_MLEVEL(pkt) > GET_MLEVEL(ptr))
				break;
		}
		if (ptr1 != NULL) {	
			pkt->next_ep = ptr1->next_ep;
			ptr1->next_ep = pkt;
		} else {
			pkt->next_ep = mbq.mbq_head;
			mbq.mbq_head = pkt;
		}

		/* 
		 * If the ptr == NULL, then it means that the incoming
		 * packet is added to the tail of the MBQ. Hence I should
		 * update the mbq_tail of MBQ to that incoming packet.
		 */
		if (ptr == NULL)
			mbq.mbq_tail = pkt;

	}
	mbq.mbq_len ++;

	RegToMBPoller(this);

	return(dq_pkt);

}


void MBinder::try_upcall() { 

	switch(mbq.co_type) {

       		case NO_UPCALL: return;

        	case MEMB_UPCALL: (mbq.coObj->*mbq.m_upcall)(this); return;

        	case FUNC_UPCALL: mbq.f_upcall(this); return;

        	default: assert(0);
        }
}



