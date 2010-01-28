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

#ifndef _ROUTE_H_
#define _ROUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "generic.h"
#include "lists.h"

#define RTYPE_AP		0
#define RTYPE_MOBILE		1

struct mesh_if{
	unsigned char		type;
	unsigned char 		index;
        unsigned int 		conn_fixed_network_flag;
	keytype			name;
	unsigned char		state;
	void*			ptr;
};

struct mesh_rtb{
	char*			name;
	list 			hooks;
};

struct mesh_rte{
	node			n;
	unsigned char 		type;
	keytype			target;
	keytype 		nexthop;
	int			hop;
	unsigned long		timestamp;
	unsigned char		weight;
	struct mesh_if		*iface;
};

void rtb_init(struct mesh_rtb* tb);
struct mesh_rte* rtb_find(struct mesh_rtb* tb, keytype k);
void rtb_add(struct mesh_rtb* rtb, struct mesh_rte* rte);
void rtb_del(struct mesh_rte* rte);

#ifdef __cplusplus
}
#endif

#endif /* _ROUTE_H_ */
