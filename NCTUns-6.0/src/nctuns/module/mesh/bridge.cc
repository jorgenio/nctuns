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
#include <nodetype.h>
#include <mesh/bridge.h>
#include <ethernet.h>
#include <random.h>
#include <packet.h>
#include <mbinder.h>
#include <netinet/in.h>

#include <fstream>
#include <stdlib.h>
#include <string>

#define BGDEBUG if(0) printf

extern RegTable                 RegTable_;
extern typeTable                *typeTable_;

MODULE_GENERATOR(Bridge);


Bridge::Bridge(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
   
	ifaces_num = 0;

}

Bridge::~Bridge() {

}


/* main behaviors of switch are in send function */
int Bridge::send(ePacket_ *pkt) {

        keytype		srcMac, dstMac;
	Packet		*pk = (Packet*)pkt->DataInfo_;
	struct ether_header *eh;
	struct neighbor *ne;
	ePacket_	*ep;
	Packet		*epk;

	eh = (struct ether_header*)pk->pkt_get();

	getSrcDstMac(&srcMac, &dstMac, pkt);

	ne = neigh_find( &neighbors, dstMac);

	if (!ne){
		
		if (dstMac.key[5] == 255){
			for (int i = 0; i < ifaces_num; i++){
				ep = pkt_copy(pkt);
				epk = (Packet*)ep->DataInfo_;
				eh = (struct ether_header*)epk->pkt_get();
				memcpy(eh->ether_shost, ifaces[i]->name.key, 6);
				if (put( ep, (MBinder*)ifaces[i]->ptr) < 0) printf("%s broadcast error\n", get_name());
			}
			BGDEBUG("%s broadcast\n",get_name());
		}
		else
		BGDEBUG("%s no %x:%x:%x:%x:%x:%x\n", get_name(), 
				dstMac.key[0], dstMac.key[1], dstMac.key[2], dstMac.key[3], dstMac.key[4], dstMac.key[5]);
		
		freePacket(pkt);
		return 1;
	}else {
		BGDEBUG("%s send \n", get_name());
		memcpy(eh->ether_shost, ne->iface->name.key, 6);
		return put( pkt, (MBinder*)ne->iface->ptr);
	}

        return 1;
}

int Bridge::get(ePacket_ *pkt, MBinder *frm){
	struct mesh_if		*iface = NULL;
	int i;
        keytype		srcMac, dstMac;
	struct neighbor *ne;
	Packet		*pk;
	
	pk = (Packet*) pkt->DataInfo_;

	for ( i = 0 ; i < ifaces_num; i++){
		if (((MBinder*)ifaces[i]->ptr)->bindModule() == frm->myModule()){
			iface = ifaces[i];
			break;
		}
	}

	if (!iface)
		return NslObject::get(pkt,frm);

	getSrcDstMac(&srcMac, &dstMac, pkt);
	ne = neigh_find( &neighbors, srcMac);
	if (!ne){
		ne = (struct neighbor*)malloc( sizeof(struct neighbor));
		keycopy( ne->id , srcMac);
		ne->iface = iface;
		ne->state = 1;
		ne->type = NEIGH_MOBILE;
		ne->snr = 0;
		neigh_add( &neighbors, ne);
		BGDEBUG("%s add %x:%x:%x:%x:%x:%x\n", get_name(), 
				srcMac.key[0], srcMac.key[1], srcMac.key[2], srcMac.key[3], srcMac.key[4], srcMac.key[5]);
	}
	if ((unsigned int)pk->pkt_getlen() <= sizeof (struct ether_header) + 1) {
		freePacket(pkt);
		return 1;
	}
	return NslObject::get(pkt,frm);
}

int Bridge::recv(ePacket_ *pkt) {
	keytype srcMac, dstMac;
	getSrcDstMac(&srcMac, &dstMac, pkt);
	BGDEBUG("%s recv %x:%x:%x:%x:%x:%x\n", get_name(), 
				srcMac.key[0], srcMac.key[1], srcMac.key[2], srcMac.key[3], srcMac.key[4], srcMac.key[5]);
	
	return NslObject::recv(pkt);
}

int Bridge::init() {
	unsigned char* mac;
	int i;

	for ( i = 0; i < ifaces_num; i++){
		if (! ifaces[i])continue;
		mac = GET_REG_VAR(((MBinder*)ifaces[i]->ptr)->bindModule()->get_port(),"MAC",u_char*);
		memcpy(ifaces[i]->name.key, mac,6);
	}

	neigh_init(&neighbors);	

	return(1);  

}

  
int Bridge::command(int argc, const char *argv[]) {

    NslObject                       *obj;
    struct mesh_if			*iface;

    if ( argc == 4 ) {

        bool uncanonical_module_name_flag = 0;

        /* The "." sign is not allowed as part of a module name. */
        if ( strstr(argv[3],".") ) {
            uncanonical_module_name_flag = 1;
        }
        /* The RegTable->lookup_Instance() with two parameter version
        * should use the canonical name of a module. If an illegal
        * module name is found, we should return immediately.
        * A canonical module name should start with
        * a prefix "NODE." As such, for any module names
        * without this prefix, this function simply returns
        * the original string as its output because the
        * name translation process may fail with incorrect
        * input names.
        */
        if (strncasecmp("NODE", argv[3], 4))
            uncanonical_module_name_flag = 1;

        /* A canonical module name should start with
        * a prefix "NODE." As such, for any module names
        * without this prefix, this function simply returns
        * the original string as its output because the
        * name translation process may fail with incorrect
        * input names.
        */

        /* Connectivity */
        if (!uncanonical_module_name_flag) {

            obj = RegTable_.lookup_Instance(get_nid(), argv[3]);
            if (!obj) {
                BGDEBUG("The instance of %s does not exist.\n\n", argv[3]);
                return(-1);
            }

        }
    }

    /* support port should be added here */
    MBinder		*tmpMBinder;
    u_int32_t	portNum;

    if (!strncmp(argv[1], "port", 4)) {
        sscanf(argv[1], "port%d", &portNum);

        iface = (struct mesh_if*) malloc( sizeof(struct mesh_if));

        tmpMBinder = new MBinder(this);
        assert(tmpMBinder);
        tmpMBinder->bind_to(obj);

        /*
        unsigned char* mac;
        mac = GET_REG_VAR(obj->get_port(),"MAC",u_char*);
        memcpy( iface->name.key, mac, 6);
        */
        iface->ptr = tmpMBinder;
        iface->state = 1;

        ifaces[portNum - 1] = iface;
        ifaces_num++;

    }
    else {
        return NslObject::command( argc, argv);
    }

    return(1);
}

int Bridge::getSrcDstMac(keytype *src, keytype *dst, ePacket_ *pkt) {

	Packet	*pkt_ = (Packet *)pkt->DataInfo_;
	assert(pkt_);

	/* decapsulate ether header */
	struct ether_header *eh = (struct ether_header *)pkt_->pkt_get();

	/* get ether src and ether dst */
	bcopy(eh->ether_shost, src->key, 6);
	bcopy(eh->ether_dhost, dst->key, 6);

        return 1;
}

