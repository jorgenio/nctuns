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

#include <stdio.h>
#include <stdlib.h>
#include <timer.h>
#include <assert.h>
#include <nctuns_api.h>
#include <scheduler.h>

extern scheduler		*scheduler_;
extern u_int64_t		*currentTime_; 

timerObj::timerObj()
{
	/* initialize timer state */
	busy_ = 0;
	paused_ = 0;
	rtime_ = 0;  
    
	/* create a timer */
	CREATE_EVENT(callout_event);
	callout_event->DataInfo_ = (void *)this;  
}

  
timerObj::~timerObj()
{
	/* for avoiding double free */
	callout_event->DataInfo_ = 0;  
	FREE_EVENT(callout_event); 
}


void timerObj::init() {

	busy_ = 0;
  	paused_ = 0;
  	rtime_ = 0;  
}


void timerObj::cancel() {

	busy_ = 0;
	paused_ = 0;  
	rtime_ = 0;
	
    	/* dequeue from timer queue */
	scheduler_->cancel_timer(callout_event); 
}


void timerObj::start(u_int64_t time, u_int64_t pero) {

	u_int64_t		expire;

	/* Check to see if the timer is already
	 * in timer-list. If yes, cancel it and 
	 * reset.
	 */
	if (busy_) scheduler_->cancel_timer(callout_event);
 
	busy_ = 1;
	paused_ = 0;  
	rtime_ = 0;

	/* setting timestamp */
	expire = currentTime_[0] + time;
	if (expire < currentTime_[0]) {
		printf("ERROR: I can't revert current time back to past..Can you?\n");
		assert(0);
	}
	SET_EVENT_TIMESTAMP(callout_event, expire, pero);

	/* insert into queue */
	scheduler_->schedule_timer(callout_event); 

}


void timerObj::pause() {

	paused_ = 1;
	if (currentTime_[0] > callout_event->timeStamp_)
		rtime_ = 0;
  	else 	rtime_ = callout_event->timeStamp_ - currentTime_[0];
	scheduler_->cancel_timer(callout_event); 
}

int timerObj::resume(u_int64_t time) {

	paused_ = 0;

	callout_event->timeStamp_ = currentTime_[0] + time + rtime_;
	rtime_ = 0;
	return(scheduler_->schedule_timer(callout_event));     
}

int timerObj::resume() {

	paused_ = 0;

	callout_event->timeStamp_ = currentTime_[0] + rtime_;
	return(scheduler_->schedule_timer(callout_event)); 
}

int timerObj::setCallOutObj(NslObject *obj, 
	       int (NslObject::*memfunc)(Event_ *)) 
{
	 return(setEventCallOutObj(callout_event,
			   		obj,
			   		memfunc,
			   		(void *)this)
	);
}


int timerObj::setCallOutFunc(int (*func)(Event_ *)) {

	return(setEventCallOutFunc(callout_event,
			    func,
			    (void *)this)
	);
}

                
