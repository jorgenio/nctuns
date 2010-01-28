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
#include <maptable.h>
#include <nodetype.h>
#include "bssrelay.h"
#include <stdlib.h>
#include <gbind.h>
#include <mbinder.h>
#include <packet.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

MODULE_GENERATOR(BSSRelay);


BSSRelay::BSSRelay(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	vBind("port1",&MSPortName);
	vBind("port2",&SGSNPortName);

}


BSSRelay::~BSSRelay() {

}

int BSSRelay::init()
{
	NslObject* obj = RegTable_.lookup_Instance(get_nid(), MSPortName);
	MSport = new MBinder(this);
	MSport->bind_to(obj);

	obj = RegTable_.lookup_Instance(get_nid(), SGSNPortName);
	SGSNport = new MBinder(this);
	SGSNport->bind_to(obj);
	sendtarget_ = MSport;
	recvtarget_ = SGSNport;
        printf("BSSRelay::init(): MS port module = %s SGSN port = %s \n", 
	    (sendtarget_->bindModule())->get_name() , (recvtarget_->bindModule())->get_name());
	return 1;  
}

/* main behaviors of BSSRelay are in send function */
int BSSRelay::send(ePacket_ *pkt, MBinder *frm) {

        //printf("send module= %s \n", frm->myModule()->get_name() );
	if (frm->myModule() == MSport->bindModule() ){
		put(pkt,SGSNport);
	}else{
		put(pkt,MSport);
	}
        return 1;
}


int BSSRelay::recv(ePacket_ *pkt) {

	return 0;
}


int BSSRelay::get(ePacket_ *pkt, MBinder *frm) {

	/* change pkt flow to SEND_FLOW since BSSRelay is the top module */
	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	pkt_->pkt_setflow(PF_SEND);

	return send(pkt, frm);
}

