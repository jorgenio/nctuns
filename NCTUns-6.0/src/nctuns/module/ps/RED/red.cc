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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <nctuns_api.h>
#include <math.h>
#include <time.h>
#include <mbinder.h>
#include "red.h"

MODULE_GENERATOR(RED);

RED::RED(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : NslObject(type, id, pl, name)
{
	/* 
	 * Disable flow control 
	 */
    s_flowctl = DISABLED;
	r_flowctl = DISABLED;

	/* 
	 * Initialize interface queue 
	 */
	if_snd.ifq_head = if_snd.ifq_tail = 0;
	if_snd.ifq_len = 0;
	if_snd.ifq_drops = 0;
	if_snd.ifq_maxlen = 0;
	edp_.th_min = 0;
	edp_.th_max = 0;
	edp_.q_w = 0.0;
	edp_.max_p = 0.0;

	/* 
	 * Bind variables
	 */
	vBind("max_qlen", &if_snd.ifq_maxlen);
	vBind("min_threshold", &edp_.th_min);
	vBind("max_threshold", &edp_.th_max);
	vBind("queue_weight", &edp_.q_w);
	vBind("max_drop_prob", &edp_.max_p);
}

RED::~RED() {
}

int RED::init() {
    int (NslObject::*upcall)(MBinder *);

    bw_ = GET_REG_VAR(get_port(), "BW", double *);


    if (if_snd.ifq_maxlen <= 0)	if_snd.ifq_maxlen = 50; /* default queue size    */
    if (edp_.th_min <= 0)		edp_.th_min = 15;       /* default min threshold */
    if (edp_.th_max <= 0)		edp_.th_max = 45;		/* default max threshold */
    if (edp_.q_w <= 0)			edp_.q_w = 0.02;		/* default queue weight  */
    if (edp_.max_p <= 0)		edp_.max_p = 0.01; 		/* default max dropping probability */
    
    upcall = (int (NslObject::*)(MBinder *))&RED::intrq; /* Set upcall */
    sendtarget_->set_upcall(this, upcall);
    return(1);
}

int RED::send(ePacket_ *pkt) {
	
	double 		m = 1;
	double 		p_b = 0, p_a = 0;
	u_int64_t	tick;
	double		edp_ptc;
	assert(pkt&&pkt->DataInfo_);

	/*
	 * If Module-Binder Queue(MBQ) is full, we should
	 * insert the outgoing packet into the interface 
	 * queue. If MBQ is not full, we can call the 
	 * put() or NslObject::send() method to pass the 
	 * outgoing packet to next module.
	 */
	if( sendtarget_->qfull() ) {
		/* MBQ is full, insert to ifq */
		if (IF_QFULL(&if_snd)) {
			/* ifq full, drop it! */
			IF_DROP(&if_snd);
			freePacket(pkt);
			return(1);
		}
		else {
			if (if_snd.ifq_len > 0) {
				edv_.avg_queue = (1 - edp_.q_w) * edv_.avg_queue + edp_.q_w * if_snd.ifq_len;
			}
			else {
				edp_ptc = (*bw_) / (8 * 1500);
				SEC_TO_TICK(tick, 1);
				m = (edp_ptc * ((double) (GetCurrentTime() - edv_.q_time) / (double)tick));
				edv_.avg_queue = pow( (1 - edp_.q_w), m ) * edv_.avg_queue;
			}
			
			/*
			 * if min <= avg < max
			 * increment count and calculate probability pa
			 */
			 
			if (edv_.avg_queue >= edp_.th_min && edv_.avg_queue <= edp_.th_max) {
				++edv_.count;
				p_b = edp_.max_p * (edv_.avg_queue - edp_.th_min) / (edp_.th_max - edp_.th_min);
				p_a = p_b / (1 - edv_.count * p_b);

                                // Should delete the following statement !!!
				// edp_.max_p = (double)1 / (double)if_snd.ifq_maxlen;
			        
				/*
				 * with probability pa
				 * drop the arriving packet
				 * this fragment should be modified
				 */  
/*
				 srand(time(NULL)+edv_.count);
*/
				 double pp = rand() % 1000 + 1;

				 if (p_a < 0) p_a = 0;
				 
				 if (p_a * 1000 > pp) {
					IF_DROP(&if_snd);

                          printf("CurTime %llu: Drop a packet ... p_a %12.5f, edv_.avg_queue %d \n", GetCurrentTime(), p_a, (int)edv_.avg_queue);

					freePacket(pkt);
					edv_.count = 0;
					return (1);
				}
				else {
					IF_ENQUEUE(&if_snd, pkt);
					return (1);
				}
			}
			else if (edp_.th_max <= edv_.avg_queue) { 
			/* max <= avg --> drop all packets */
				IF_DROP(&if_snd);
				freePacket(pkt);
				edv_.count = 0;
				return (1);
			}
			else {
				IF_ENQUEUE(&if_snd, pkt);
				edv_.count = -1;
				return (1);
			}	
		}
		
		if (if_snd.ifq_len == 0)
			edv_.q_time = GetCurrentTime();

	} 
	else {
		/* 
		 * MBQ is not full, pass outgoing packet
		 * to next module.
		 */
		return(NslObject::send(pkt)); 
	}
}

int RED::recv(ePacket_ *pkt) {
	/* Just by pass incoming packet */
    assert(pkt&&pkt->DataInfo_);
    return(NslObject::recv(pkt));
}


int RED::intrq(MBinder *port) {

	ePacket_	*pkt;

	/*
	 * Push the packet in the interface queue
	 * to the MBQ. Whenever the pakcet in the
	 * MBQ is sent, the scheduler will call this
	 * member function to give fifo module a 
	 * chance to send the next packet in the
	 * interface queue.
	 */
	IF_DEQUEUE(&if_snd, pkt);
	
	if (pkt != NULL) {
		/*
		 * If still exist packet in the interface
		 * queue, we try to push it to the MBQ,
		 */
		assert(sendtarget_->enqueue(pkt) == 0);
	}
	return(1);
}
