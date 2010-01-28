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
#include <timer.h>
#include <scheduler.h>
#include <maptable.h>
#include <nodetype.h>
#include "rwa.h"
#include <stdlib.h>
#include <gbind.h>
#include <module/opt/readpath.h>
#include <packet.h>
#include <mbinder.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;
extern ReadPath			*readpath_;

int	autogen = 1;

MODULE_GENERATOR(rwa);

rwa::rwa(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	WT = NULL;
	WA = NULL;
	pktq = NULL;
	WTnum = 0;
	WAnum = 0;
	PQnum = 0;

	vBind("AutogenShortestPath", &autogen);
	vBind("WavelenConversion", &WavelenConversion);
	vBind("MaxWave",&max_wave);
	vBind("ChooseWave",&choose_wave);
	vBind("nodekindfile",&nodekindfile);
	vBind("nodeconnectfile",&nodeconnectfile);
	vBind("ringfile",&ringfile);
	vBind("pathfile",&nodepathfile);
        
	vBind("Wa_lifetime", &Wa_lifetime);
        vBind("Wa_polling", &Wa_polling);
	
	Wa_lifetime = 300; //unit is second
	Wa_polling = 3;    //unit is second
	AWA = NULL;
}

rwa::~rwa() {
}

void rwa::Initial_AWA(){

	struct AvailWA * ptr = AWA;
	
	for(int i =0 ; i < max_wave ; i++){
		if (i == 0){
			AWA = (struct AvailWA *)malloc(sizeof(struct AvailWA));
			ptr = AWA;
		}
		else{
			ptr ->next = (struct AvailWA *)malloc(sizeof(struct AvailWA));
			ptr = ptr->next;
		}
		ptr->used = 0;
		if (i == max_wave -1) // last one 
			ptr->next = NULL;
	}
	
}

void rwa::Set_wave_avail(int wave){
				
	struct AvailWA * ptr = AWA;
		
	for(int i =0 ; i < wave ; i++){
		ptr = ptr->next;
	}

	ptr->used = 1;
}

void rwa::Set_wave_unavail(Event_ *ep){
				
	struct Event_pinfo        *eInfo;

	eInfo =  (struct Event_pinfo *)ep->DataInfo_;
	
	struct AvailWA * ptr = AWA;
	int wave = eInfo->wave;
	
	for(int i =0 ; i < wave ; i++){
		ptr = ptr->next;
	}

	ptr->used = 0;

	freeEvent(ep);

}

int rwa::Is_avail_wave(int wave){
				
	struct AvailWA * ptr = AWA;

	if (wave > max_wave)
		return 0;
	int i;		
	for( i =0 ; i < wave ; i++){
		ptr = ptr->next;
	}
	if (ptr->used == 0)
		return 1;
	else
		return 0;
}


int rwa::get_avail_wave(){
				
	struct AvailWA * ptr = AWA;
		
	for(int i =0 ; i < max_wave  ; i++){
		if (ptr == NULL)
			return -1;
		if (ptr->used == 0){
			return (i+1);
		}
		ptr = ptr->next;
	}
		return -1;
}

	//send has to process 2 functions:
	//1. normal packet from interface, check the table, yes, add LP function & send
	//   no, send WA configure packet.

int rwa::recv(ePacket_ *pkt){
	
	return NslObject::recv(pkt);
}

int rwa::send(ePacket_ *pkt) {
		
	u_long  	next;
	struct  	WaTable *tmp = WT;
	Packet		*p = (Packet *)pkt->DataInfo_;
	NslObject 	*obj = sendtarget_->bindModule();
	struct lheader	*lh;

	next = (u_long)p->rt_gateway();

	if (autogen == 1){
		int wave ,wave_get_from_short;
		wave_get_from_short = readpath_->RWA_get_wave(get_nid(),ipv4addr_to_nodeid(next));
		wave = wave_get_from_short;
		
		if (choose_wave > 0)
			wave = choose_wave;

		struct plist * tmplist;
		tmplist =  (struct plist *)malloc(sizeof(struct plist));
		tmplist->pid = get_port();
		tmplist->next =  (struct plist *)malloc(sizeof(struct plist));
		tmplist->next->pid = wave;
		tmplist->next->next = NULL;

		int length = p->pkt_getlen();
		double*	bandwidth =(double *) get_regvar(get_nid(),tmplist,"BW");	

		free(tmplist->next);
		free(tmplist);
		

		Event_*	 			tep;
		tep = createEvent();
		
					
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(rwa,Set_wave_unavail);
		
		/* set time to release wave */

		u_int64_t       onesec;
		SEC_TO_TICK(onesec,1);
		u_int64_t       delayTick ;
		double 	        delayTick_double ,dt;
		delayTick_double = length * 8 * onesec / (*bandwidth);
		delayTick = (u_int64_t)delayTick_double;
		dt = (delayTick_double - delayTick) * 10;
		if (dt >= 5)
			delayTick++;
		
		struct Event_pinfo *info ;
		info = (struct Event_pinfo *)malloc(sizeof(struct Event_pinfo));
		info->wave = wave;
        	assert(info);
	
	
		tep->DataInfo_ = (void *)info;
		setObjEvent(tep,GetCurrentTime()+delayTick,0,this,type,tep->DataInfo_);//
		
		p->pkt_addinfo("OLDW",(char *)&wave_get_from_short,sizeof(int));
		lh = (struct lheader *)p->pkt_malloc(sizeof(struct lheader));
		lh->dstIP = next;
		lh->wave = wave;
		lh->option = 0;
		lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
		Set_wave_avail(wave);
		return NslObject::send(pkt);
	}

	/* if autogen == 0, then below will execute */
	
	while(tmp){
		if((tmp->LPID == next) && (tmp->status == 1))
			break;
		else if((tmp->LPID == next) && (tmp->status == 0)){
			qpacket(next, pkt);
			return 1;
		}
		else
			tmp = tmp->next;
	}

	if(tmp){
		//the LP table is found, then add info "srcIP"
		//and "dstIP" for LP ID
		lh = (struct lheader *)p->pkt_malloc(sizeof(struct lheader));
		lh->dstIP = next;
		lh->wave = tmp->wave;
		lh->option = 0;
		lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
		tmp->time = GetCurrentTime();
		return NslObject::send(pkt);
	}
	else{
		//the LP not found, then create & send the 
		//LP request packet. after setting up, then
		//send the normal packet out.
		if(WTnum >= obj->PortNum){
			//the wavelength is used out
			freePacket(pkt);
			return 1;
		}
		else{
			ePacket_	*LPR = createPacket();
			struct lheader	*lh;
			char		hop = 0, wave = 1;
			struct WaTable	*wt = WT, *lpt = WT, *lpt1;

			qpacket(next, pkt);
			//make sure that wave is the minimal-non-used one
			while(wt){
				if(wave == wt->wave){
					wave++;
					wt = wt->next;
				}
				else
					break;
			}
			
			//add the wave we want to use
			while(lpt){
				if(WT->wave > wave){
					lpt = NULL;
					break;
				}
				if(lpt->wave < wave)
					lpt = lpt->next;
				else
					break;
			}
			if(!lpt){
				lpt1 = WT;
				lpt = (struct WaTable *)malloc(sizeof(struct WaTable));
				lpt->next = lpt1;
				WT = lpt;
			}
			else{
				lpt1 = WT;
				while(lpt1){
					if(lpt1->next == lpt)
						break;
					else
						lpt1 = lpt1->next;
				}
				lpt1->next = (struct WaTable *)malloc(sizeof(struct WaTable));
				lpt1->next = lpt;
				lpt = lpt1;
			}
			lpt->LPID = next;
			lpt->wave = wave;
			lpt->status = 0;
			WTnum++;
			
			printf("%s -------- %d\n", get_name(), wave);
			//send out LPR packet
			p = (Packet *)LPR->DataInfo_;
			p->pkt_addinfo("curHop", &hop, sizeof(char));
			lh = (struct lheader *)p->pkt_malloc(sizeof(struct lheader));
			lh->dstIP = next;
			lh->wave = wave;
			lh->option = 16;
			lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
			return NslObject::send(LPR);
		}
	}
	return 1;
}


int rwa::qpacket(u_long LPID, ePacket_ *pkt){

	struct PKTQ	*q = pktq;

	while(q){
		if(q->next != NULL)
			q = q->next;
		else
			break;
	}
	
	if(!q){
		pktq = (struct PKTQ *)malloc(sizeof(struct PKTQ));
		pktq->next = NULL;
		q = pktq;
	}
	else{
		q->next = (struct PKTQ *)malloc(sizeof(struct PKTQ));
		q = q->next;
		q->next = NULL;
	}
	q->LPID = LPID;
	q->pkt = pkt;
	PQnum++;
	return 1;
}

int rwa::resume(u_long LPID, char wave){
	
	struct PKTQ	*q = pktq;
	struct PKTQ	*tmp;
	ePacket_	*pkt;

	while(q){
		if(q->LPID == LPID){
			//send the packet and free the queue entry
			//for efficiency, we add header here and send it directly
			struct lheader *lh;
			Packet  *p;
			
			pkt = q->pkt;
			p = (Packet *)pkt->DataInfo_;
			lh = (struct lheader *)p->pkt_malloc(sizeof(struct lheader));
			lh->dstIP = LPID;
			lh->wave = wave;
			lh->option = 0;
			NslObject::send(pkt);

			//free this entry
			tmp = pktq;
			if(PQnum == 1){
				free(pktq);
				pktq = NULL;
				q = pktq;
				PQnum = 0;
			}
			else if(tmp == q){
				pktq = pktq->next;
				free(q);
				q = pktq;
				PQnum--;
			}
			else{
				while(tmp->next != q)
					tmp = tmp->next;
				tmp->next = q->next;
				free(q);
				q = tmp->next;
				PQnum--;
			}
		}
		else
			q = q->next;
	}

	return 1;
}

int rwa::freequeue(u_long LPID){
	
	struct PKTQ	*q = pktq;
	struct PKTQ	*tmp;

	while(q){
		if(q->LPID == LPID){
			//free this entry
			tmp = pktq;
			if(PQnum == 1){
				free(pktq);
				pktq = NULL;
				PQnum = 0;
			}
			else if(tmp == q){
                                pktq = pktq->next;
                                free(q);
				q = pktq;
                                PQnum--;
                        }
			else{
				while((tmp->next != q) && tmp)
					tmp = tmp->next;
				tmp->next = q->next;
				free(q);
				q = tmp->next;
				PQnum--;
			}
		}
		else
			q = q->next;
	}
	return 1;
}

int rwa::get(ePacket_ *pkt, MBinder *frm) {

	Packet	*p = (Packet *)pkt->DataInfo_;
	struct lheader *lh = (struct lheader *)p->pkt_get();
	
	if(p->pkt_getflags() & PF_RECV){
		//normal packet received
		
		
		if((lh->option & 0xf0) == 0){
			if(lh->dstIP == nodeid_to_ipv4addr(get_nid(), get_port())){
				
				//!!!!RECV
				return recv(pkt);
			}
			else
				freePacket(pkt);
		}
		//we get a LP request packet
		//we have to check the wavelength is used up or not...
		//then send the reply
		else if((lh->option & 0xf0) == 16){
			char 			*hop;
			int			mark = 0;
			struct WAssigned	*wa = WA;

			if(lh->dstIP != nodeid_to_ipv4addr(get_nid(), get_port())){
				freePacket(pkt);
				return 1;
			}
			//find out the requesteded wavelength is assigned or not
			while(wa){
				if(lh->wave == wa->wave){
					mark = 1;
					break;
				}
				else{
					if(wa->next)
						wa = wa->next;
					else
						break;
				}
			}
		
			hop = p->pkt_getinfo("curHop");
			if(lh->option == 17)
				hop[0] += 2;
			
			//send out reply
			if(!mark){
				//success, the requested wavelength is not used yet
				//so add the WA table.
				if(!wa){
					WA = (struct WAssigned *)malloc(sizeof(struct WAssigned));
					WA->next = NULL;
					wa = WA;
				}
				else{
					wa->next = (struct WAssigned *)malloc(sizeof(struct WAssigned));
					wa = wa->next;
					wa->next = NULL;
				}
				WAnum++;
				wa->wave = lh->wave;
				wa->LPID = lh->srcIP;
				wa->status = 0; //wait
				lh->option = 33;
			}
			else{
				//the requested wave is used 
				lh->option = 32;
			}
			lh->dstIP = lh->srcIP;
			lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
			p->pkt_setflow(PF_SEND);
			//!!!!NSL SEND
			(void)NslObject::send(pkt);
			return 1;
		}
		//we get a LP reply
		//if success, we build up the entry and set it up
		//if fail send another LP request
		else if((lh->option & 0xf0) == 32){
			struct WaTable		*lpt = WT;
			char			c = 0;
		
			if((lh->option & 0x01)){
				//success
				while(lpt){
					if(lpt->LPID == lh->srcIP){
						lpt->status = 1;
						lpt->time = GetCurrentTime();
						break;
					}
					else
						lpt = lpt->next;
				}
				
				//send out LP request success packet
				lh->option = 65;
				lh->dstIP = lh->srcIP;
				lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
				p->pkt_setflow(PF_SEND);
				p->pkt_addinfo("curHop", &c, 1);
				//!!!NSL SEND
				NslObject::send(pkt);
				//resume, send the queued packet
				resume(lh->dstIP, lh->wave);
				return 1;
			}	
			else{
				//failed, remote router used out the wavelength or intermediate
				//switch get a conflict wavelength so it changes the flag to fail
				
				NslObject 	*obj = sendtarget_->bindModule();
				u_long		freetarget = lh->srcIP;	
				struct WaTable	*lpt = WT, *lpt1 = WT;

				while(lpt){
					if(lpt->LPID == lh->srcIP)
						break;
					else
						lpt = lpt->next;
				}

				//found the free target
				if(lpt){
					if(lpt1 == lpt){
						free(WT);
						WTnum--;
						WT = NULL;
					}
					else{
						while(lpt1)
							if(lpt1->next != lpt){
								lpt1 = lpt1->next;
								break;
							}
						lpt1->next = lpt->next;
						free(lpt);
						WTnum--;
					}
				}

				lh->dstIP = lh->srcIP;
				lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
				lh->option = 64;
				p->pkt_setflow(PF_SEND);
		
				if(WTnum + 1>= obj->PortNum){
					//we ran out of the wavelength, so we have to tell remote
					//to tear down the ready assignment
					freequeue(freetarget);
					//!!!NSL SEND
					return NslObject::send(pkt);
				}
				else{
					//send LPR 3rd fail packet and another LPR again...
					int		i;
					ePacket_	*LPR;
					Packet		*LPRC;
					struct WaTable  *wt = WT, *lpt1, *lpt = WT;
					char		wave = lh->wave + 1;
					//make sure that wave is the minimal-non-used one
					while(wt){
						if(wave == wt->wave){
							wave++;
							wt = wt->next;
						}
						else
							break;
					}
		
					//add the wave
					while(lpt){
						if(WT->wave > lh->wave){
							lpt = NULL;
							break;
						}
						if(lpt->wave < lh->wave)
							lpt = lpt->next;
						else
							break;
					}
					if(!lpt){
						lpt1 = WT;
						lpt = (struct WaTable *)malloc(sizeof(struct WaTable));
						lpt->next = lpt1;
						WT = lpt;
					}
					else{
						lpt1 = WT;
						while(lpt1){
							if(lpt1->next == lpt)
								break;
							else
								lpt1 = lpt1->next;
						}
						lpt1->next = (struct WaTable *)malloc(sizeof(struct WaTable));
						lpt1->next = lpt;
						lpt = lpt1;
					}
					lpt->LPID = lh->dstIP;
					lpt->wave = wave;
					lpt->status = 0;
					WTnum++;

					//change the intermediate wave
					for(i=0; i<255; i++){
						char	str[8], *inter;
						
						sprintf(str, "int%d", i);
						inter = p->pkt_getinfo(str);
						if(inter)
							inter[2] = wave;
						else
							break;
					}
			
					p->pkt_addinfo("curHop", &c, 1);
					LPR = pkt_copy(pkt);
					LPRC = (Packet *)LPR->DataInfo_;
					lh = (struct lheader *)LPRC->pkt_get();
					lh->option = 17;
					lh->wave++;
					//NSL SEND
					(void)NslObject::send(pkt);
					(void)NslObject::send(LPR);
					return 1;
				}
			}
		}
		//we recv a LP tear down packet
		//so we remove the specified WA entry
		else if((lh->option & 0xf0) == 48){
			//it is a LP tear down packet
			struct WAssigned *wa, *tmp;

			wa = WA;
			while(wa){
				if(wa->LPID == lh->srcIP){
					tmp = WA;
					if(tmp == wa){
						WA = WA->next;
						free(tmp);
						wa = WA;
					}
					else{
						while(tmp->next != wa)
							tmp = tmp->next;
						tmp->next = wa->next;
						free(wa);
						wa = tmp->next;
					}
					WAnum--;
					printf("%s ready to free WA, %d\n", get_name(), WAnum);
					freePacket(pkt);
					return 1;
				}
				else
					wa = wa->next;
			}
			return 1;
		}
		//we get a LP 3rd checking packet
		//success, we change the status to OK,
		//failed, we remove the entry
		else if((lh->option & 0xf0) == 64){
			//3 way handshake LP setup is down...
			struct WAssigned *wa = WA;

			if(lh->option & 0x01){
				//success so make the state ok
				while(wa){
					if(wa->LPID == lh->srcIP){
						wa->status = 1;
						break;
					}
					else
						wa = wa->next;
				}
				freePacket(pkt);
			}
			else{
				//failed so get off the WA entry
				struct WAssigned *tmp;

				while(wa){
					tmp = WA;
					if(wa->LPID == lh->srcIP){
						if(WAnum == 1){
							free(WA);
							WA = NULL;
							WAnum = 0;
						}
						else{	
							while(tmp->next != wa)
								tmp = tmp->next;
							tmp->next = wa->next;
							free(wa);
							wa = tmp->next;
							WAnum--;
						}
						break;
					}
					else
						wa = wa->next;
				}
				freePacket(pkt);
			}
			return 1;
		}
	}
	else{

		//SEND
		return send(pkt);
	}
	return 1;
}

void rwa::wave_tear(){
	struct WaTable	*wt = WT;
	while(wt){
		if(GetCurrentTime() - wt->time > walf){
			//expire, so send tear down packet
			struct WaTable	*wt1 = WT;
			ePacket_	*tear = createPacket();
			Packet		*p = (Packet *)tear->DataInfo_;
			struct lheader	*lh = (struct lheader *)p->pkt_malloc(sizeof(struct lheader));

			p->pkt_setflow(PF_SEND);
			lh->dstIP = wt->LPID;
			lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
			lh->option = 48;
			lh->wave = wt->wave;
			NslObject::send(tear);

			//free this entry
			if(wt1 == wt){
				WT = wt->next;
				wt = WT;
				free(wt1);
			}
			else{
				while(wt1->next != wt)
					wt1 = wt1->next;
				wt1->next = wt->next;
				free(wt);
				wt = wt1->next;
			}
			WTnum--;
		}
		else
			wt = wt->next;
	}
}

int rwa::init() {
	
	timerObj	*timer = new timerObj();


        SEC_TO_TICK(walf, Wa_lifetime);
        SEC_TO_TICK(wapl, Wa_polling);
	
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(rwa, wave_tear);
	timer->setCallOutObj(this, type);
	timer->start(0, wapl);

	if (autogen == 1){
	    
		char * ndt_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(nodekindfile)+1);
		sprintf(ndt_FILEPATH,"%s%s",GetConfigFileDir(),nodekindfile);

		char * gph_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(nodeconnectfile)+1);
		sprintf(gph_FILEPATH,"%s%s",GetConfigFileDir(),nodeconnectfile);

		char * osr_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(ringfile)+1);
		sprintf(osr_FILEPATH,"%s%s",GetConfigFileDir(),ringfile);
		
		char * osp_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(nodepathfile)+1);
                sprintf(osp_FILEPATH,"%s%s",GetConfigFileDir(),nodepathfile);	

		char * Configure_FileDir = (char *) malloc(strlen(GetConfigFileDir())+1);
		sprintf(Configure_FileDir,"%s",GetConfigFileDir());
		
		readpath_->Start(ndt_FILEPATH,gph_FILEPATH,osr_FILEPATH,osp_FILEPATH,Configure_FileDir);
		
		free(ndt_FILEPATH);
		free(osr_FILEPATH);
   	        free(gph_FILEPATH);
		free(osp_FILEPATH);
                free(Configure_FileDir);
														
	}
	
	Initial_AWA();

	if (choose_wave > 0)
		WavelenConversion =0; 
	return(1);  
}
