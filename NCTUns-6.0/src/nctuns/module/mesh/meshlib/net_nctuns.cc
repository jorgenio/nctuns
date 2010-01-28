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

#include "net.h"
#include <nctuns_api.h>
#include <event.h>
#include <object.h>
#include <ethernet.h>
#include <netinet/in.h>
#include <packet.h>
#include <mbinder.h>

int showip(ulong ip) {
    char* str=(char*) &ip;
    for ( int i= 0 ; i<4 ; ++i)
        printf(" %d ", (int) str[i]);

    return 1;
}

int showmac(char* mac) {

    char* str=mac;
    for ( int i= 0 ; i<6 ; ++i)
        printf(" %d ",str[i]);

    return 1;
}

int showmac(unsigned char* mac) {

    unsigned char* str=mac;
    for ( int i= 0 ; i<6 ; ++i)
        printf(" %d ",  str[i]);

    return 1;
}


int send_packet( struct mesh_if* iface, void* buf, int s){
	Event_ *ep;
	Packet *pkt;
	char* p;
	NslObject* obj;

	ep = createEvent();
	pkt = new Packet;
	pkt->pkt_setflow(PF_SEND);
	p = pkt->pkt_malloc(s);
	memcpy(p, buf, s);

	ep->DataInfo_ = (void*) pkt;
	obj = ((MBinder*)iface->ptr)->myModule();

	return obj->put (ep, (MBinder*)iface->ptr);
}

int broadcast_packet( struct mesh_if* iface, void* buf, int s){
	Event_ *ep;
	Packet *pkt;
	char* p;
	NslObject* obj;
	struct ether_header* eh;

	ep = createEvent();
	pkt = new Packet;
	pkt->pkt_setflow(PF_SEND);
	p = pkt->pkt_malloc(s);
	memcpy(p, buf, s);

	eh = (struct ether_header*)pkt->pkt_malloc(sizeof(struct ether_header));
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_dhost, "\xff\xff\xff\xff\xff\xff",6);
	memcpy( eh->ether_shost, iface->name.key,6);
	ep->DataInfo_ = (void*) pkt;
	obj = ((MBinder*)iface->ptr)->myModule();

	return obj->put (ep, (MBinder*)iface->ptr);
}

int unicast_packet( struct mesh_if* iface, void* buf, int s){
	Event_ *ep;
	Packet *pkt;
	char* p;
	NslObject* obj;
	struct ether_header* eh;

	ep = createEvent();
	pkt = new Packet;
	pkt->pkt_setflow(PF_SEND);
	p = pkt->pkt_malloc(s);
	memcpy(p, buf, s);

	eh = (struct ether_header*)pkt->pkt_malloc(sizeof(struct ether_header));
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_dhost, "\xff\xff\xff\xff\xff\xff",6);
	memcpy( eh->ether_shost, iface->name.key,6);
	ep->DataInfo_ = (void*) pkt;
	obj = ((MBinder*)iface->ptr)->myModule();

	return obj->put (ep, (MBinder*)iface->ptr);
}

int recv_packet(struct mesh_if* iface, void* buf, int s){
	return 0;
}
