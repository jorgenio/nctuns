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
#include "ma.h"
#include <stdlib.h>
#include <gbind.h>
#include <packet.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

MODULE_GENERATOR(ma);
ma::ma(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	LPT = NULL;
	LPTc = 0;
}

ma::~ma(){
}

/* main behaviors of osw are in send function (check LP table and send it)*/
int ma::send(ePacket_ *pkt) {

        Packet* 		p = (Packet *)pkt->DataInfo_;
	ePacket_		*ep;
	char			*from, *next;
	//struct LPTable		*tmpTable = LPT;
	struct lheader		*lh = (struct lheader *)p->pkt_get();
	struct MBlist		*bl = BinderList;
	
	if(lh->option == 16){
		from = p->pkt_getinfo("From");
                while(bl){	
                        if(from[0] != bl->portnum){
                                ep = pkt_copy(pkt);
                                put(ep, bl->sendt);
                        }
                        bl = bl->next;
                }
	        return 1;
	}
	else{
		next = p->pkt_getinfo("Next");
		while(bl){
			if(bl->portnum == next[0])
				return put(pkt, bl->sendt);
			else
				bl = bl->next;
		}
	}

	return 0;
}

int ma::init() {

	r_flowctl = DISABLED;
	s_flowctl = DISABLED;
		

	return(1);  
}
