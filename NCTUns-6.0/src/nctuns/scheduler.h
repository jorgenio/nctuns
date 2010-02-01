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

#ifndef	__NCTUNS_scheduler_h__
#define __NCTUNS_scheduler_h__

#include <event.h>
#include <heap.h>

/*=====================================================================
   Define Macros
  =====================================================================*/
#define SCHEDULER_INSERT_EVENT(schdlr, ep)			\
	(schdlr)->setEvent(ep)


  
/*=====================================================================
   Define Class
  =====================================================================*/

class HeapObject;

class scheduler
{
private:
	u_int64_t	simulateTime_;	/* time to simulate */ 
	Event_		*timerHead_;

	int		t0efd;	/* for event tunnel fd */

	int		executeEvent(); 
        int 		triggerKernelCallouts();
	int 		readt0e();
	int 		deqheap();
	int 		deqtimer();
	int 		chktunnel(const int tunid);

public: 
	HeapObject 	heap_;		/* heap to store events */
	
 	scheduler();
 	~scheduler();
 	 
	void 		chkt0e();
	void		run(u_int64_t timInSec);
	int 		setEvent(Event_ *ep);
	int		numEvent(); 
	u_int64_t	numInsertEvent();
	u_int64_t	numDequeueEvent();
	u_int64_t 	maxsimtime();

	/* timer implementation */
	int		schedule_timer(Event_ *T);
	int		cancel_timer(Event_ *T);   
	Event_ 		*dequeue_timer(); 
};
 
#endif /* __NCTUNS_scheduler_h__ */

