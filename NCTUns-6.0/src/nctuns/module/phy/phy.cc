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
#include <assert.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <phy/phy.h>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <con_list.h>
#include <phyInfo.h>
#include <nctuns-dep/link.h>
#include <packet.h>
#include <mbinder.h>

#include <sys/ioctl.h>
#include <if_tun.h>

extern RegTable                 RegTable_;
extern SLIST_HEAD(headOfLink, con_list)        headOfWire_;

MODULE_GENERATOR(phy);

phy::phy(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	 r_flowctl = DISABLED;
	s_flowctl = DISABLED;  
	
	bw_Mbps = 10;
	PropDelay_micro = 0;
	PropDelay_tick = 10; // 10 ticks by default
	BER = 0.0;
	LinkFailFlag = 0;

	/* bind variable */
	vBind("Bw", &bw_Mbps); 
	vBind("BER", &BER); 
	vBind("PropDelay", &PropDelay_micro); 

	vBind("linkfail", &_linkfail);
	vBind("linkfailFileName", &linkfailFileName);
	REG_VAR("BW", &bw_);    
}

phy::~phy() {


}



u_int64_t phy::Tx_Time(ePacket_ *pkt) {

	double			time, dt;
 	u_int64_t		retime;
 	Packet  		*p;


	p = (Packet *)pkt->DataInfo_;
        time = ((p->pkt_getlen()*8.0)*(1000000/bw_)) * (1000.0/TICK);
	retime = (u_int64_t)time;

	dt = (time - retime) * 10.0;
        if(dt >= 1)     retime++;

	return(retime); 
}




int phy::recv(ePacket_ *pkt) {
	Packet			*p;	
	struct phyInfo		*phyinfo;


	assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	if ( LinkFailFlag > 0 ) {
		freePacket(pkt);
		return(1);
	}
	/* set packet bit error rate */
	phyinfo = (struct phyInfo *)p->pkt_getinfo("PHY");
	assert(phyinfo);
	if( BitError(phyinfo->BER, p->pkt_getlen()) == 1 )
		p->pkt_err_ = 1;	

	return(put(pkt, recvtarget_)); 
}
  

int phy::send(ePacket_ *pkt) {

	struct con_list         *cl;
	Packet			*p;
	struct phyInfo		*phyinfo;


	assert(pkt&&(p=(Packet *)pkt->DataInfo_));
	if ( LinkFailFlag > 0 ) {
		freePacket(pkt);
		return(1);
	}

	p->pkt_setflow(PF_RECV);
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(phy, get);


	phyinfo = (struct phyInfo *)malloc(sizeof(struct phyInfo));
	assert(phyinfo);
	phyinfo->TxBW = bw_;
	phyinfo->BER = BER;

	p->pkt_addinfo("PHY", (const char *)phyinfo, sizeof(struct phyInfo));
	free(phyinfo);

        SLIST_FOREACH(cl, &headOfWire_, nextLoc) {
		        char            mark = 0;
	                struct plist    *clp = cl->obj->get_portls();
	                struct plist    *obp = get_portls();

	                while(clp && obp){
	                        if(clp->pid != obp->pid){
	                                mark = 1;
	                                break;
        	                }
		                else{
			                clp = clp->next;
	                                obp = obp->next;
	                        }
	                }
	                if(clp || obp)
	                        mark = 1;
			
	                if ((cl->obj->get_nid() == get_nid())&& !mark){
	                        /* simulate propagation delay here */
	                        setObjEvent(pkt, GetCurrentTime() + PropDelay_tick, 0,
                                cl->obj->sendtarget_->bindModule(), type,
                                pkt->DataInfo_);
	                        return(1);
	                }
	}
	return(-1);
}


int phy::init() {

	double *test;
	NslObject::init();


	/* set random seed */

	/* set bandwidth */
	bw_ = bw_Mbps * 1000000.0;
	test = GET_REG_VAR1(get_portls(), "BW", double*);
	//printf(" %s port %d bw: %10.3f pointer : %d\n", get_name(), get_port(), bw_, &bw_);
	//printf(" %s port %d bw: %10.3f pointer : %d\n", get_name(), get_port(), *test, test);
	/* set propagation delay */
	if(PropDelay_micro != 0) {
		 MICRO_TO_TICK(PropDelay_tick, PropDelay_micro);
	}

	char		line[128];

	if( _linkfail && !strcmp(_linkfail, "on") ) {
	
		tunfd_ = GET_REG_VAR1(get_portls(), "TUNFD", int *);
		/*
		 * if tunfd_ equal to NULL, that means
		 * this is a layer-1 or layer-2 device.
		 * NO TYNFD is supposed to be got.
		 */

		char *FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
						strlen(linkfailFileName) + 1);
		sprintf(FILEPATH,"%s%s", GetConfigFileDir(), linkfailFileName);

		linkfailFile = fopen(FILEPATH,"r");
		free(FILEPATH);

		if( linkfailFile == NULL ) {
			printf("Warning : Can't read file %s\n", FILEPATH);
		}
		else {
			double		StartTime, StopTime;
			Event_		*start_ep;
			Event_		*stop_ep;
			u_int64_t	StartTimeTick, StopTimeTick;
			BASE_OBJTYPE(typeStart);
			BASE_OBJTYPE(typeStop);

			typeStart = POINTER_TO_MEMBER(phy, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(phy, TurnOffLinkFailFlag);

			while( !feof(linkfailFile) ) {
				line[0] = '\0';
				fgets(line, 127, linkfailFile);
				if ((line[0]=='\0')||(line[0]=='#'))
					continue;
				if ( 2 == sscanf(line, "%lf %lf",
					&StartTime, &StopTime) ) {

					if( StartTime >= StopTime )
						continue;
					/* handle start evnet */
					SEC_TO_TICK(StartTimeTick, StartTime);
					start_ep =  createEvent();
					setObjEvent(start_ep,
						    StartTimeTick,
						    0,this,typeStart,
						    (void *)NULL);

					/* handle stop event */
					SEC_TO_TICK(StopTimeTick, StopTime);
					stop_ep =  createEvent();
					setObjEvent(stop_ep,
						    StopTimeTick,
						    0,this,typeStop,
						    (void *)NULL);
				}
			}
			fclose(linkfailFile);
		}
	}

	return(1);  
}

int phy::BitError(double BER_, int plen) {
	double	x,PER;

	if (BER_ == 0.0)
		return(0);	// no bit error
	else {

		PER = plen * 8 * BER_; /* PER = 1-(1-BER)^n */
		if(PER >= 1.0)
			return(1);

		PER *= 1e9;

		x = random() % (1000000000);
  		if (x < PER)
			return(1);	// bit error
		else 
			return(0);	// no bit error  
	}

}

int phy::Debugger() {

	NslObject::Debugger();
	printf("   Bandwidth: %13.3lf\n", bw_);
	return(1);  
}

void phy::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag ++;
}

void phy::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag --;
}



