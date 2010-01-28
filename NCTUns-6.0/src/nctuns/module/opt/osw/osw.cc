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
#include <regcom.h>
#include <nctuns_api.h>
#include <scheduler.h>
#include <maptable.h>
#include <nodetype.h>
#include "osw.h"
#include <stdlib.h>
#include <gbind.h>
#include <module/opt/readpath.h>
#include <packet.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;
extern ReadPath                 *readpath_;

MODULE_GENERATOR(osw);
osw::osw(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	LPT = NULL;
	LPTc = 0;
}



/* main behaviors of osw are in send function (check LP table and send it)*/
int osw::send(ePacket_ *pkt) {

        Packet* 		p = (Packet *)pkt->DataInfo_;
	char			*from;
	struct LPTable		*tmpTable = LPT;
	struct lheader		*lh;
	int             	*oldwave;
	int             next_port;
	int             next_wave;
	char            *nextport;
				

	
	
	lh = (struct lheader *)p->pkt_get();
	from = p->pkt_getinfo("From");
	//LP request packet is broadcast, but others are not
	oldwave=(int *)p->pkt_getinfo("OLDW");


	if(oldwave!=NULL){
		next_port=readpath_->SW_get_nextport(get_nid(),from[0],*oldwave);
		next_wave=lh->wave;
		nextport=(char *)&next_port;
		p->pkt_addinfo("Next", nextport, 1);
		lh->wave=next_wave;
	}

	
	else if(oldwave==NULL){

		//find the mapping
		while(tmpTable){
			if((tmpTable->from[0] == from[0]) && (lh->wave == tmpTable->from[1])){
				break;
			}
			else
				tmpTable = tmpTable->next;
		}
		//should not happen
		if(tmpTable == NULL){
			freePacket(pkt);
			return 1;
		}
		//printf("%d 1_2 Next port =%d \n",get_nid(),tmpTable->to[0]);	
		p->pkt_addinfo("Next", &(tmpTable->to[0]), 1);
		lh->wave = tmpTable->to[1];
	}
	return NslObject::send(pkt);
}

int osw::recv(ePacket_ *pkt){
	return NslObject::recv(pkt);//0420 modiify by chenyuan
}

//mainly process the switching-setup packet here
int osw::get(ePacket_ *pkt, MBinder *frm) {
	
	Packet *p = (Packet *)pkt->DataInfo_;
	struct LPTable	*t = LPT;
	char 	*tmp, *tmp1;
	tmp = p->pkt_getinfo("Setup");
	tmp1 = p->pkt_getinfo("Tear");

	//inter module communication
	if(tmp){
		while(t){
			if(t->next)
				t = t->next;
			else
				break;
		}
		
		if(!t){
			LPT = (struct LPTable *)malloc(sizeof(struct LPTable));
			LPT->next = NULL;
			t = LPT;
		}
		else{
			t->next = (struct LPTable *)malloc(sizeof(struct LPTable));
			t = t->next;
			t->next = NULL;
		}
		t->from[0] = tmp[0];
		t->from[1] = tmp[1];
		t->to[0] = tmp[2];
		t->to[1] = tmp[3];
		LPTc++;
		freePacket(pkt);
	}
	else if(tmp1){
		while(t){
			if(((tmp1[0] == t->to[0]) && (tmp1[1] == t->to[1])) && 
			   ((tmp1[2] == t->from[0]) && (tmp1[3] == t->from[1]))){
				struct LPTable	*t1 = LPT;

				//printf("%s free the frm:%d %d to:%d %d\n", get_name(), tmp1[2], tmp1[3], tmp1[0], tmp1[1]);
				if(t == t1){
					LPT = LPT->next;
					free(t1);
					t = LPT;
				}
				else{
					while(t1->next != t)
						t1 = t1->next;
					t1->next = t->next;
					free(t);
					t = t1->next;
				}
				LPTc--;
				freePacket(pkt);
				return 1;
			}
			else
				t = t->next;
		}
	}
	else{
		p->pkt_setflow(PF_SEND);
		return send(pkt);
	}

	return 1;
}


int osw::init() {

	return(1);  
}

osw::~osw() {

}
 
