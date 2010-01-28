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

/* 
 *    Ad hoc On-Demand Distance Vector (AODV) Routing
 *    reference: draft-ietf-manet-aodv-12.txt
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <assert.h>
#include <object.h>
#include <event.h>
#include <regcom.h>
#include <scheduler.h>
#include <nodetype.h>
#include <timer.h>
#include <nctuns_api.h>
#include <ethernet.h>
#include <ip.h>
#include <random.h>
#include <packet.h>
#include <route/aodv/AODV.h>
#include <mbinder.h>

using namespace AODVd; 

#define LINK_LAYER_RETRY
/* #define LINK_LAYER_DROP */

extern	RegTable	RegTable_;
extern	typeTable	*typeTable_;
extern	scheduler	*scheduler_;




MODULE_GENERATOR(AODV);

AODV::AODV(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : NslObject(type, id, pl, name)
{
        s_flowctl = DISABLED;
        r_flowctl = DISABLED;
    
	rreq_id = 0; /* ? */
	link_fail_list.slh_first = 0;
        bcache.slh_first = 0; 

	acc_rreq = 0;
	acc_rerr = 0;

	qcur_ =0;
	rd_head = rd_tail = 0;
	qmax_ = 5;
        
	/* bind input file name */
	vBind("HELLO_INTERVAL",       &HELLO_INTERVAL);
        vBind("ALLOWED_HELLO_LOSS",   &ALLOWED_HELLO_LOSS); 
	vBind("ACTIVE_ROUTE_TIMEOUT", &ACTIVE_ROUTE_TIMEOUT);
	vBind("DELETE_PERIOD",        &DELETE_PERIOD); 
	vBind("NET_DIAMETER",         &NET_DIAMETER); 
	vBind("NODE_TRAVERSAL_TIME",  &NODE_TRAVERSAL_TIME); 
	vBind("RREQ_RETRIES",         &RREQ_RETRIES); 
	vBind("RREQ_RATELIMIT",       &RREQ_RATELIMIT); 
	vBind("RERR_RATELIMIT",       &RERR_RATELIMIT); 
}

AODV::~AODV() {
}

int AODV::init() {

	mip = GET_REG_VAR(get_port(), "IP", u_long *);

	Rt_entry *r = new Rt_entry;

	// the first entry is for its own
	r->rt_dst     = *mip;
	r->rt_nexthop = *mip;
	r->rt_valid_dst_seqno = true;
	r->rt_seqno   = 1;
	r->rt_hopcount= 0;
	r->rt_flags   = RTF_VALID;
	r->rt_time    = INFINITY_LIFETIME;

	rtable.insert(r);
      
	MILLI_TO_TICK(hello_interval_,  (u_int64_t)HELLO_INTERVAL);
	MILLI_TO_TICK(active_route_timeout_, (u_int64_t)ACTIVE_ROUTE_TIMEOUT);
	MILLI_TO_TICK(node_traversal_time_, (u_int64_t)NODE_TRAVERSAL_TIME);
	MILLI_TO_TICK(delete_period_, (u_int64_t)DELETE_PERIOD);

	MY_ROUTE_TIMEOUT = 2 * ACTIVE_ROUTE_TIMEOUT;
	MILLI_TO_TICK(my_route_timeout_, (u_int64_t)MY_ROUTE_TIMEOUT);
	NET_TRAVERSAL_TIME = (3 * NODE_TRAVERSAL_TIME * NET_DIAMETER /2);
	MILLI_TO_TICK(net_traversal_time_, (u_int64_t)NET_TRAVERSAL_TIME);
	PATH_DISCOVERY_TIME = (2 * NET_TRAVERSAL_TIME);
	MILLI_TO_TICK(path_discovery_time_,  (u_int64_t)PATH_DISCOVERY_TIME); 

	MILLI_TO_TICK(route_check_timer_,  (u_int64_t)ROUTE_CHECK);
	MILLI_TO_TICK(rreq_check_timer_,  (u_int64_t)PENDING_RREQ_CHECK);
	MILLI_TO_TICK(recent_rreq_list_timer_,  (u_int64_t)RECENT_RREQ_LIST_CHECK);
	MILLI_TO_TICK(accumulated_rreq_rerr_timer_,  (u_int64_t)ACCUMULATED_RREQ_RERR_TIMER);
	MILLI_TO_TICK(nei_list_check_timer_,  (u_int64_t)NEI_LIST_CHECK);
	MILLI_TO_TICK(sendhello_timer_,  (u_int64_t)SENDHELLO_TIMER);
	MILLI_TO_TICK(link_fail_list_check_timer_,  (u_int64_t)LINK_FAIL_LIST_CHECK);
	/* MILLI_TO_TICK(printLoc_timer_,  (u_int64_t)5000); */

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(AODV, sendHello);
	SendHello_timer.setCallOutObj(this, type);
	SendHello_timer.start((sendhello_timer_ + Random()%100000), 0); 
	 
	BASE_OBJTYPE(type_r);
	type_r = POINTER_TO_MEMBER(AODV, RTTimer);
	RT_timer.setCallOutObj(this, type_r);
	RT_timer.start(route_check_timer_, 0);
	
	BASE_OBJTYPE(type_rreq);
	type_rreq = POINTER_TO_MEMBER(AODV, RREQ_retry);
	SendRREQ_timer.setCallOutObj(this, type_rreq);
	SendRREQ_timer.start(rreq_check_timer_, 0);
	
	BASE_OBJTYPE(type_recent_rreq);
	type_recent_rreq = POINTER_TO_MEMBER(AODV, CheckRecentRREQ);
	RecentRREQ_timer.setCallOutObj(this, type_recent_rreq);
	RecentRREQ_timer.start(recent_rreq_list_timer_, 0);

	BASE_OBJTYPE(type_acc_rreq_rerr);
	type_acc_rreq_rerr = POINTER_TO_MEMBER(AODV, ClearAccRREQ_RERR);
	AccRREQ_RERR_timer.setCallOutObj(this, type_acc_rreq_rerr);
	AccRREQ_RERR_timer.start(accumulated_rreq_rerr_timer_, 0);


	/*
	 * If an AODV network has greedy traffic flows, the hello
	 * packets will frequently collide with data packets and
	 * not be received successfully. This will casue the 
	 * CheckNeiList function to frequently detect the neighbor-
	 * losing event so that many RRER packets are sent out. 
	 * The uncessary RRER packets may seriously reduce the 
	 * AODV network's connectiviy; therefore, we disable this
	 * function here.
	 */
	/*
	BASE_OBJTYPE(type_nei_list);
	type_nei_list = POINTER_TO_MEMBER(AODV, CheckNeiList);
	Nei_List_timer.setCallOutObj(this, type_nei_list);
	Nei_List_timer.start(nei_list_check_timer_, 0);
	*/

	BASE_OBJTYPE(type_link_fail_list);
	type_link_fail_list = POINTER_TO_MEMBER(AODV, CheckLinkFailList);
	RecentRREQ_timer.setCallOutObj(this, type_link_fail_list);
	RecentRREQ_timer.start(link_fail_list_check_timer_, 0);

	/* BASE_OBJTYPE(type_loc);
	type_loc = POINTER_TO_MEMBER(AODV, PrintLoc);
	PrintLoc_timer.setCallOutObj(this, type_loc);
	PrintLoc_timer.start(printLoc_timer_, 0);  */

	return (1);
}

int AODV::RTTimer(){
    
	u_int64_t nowtime = GetCurrentTime();

	// skip the route entry for its own
	Rt_entry *p_pre = rtable.rt_getHead(); 
	Rt_entry *p_rt  = rtable.rt_getHead()->next; 
	Rt_entry *p_tmp;

	while(p_rt) {
		if(p_rt->rt_flags == RTF_VALID) {
			// turn into INVALID state
			if(p_rt->rt_time < nowtime) {
				p_rt->rt_flags = RTF_INVALID;
				p_rt->rt_time = nowtime + delete_period_;

				// if the entry is for a neighbor(hopcount=1),
				// we delete the corresponding entry in the
				// neighborr-list.
				/*if(p_rt->rt_hopcount == 1) {
					nei_list.remove(p_rt->rt_dst);
				}*/

			}

			p_pre = p_rt;
			p_rt  = p_rt->next;

		}else if(p_rt->rt_flags == RTF_INVALID) {
			if(p_rt->rt_time < nowtime) {
				p_tmp = p_rt->next;
				rtable.removeEntry(p_rt, p_pre);
				p_rt  = p_tmp;
			}else {
				p_pre = p_rt;
				p_rt  = p_rt->next;
			}
		}else {
			p_pre = p_rt;
			p_rt  = p_rt->next;
		}
	
		//p_pre = p_rt;
		//p_rt  = p_rt->next;
	}

	BASE_OBJTYPE(type_r);
	type_r = POINTER_TO_MEMBER(AODV, RTTimer);
	RT_timer.setCallOutObj(this, type_r);
	RT_timer.start(route_check_timer_, 0);

	return (1);
}

int AODV::RREQ_retry(){

	u_int64_t nowtime = GetCurrentTime();
	Ctrl_entry *p_ctrl = ctrl_table.getHead();
	Ctrl_entry *p_ctrl_deleted;

	while(p_ctrl) {
		if(p_ctrl->rreq_lifetime <= nowtime) { 

			if(p_ctrl->rreq_retries < (unsigned int)RREQ_RETRIES) {

				if(acc_rreq <= RREQ_RATELIMIT){
					/* send retry RREQ */
					sendRREQ(p_ctrl->dst_ip, NET_DIAMETER);
					acc_rreq++;
	
					p_ctrl->rreq_retries += 1 ;
	
					p_ctrl->rreq_lifetime = nowtime + net_traversal_time_;
				}
			}else{
				/* free all queued pkts and remove the ctrl entry */
				struct buf_list *b_list = p_ctrl->buffer_list;
				for(; b_list != NULL ; b_list = b_list->next){
					freePacket(b_list->queued_pkt);
				}
	
				p_ctrl_deleted = p_ctrl;
				p_ctrl = p_ctrl->next;
    				ctrl_table.remove(p_ctrl_deleted->dst_ip);
				continue;
			}
		}

		p_ctrl = p_ctrl->next;
	}

	LocalRepair_entry *p_local_rep = local_repair_table.getHead();
	LocalRepair_entry *p_local_rep_deleted;

	// for local repair
	while(p_local_rep) {
		if(p_local_rep->local_repair_lifetime <= nowtime){
		/* local repair fails, free all queued pkts and remove the ctrl entry */

			// turn the flag of corresponding route entry
			// from RTF_BEING_REPAIRED into RTF_INVALID
			Rt_entry *p_rt =rtable.rt_get(p_local_rep->dst_ip);
			if(p_rt) {
				p_rt->rt_flags = RTF_INVALID;

				// shortage of rfc.3561
				// Without the decrease, after local-repair 
				// fails, no AODV-pkt can update this route.
				// Since it is equipped with the highest dst 
				// seqno of the network
				p_rt->rt_seqno--; 
			}

			struct buf_list *b_list = p_local_rep->buffer_list;
			for(; b_list != NULL ; b_list = b_list->next){
				freePacket(b_list->queued_pkt);
			}
	

			// for the link-layer upcall, we generate
			// the unreach-list, then send RERR
			if(p_local_rep->brokenlink_node != NO_USE) {
				Unreach_list *unr_list = new Unreach_list();

				Rt_entry *p_rt = rtable.rt_getHead();
				while(p_rt) {
					if(p_rt->rt_nexthop == p_local_rep->brokenlink_node) 
					{
						if(p_rt->rt_flags == RTF_VALID){
							p_rt->rt_seqno++;
							p_rt->rt_flags = RTF_INVALID;
							p_rt->rt_time  = nowtime + delete_period_;
						}
						unr_list->insert(p_rt->rt_dst, p_rt->rt_seqno);
					}
			
					p_rt = p_rt->next;
				}

				if(acc_rerr < RERR_RATELIMIT) {
 	 				bcastRERR(unr_list); 
	 				acc_rerr++;
				}
				
				delete unr_list;
			}

			p_local_rep_deleted = p_local_rep;
			p_local_rep = p_local_rep->next;
    			local_repair_table.remove(p_local_rep_deleted->dst_ip);
			continue;
		}
		p_local_rep = p_local_rep->next;
	}

	BASE_OBJTYPE(type_rreq);
	type_rreq = POINTER_TO_MEMBER(AODV, RREQ_retry);
	SendRREQ_timer.setCallOutObj(this, type_rreq);
	SendRREQ_timer.start(rreq_check_timer_, 0);

	return (1);
}
/* end of revise */

int AODV::CheckRecentRREQ() {
	BroadcastID *b;
	u_int64_t    now = GetCurrentTime();

	SLIST_FOREACH(b, &bcache, nexB){
		if(b->lifetime < now)
			SLIST_REMOVE(&bcache, b, BroadcastID, nexB);
	}


	BASE_OBJTYPE(type_recent_rreq);
	type_recent_rreq = POINTER_TO_MEMBER(AODV, CheckRecentRREQ);
	RecentRREQ_timer.setCallOutObj(this, type_recent_rreq);
	RecentRREQ_timer.start(recent_rreq_list_timer_, 0);

	return (1);
}

// clear the accumulated count for RRER/RERR to 0 every second
int AODV::ClearAccRREQ_RERR() {
	acc_rreq = 0;
	acc_rerr = 0;

	BASE_OBJTYPE(type_acc_rreq_rerr);
	type_acc_rreq_rerr = POINTER_TO_MEMBER(AODV, ClearAccRREQ_RERR);
	AccRREQ_RERR_timer.setCallOutObj(this, type_acc_rreq_rerr);
	AccRREQ_RERR_timer.start(accumulated_rreq_rerr_timer_, 0);

	return (1);
}

// regularly check link connection of the neighbors
int AODV::CheckNeiList() {
	Nei_entry *p_nei   = nei_list.getHead();
	Nei_entry *p_nei_deleted;
	u_int64_t  nowtime = GetCurrentTime();


	while(p_nei) {
		// no Hello is received for ALLOWED_HELLO_LOSS*HELLO_INTERVAL
		if(p_nei->nei_time < nowtime) {
			Unreach_list *unr_list = new Unreach_list();

			Rt_entry *p_rt_nei = rtable.rt_get(p_nei->nei_addr);
			if(p_rt_nei) {

				/* if(p_rt_nei->rt_valid_dst_seqno == true)
					p_rt_nei->rt_seqno++; */
				// invalidate route entry for the lost neighbor
				p_rt_nei->rt_flags = RTF_INVALID;
				p_rt_nei->rt_time  = nowtime + delete_period_;
			}

			Rt_entry *p_rt = rtable.rt_getHead();
			// insert those route entries which use the lost 
			// neighbor as next hop into unreach-list
			while(p_rt) { 
				if(p_rt->rt_nexthop == p_nei->nei_addr) {
					unr_list->insert(p_rt->rt_dst, p_rt->rt_seqno);
					// link is broken, mark RTF_REPAIRABLE
					p_rt->rt_flags = RTF_REPAIRABLE;
					p_rt->rt_time  = GetCurrentTime() + delete_period_;
				}

				p_rt = p_rt->next;
			}
			if(acc_rerr <= RERR_RATELIMIT) {
				bcastRERR(unr_list);
				acc_rerr++;
			}

			delete unr_list;

			p_nei_deleted = p_nei;
			p_nei = p_nei->next;
			nei_list.remove(p_nei_deleted->nei_addr);

			/* 
			 * The local repair fuction is not provided yet.
			 */
			// local repair
			//sendRREQ();
			continue;
		}
		p_nei = p_nei->next;
	}

	BASE_OBJTYPE(type_nei_list);
	type_nei_list = POINTER_TO_MEMBER(AODV, CheckNeiList);
	Nei_List_timer.setCallOutObj(this, type_nei_list);
	Nei_List_timer.start(nei_list_check_timer_, 0);

	return 0;
}

int AODV::CheckLinkFailList() {
	struct Link_fail_entry *l;
	u_int64_t    now = GetCurrentTime();

	SLIST_FOREACH(l, &link_fail_list, next){
		if(l->lifetime < now)
			SLIST_REMOVE(&link_fail_list, l, Link_fail_entry, next);
	}

	BASE_OBJTYPE(type_link_fail_list);
	type_link_fail_list = POINTER_TO_MEMBER(AODV, CheckLinkFailList);
	RecentRREQ_timer.setCallOutObj(this, type_link_fail_list);
	RecentRREQ_timer.start(link_fail_list_check_timer_, 0);

	return 0;
}

int
AODV::recv(ePacket_ *pkt) {

	u_long dst_ip, src_ip;

	Packet *p;
	struct AODV_packet *my_pkt;
	Rt_entry *dst_route;
	/* int lastseqno; */
	 
	assert(pkt&&(p=(Packet *)pkt->DataInfo_));
	GET_PKT(p, pkt);
	 
	char pkttype[5];
	strncpy(pkttype, p->pkt_get(), 4);
	pkttype[4]='\0';
	 
	my_pkt = (struct AODV_packet *)p->pkt_get(); 
	

	if(strcmp(pkttype,"AODV") == 0){
        	dst_ip = my_pkt->dst_ip;
		src_ip = my_pkt->src_ip;
	}
	else {
		IP_DST(dst_ip, p->pkt_sget());
		IP_SRC(src_ip, p->pkt_sget());

	}
	 
	/*  Receive normal packet, we must help it forward to 
	*   next node ,or if it is my packet, I pass it to interface layer.
	*/
	if(bcmp(my_pkt->pro_type, "AODV", 4) != 0 ) {

		/* hwchu: moved to below
		struct ip *iphdr;
        	iphdr = (struct ip *)p->pkt_sget();
        	--iphdr->ip_ttl;
        	if (iphdr->ip_ttl == 0) {
		     	return (put(pkt, recvtarget_));
		}
		*/

		// use the src_ip of the normal pkt as index,
		// to update the lifetime of corresponding route entry
		/* Rt_entry *p_rt_src = rtable.rt_get(src_ip);
		p_rt_src->rt_flags = RTF_VALID;
		p_rt_src->rt_time  = GetCurrentTime() + active_route_timeout_; */

        	/*
         	*   if I am dst or it's a broadcast pkt, pass to upper layer
	 	*/
        	if((dst_ip == *mip) || is_ipv4_broadcast(get_nid(), dst_ip)){
			return (put(pkt, recvtarget_));
		} else {
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
		}

		int lookup_result = rtable.rt_lookup(dst_ip);
        	if(lookup_result == RTF_VALID){
			dst_route = rtable.rt_get(dst_ip); 

			// each time the route is used for forwarding, 
			// update its lifetime
			dst_route->rt_time = GetCurrentTime() + active_route_timeout_;

			p->rt_setgw(dst_route->rt_nexthop);
			p->pkt_setflow(PF_SEND);
 
			return (sendToQueue(pkt));
		}
              /* else {
       			if (ctrl_table.ifExist(dst_ip) < 0) {
                		ctrl_table.addEntry(dst_ip, 0);
       			}		
       			ctrl_table.attachPKT(dst_ip,pkt);

       			sendRERR(dst_ip); 
			return(1);
	      } */
        	else if(lookup_result == RTF_INVALID){

			Unreach_list *unr_list = new Unreach_list();
			Rt_entry *rt_unreach   = rtable.rt_get(dst_ip);

			unr_list->insert(dst_ip, rt_unreach->rt_seqno);

			Nei_entry *p_nei = rt_unreach->rt_preclist->getHead();
			// sendRERR to each precursors of rt_entry for 
			// the dst_ip of the received data pkt.
			while(p_nei) {
				if(acc_rerr <= RERR_RATELIMIT) {
					sendRERR(p_nei->nei_addr, unr_list);
					acc_rerr++;
				}

				p_nei = p_nei->next;
			}

			delete unr_list;

		}else if(lookup_result == RTF_REPAIRABLE){
			dst_route = rtable.rt_get(dst_ip); 

                	if (!local_repair_table.ifExist(dst_ip)) {

		 		local_repair_table.insert(dst_ip, GetCurrentTime()+net_traversal_time_, NO_USE);
		 		if(local_repair_table.attachPkt(dst_ip, pkt) < 0){
					freePacket(pkt);
					return 1;
				}

				if(acc_rreq <= RREQ_RATELIMIT) {
					// local repair
					dst_route->rt_seqno++;
                 			sendRREQ(dst_ip, dst_route->rt_hopcount + TTL_THRESHOLD);
					acc_rreq++;

					dst_route->rt_flags = RTF_BEING_REPAIRED;
				}
				/* cclin: need to be examined to see if 
				 * we should free the packet before this function
				 * returns.
				 */
				freePacket(pkt);
		 		return (1);

                	}else{
		       		if(local_repair_table.attachPkt(dst_ip, pkt) < 0)
					freePacket(pkt);
		       		return (1);
			}
		}else if(lookup_result == RTF_BEING_REPAIRED){

			// These pkts being received will be queued, 
			// may be dropped after repairing timeout.
                	if (!local_repair_table.ifExist(dst_ip)) {
		 		local_repair_table.insert(dst_ip, GetCurrentTime()+net_traversal_time_, NO_USE);
		 		if(local_repair_table.attachPkt(dst_ip, pkt) < 0)
					freePacket(pkt);

		 		return (1);
                	}else{
		       		if(local_repair_table.attachPkt(dst_ip, pkt) < 0)
					freePacket(pkt);
		       		return (1);
			}

		}else if(lookup_result == RT_NOT_EXIST){
		}

		/* cclin: need to be examined to see if 
		 * we should free the packet before this function
		 * returns.
		 */
		freePacket(pkt);
		return (1);
	}


	if(bcmp(my_pkt->pro_type, "AODV_RREQ", 10) == 0){

   		struct RREQ_msg  *my_rreq;
		
   		my_rreq = (struct RREQ_msg *)((char *)my_pkt+sizeof(AODV_packet));

		// create/update immediate reverse route to previous hop
		updateSimpleRRoute(src_ip);

		// not entirely conform to the spec ><
		if(my_rreq->rreq_src_addr == *mip) {
			freePacket(pkt);
			return (1);
		}
	
	 	// We must update the broadcast id cache.
		bool found_in_bcache = false;
		BroadcastID *b;
		SLIST_FOREACH(b, &bcache, nexB){
			if(b->addr == my_rreq->rreq_src_addr) {

				found_in_bcache = true;

				if(b->bid > my_rreq->rreq_id) { 
					freePacket(pkt); // free duplicate RREQ 
					return (1);
				}else if(b->bid == my_rreq->rreq_id) {
					// Let lower-hopcnt RREQ pass through
					// to achieve shortest-path routing.
					if(b->hopcnt <= my_rreq->rreq_hopcount){
						freePacket(pkt);
						return (1);
					}else
						b->hopcnt = my_rreq->rreq_hopcount;
				}else {
					b->bid = my_rreq->rreq_id;
				}
			}
		}

		if(!found_in_bcache) {
        		BroadcastID *b = new struct BroadcastID;

			b->addr = my_rreq->rreq_src_addr;
			b->bid = my_rreq->rreq_id;
			b->hopcnt = my_rreq->rreq_hopcount;
			b->lifetime = GetCurrentTime() + path_discovery_time_; 
			SLIST_INSERT_HEAD(&bcache, b, nexB);
		}


		my_rreq->rreq_hopcount++;

		u_int64_t minimalLifetime = GetCurrentTime() + 2*net_traversal_time_ - 2*(my_rreq->rreq_hopcount)*node_traversal_time_;

		// add route to RREQ originator(ie. reverse route )
		if((rtable.rt_lookup(my_rreq->rreq_src_addr)) == RT_NOT_EXIST) {
			Rt_entry *r = new Rt_entry;

			r->rt_dst             = my_rreq->rreq_src_addr;
			r->rt_nexthop         = src_ip;
			r->rt_valid_dst_seqno = true;
			r->rt_seqno           = my_rreq->rreq_src_seqno;
			r->rt_hopcount        = my_rreq->rreq_hopcount;
			r->rt_flags           = RTF_VALID;
			r->rt_time            = minimalLifetime;

			rtable.insert(r);
		}
		else {
			Rt_entry *rt1 = rtable.rt_get(my_rreq->rreq_src_addr);
			/* if((rt1.rt_hopcount > my_rreq->rreq_hopcount)||
		 	(rt1.rt_seqno < my_rreq->rreq_src_seqno)) {
				updateRT(src_ip,my_rreq,lastip, lastseqno);
			} */

			rt1->rt_nexthop         = src_ip;
			if(rt1->rt_valid_dst_seqno)
				rt1->rt_seqno = (my_rreq->rreq_src_seqno > rt1->rt_seqno) ? my_rreq->rreq_src_seqno : rt1->rt_seqno;
			else
				rt1->rt_seqno = my_rreq->rreq_src_seqno;

			rt1->rt_valid_dst_seqno = true;

			rt1->rt_hopcount        = my_rreq->rreq_hopcount;
			rt1->rt_flags           = RTF_VALID; // ?
			rt1->rt_time            = (minimalLifetime > rt1->rt_time) ? minimalLifetime : rt1->rt_time;
		}

        	processBuffered(my_rreq->rreq_src_addr);
        	processBuffered(src_ip);

		// If I am the dst, I reply RREP.
		if(my_rreq->rreq_dst_addr == *mip){
			if(!(my_rreq->U)) {
				// receive RREQ from local repairing node
				if(my_rreq->rreq_dst_seqno == (rtable.myOwnSeqno() + 1))
					rtable.incMyOwnSeqno();
			}

			sendRREP(my_rreq->rreq_dst_addr, my_rreq->rreq_src_addr, src_ip, 0, rtable.myOwnSeqno(), my_route_timeout_);

			freePacket(pkt);

			return (1);
		}
		// If intermediate node has "fresh enough" route, it may
		// reply RREP.
		else if((rtable.rt_lookup(my_rreq->rreq_dst_addr) == RTF_VALID)
		       && (rtable.rt_get(my_rreq->rreq_dst_addr)->rt_valid_dst_seqno)
		       && (rtable.rt_get(my_rreq->rreq_dst_addr)->rt_seqno >= my_rreq->rreq_dst_seqno)
		       && (!(my_rreq->D)))
		{
			Rt_entry *rt_d = rtable.rt_get(my_rreq->rreq_dst_addr);
			Rt_entry *rt_s = rtable.rt_get(my_rreq->rreq_src_addr);


			rtable.updatePrecursorList(my_rreq->rreq_dst_addr, src_ip);
			rtable.updatePrecursorList(my_rreq->rreq_src_addr, rt_d->rt_nexthop);
			sendRREP(my_rreq->rreq_dst_addr, my_rreq->rreq_src_addr, src_ip, rt_d->rt_hopcount, rt_d->rt_seqno, (rt_d->rt_time - GetCurrentTime()));

			// gratuitous RREP to the dst
			//if(my_rreq->G & 0x1)
			if(my_rreq->G) {
				sendRREP(my_rreq->rreq_src_addr, my_rreq->rreq_dst_addr, rt_d->rt_nexthop, rt_s->rt_hopcount, my_rreq->rreq_src_seqno, (rt_s->rt_time - GetCurrentTime()));

			}

			freePacket(pkt);

			return (1);
		}
		else {
			if(--my_pkt->ttl != 0)
        			forwardRREQ(my_rreq, my_pkt->ttl);

			freePacket(pkt); 

			return (1); 
		}
      }
      else if(bcmp(my_pkt->pro_type, "AODV_RREP", 10) == 0){
       
		struct RREP_msg * my_rrep;
		my_rrep = (struct RREP_msg *)((char *)my_pkt+sizeof(AODV_packet));

		// Hello message!!!
		if(dst_ip == inet_addr("1.0.255.255")) {
			int lookup_result = rtable.rt_lookup(my_rrep->rrep_dst_addr);
			if(lookup_result == RT_NOT_EXIST) {
				Rt_entry *r = new Rt_entry;
	
				r->rt_dst             = my_rrep->rrep_dst_addr;
				r->rt_nexthop         = src_ip;
				r->rt_valid_dst_seqno = true;
				r->rt_seqno           = my_rrep->rrep_dst_seqno;
				r->rt_hopcount        = my_rrep->rrep_hopcount + 1;
				r->rt_flags           = RTF_VALID;
				r->rt_time            = GetCurrentTime() + my_rrep->rrep_lifetime;

				// insert new entry
				rtable.insert(r);
			}
			else {
				Rt_entry *r = rtable.rt_get(my_rrep->rrep_dst_addr);
				if(!r)
					printf("AODV error.\n");

				r->rt_nexthop         = src_ip;
				r->rt_valid_dst_seqno = true;
				r->rt_seqno           = my_rrep->rrep_dst_seqno;
				r->rt_hopcount        = my_rrep->rrep_hopcount + 1; // actually 1
				r->rt_flags           = RTF_VALID;
				r->rt_time            = GetCurrentTime() + my_rrep->rrep_lifetime;
			}

			// update the neighbor list
			nei_list.update(my_rrep->rrep_dst_addr, (GetCurrentTime() + (ALLOWED_HELLO_LOSS * hello_interval_)));
			
			/* cclin: need to be examined to see if 
			 * we should free the packet before this function
			 * returns.
			 */

			freePacket(pkt);
			return 0; // Hello msg won't be forwarded.
		} // HELLO pkt

		// create/update immediate reverse route to previous hop
		updateSimpleRRoute(src_ip);


		my_rrep->rrep_hopcount++;

		// add route to RREP-sender(ie. reverse route )
		int lookup_result = rtable.rt_lookup(my_rrep->rrep_dst_addr);
		if(lookup_result == RT_NOT_EXIST) {
			Rt_entry *r = new Rt_entry;

			r->rt_dst             = my_rrep->rrep_dst_addr;
			r->rt_nexthop         = src_ip;
			r->rt_valid_dst_seqno = true;
			r->rt_seqno           = my_rrep->rrep_dst_seqno;
			r->rt_hopcount        = my_rrep->rrep_hopcount;
			r->rt_flags           = RTF_VALID;
			r->rt_time            = GetCurrentTime() + my_rrep->rrep_lifetime;

			// insert new entry
			rtable.insert(r);
		}
		else {
			Rt_entry *r = rtable.rt_get(my_rrep->rrep_dst_addr);
			if(!r)
				printf("AODV error.\n");

			bool need_update = false;

			if(r->rt_valid_dst_seqno == false) {
				need_update = true;
			}
			if((my_rrep->rrep_dst_seqno > r->rt_seqno) && 
			   (r->rt_valid_dst_seqno == true)) {
				need_update = true;
			}
			if(my_rrep->rrep_dst_seqno == r->rt_seqno) {
				if(r->rt_flags != RTF_VALID) // ?
					need_update = true;
				if(my_rrep->rrep_hopcount < r->rt_hopcount)
					need_update = true;
			}

			// update the exist routing 
			if(need_update) {
				r->rt_flags = RTF_VALID;
				r->rt_valid_dst_seqno = true;
				r->rt_nexthop = src_ip;
				r->rt_hopcount = my_rrep->rrep_hopcount;
				r->rt_time = GetCurrentTime() + my_rrep->rrep_lifetime;
				r->rt_seqno = my_rrep->rrep_dst_seqno;
			}
		}
		/* else if(lookup_result == RTF_VALID){
			Rt_entry *r = rtable.rt_get(my_rrep->rrep_dst_addr);

			if(my_rrep->rrep_dst_seqno > r->rt_seqno) {
			}
		}
		else if(lookup_result == RTF_INVALID){
		}
		else if(lookup_result == RTF_REPAIRABLE){
		}
		else if(lookup_result == RTF_BEING_REPAIRED){
		} */

		// process the buffered pkts with the new routes
		processBuffered(my_rrep->rrep_dst_addr);
		processBuffered(src_ip);
	 
		if(my_rrep->rrep_ori_addr == *mip){
			freePacket(pkt);
                   	return (1);
		} else {
			Rt_entry *rt_s = rtable.rt_get(my_rrep->rrep_ori_addr);

			if(rt_s) {
				// only forwarding nodes need to update precursor list
				rtable.updatePrecursorList(my_rrep->rrep_dst_addr, rt_s->rt_nexthop);
				rtable.updatePrecursorList(src_ip, rt_s->rt_nexthop);
				//rtable.updatePrecursorList(my_rrep->rrep_ori_addr, src_ip);

				// rt_s->rt_time = max(existingLT, now + active_route);
				if(rt_s->rt_time < (GetCurrentTime() + active_route_timeout_))
					rt_s->rt_time = GetCurrentTime() + active_route_timeout_;

				if(--my_pkt->ttl != 0)
        				forwardRREP(my_rrep, my_pkt->ttl);
			}

        		freePacket(pkt);
        		return (1);
		}
	}


	else if(bcmp(my_pkt->pro_type, "AODV_RERR", 10) == 0){

		struct RERR_msg  *my_rerr;
		
		my_rerr = (struct RERR_msg *)((char *)my_pkt+sizeof(AODV_packet));
		Unreach_list *unr_list = new Unreach_list();

		struct unreach_tuple *p_tuple = &(my_rerr->unreach_e);
		u_char num_tuple              = my_rerr->destCount;

		// generate my own unreach_list according to the received RERR
		while(num_tuple > 0) {
			Rt_entry *p_rt = rtable.rt_get(p_tuple->unreach_dip);
			if((p_rt != NULL) && (p_rt->rt_nexthop == src_ip)) {
				unr_list->insert(p_tuple->unreach_dip, p_tuple->unreach_dseq);
				// invalidate those route entries which use 
				// the node(sending RERR) as next hop.
				p_rt->rt_flags = RTF_INVALID;
			}

			num_tuple--;
			p_tuple++;
		}

		//sendRERR to every precursors of each entry of the unreach_list
		struct unreach_entry *p_ue = unr_list->getHead();
		while(p_ue) {
			Rt_entry *p_rt = rtable.rt_get(p_ue->unreach_dip);
			if(p_rt != NULL) {
				Nei_entry *p_nei = p_rt->rt_preclist->getHead();
				while(p_nei) {
					if(acc_rerr <= RERR_RATELIMIT) {
						sendRERR(p_nei->nei_addr, unr_list);
						acc_rerr++;
					}
					p_nei = p_nei->next;
				}
			} else {
			}

			p_ue = p_ue->next;
		}

		delete unr_list;

		freePacket(pkt);
		return (1);
      }

      // not RREQ,RREP,RERR packet
      else{
		printf("[%u]: receive AODV_unknown pkt (type:%s) at tick=%llu\n", 
		    get_nid(), my_pkt->pro_type, GetCurrentTime());
		//assert(0);
		return 1;
      }

      freePacket(pkt);
      return (1);

}


int
AODV::send(ePacket_ *pkt) {

       Packet		*p;
       u_long		dst_ip, src_ip;

       assert(pkt&&(p=(Packet *)pkt->DataInfo_));

       GET_PKT(p, pkt);

       IP_DST(dst_ip, p->pkt_sget());
       IP_SRC(src_ip, p->pkt_sget());

       if (is_ipv4_broadcast(get_nid(), dst_ip)){
	       // It's a broadcast pkt. Just send it.
	       sendToQueue(pkt);
	       return 1;
       }

       int lookup_result = rtable.rt_lookup(dst_ip);
       if(lookup_result != RTF_VALID) {
      
                if (!ctrl_table.ifExist(dst_ip)) {

		 	ctrl_table.insert(dst_ip, GetCurrentTime()+net_traversal_time_);
		 	if(ctrl_table.attachPkt(dst_ip, pkt) < 0) {
				freePacket(pkt);
				return (1);
			}

			if(acc_rreq <= RREQ_RATELIMIT) {
                 		sendRREQ(dst_ip, NET_DIAMETER);
				acc_rreq++;
			}
		 	return (1);

                }else{
		       if(ctrl_table.attachPkt(dst_ip, pkt) < 0)
			       freePacket(pkt);
		       return (1);
		}
       }
       else {
            	Rt_entry *rt0 = rtable.rt_get(dst_ip);

		// each time the route is used for transmission, 
		// update its lifetime
		rt0->rt_time = GetCurrentTime() + active_route_timeout_;

		p->rt_setgw(rt0->rt_nexthop);
		  
            	sendToQueue(pkt);
		return (1);
       }
}



int AODV::sendToQueue(ePacket_ *pkt) {

       int		(NslObject::*upcall)(MBinder *);
                        /* Do flow control for myself with s_queue */
       
       if ( sendtarget_->get_curqlen() > 0 ) {
               /* Insert to rd_queue */
	       if (qcur_ < qmax_ ){
	                 if (qcur_ == 0){
			         /* The first ePacket I want to send */
				 rd_head = pkt;
				 rd_tail = pkt;
				 pkt->next_ep = 0;
                         }else{
			         rd_tail->next_ep = pkt;
				 rd_tail = pkt;
			 }/* if (qcur_ == 0) */
			 qcur_++;
	       }else{
	                 /* queue is full, drop the ePacket */
			 freePacket(pkt);
	       }
               return (1);
       }else{
               /* call put() method */
	       upcall = (int (NslObject::*)(MBinder *))&AODV::push;

	       BASE_OBJTYPE(type);
	       type = POINTER_TO_MEMBER(AODV,LinkLayerCall);
	       Packet           *p;
               GET_PKT(p, pkt);
	       p->pkt_setHandler(this,type);
	       sendtarget_->set_upcall(this, upcall);
	       return (put(pkt, sendtarget_));
       }
}


int AODV::push(void) {
       
       ePacket_      *pkt;
       int           (NslObject::*upcall)(MBinder *);
       if (qcur_ > 0){
               pkt = rd_head;
	       rd_head = rd_head->next_ep;
	       qcur_--;

	       upcall = (int (NslObject::*)(MBinder *))&AODV::push;
	       sendtarget_->set_upcall(this, upcall);
	       assert(put(pkt, sendtarget_) > 0);			
       }       
       return (1);
}


int AODV::processBuffered(u_long new_rt_addr) {
	Packet     *p;
	Ctrl_entry *p_c = ctrl_table.getEntry(new_rt_addr);
	struct buf_list *p_buf, *p_pre;
	Rt_entry *new_rt;
	int queuecnt = 0;


	if(p_c) {
		p_buf = p_c->buffer_list;
		if(rtable.rt_lookup(new_rt_addr) == RTF_VALID) {
			new_rt = rtable.rt_get(new_rt_addr);

			while(p_buf) { // FIFO
				GET_PKT(p, p_buf->queued_pkt);
				p->rt_setgw(new_rt->rt_nexthop);
				sendToQueue(p_buf->queued_pkt);
				queuecnt++;

				p_pre = p_buf;
				p_buf = p_buf->next;

				//free(p_pre);
			}
		 } else {
			 return 0;
		 }
	}


	ctrl_table.remove(new_rt_addr);

	LocalRepair_entry *p_local_rep = local_repair_table.getEntry(new_rt_addr);
	p_buf = NULL; p_pre = NULL;
	new_rt = NULL;
	queuecnt = 0;

	if(p_local_rep) {
		p_buf = p_local_rep->buffer_list;
		if(rtable.rt_lookup(new_rt_addr) == RTF_VALID) {
			new_rt = rtable.rt_get(new_rt_addr);

			while(p_buf) { // FIFO
				GET_PKT(p, p_buf->queued_pkt);
				p->rt_setgw(new_rt->rt_nexthop);
				sendToQueue(p_buf->queued_pkt);
				queuecnt++;

				p_pre = p_buf;
				p_buf = p_buf->next;

				//free(p_pre);
			}
		} else {
			 return 0;
		}
	}


	local_repair_table.remove(new_rt_addr);

	return 0;
}



/*
 *   Regulary Send Broadcast HELLO message.
 */
int AODV::sendHello() {

	// Helllo msg will be sent if there is one or 
	// more than one active-routes(hopcount >= 2) in the route table.
	// ps: Since receiving Hello pkt may generate route with 1 hopcount,
	//     to avoid infinite sendHello, we take those route with at least
	//     2 hopcount as active.
	if(rtable.rt_activeCnt() == 0) {
		int tmprandom = Random();
		int sign=0;
		if (tmprandom%2 == 0) 
        		sign = 1;     
		else
        		sign = -1;

		tmprandom = tmprandom%10000 * sign;

		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(AODV, sendHello);
		SendHello_timer.setCallOutObj(this, type);
		SendHello_timer.start((sendhello_timer_ + tmprandom), 0);

		return 0;
	}

	struct AODV_packet *mypkt;
	struct RREP_msg    *my_hello;
	
	
	ePacket_   *pkt;
	Packet *p = new Packet;
	pkt = createEvent();
	 
	mypkt = (struct AODV_packet *)p->pkt_malloc(sizeof(struct AODV_packet)
                                          	+ sizeof(struct RREP_msg));
	strcpy(mypkt->pro_type,"AODV_RREP");
	mypkt->dst_ip = inet_addr("1.0.255.255");
	mypkt->src_ip = *mip;
	mypkt->ttl = 1;
	
	my_hello = (struct RREP_msg *)((char *)mypkt+sizeof(AODV_packet)); 
	my_hello->rrep_hopcount = 0;
	my_hello->rrep_dst_addr = *mip;
	my_hello->rrep_dst_seqno = rtable.myOwnSeqno();
	//my_hello->rrep_ori_addr = ;
	my_hello->rrep_lifetime = ALLOWED_HELLO_LOSS * hello_interval_;

	
	p->pkt_addinfo("isAODV", "yes", 4);

	p->rt_setgw(inet_addr("1.0.255.255"));
	p->pkt_setflow(PF_SEND);
	 
	ATTACH_PKT(p, pkt);

	int tmprandom = Random();
	int sign=0;
	if (tmprandom%2 == 0) 
        	sign = 1;     
	else
        	sign = -1;

	tmprandom = tmprandom%10000 * sign;

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(AODV, sendHello);
	SendHello_timer.setCallOutObj(this, type);
	// send hello every HELLO_INTERVAL
	SendHello_timer.start((hello_interval_ + tmprandom), 0);
	
	return (put(pkt, sendtarget_));
}

/* insert the reverse route to the previous hop from which AODV msg received */
int AODV::updateSimpleRRoute(u_long prevhop_ip) {

	if((rtable.rt_lookup(prevhop_ip)) == RT_NOT_EXIST) {

        	Rt_entry  *r = new Rt_entry;

		r->rt_dst = prevhop_ip;
		r->rt_flags = RTF_VALID;
		r->rt_nexthop = prevhop_ip; 
		r->rt_valid_dst_seqno = false; /* seqno field is invalid */
        	r->rt_hopcount = 1;
		r->rt_time = GetCurrentTime() + my_route_timeout_; /* ? */

		rtable.insert(r);
	}else{
		Rt_entry *tmp;

		tmp = rtable.rt_get(prevhop_ip);

		tmp->rt_flags = RTF_VALID; 
		tmp->rt_nexthop = prevhop_ip; 
		//tmp->rt_valid_dst_seqno = false; 
        	tmp->rt_hopcount = 1;
		tmp->rt_time = GetCurrentTime() + my_route_timeout_;
	}

	return 0;
}

int AODV::updateRT(u_long dst, u_long nexthop, u_int32_t seqno, u_int16_t hopcount, u_int64_t lifetime) {

        Rt_entry  *r = new Rt_entry;
        
	r->rt_dst = dst;
	r->rt_flags = RTF_VALID;
	r->rt_nexthop = nexthop; 
	r->rt_seqno = seqno;
        r->rt_hopcount = hopcount;
	r->rt_time = lifetime;

	rtable.insert(r);

	return 1;
}

int AODV::sendRREQ(u_long dst, const u_char ttl) {

	struct AODV_packet *mypkt;
	struct RREQ_msg * my_rreq;
	
	ePacket_   *pkt;
	Packet *p = new Packet;
	pkt = createEvent();

	 
	mypkt = (struct AODV_packet *)p->pkt_malloc(sizeof(struct AODV_packet)
                                          	+ sizeof(struct RREQ_msg));
	strcpy(mypkt->pro_type, "AODV_RREQ");
	mypkt->dst_ip = inet_addr("1.0.255.255"); // broadcast
	mypkt->src_ip = *mip;
	mypkt->ttl    = ttl;
	
	my_rreq = (struct RREQ_msg *)((char *)mypkt+sizeof(struct AODV_packet)); 
	my_rreq->rreq_dst_addr = dst;
	my_rreq->rreq_src_addr = *mip;
	
	/* my_rreq->rreq_dst_seqno = 1; *//* ? */
	
	my_rreq->G = 0; // ?
	my_rreq->D = 0; // ?
	if(rtable.rt_lookup(dst) == RT_NOT_EXIST){
		my_rreq->U = 1;
	}else{
	
		Rt_entry *tmp = rtable.rt_get(dst);
	 
		if(tmp->rt_valid_dst_seqno) {
			my_rreq->U = 0;
			my_rreq->rreq_dst_seqno = tmp->rt_seqno;
		} else {
			my_rreq->U = 1;
		}
	}

	my_rreq->rreq_src_seqno = rtable.myOwnSeqno();
	
	rreq_id++; // increment by 1 for each attempt
	my_rreq->rreq_id = rreq_id; 
	
	my_rreq->rreq_hopcount = 0; /* 0? */

	p->pkt_addinfo("isAODV", "yes", 4);
	p->rt_setgw(inet_addr("1.0.255.255"));
	p->pkt_setflow(PF_SEND);

	// insert or update the bcache
	bool found_in_bcache = false;
	BroadcastID *b;
	SLIST_FOREACH(b, &bcache, nexB){
		if(b->addr == my_rreq->rreq_src_addr) {

			found_in_bcache = true;
			// update to the greatest rreq_id
			b->bid = (b->bid > my_rreq->rreq_id) ? b->bid : my_rreq->rreq_id;
			b->lifetime = GetCurrentTime() + path_discovery_time_; 
			break;
		}
	}

	if(!found_in_bcache) {
	// buffer its own ip and rreq_id for path_discovery_time
        	BroadcastID *b = new struct BroadcastID;

		b->addr = *mip;
		b->bid = rreq_id; // already +1
		b->lifetime = GetCurrentTime() + path_discovery_time_; 
		SLIST_INSERT_HEAD(&bcache, b, nexB);
	}

	ATTACH_PKT(p, pkt);
	return (put(pkt,sendtarget_));

}



int AODV::forwardRREQ(RREQ_msg * my_rreq, const u_char cur_ttl) {

	struct AODV_packet *mypkt;
	struct RREQ_msg *my_rreq_f;

	ePacket_   *pkt;
	Packet *p = new Packet;
	pkt = createEvent();

	 
	mypkt = (struct AODV_packet *)p->pkt_malloc(sizeof(struct AODV_packet)
                                          	+ sizeof(struct RREQ_msg));
	strcpy(mypkt->pro_type,"AODV_RREQ");
	mypkt->dst_ip = inet_addr("1.0.255.255");
	mypkt->src_ip = *mip;
	mypkt->ttl = cur_ttl;
	
	my_rreq_f = (struct RREQ_msg *)((char *)mypkt+sizeof(AODV_packet)); 
	my_rreq_f->G             =my_rreq->G;
	my_rreq_f->D             =my_rreq->D;
	my_rreq_f->U             =my_rreq->U;
	my_rreq_f->rreq_dst_addr = my_rreq->rreq_dst_addr;
	my_rreq_f->rreq_src_addr = my_rreq->rreq_src_addr;
	my_rreq_f->rreq_dst_seqno = my_rreq->rreq_dst_seqno;
	my_rreq_f->rreq_src_seqno = my_rreq->rreq_src_seqno;
	my_rreq_f->rreq_id = my_rreq->rreq_id;
	my_rreq_f->rreq_hopcount = my_rreq->rreq_hopcount; //already increased

	p->pkt_addinfo("isAODV", "yes", 4);

	p->rt_setgw(inet_addr("1.0.255.255"));
	p->pkt_setflow(PF_SEND);
	
	ATTACH_PKT(p, pkt);
	
	return (put(pkt,sendtarget_));
}


int AODV::sendRREP(u_long dst,      u_long src,      u_long toward, 
		   u_int8_t hopcount, u_int32_t seqno, u_int64_t lifetime){
	struct AODV_packet *mypkt;
	struct RREP_msg * my_rrep;
	
	
	ePacket_   *pkt;
	Packet *p = new Packet;
	pkt = createEvent();
	 
	mypkt = (struct AODV_packet *)p->pkt_malloc(sizeof(struct AODV_packet)
                                          	+ sizeof(struct RREP_msg));
	strcpy(mypkt->pro_type,"AODV_RREP");
	mypkt->dst_ip = toward;
	mypkt->src_ip = *mip;
	mypkt->ttl = NET_DIAMETER; 
	
	my_rrep = (struct RREP_msg *)((char *)mypkt+sizeof(AODV_packet)); 
	my_rrep->rrep_hopcount = hopcount;
	my_rrep->rrep_dst_addr = dst;
	my_rrep->rrep_dst_seqno = seqno;
	my_rrep->rrep_ori_addr = src;
	my_rrep->rrep_lifetime = lifetime;

	p->pkt_addinfo("isAODV", "yes", 4);

	p->rt_setgw(toward);
	p->pkt_setflow(PF_SEND);
	 
	ATTACH_PKT(p, pkt);
	
	return (put(pkt, sendtarget_));
}


int AODV::forwardRREP(struct RREP_msg *my_rrep, const u_char cur_ttl) {

	struct AODV_packet *mypkt;
	struct RREP_msg * my_rrep_f;
	
	Rt_entry *rt_ori;
	if(rtable.rt_lookup(my_rrep->rrep_ori_addr) == RTF_VALID)
		rt_ori = rtable.rt_get(my_rrep->rrep_ori_addr);
	else {
		return (-1);
	}
	
	ePacket_   *pkt;
	Packet *p = new Packet;
	pkt = createEvent();
	 
	mypkt = (struct AODV_packet *)p->pkt_malloc(sizeof(struct AODV_packet)
                                          	+ sizeof(struct RREP_msg));
	strcpy(mypkt->pro_type,"AODV_RREP");
	mypkt->dst_ip = rt_ori->rt_nexthop; 
	mypkt->src_ip = *mip;
	mypkt->ttl = cur_ttl;
	
	my_rrep_f = (struct RREP_msg *)((char *)mypkt+sizeof(AODV_packet)); 
	my_rrep_f->rrep_dst_addr  = my_rrep->rrep_dst_addr;
	my_rrep_f->rrep_dst_seqno = my_rrep->rrep_dst_seqno;
	my_rrep_f->rrep_ori_addr  = my_rrep->rrep_ori_addr;
	my_rrep_f->rrep_hopcount  = my_rrep->rrep_hopcount; // already +1
	my_rrep_f->rrep_lifetime  = my_rrep->rrep_lifetime;
	 
	p->pkt_addinfo("isAODV", "yes", 4);

	p->rt_setgw(rt_ori->rt_nexthop);
	p->pkt_setflow(PF_SEND);

	ATTACH_PKT(p, pkt);
	
	return (put(pkt,sendtarget_));
}

            
// unicast RERR, fill the field according to the unreach_list
int AODV::sendRERR(u_long precursor_ip, Unreach_list *p_ulist) {
	struct AODV_packet *mypkt;
	struct RERR_msg    *my_rerr;
                   	 

	ePacket_   *pkt;
	Packet *p = new Packet;
	pkt = createEvent();

	// First unreach_tuple is in struct RERR_msg
	mypkt = (struct AODV_packet *)p->pkt_malloc( 
		  sizeof(struct AODV_packet)
		+ sizeof(struct RERR_msg) 
		+ (p_ulist->unreach_count() - 1)* sizeof(struct unreach_tuple));
	strcpy(mypkt->pro_type,"AODV_RERR");
	mypkt->dst_ip = precursor_ip;
	mypkt->src_ip = *mip;
	 
	my_rerr = (struct RERR_msg *)((char *)mypkt+sizeof(AODV_packet)); 
	my_rerr->destCount = p_ulist->unreach_count();

	struct unreach_tuple *p_utuple = &(my_rerr->unreach_e);

	// fill in tuples of unreach ip and seq
	struct unreach_entry *p_ue = p_ulist->getHead();
	while(p_ue) {
		p_utuple->unreach_dip  = p_ue->unreach_dip;
		p_utuple->unreach_dseq = p_ue->unreach_dseq;

		p_utuple++;
		p_ue = p_ue->next;
	}

	p->pkt_addinfo("isAODV", "yes", 4);

	p->rt_setgw(precursor_ip); // ?
	p->pkt_setflow(PF_SEND);
		
	ATTACH_PKT(p, pkt);
		
	return(put(pkt, sendtarget_));
}

// broadcast RERR, fill the field according to the unreach_list
int AODV::bcastRERR(Unreach_list *p_ulist) {
	struct AODV_packet *mypkt;
	struct RERR_msg    *my_rerr;
                   	 

	ePacket_   *pkt;
	Packet *p = new Packet;
	pkt = createEvent();
	// First unreach_tuple is in struct RERR_msg
	mypkt = (struct AODV_packet *)p->pkt_malloc( 
		  sizeof(struct AODV_packet)
		+ sizeof(struct RERR_msg) 
		+ (p_ulist->unreach_count() - 1)* sizeof(struct unreach_tuple));
	strcpy(mypkt->pro_type,"AODV_RERR");
	mypkt->dst_ip = inet_addr("1.0.255.255");
	mypkt->src_ip = *mip;
	 
	my_rerr = (struct RERR_msg *)((char *)mypkt+sizeof(AODV_packet)); 
	my_rerr->destCount = p_ulist->unreach_count();

	struct unreach_tuple *p_utuple = &(my_rerr->unreach_e);

	// fill in tuples of unreach ip and seq
	struct unreach_entry *p_ue = p_ulist->getHead();
	while(p_ue) {
		p_utuple->unreach_dip  = p_ue->unreach_dip;
		p_utuple->unreach_dseq = p_ue->unreach_dseq;

		p_utuple++;
		p_ue = p_ue->next;
	}

	p->pkt_addinfo("isAODV", "yes", 4);

	p->rt_setgw(inet_addr("1.0.255.255"));
	p->pkt_setflow(PF_SEND);
		
	ATTACH_PKT(p, pkt);
		
	return(put(pkt, sendtarget_));
}


// mac layer reports transmission error and return the transmitted pkt.
int AODV::LinkLayerCall(ePacket_ *pkt){

 	Packet *p;
 	u_long dst;
	struct Link_fail_entry *p_link_f;
	bool   found=false;
#ifdef LINK_LAYER_RETRY
	bool retry=false;
#else
	bool drop=false;
#endif
	 
 	GET_PKT(p, pkt);
 	IP_DST(dst, p->pkt_sget());


 	if (!local_repair_table.ifExist(dst)) {
		SLIST_FOREACH(p_link_f, &link_fail_list, next) {
			if(p_link_f->dst_ip == dst) {
				found = true;

				p_link_f->acc_cnt++;
				p_link_f->lifetime = LINK_FAIL_LIFETIME;

				if(p_link_f->acc_cnt < LINK_FAIL_THRESHOLD) {
#ifdef LINK_LAYER_RETRY
					retry = true;
#else
					drop = true;
#endif
				} else {
#ifdef LINK_LAYER_RETRY
					retry = false;
#else
					drop = false;
#endif
				}
	
				break;
			}
		}

		if(!found) {
			p_link_f = new struct Link_fail_entry;
			p_link_f->dst_ip = dst;
			p_link_f->acc_cnt = 1;
			p_link_f->lifetime = LINK_FAIL_LIFETIME;
			//p_link_f->next = NULL;

			SLIST_INSERT_HEAD(&link_fail_list, p_link_f, next);

#ifdef LINK_LAYER_RETRY
			retry = true;
#else
			drop  = true;
#endif
		}

		// If the LinkLayerCall does not reach LINK_FAIL_THRESHOLD,
		// we try to send back to link-layer for outgoing again.
#ifdef LINK_LAYER_RETRY
		if(retry) {
			return(put(pkt, sendtarget_));
		}
#else
		if(drop) {
			freePacket(pkt);
			return 0;
		}
#endif

          	local_repair_table.insert(dst, GetCurrentTime()+net_traversal_time_, p->rt_gateway());
 		if(local_repair_table.attachPkt(dst, pkt) < 0){ // queue full
			freePacket(pkt);
			return 0;
		}

 	} else {
 		if(local_repair_table.attachPkt(dst, pkt) < 0){ // queue full
			freePacket(pkt);
		}

		return 0; // local-repair already took place before
	}

	// local repair
	Rt_entry *p_rt_broken = rtable.rt_get(dst);
	if(p_rt_broken) {
		// local repair only once
		if(p_rt_broken->rt_flags != RTF_BEING_REPAIRED) {
			p_rt_broken->rt_seqno++;   // dst_seqno++ to avoid route loop

			p_rt_broken->rt_flags = RTF_BEING_REPAIRED;
			sendRREQ(dst, p_rt_broken->rt_hopcount);
		}
	}

 	return 0;
}

