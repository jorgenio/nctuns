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

#include "leakbucket.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <packet.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <maptable.h>
#include <gbind.h>
#include <phyInfo.h>
#include <nctuns_api.h>
#include <gprs/include/bss_message.h>

LeakBucket::LeakBucket(NslObject* obj, int (NslObject::*func)(Event_ *)) {
	Bmax = 0;
	B = 0;
	Tp = 0;
	R = 1;
	Bsize = 0;

	object = obj;
	memfunc = func;
	
}

LeakBucket::~LeakBucket(){
}

void LeakBucket::setBmax(int bmax){
	Bmax = bmax;
}

int LeakBucket::getBmax(){
	return Bmax;
}

void LeakBucket::setRate(double r){
	R = r;
}

double LeakBucket::getRate(){
	return R;
}

int LeakBucket::deliver(Event* ep, u_int64_t time){
	
	u_int64_t Tc = time;
	double t;
	
	TICK_TO_SEC(t,(Tc - Tp));
	bss_message* bmsg_p = reinterpret_cast<bss_message*> (ep->DataInfo_);
	
	int len = bssall_len(bmsg_p);

	int b = (int)(B + len - t * R);

	if (b <= len ) {
	
            B = len;
            Tp = Tc;
            return 1;
	
	}
	else if ( b <= Bmax){

             B = b;
             Tp = Tc;


	     double sec = B/R;

	     u_int64_t tick;

             SEC_TO_TICK(tick,sec);
             tick += Tp + 10;
		
	     Event* new_ep = copy_gprs_pkt(ep);
	     bss_message* new_bmsg_p = reinterpret_cast<bss_message*> (new_ep->DataInfo_);

             setObjEvent(new_ep, tick, 0, object, memfunc, (void*)new_bmsg_p);
 	     return 0;
	}
	else {

             return -1;
	
	}
}
