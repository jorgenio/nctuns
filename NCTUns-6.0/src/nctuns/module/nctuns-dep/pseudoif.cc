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
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <heap.h>
#include <packet.h>
#include <nctuns-dep/pseudoif.h>
#include <nctuns-dep/node.h>
#include <nctuns_api.h>
#include <ethernet.h>

#include <fcntl.h>
#include <ip.h>

extern NslObject		**nodelist;

MODULE_GENERATOR(PseudoIf);

PseudoIf::PseudoIf(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)  
{

	/* turn off flow control */
	r_flowctl = s_flowctl = DISABLED;

	/* bind variable */
	vBind_ip("ip", &ip_);
	vBind_ip("netmask", &netmask_);

	/* register variable */
        REG_VAR("IP", &ip_);
        REG_VAR("NETMASK", &netmask_);
}  

PseudoIf::~PseudoIf() { }

int PseudoIf::init() {

	/* get register variable */
	mac_ = GET_REG_VAR(get_port(), "MAC", u_char *);  

	return(1);  
}
  
int PseudoIf::recv(ePacket_ *pkt) {

	assert(pkt);
	int nodeType = 0;

	((Node *)nodelist[get_nid()])->getNodeType(nodeType);
	/*
	 * Simulate receive the packet and send it to node module.
	 */
	if(nodeType == 1)
	{
		// large scale car
		((Node *)nodelist[get_nid()])->nodeRecv(pkt);
	}
	else
	{
		freePacket(pkt);
	}
	return 1;
}


int PseudoIf::send(ePacket_ *pkt) {

	assert(pkt);
	return(put(pkt, sendtarget_)); 
}


/*
 * This member function should be called by event
 * scheduler. Whenever this member function is called,
 * it represents that this pseudo-interface has packet to 
 * be sent.
 */
#include <com_HEADER/tcp.h>
#include <arpa/inet.h>

int PseudoIf::Debugger(Event *e) {

	u_char			*p;
 
	printf("DEBUG: PseudoIf object\n");
	NslObject::Debugger();
  
	p = (u_char *)&ip_;  
	printf("   ip = %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

	p = (u_char *)&netmask_;
	printf("   netmask = %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

	return(1);
}

