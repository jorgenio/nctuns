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
#include <packet.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <math.h>
#include <time.h>
#include <mbinder.h>

#include <signal.h>
#include <unistd.h>
#include <event.h>

#include "wan.h"

MODULE_GENERATOR(WAN);


WAN::WAN(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : NslObject(type, id, pl, name)
{
	/* 
	 * Disable flow control 
	 */
	s_flowctl = DISABLED;
	r_flowctl = DISABLED;

	/* 
	 * Bind variables
	 */
	vBind("pkt_loss", &lost_opt);
	vBind("pkt_delay", &dtime_opt);
	vBind("pkt_reorder", &reorder_opt);
	
	vBind("delay_dis", &delay_dis);
	vBind("reorder_dis", &reorder_dis);
	
	vBind("pkt_loss_rate", &LostRate);
	vBind("pkt_reorder_rate", &R_rate);
	
	vBind("dcon_mean", &dcon_mean);
	vBind("duni_min", &duni_min);
	vBind("duni_max", &duni_max);
	vBind("dexp_min", &dexp_min);
	vBind("dexp_mean", &dexp_mean);
	vBind("dexp_max", &dexp_max);
	vBind("dnor_mean", &dnor_mean);
	vBind("dnor_var", &dnor_var);

	vBind("rcon_mean", &rcon_mean);
	vBind("runi_min", &runi_min);
	vBind("runi_max", &runi_max);
	vBind("rexp_min", &rexp_min);
	vBind("rexp_mean", &rexp_mean);
	vBind("rexp_max", &rexp_max);
	vBind("rnor_mean", &rnor_mean);
	vBind("rnor_var", &rnor_var);

	
	
	/* default value */
	lost_opt = NULL;
	dtime_opt = NULL;
	reorder_opt = NULL;

	delay_dis = NULL;
	reorder_dis = NULL;
	
	LostRate = 1;
	R_rate = 1;
	CASE = 0;

	// milisecond 
	dcon_mean = 1;
	duni_min  = 1;
	duni_max  = 5;
	dexp_min  = 1;
	dexp_mean = 3;
	dexp_max  = 5;
	dnor_mean = 0;
	dnor_var  = 1;
	
	rcon_mean = 1;
	runi_min  = 1;
	runi_max  = 5;
	rexp_min  = 1;
	rexp_mean = 3;
	rexp_max  = 5;
	rnor_mean = 0;
	rnor_var  = 1;
}

WAN::~WAN() {
}

int WAN::init() {
    
	if(lost_opt && (!strcmp(lost_opt,"on")) ){
		CASE |= LOSTRATE;
	}
	if(dtime_opt && (!strcmp(dtime_opt,"on")) ){
		CASE |= DELAYTIME;
	}
	if(reorder_opt && (!strcmp(reorder_opt,"on")) ){
		CASE |= REORDER;
	}
	drops = 0;


	if( CASE&DELAYTIME ){ 
		
		if( delay_dis && (!strcmp(delay_dis,"d_nor")) ){
			d_normal = new double[100000];
			normal(dnor_mean, dnor_var, d_normal);
		}
	}
	if( CASE&REORDER ){ 
		if( reorder_dis && (!strcmp(reorder_dis,"r_nor")) ){
			r_normal = new double[100000];
			normal(rnor_mean, rnor_var, r_normal);
		}
	}

	return(1);
}



int WAN::send(ePacket_ *pkt) {
	
	u_int64_t	pkt_tick=0;
	u_int64_t	ticktmp=0;
	Event		*ep;
	int		nor_idx=0;
	
	assert(pkt&&pkt->DataInfo_);


	if(CASE & LOSTRATE){
		if(uniform(0,10000) <= (int)(LostRate*100)){
			freePacket(pkt);
			return(1);
		}
	}

	if(CASE & DELAYTIME){
		if(delay_dis && (!strcmp(delay_dis,"d_con"))){
			MILLI_TO_TICK(pkt_tick, dcon_mean);
		}
		else if(delay_dis && (!strcmp(delay_dis,"d_uni"))){
			MILLI_TO_TICK(pkt_tick, uniform(duni_min, duni_max));
		}
		else if(delay_dis && (!strcmp(delay_dis,"d_exp"))){
			MILLI_TO_TICK(pkt_tick, expo(dexp_min, dexp_mean, dexp_max));
		}
		else if(delay_dis && (!strcmp(delay_dis,"d_nor"))){
			nor_idx = (int)(((double)random()/(double)((unsigned)MAX_RANDOM + 1))*100000);
			if(d_normal[nor_idx] < 0)
				pkt_tick = 1000;
			else
				MILLI_TO_TICK(pkt_tick,(int)d_normal[nor_idx]); 
		}
		else{
			printf("[WAN] delay time distribution error!!\n");	
		}
	}


	if(CASE & REORDER){
		if(uniform(0,10000) > (int)(R_rate*100)){
			goto send;
		}
		
		if(reorder_dis && (!strcmp(reorder_dis,"r_con"))){
			MILLI_TO_TICK(ticktmp, rcon_mean);
			pkt_tick += ticktmp;
		}
		else if(reorder_dis && (!strcmp(reorder_dis,"r_uni"))){
			MILLI_TO_TICK(ticktmp, uniform(runi_min, runi_max));
			pkt_tick += ticktmp;
		}
		else if(reorder_dis && (!strcmp(reorder_dis,"r_exp"))){
			MILLI_TO_TICK(ticktmp, expo(rexp_min, rexp_mean, rexp_max));
			pkt_tick += ticktmp;
		}
		else if(reorder_dis && (!strcmp(reorder_dis,"r_nor"))){
			nor_idx = (int)(((double)random()/(double)((unsigned)MAX_RANDOM + 1))*100000);
			if(r_normal[nor_idx] < 0)
				ticktmp=1000;
			else
				MILLI_TO_TICK(ticktmp, (int)r_normal[nor_idx]);
			pkt_tick += ticktmp;
		}
		else{
			printf("[WAN] reorder distribution error!!\n");	
		}
	}

send:
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(WAN, TimeToSendPkt);
	ep = createEvent();
	setObjEvent(ep,
		    GetNodeCurrentTime(get_nid()) + pkt_tick,
		    0,this,type,(void *)pkt);
	
	return(1);

}


int WAN::uniform(double min, double max){

	if (min > max) {
		printf("\n\n\n[WAN module] ERROR: uniform min < max\n\n");
		return(0);
	}
	return( (min==max)? 
		(int)min :
		(int)( (max-min)*( ((double)random())/((double)((unsigned)MAX_RANDOM + 1)+ min) ))
		);
}

int WAN::expo(double min, double mean, double max){
	
	double temp;

	if (min>mean || mean>max || min>max) {
		printf("\n\n\n[WAN module] ERROR: exponential min < mean < max\n\n");
		return(0);
	}

	do{
		temp = -mean * log( ((double)(random()+1)) / (double)((unsigned)MAX_RANDOM+1) );
	}while (temp<min || temp>max);
		
	return((int)temp);
}

void WAN::normal(double mean, double var, double *in_normal){
	
	double x=0, y=0, y2=0, step=0, px=0;
	int    cy=0, py=0;
	int    i=0;

	step = (6*var)/1000;
	x = mean - 3*var;
	px = x;
	while(i<1000 && x<=(3*var+mean)){
		y = 1.0/(var*sqrt(2*M_PI)) * exp(-pow(x-mean,2)/(2*var*var));
		y2 = y2 + y*step;

		cy = (int)(y2*100000);
		while(py<cy){
			in_normal[py]=px;
			py++;
		}
		py = cy;
		px = x;
		
		x += step;
		i++;
	}
	while(py<100000){
		in_normal[py] = mean+3*var;
		py++;
	}

	return ;
}


int WAN::TimeToSendPkt(ePacket_ *pkt){
	
	ePacket_ *ep;
	
	assert(pkt);
	assert(pkt->DataInfo_);

	ep = (ePacket_ *)pkt->DataInfo_;

	if(sendtarget_->qfull()){
		if(ep){
			freeEvent(pkt);
			printf("[WAN] drop pkt: %d\n", (++drops));
			return(1);
		}
	}else{
		pkt->DataInfo_ = NULL;
		freeEvent(pkt);
		put(ep, sendtarget_);
		return(1);
	}
	return(1);
}

