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

#ifndef	__NCTUNS_god_routed_h__
#define __NCTUNS_god_routed_h__

#include <route/myrouted.h>


struct routeInfo {
	u_int32_t		dst_nid;
	u_int32_t		next_hop; 
};
 
struct mrtInfo
{
	u_int32_t		nid, dnid, nhop;	// nid = snid
	double			time;
	struct mrtInfo	*next;
};

class GodRouted : public myRouted {

 private :
	
	static bool IsLoadedMrt;
 	static int	Read_mrt(char *);
	static struct mrtInfo	*mrtHead;
	char		*fileName; 
 
 public :  

	GodRouted(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~GodRouted();
	
	int			init();
	int			recv(ePacket_ *pkt);
	int			send(ePacket_ *pkt);  

	int			UpdateRouteTBL(Event_ *ep); 
};  

#endif	/* __NCTUNS_god_routed_h__ */
