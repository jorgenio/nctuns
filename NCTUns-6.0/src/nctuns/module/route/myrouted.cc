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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <nctuns_api.h>
#include <ethernet.h>
#include <ip.h>
#include <packet.h>
#include <route/myrouted.h>
#include <exportStr.h>


MODULE_GENERATOR(myRouted);


myRouted::myRouted(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	s_flowctl = DISABLED;
	r_flowctl = DISABLED; 

	rt.slh_first = 0;  

}


myRouted::~myRouted() {

}


int myRouted::init() {

	/* export variable */
 	EXPORT("Routing-Table", E_RONLY);

	/* get my ip address */
	mip = GET_REG_VAR(get_port(), "IP", u_long *);  
	return(1); 
}


int myRouted::send(ePacket_ *pkt) {

	Packet			*p; 
	u_long			dst;

	GET_PKT(p, pkt); 

	/* get destination ip address. */
  	IP_DST(dst, p->pkt_sget()); 

	/* check to see if the broadcast address */
	if (is_ipv4_broadcast(get_nid(), dst))
		return(put(pkt, sendtarget_));

	/* do routing */ 
	if (routing(dst, p) < 0) {
		freePacket(pkt);
		//printf("myRoutd::No routing entry found...\n");
		return(1);
	} 
 
	return(put(pkt, sendtarget_));    
}


int myRouted::recv(ePacket_ *pkt) {

	Packet			*p;
	u_long			dst;


	assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	/* get destination ip address */
	IP_DST(dst, p->pkt_sget()); 

	/* filter destination address */
	if((*mip == dst)||is_ipv4_broadcast(get_nid(), dst)) {
		/* if the destination == myip, pass
		 * to upper layer
		 */
		return(put(pkt, recvtarget_));   
	}
	else {	/* otherwise, we should modify the flow 
		 * direction and do routing.
		 */
		p->pkt_setflow(PF_SEND); 
  		if (routing(dst, p) < 0) {
			freePacket(pkt);
			return(1); 
		}

		/* hwchu:
		 *   There is no chance for this packet to enter the kernel, 
		 *   so we decrement its TTL here.
		 */
		u_char ttl;

		GET_IP_TTL(ttl, p->pkt_sget());
		if (ttl <= 1) {
			return put(pkt, recvtarget_);
		}
		IP_DEC_TTL(p->pkt_sget());

		return(put(pkt, sendtarget_));  
	}
}


int myRouted::routing(u_long dst, Packet *pkt) {

	struct rtentry		*r;

	SLIST_FOREACH(r, &rt, nextrt) {
		if(r->dst == dst) {
			pkt->rt_setgw(r->nexthop); 
   			return(1); 
		}  
	}
	return(-1); 
}


int myRouted::update_rtbl(u_long dstip, u_long nxtip) {

	struct rtentry		*rtn;


	/*
	 * Check to see if the routing entry exist
	 * or not.
	 */
	SLIST_FOREACH(rtn, &rt, nextrt) {
		if (rtn->dst == dstip) {
  			rtn->nexthop = nxtip;
			return(1);
 		}
	}

	/*
	 * Otherwise, we have to add a new entry.
	 */
 	rtn = (struct rtentry *) malloc(sizeof(struct rtentry));
 	assert(rtn);
 	
	rtn->dst = dstip;
  	rtn->nexthop = nxtip;
  	SLIST_INSERT_HEAD(&rt, rtn, nextrt);
 	return(1); 
}


int myRouted::command(int argc, const char *argv[]) {

	char			tmpBuf[100];
	struct ExportStr	*ExpStr; 
	u_int32_t		row,column;
	struct rtentry		*r;
	u_char			*st,*gt;

	if (!strcmp(argv[0], "Get")&&(argc==2)) {
		if (!strcmp(argv[1], "Routing-Table")) {
	
			ExpStr = new ExportStr(2);
	
			sprintf(tmpBuf, "Routing Table:");
 			ExpStr->Insert_comment(tmpBuf);
			sprintf(tmpBuf,"Dst <---> Gateway\n");
			ExpStr->Insert_comment(tmpBuf);
			sprintf(tmpBuf,"=================\n");
			ExpStr->Insert_comment(tmpBuf);

 			SLIST_FOREACH(r, &rt, nextrt) {
				st = (u_char *)&(r->dst);
 				gt = (u_char *)&(r->nexthop);  

				row = ExpStr->Add_row();
				column = 1;

				sprintf(tmpBuf,"%d.%d.%d.%d",
					st[0], st[1], st[2], st[3]);
				ExpStr->Insert_cell(row,column++,
					tmpBuf, " <---> ");

				sprintf(tmpBuf,"%d.%d.%d.%d",
					gt[0], gt[1], gt[2], gt[3]);
	
				ExpStr->Insert_cell(row,column++,
					tmpBuf, "\n");
			}
			EXPORT_GET_SUCCESS(ExpStr);
			return(1); 
		}
	} 

	return(NslObject::command(argc, argv));  
}


int myRouted::Debugger() {

	struct rtentry		*r; 
	char			ip[20];
	u_char			*dt, *nt;

 
	printf("DEBUG: myRouted\n");              
	NslObject::Debugger(); 
	ipv4addr_to_str(*mip, ip); 
	printf("   ip: %s\n", ip);
	printf("\n=========Routing Table(%d)========\n", get_nid());

	SLIST_FOREACH(r, &rt, nextrt) {
		dt = (u_char *)&r->dst;
		nt = (u_char *)&r->nexthop;

		if (r->nexthop == 999999) {
			printf("dst: %d.%d.%d.%d  next-hop: %lu\n",
			dt[0], dt[1], dt[2], dt[3], r->nexthop);  
			continue; 
		}  
		printf("dst: %d.%d.%d.%d  nex-thop: %d.%d.%d.%d\n",	
		dt[0], dt[1], dt[2], dt[3], nt[0], nt[1], nt[2], nt[3]); 
	}
	return(1);  
}


  
