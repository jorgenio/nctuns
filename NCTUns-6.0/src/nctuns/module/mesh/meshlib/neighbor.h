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

#ifndef _NEIGHBOR_H_
#define _NEIGHBOR_H_

#include "route.h"

#define NEIGH_AP 0
#define NEIGH_MOBILE 1

struct neighbor{
	node			n;
	keytype			id;
	unsigned char		state;
	unsigned char		type;
	char			snr;
	struct mesh_if		*iface;
	unsigned long		timestamp;
};

#define neigh_init(l) init_list(l)
#define neigh_add(l, nb) add_tail( l, NODE (nb))
#define neigh_del(l, nb) rem_node( NODE (nb))

inline struct neighbor* neigh_find(list* l,keytype k){
	struct neighbor* nb;
	WALK_LIST(nb, *l, struct neighbor* ){
		if (keycmp(nb->id, k) == 0)return nb;
	}
	return NULL;
}

#endif /* _NEIGHBOR_H_ */

