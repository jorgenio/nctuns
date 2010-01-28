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

#ifndef	__NCTUNS_myrouted_h__
#define __NCTUNS_myrouted_h__

#include <event.h>
#include <object.h>
#include <mylist.h>

class Packet;

struct rtentry {
	SLIST_ENTRY(rtentry)		nextrt; 
	u_long				dst;
	u_long				nexthop;
}; 


class myRouted : public NslObject {

 private:
 
 	SLIST_HEAD(, rtentry)		rt; 

 public:

	u_long				*mip;  
	
 	myRouted(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	~myRouted(); 
 	 
 	int			init();
 	int			recv(ePacket_ *pkt);
 	int			send(ePacket_ *pkt);
 	int			command(int argc, const char *argv[]);
 	virtual int		Debugger();

	int			update_rtbl(u_long, u_long); 
	int			routing(u_long dst, Packet *pkt); 
};
 
 
#endif	/* __NCTUNS_myrouted_h__ */
