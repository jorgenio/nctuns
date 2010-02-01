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
#include <heap.h>
#include <nctuns_api.h>
#include <scheduler.h>

extern  scheduler *scheduler_;

HeapObject::HeapObject()
: maxEvent_(MAX_EVENT)
, numEvent_(0)
, numInsertEvent_(0llu)
, numDequeueEvent_(0llu)
, eventHeap_(new Message_*[MAX_EVENT])
{
	if (!eventHeap_)
		ErrorMesg("Can not allocate heap buffer!");  	

	/*
	 * initialize the heap buffer
	 */
	for(int i = 0; i < maxEvent_; i++ )
		eventHeap_[i] = NULL;
}
  

HeapObject::HeapObject(int max_heap)
: maxEvent_(max_heap)
, numEvent_(0)
, numInsertEvent_(0llu)
, numDequeueEvent_(0llu)
, eventHeap_(new Message_*[max_heap])
{
	if ( !eventHeap_ )
		ErrorMesg("Can not allocate heap buffer!");         

	/*
	 * initialize the heap buffer
	 */
	for(int i = 0; i < maxEvent_; i++ )
		eventHeap_[i] = NULL;
}


HeapObject::~HeapObject()
{
	if (eventHeap_)
		delete [] eventHeap_;
}    


#define TRUE		1
#define FALSE		0
  
int HeapObject::insertEvent(Message_ *ep) {

	int 		i, NotDone;


	/* Check the legality of event */
	if (ep == NULL) return(-1);
	if ( numEvent_ >= (maxEvent_-1) ) 
		ErrorMesg("Event heap is full!"); 

	/* 
	 * If the timeStamp of event is less than
	 * system current time, then we reject to
	 * insert this event into event shceduler.
	 */
	if (ep->timeStamp_ < GetCurrentTime()) {
		printf("HeapObject::insertEvent(): ep->timeStamp_ %llu ", ep->timeStamp_);
		printf(" < GetCurrentTime() %llu \n", GetCurrentTime());
		exit(0);
	}

	//
	// If the timeStamp of event exceeds the max simulation time,
	// then we rejet to insert this event.
	//
	if (scheduler_->maxsimtime() > 0 && ep->timeStamp_ > scheduler_->maxsimtime()) {    
		return (0);
	}

	numEvent_++;
	numInsertEvent_++;
	i = numEvent_;
	NotDone = TRUE;
	while(NotDone) {
		if (i == 1) 
			NotDone = FALSE; /* at root */
		else {
			if (ep->timeStamp_ > eventHeap_[i/2]->timeStamp_)
				NotDone = FALSE;
			/* if the time stamp is equal, we will compare priority of event 
 			 * case 1: new event priority less than event in heap, new event be inserted heap tail
 			 */
			else if ( (ep->timeStamp_ == eventHeap_[i/2]->timeStamp_) && (ep->priority_ <= eventHeap_[i/2]->priority_) )
				NotDone = FALSE;
			// case 2: new event priority more than event in heap, recursive find correct position for every heap element
			else if ( (ep->timeStamp_ == eventHeap_[i/2]->timeStamp_) && (ep->priority_ > eventHeap_[i/2]->priority_) ) {
				eventHeap_[i] = eventHeap_[i/2];
				i = i / 2;
			}
			else {
				eventHeap_[i] = eventHeap_[i/2];
				i = i / 2;
			}
		}    
	}       
	eventHeap_[i] = ep;  

	return 1;
}


Message_ * HeapObject::dequeueEvent(u_int64_t curtime) {

	Message_		*ep = NULL, *epLast = NULL;
	int			i, j, NotDone;


	if ( numEvent_ == 0 ) 
		return(NULL);
	     
#if 0	// obsolete
	if (eventHeap_[1]->timeStamp_ > curtime )
		return(NULL);
#endif

	NotDone = TRUE;
	ep = eventHeap_[1];
	epLast = eventHeap_[numEvent_];
	numEvent_--;
	numDequeueEvent_++;

	/* j is the left child of i */
	i = 1; j = 2;             

	while((j <= numEvent_) && NotDone ) {
		if (j < numEvent_) {
			if (eventHeap_[j]->timeStamp_ >= 
			    eventHeap_[j+1]->timeStamp_)
				/* now j points to the smaller child */
				j++;   
			else if( (eventHeap_[j]->timeStamp_ == eventHeap_[j+1]->timeStamp_) &&
				 (eventHeap_[j]->priority_ < eventHeap_[j+1]->priority_) )
				j++;
		}
		
		if (epLast->timeStamp_ < eventHeap_[j]->timeStamp_)
			NotDone = FALSE;
		else if ( (epLast->timeStamp_ == eventHeap_[j]->timeStamp_) && (epLast->priority_ > eventHeap_[j]->priority_) )
			NotDone = FALSE;
		else {
			eventHeap_[i] = eventHeap_[j];
			i = j;
			j = 2 * i;      
		}  
	} 
	eventHeap_[i] = epLast;
	return(ep);   
}

int HeapObject::DumpNumEvent() {
	return(numEvent_);
}

u_int64_t HeapObject::DumpNumInsertEvent() {
	u_int64_t return_value;
	
	return_value = numInsertEvent_;
	numInsertEvent_ = 0;
	return(return_value);
}

u_int64_t HeapObject::DumpNumDequeueEvent() {
	u_int64_t return_value;
	
	return_value = numDequeueEvent_;
	numDequeueEvent_ = 0;
	return(return_value);
}

