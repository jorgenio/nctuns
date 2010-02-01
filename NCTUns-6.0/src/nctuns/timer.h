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

#ifndef	__NCTUNS_timer_h__
#define	__NCTUNS_timer_h__

#include <event.h>
  
/*======================================================================
   Define Class
  ======================================================================*/                                                                      
class timerObj {
private:

public: 
 	Event_ *callout_event;
	
	/* timer state */
	u_char busy_;
	u_char paused_; 
	u_int64_t rtime_; 	/* remainder time */
	
	timerObj();
	~timerObj();
	void init(); 
	void cancel();

	/*-------------------------------------------------------------------
	 * start()
	 *
	 * Insert the timer into timer-callout queue. Upon the time expired,
	 * the specified function or member function of Object will be 
	 * called. The callout function or member function of Object should
	 * be in the following form:
	 *	int (*)(Event_ *)  	      	for callout function
	 *	int (NslObject::*)(Event_ *)  	for callout object
	 * The argument of callout function or member function will be
	 * always NULL.
	 *
	 * Arguments:
	 *	time		the expired time in clock tick.
	 *	pero		the peridocal time to trigger timer, if
	 *			this parameter is set to zero, then the
	 *			timer will be triggered once.
	 *
	 * Returns:
	 *	nothing.
	 *
	 * Side effects:
	 *	the timer will be inserted into the timer callout queue.
	 *-------------------------------------------------------------------*/
	void start(u_int64_t time, u_int64_t pero);

	/*-------------------------------------------------------------------
	 * pause()
	 *
	 * Pause the timer. If you call this method, the timer you specified 
	 * will be paused immediately and the remaining time will be count
	 * upon resume this timer.
	 *
	 * Arguments:
	 *	nothing.
	 *
	 * Returns:
	 *	nothing.
	 *
	 * Side effects:
	 *	the specified timer will not be count and will be dequeue
	 *	from the timer callout queue.
	 *-------------------------------------------------------------------*/
	void pause();

	/*-------------------------------------------------------------------
	 * resume()
	 *
	 * Resume the paused timer. Once the timer is paused, you can call
	 * the resume() method to continue count the timer. The difference
	 * between resume() and resume(time) is that the resume(time) can
	 * specify how many tick will be added to remaining time; but the
	 * resume() can't.
	 *
	 * Arguments:
	 *	time		specified how many time will be added into
	 *			remaining time. it is also in clock tick.
	 *
	 * Returns:
	 *	1		success
	 * 	< 0 		failure
	 *
	 * Side effect:
	 *	the paused timer will be inserted into the timer callout
	 *	queue again and continue to be count.
	 *-------------------------------------------------------------------*/
	int resume(u_int64_t time); 

	/*-------------------------------------------------------------------
	 * resume()
	 *
	 * Resume the paused timer. Once the timer is paused, you can call
	 * the resume() method to continue count the timer. The difference
	 * between resume(0 and resume(time) is that the resume(time) can
	 * specify how many tick will be added to remaining time; but the
	 * resume() can't.
	 *
	 * Argument:
	 *	nothing
	 *
	 * Return:
	 *	1		success
	 *	< 0		failure
	 *
	 * Side effecst:
	 *	the paused timer will be inserted into the timer callout
	 *	queue again and continue to be count.
	 *-------------------------------------------------------------------*/
	int	resume(); 

	/*-------------------------------------------------------------------
	 * expire()
	 *
	 * Get time timer expired time.
	 *
	 * Arguments:
	 *	nothing
	 *
	 * Returns:
	 *	the expired time of specified timer in clock tick.
	 *
	 * Side effects:
	 *	no side effect.
	 *-------------------------------------------------------------------*/
	inline u_int64_t expire() { return(callout_event->timeStamp_); }; 

	/*-------------------------------------------------------------------
	 * setCallOutObj()
	 *
	 * Set the callout member function of timer. Whenever a timer is
	 * inserted into the timer callout queue, either setCallOutObj()
	 * or setCallOutFunc() method should be called to set timer's
	 * callout function or member function.
	 *
	 * Arguments:
	 *	obj		a pointer to an Instance interited from 
	 *			NslObject type.
	 *	memfunc		a pointer to a member function of Object
	 *			which you want to call upon time expire.
	 *			it should be in the following form:
	 *			-> int (NslObject::*)(Event_ *), the 
	 *			parameter is always NULL.
	 *
	 * Returns:
	 *	1		success
	 *	< 0		failure
	 *
	 * Side effects:
	 * 	the callout field of timer structure will be filled.
	 *-------------------------------------------------------------------*/
	int	setCallOutObj(NslObject *obj, 
   	int (NslObject::*memfunc)(Event_ *)); 

	/*-------------------------------------------------------------------
	 * setCallOutFunc()
	 *
	 * Set the callout function of timer. Whenever a timer is inserted
	 * into the timer callout queue, either the setCallOutFunc() or
	 * setCallOutObj() should be called to set the timer's callout
	 * function or member function.
	 *
	 * Arguments:
	 *	func		a pointer to callout function, it should
	 *			be in the following form:
	 *			-> int (*)(Event_ *). the parameter is 
	 *			always NULL.
	 *
	 * Returns:
	 *	1 		success
	 *	< 0		failure
	 *
	 * Side effect:
	 *	the callout field of timer structure will be filled.
	 *-------------------------------------------------------------------*/
	int	setCallOutFunc(int (*func)(Event_ *ep)); 
};
 
#endif	/* __NCTUNS_timer_h__ */

