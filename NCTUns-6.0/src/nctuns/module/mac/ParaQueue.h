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

#ifndef __NCTUNS_MAC_ParaQueue_h
#define __NCTUNS_MAC_ParaQueue_h

#include <event.h>                                                                          
#include <nctuns_api.h>                                                                          

class ParaQueue {
 public:
	ParaQueue(){
        	_size = 0;
        	_len = 0;
        	_pri = 0;
       	 	_head = 0;
        	_tail = 0;
		_delaybound = 0;
	}

	~ParaQueue(){
		empty_queue();
	}

	void enque (ePacket_ *p) {
        	if (_len < MAX_QUEUE_LEN) {
                	if (_tail == 0)
                        	_head = p;
                	else
                        	_tail->next_ep = p;
               	 	_tail = p;
                	_len++;
        	}
	}

	void top (ePacket_ *&p) {
        	if (_len > 0)
                	p = _head;
        	else
                	p = 0;
	}

	void deque () {
        	ePacket_ *p;
        	if (_len > 0) {
                	p = _head;
                	if (p->next_ep == 0)
                        	_tail = 0;
               	 	_head = p->next_ep;
                	_len--;
                	freePacket(p);
        	}
	}

	int qlen() {
        	int i = _len;
        	return i;
	}

	void second (ePacket_ *&p)
	{
        	if ( _len > 1 )
                	p = _head->next_ep;
        	else
                	p = 0;
	}

	unsigned int		_size;
	unsigned int		_pri;
	unsigned int		_len;
	unsigned int		_delaybound;
 private:
        ePacket_		*_head;
        ePacket_		*_tail;

	void	empty_queue(void){
        	ePacket_ *p;
    	    	while (_len > 0) {
                	p = _head;
                	_head = _head->next_ep;
                	freePacket(p);
                	_len--;
        	}
		_head = _tail = 0;
		_size = 0;
	}
};

#endif
