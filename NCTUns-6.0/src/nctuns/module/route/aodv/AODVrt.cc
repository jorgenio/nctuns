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
#include <module/route/aodv/AODV.h>
#include <nctuns_api.h>
//#include <dmalloc.h>
//
/* #define DEBUG_HSIYUN */

using namespace AODVd; 

/// Nei_entry //////////////////////////////////
Nei_entry::Nei_entry(u_long addr, u_int64_t lifetime) {
	nei_addr = addr;
	nei_time = lifetime;
	next = NULL;
}

Nei_entry::~Nei_entry() {
}

/// Neighbor //////////////////////////////////
Neighbor::Neighbor() {
	nei_head = NULL;
}

Neighbor::~Neighbor() {
	Nei_entry *n = nei_head;
	Nei_entry *tmp;

	while(n){
		tmp = n;
		n = n->next;
		delete tmp;
	}
}

// create new entry or update the existing lifeitme
int Neighbor::update(u_long addr, u_int64_t lifetime) {

	Nei_entry *p_nei = getEntry(addr);
	if(!p_nei) {  // not exist
		Nei_entry *n_e = new Nei_entry(addr, lifetime);
		if(n_e == NULL) return -1; 

		// insert into the head
		n_e->next = nei_head;
		nei_head  = n_e;
		return 0;
	}else {
		p_nei->nei_time = lifetime;

		return 1;
	}

}

int Neighbor::remove(u_long addr) {
	Nei_entry *p_nei = nei_head;
	Nei_entry *p_pre = NULL;

	while(p_nei) {
		if(p_nei->nei_addr == addr) {
			if(p_pre == NULL) {
				nei_head = p_nei->next;
			}else {
				p_pre->next = p_nei->next;
			}

			delete p_nei;

			return 0;
		}
		p_pre = p_nei;
		p_nei = p_nei->next;
	}

	// NOT found
	return -1;
}

Nei_entry* Neighbor::getHead() {
	return nei_head;
}

Nei_entry* Neighbor::getEntry(u_long addr) {
	Nei_entry *p_nei = nei_head;

	while(p_nei) {
		if(p_nei->nei_addr == addr)
			return p_nei;

		p_nei = p_nei->next;
	}

	// NOT found
	return NULL;
}

/// Rt_entry //////////////////////////////////
Rt_entry::Rt_entry() {
	rt_preclist = new Neighbor; 
	next = NULL;
}

Rt_entry::~Rt_entry() {
	delete rt_preclist;
}



/// AODV_RtTable /////////////////////////////
AODV_RtTable::AODV_RtTable() {
	rt_head = NULL;
	rt_tail = NULL;

}

AODV_RtTable::~AODV_RtTable() {
	Rt_entry *r = rt_head;
	Rt_entry *tmp;

	while(r) {
		tmp = r;
		r = r->next;
		delete tmp;
	}
}

int AODV_RtTable::insert(Rt_entry *r){
	if((rt_head == NULL) && (rt_tail == NULL)) {
		rt_head = r;
		rt_tail = r;
	}else {
		// insert into tail
		rt_tail->next = r;
		rt_tail = r;
	}

	return 0;
}

int AODV_RtTable::remove(u_long addr){
	Rt_entry *r   = rt_head;
	Rt_entry *pre = NULL;

	while(r) {
		if(r->rt_dst == addr) {
			if(pre == NULL) { 
				//the first entry(for its own) should not be deleted
				printf("AODV error.\n");
				//rt_head = r->next;
				//delete r;
			}else {
				pre->next = r->next;
				if(r == rt_tail)
					rt_tail = pre;

				delete r;
			}
			return 0; // found and deleted
		}
		pre = r;
		r = r->next;
	}

	return -1; // entry not found
}

int AODV_RtTable::removeEntry(Rt_entry *target, Rt_entry *pre){

	pre->next = target->next;
	if(target == rt_tail)
		rt_tail = pre;
	delete target;
	//target->~Rt_entry();

	return 0;
}

int AODV_RtTable::updatePrecursorList(u_long dst_addr, u_long nei_addr){
	Rt_entry *tmp = rt_head;

	while(tmp) {
		if(tmp->rt_dst == dst_addr) {
			// lifetime is not necessary for precursor
			tmp->rt_preclist->update(nei_addr, 0);
			return 0;
		}

		tmp = tmp->next;
	}
	// corresponding route entry not found 
	
	return -1;
}


int AODV_RtTable::rt_lookup(u_long ip) {
	Rt_entry *tmp = rt_head;

	while(tmp) {
		if(tmp->rt_dst == ip) {
			return tmp->rt_flags;
		}

		tmp = tmp->next;
	}

	return RT_NOT_EXIST;
}
Rt_entry* AODV_RtTable::rt_get(u_long ip) {
	Rt_entry *tmp = rt_head;

	while(tmp) {
		if(tmp->rt_dst == ip) {
			return tmp;
		}

		tmp = tmp->next;
	}

	return NULL;
}

Rt_entry* AODV_RtTable::rt_getHead() {

	return rt_head;
}

// the first routing entry is for its own(including its own seqno)
u_int32_t AODV_RtTable::myOwnSeqno() {
	return rt_head->rt_seqno;
}

int AODV_RtTable::incMyOwnSeqno() {
	rt_head->rt_seqno++;

	return 0;
}

int AODV_RtTable::rt_activeCnt() {
	int ret = 0;

	// The first entry for its own is excluded.
	Rt_entry *tmp = rt_head->next;

	while(tmp) {
		if((tmp->rt_flags == RTF_VALID) && (tmp->rt_hopcount >=2))
			ret++;

		tmp = tmp->next;
	}

	return ret;
}

/// Ctrl_entry ////////////////////////////
Ctrl_entry::Ctrl_entry() {
	buf_pkt_cnt = 0;
	buffer_list = NULL;
	rreq_retries = 0;

	next = NULL;
}

Ctrl_entry::~Ctrl_entry() {
	struct buf_list *p_buf = buffer_list;
	struct buf_list *p_tmp;

	while(p_buf) {
		p_tmp = p_buf;
		p_buf = p_buf->next;
		free(p_tmp);
	}
}

/// CtrlTable /////////////////////////////
CtrlTable::CtrlTable() {
	ctrl_head = NULL;
}

CtrlTable::~CtrlTable() {
	Ctrl_entry *c = ctrl_head;
	Ctrl_entry *tmp;

	while(c) {
		tmp = c;
		c = c->next;
		delete tmp;
	}
}

int CtrlTable::insert(u_long dst, u_int64_t rreq_lt) {
	Ctrl_entry *c = new Ctrl_entry;


	c->dst_ip = dst;
	c->rreq_lifetime = rreq_lt;

	c->next = ctrl_head; // insert into head
	ctrl_head = c;

	return 0;
}

int CtrlTable::attachPkt(u_long in_dst_ip, ePacket_ *pkt) {


    if (!ifExist(in_dst_ip))
         return (-1);   /* not found control entry for this IP */

    Ctrl_entry *p_c = ctrl_head;
    // determine the correspoding entry
    while(p_c) {
	    if(p_c->dst_ip == in_dst_ip)
		    break;
	    p_c = p_c->next;
    }

    if(p_c->buf_pkt_cnt >= AODV_MAXQUEUELEN)
	    return (-1);

    struct buf_list  *p_buf = p_c->buffer_list;

    // traverse to the tail
    while(p_buf) {
	    if(p_buf->next == NULL)
		    break;
	    p_buf = p_buf->next;
    }

    if(!p_buf) { // list is empty
    	p_buf = (struct buf_list*)malloc(sizeof(struct buf_list));
    	p_buf->queued_pkt = pkt;
    	p_buf->next = NULL;

	p_c->buffer_list = p_buf;
    }else {
    	// attach to the tail (for FIFO processing queued pkts)
    	p_buf->next = (struct buf_list*)malloc(sizeof(struct buf_list));
    	p_buf->next->queued_pkt = pkt;
    	p_buf->next->next = NULL;
    }

    p_c->buf_pkt_cnt++;
    // p_c->rreq_enable = true;
    
    return (1);	 
}

int CtrlTable::remove(u_long dst) {
	Ctrl_entry *c   = ctrl_head;
	Ctrl_entry *pre = NULL;

	while(c) {
		if(c->dst_ip == dst) {
			if(pre == NULL) { //delete first entry
				ctrl_head = c->next;
				delete c;
			}else {
				pre->next = c->next;
				delete c;
			}
			return 0; // found and deleted
		}
		pre = c;
		c = c->next;
	}

	return -1; // entry not found
}

bool CtrlTable::ifExist(u_long addr) {
	Ctrl_entry *tmp = ctrl_head;

	while(tmp) {
		if(tmp->dst_ip == addr)
		        return true;
		tmp = tmp->next;
       }

       /* NOT found */
       return false;
}

Ctrl_entry* CtrlTable::getHead() {
	return ctrl_head;
}

Ctrl_entry* CtrlTable::getEntry(u_long addr) {
	Ctrl_entry *ret = ctrl_head;

	while(ret) {
		if(ret->dst_ip == addr)
			return ret;
		ret = ret->next;
	}

	// NOT found
	return NULL;
}


/// LocalRepair_entry ////////////////////////////
LocalRepair_entry::LocalRepair_entry() {
	buf_pkt_cnt = 0;
	buffer_list = NULL;
	next = NULL;
}

LocalRepair_entry::~LocalRepair_entry() {
	struct buf_list *p_buf = buffer_list;
	struct buf_list *p_tmp;

	while(p_buf) {
		p_tmp = p_buf;
		p_buf = p_buf->next;
		free(p_tmp);
	}
}

/// LocalRepairTable /////////////////////////////
LocalRepairTable::LocalRepairTable() {
	local_repair_head = NULL;
}

LocalRepairTable::~LocalRepairTable() {
	LocalRepair_entry *c = local_repair_head;
	LocalRepair_entry *tmp;

	while(c) {
		tmp = c;
		c = c->next;
		delete tmp;
	}
}

int LocalRepairTable::insert(u_long dst, u_int64_t rreq_lt, u_long disc_node) {
	LocalRepair_entry *c = new LocalRepair_entry;


	c->dst_ip = dst;
	c->local_repair_lifetime = rreq_lt;
	if(disc_node != NO_USE)
		c->brokenlink_node = disc_node;

	c->next = local_repair_head; // insert into head
	local_repair_head = c;

	return 0;
}

int LocalRepairTable::attachPkt(u_long in_dst_ip, ePacket_ *pkt) {


    if (!ifExist(in_dst_ip))
         return (-1);   /* not found control entry for this IP */

    LocalRepair_entry *p_c = local_repair_head;
    // determine the correspoding entry
    while(p_c) {
	    if(p_c->dst_ip == in_dst_ip)
		    break;
	    p_c = p_c->next;
    }

    if(p_c->buf_pkt_cnt >= AODV_MAXQUEUELEN)
	    return (-1);

    struct buf_list  *p_buf = p_c->buffer_list;

    // traverse to the tail
    while(p_buf) {
	    if(p_buf->next == NULL)
		    break;
	    p_buf = p_buf->next;
    }

    if(!p_buf) { // list is empty
    	p_buf = (struct buf_list*)malloc(sizeof(struct buf_list));
    	p_buf->queued_pkt = pkt;
    	p_buf->next = NULL;

	p_c->buffer_list = p_buf;
    }else {
    	// attach to the tail (for FIFO processing queued pkts)
    	p_buf->next = (struct buf_list*)malloc(sizeof(struct buf_list));
    	p_buf->next->queued_pkt = pkt;
    	p_buf->next->next = NULL;
    }

    p_c->buf_pkt_cnt++;
    // p_c->rreq_enable = true;
    
    return (1);	 
}

int LocalRepairTable::remove(u_long dst) {
	LocalRepair_entry *c   = local_repair_head;
	LocalRepair_entry *pre = NULL;

	while(c) {
		if(c->dst_ip == dst) {
			if(pre == NULL) { //delete first entry
				local_repair_head = c->next;
				delete c;
			}else {
				pre->next = c->next;
				delete c;
			}
			return 0; // found and deleted
		}
		pre = c;
		c = c->next;
	}

	return -1; // entry not found
}


bool LocalRepairTable::ifExist(u_long addr) {
	LocalRepair_entry *tmp = local_repair_head;

	while(tmp) {
		if(tmp->dst_ip == addr)
		        return true;
		tmp = tmp->next;
       }

       /* NOT found */
       return false;
}

LocalRepair_entry* LocalRepairTable::getHead() {
	return local_repair_head;
}

LocalRepair_entry* LocalRepairTable::getEntry(u_long addr) {
	LocalRepair_entry *ret = local_repair_head;

	while(ret) {
		if(ret->dst_ip == addr)
			return ret;
		ret = ret->next;
	}

	// NOT found
	return NULL;
}
Unreach_list::Unreach_list() {
	unr_count = 0;
	unr_head = NULL;
}

Unreach_list::~Unreach_list() {
	struct unreach_entry *p_ue = unr_head;
	struct unreach_entry *p_pre = NULL;

	while(p_ue) {
		p_pre = p_ue;
		p_ue = p_ue->next;
		free(p_pre);
	}
}

int Unreach_list::insert(u_long addr, u_int32_t seqno) {
	struct unreach_entry *p_unr = (struct unreach_entry *)malloc(sizeof(struct unreach_entry));

	p_unr->unreach_dip  = addr;
	p_unr->unreach_dseq = seqno;

	// insert into the head
	p_unr->next = unr_head;
	unr_head    = p_unr;

	unr_count++;

	return 0;
}

struct unreach_entry* 
Unreach_list::getHead() {
	return unr_head;
}

u_char Unreach_list::unreach_count() {
	return unr_count;
}
