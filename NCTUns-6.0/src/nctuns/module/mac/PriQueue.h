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

#ifndef __NCTUNS_MAC_PriQueue_h
#define __NCTUNS_MAC_PriQueue_h                                                                                                                                                                                                                                                 
struct PriQueue {
	unsigned int		_pri;
	unsigned int		_length;
	unsigned int		_size;
        unsigned int            _maxLength;
	unsigned int		_drops;
        ePacket_		*_head;
        ePacket_		*_tail;
};

/* Define Macros for Priority Queues */
#define IF_ENQUEUE(ifq, m, size) { 			\
	if ((ifq)->_length < (ifq)->_maxLength){	\
        	if ((ifq)->_tail == 0) 			\
                	(ifq)->_head = m; 		\
       	 	else 					\
                	(ifq)->_tail->next_ep = m; 	\
        	(ifq)->_tail = m; 			\
		(ifq)->_length++;			\
		(ifq)->_size +=size;			\
	}						\
}

#define IF_DEQUEUE(ifq, m) { 				\
	Packet	*p;					\
        (m) = (ifq)->_head; 				\
        if (m) { 					\
		GET_PKT(p, m);				\
		(ifq)->_size -= p->pkt_getlen();	\
                if ((m)->next_ep == 0) 			\
                        (ifq)->_tail = 0; 		\
		(ifq)->_head = (m)->next_ep;		\
                (m)->next_ep = 0; 			\
                (ifq)->_length--; 			\
        } 						\
}

#endif
