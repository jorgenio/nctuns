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
#include <object.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <scheduler.h>
#include <maptable.h>
#include <nodetype.h>
#include "op.h"
#include <stdlib.h>
#include <gbind.h>
#include <packet.h>
#include <mbinder.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

MODULE_GENERATOR(op);


op::op(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
}


op::~op() {

}

//get the packet, should change the incoming item
//record the passed path and add it to the packet
int op::get(ePacket_ *pkt, MBinder* frm) {

	Packet		*p = (Packet *)pkt->DataInfo_;
	char		*tmp = NULL, tmp1[8];
	char		inter[3];
	char		from;
	int     	i;
	struct lheader	*lh = (struct lheader *)p->pkt_get();
	struct MBlist   *bl = BinderList;

	//process normal packet
	if(lh){
		if(p->pkt_getflags() & PF_RECV){
       			for(i=0; i<PortNum; i++){
               			if(frm->myModule() == bl->sendt->bindModule())
                       			break;
	               		else
        	               		bl = bl->next;
       			}
			if(lh->option == 16){
				tmp = p->pkt_getinfo("curHop");
				inter[0] = get_nid();
				inter[1] = get_port();
				inter[2] = bl->portnum;
				sprintf(tmp1, "int%d", tmp[0]);
				p->pkt_addinfo(tmp1, inter, 3);
				tmp[0]++;
			}
			from = get_port();
			p->pkt_addinfo("From", &from, 1);
			return recv(pkt);
		}	
		else if(p->pkt_getflags() & PF_SEND){
			//LP request packet should add outgoing hop information
			if(lh->option == 16){
				inter[0] = get_nid();
				inter[1] = get_port();
				inter[2] = lh->wave;
				tmp = p->pkt_getinfo("curHop");
				sprintf(tmp1, "int%d", tmp[0]);
				p->pkt_addinfo(tmp1, inter, 3);
				tmp[0]++;
			}
			return send(pkt);
		}
	}
	else{
		return recv(pkt);
	}
	return 1;
}

// op should send the packet to the correspond wavelength
int op::send(ePacket_ *pkt) {

        Packet 		*p = (Packet *)pkt->DataInfo_;
	struct MBlist   *tmp = BinderList;
	struct lheader  *lh;

	lh = (struct lheader *)p->pkt_get();
	while(tmp){
		if(tmp->portnum == lh->wave)
			break;
		else
			tmp = tmp->next;
	}

	//should not happen
	if(!BinderList)
		freePacket(pkt);

	put(pkt, tmp->sendt);
	return 1;
}

//recieve the packet, special for LPR packet
int op::recv(ePacket_ *pkt){
	return NslObject::recv(pkt);
}

int op::init() {

	r_flowctl = DISABLED;
	s_flowctl = DISABLED;
		

	return(1);  
}
