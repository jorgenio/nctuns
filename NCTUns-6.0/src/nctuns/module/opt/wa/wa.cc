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
#include <timer.h>
#include <nodetype.h>
#include "wa.h"
#include <stdlib.h>
#include <gbind.h>
#include <packet.h>
#include <mbinder.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

MODULE_GENERATOR(wa);
wa::wa(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	LPT = NULL;
	LPTc = 0;
	RLP = NULL;
	RLPc = 0;
	WC = 0;

	vBind("WavelenConversion", &WC);
}


wa::~wa() {

}

/* main behaviors of osw are in send function (check LP table and send it)*/
int wa::send(ePacket_ *pkt) {

	return NslObject::send(pkt);
}

int wa::recv(ePacket_ *pkt){

	return NslObject::recv(pkt);
}

//get packet, turn around the packet, send out the packet
//process the LP request and reply here
//request : broadcast it, and make the RLP, make sure no duplicate request
//reply   : construct LPT
int wa::get(ePacket_ *pkt, MBinder *frm) {
	
	struct lheader		*lh;
	Packet 			*p = (Packet *)pkt->DataInfo_;
	
	lh = (struct lheader *)p->pkt_get();
	if((lh->option & 0xf0) == 0){
		//normal packet, so let it pass
		return NslObject::get(pkt, frm);
	}
	else if(lh->option == 16){
		//LP request packet, we have to record it for
		//avoiding duplicated flood
		struct RLPTable	*rlp = RLP;
		bool   mark = false;

		while(rlp){
			if((rlp->LPID[0] == lh->srcIP) && (rlp->LPID[1] == lh->dstIP)){
				freePacket(pkt);
				return 1;
			}
			else{
				if(rlp->next)
					rlp = rlp->next;
				else
					break;
			}
		}

		//not come here yet, so add LPR
		//add LPR
		if(!rlp){
			RLP = (struct RLPTable *)malloc(sizeof(struct RLPTable));
			RLP->next = NULL;
			rlp = RLP;
		}
		else if(!mark){
			rlp->next = (struct RLPTable *)malloc(sizeof(struct RLPTable));
			rlp = rlp->next;
			rlp->next = NULL;
		}
		rlp->LPID[0] = lh->srcIP;
		rlp->LPID[1] = lh->dstIP;
		rlp->time = GetNodeCurrentTime(get_nid());
		RLPc++;
		
		//should not forward to osw, we turn it back here
		p->pkt_setflow(PF_SEND);
		return send(pkt);
	}
	else if((lh->option & 0xf0) == 32){
		//LP reply packet, we check the port.wave is occupied or not
		struct LPTable 	*lpt = LPT;
		char		*hop, Hop, *interfrm, *interto, mark = 0;
		char		str[8];
		
		hop = p->pkt_getinfo("curHop");
		Hop = hop[0];
		Hop -= 2;
		sprintf(str, "int%d", Hop);
		interto = p->pkt_getinfo(str);
		Hop -= 1;
		sprintf(str, "int%d", Hop);
                interfrm = p->pkt_getinfo(str);
		hop[0] -= 2;
		
		if(!WC){
			if((lh->option & 0x01) == 1){
				while(lpt){
					if((interto[1] == lpt->to[0]) && (lh->wave == lpt->to[1])){
						//failed, return failed packet
						//WC support should modify here
						mark = 1;
						break;
					}
					else{
						if(lpt->next)
							lpt = lpt->next;
						else
							break;
					}
				}
			
				if(mark){
					//failed
					lh->option &= 0xfe;
				}
				else{
					//the path you traversed is not occupied yet, so 
					//you can build the LPT (temporarily)
					if(!lpt){
						LPT = (struct LPTable *)malloc(sizeof(struct LPTable));
						lpt = LPT;
						lpt->next = NULL;
					}
					else{
						lpt->next = (struct LPTable *)malloc(sizeof(struct LPTable));
						lpt = lpt->next;
						lpt->next = NULL;
					}
					lpt->to[0] = interto[1];
					lpt->to[1] = lh->wave;
					lpt->LPID[0] = lh->dstIP;
					lpt->LPID[1] = lh->srcIP;
					LPTc++;
				}
			}
		}
		//WC process is here
		else{
			char	wave = 1;

			while(lpt){
				if(lpt->to[0] == interto[1])
					wave++;
				lpt = lpt->next;
			}
			lpt = LPT;
			if(wave > sendtarget_->bindModule()->sendtarget_->bindModule()->PortNum){
				//failed
				lh->option &= 0xfe;
			}
			else{
				//add LPT				
				if(!lpt){
					LPT = (struct LPTable *)malloc(sizeof(struct LPTable));
					lpt = LPT;
					lpt->next = NULL;
				}
				else{
					lpt->next = (struct LPTable *)malloc(sizeof(struct LPTable));
					lpt = lpt->next;
					lpt->next = NULL;
				}
				lpt->to[0] = interto[1];
				lpt->to[1] = wave;
				lpt->LPID[0] = lh->dstIP;
				lpt->LPID[1] = lh->srcIP;
				LPTc++;

				//change intermediate item
				char h = hop[0] + 1;
				char *intert;
				sprintf(str, "int%d", h);
				intert = p->pkt_getinfo(str);
				if(intert)
					intert[2] = wave;
				interto[2] = wave;
			}
		}
		
		//send to next hop
		p->pkt_setflow(PF_SEND);
		p->pkt_addinfo("Next", &(interfrm[1]), 1);
		(void)send(pkt);
		return 1;
	}
	else if((lh->option & 0xf0) == 48){
		//LP tear down , so find the LPID & remove it, then send configure packet
		//to tear it from osw module
		struct LPTable	*lpt = LPT;
		struct LPTable	*lptf = LPT;
		char		confinfo[4];
		ePacket_	*conf;
		Packet		*pc;

		while(lpt){
			if(((lpt->LPID[0] == lh->srcIP) && (lpt->LPID[1] == lh->dstIP)) &&
			    (lh->wave == lpt->to[1])){
				//forward to next hop
				char *frm = p->pkt_getinfo("From");
				
				confinfo[0] = lpt->to[0];
				confinfo[1] = lpt->to[1];
				confinfo[2] = frm[0];
                                confinfo[3] = lh->wave;
				
				p->pkt_addinfo("Next", &(lpt->to[0]), 1);
                                lh->wave = lpt->to[1];
                                p->pkt_setflow(PF_SEND);
                                send(pkt);
				
				//tear down the LP table entry
				lptf = LPT;
				if(lptf == lpt){
					LPT = LPT->next;
					free(lpt);
					lpt = LPT;
				}
				else{
					while(lptf->next != lpt)
						lptf = lptf->next;
					lptf->next = lpt->next;
					free(lpt);
					lpt = lptf->next;
				}
				LPTc--;

				//send configurateion packet
				conf = createPacket();
				pc = (Packet *)conf->DataInfo_;
				pc->pkt_addinfo("Tear", confinfo, 4);
				pc->pkt_setflow(PF_RECV);
				return NslObject::recv(conf);
			}
			else
				lpt = lpt->next;
		}
	}
	else if((lh->option & 0xf0) == 64){
		struct LPTable	*lpt = LPT;
		struct LPTable	*l;
		char		*hop, *next, *interfrm, *interto, inter[8];
		
		if(!(lh->option & 0x01)){
			//failed request
			//first we have to remove the requested entry
			while(lpt){
				if((lpt->LPID[0] == lh->srcIP)&&(lpt->LPID[1] == lh->dstIP)){
					l = LPT;
					if(l == lpt){
						LPT = LPT->next;
						free(lpt);
						lpt = LPT;
					}
					else{
						while(l->next != lpt)
							l = l->next;
						l->next = lpt->next;
						free(lpt);
						lpt = l->next;
					}
					LPTc--;
				}
				else
					lpt = lpt->next;
			}
		}
		//success, so communicate the upper to add SWTB
		else{
			char 		setup[4], hc;
			ePacket_	*conf = createPacket();
			Packet		*pc = (Packet *)conf->DataInfo_;
			
			//send the SWTB config packet
			hop = p->pkt_getinfo("curHop");
			hc = hop[0];
			hc += 1;
			sprintf(inter, "int%d", hc);
			interfrm = p->pkt_getinfo(inter);
			hc += 1;
			sprintf(inter, "int%d", hc);
			interto = p->pkt_getinfo(inter);
			setup[0] = interfrm[1];
			setup[1] = interfrm[2];
			setup[2] = interto[1];
			setup[3] = interto[2];
			pc->pkt_setflow(PF_RECV);
			pc->pkt_addinfo("Setup", setup, 4);
			recv(conf);
		}
		//send to next hop
		hop = p->pkt_getinfo("curHop");
                hop[0] += 2;
                sprintf(inter, "int%d", hop[0]);
                next = p->pkt_getinfo(inter);
		p->pkt_setflow(PF_SEND);
		p->pkt_addinfo("Next", &(next[1]), 1);
		lh->wave = next[2];
		return send(pkt);
	}
	else if(lh->option == 17){
		char *hop, inter[8], *next;
		
		hop = p->pkt_getinfo("curHop");
                hop[0] += 2;
                sprintf(inter, "int%d", hop[0]);
                next = p->pkt_getinfo(inter);
                p->pkt_setflow(PF_SEND);
                p->pkt_addinfo("Next", &(next[1]), 1);
                return send(pkt);
	}
	else{
		//should not happen
		freePacket(pkt);
		return 1;
	}
	return 1;
}

void wa::rlp_tear(){
	struct RLPTable	*rlp = RLP;
	while(rlp){
		if(GetCurrentTime() - rlp->time > RLP_lifetime){
			struct RLPTable	*rlp1 = RLP;

			//free this entry
			if(rlp1 == rlp){
				RLP = rlp->next;
				rlp = RLP;
				free(rlp1);
			}
			else{
				while(rlp1->next != rlp)
					rlp1 = rlp1->next;
				rlp1->next = rlp->next;
				free(rlp);
				rlp = rlp1->next;
			}
			RLPc--;
		}
		else
			rlp = rlp->next;
	}
}

int wa::init() {
	timerObj	*timer = new timerObj();
	
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(wa, rlp_tear);
	timer->setCallOutObj(this, type);
	timer->start(0, 10000000);
	RLP_lifetime = 30000000;
	return(1);  
}
