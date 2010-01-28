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

#ifndef _OSPF_H_
#define _OSPF_H_

#include "route.h"
#include "neighbor.h"
#include "net.h"

#define OSPF_HELO	1
#define OSPF_LSA	2
#define OSPF_BROADCAST	3
#define OSPF_UPDATE_LSA	4

#define OSPF_HELO_WINDOW	10
#define OSPF_DRATE_HIGH		8
#define OSPF_DRATE_HALF		5
#define OSPF_DRATE_LOW		2

#define OSPF_DEBUG if (0) printf

#define OSPF_ORI	100
#define OSPF_ETX	101

int    get_version(); 
int    set_version(int value);

struct ospf_header{
	unsigned char		ver;
	unsigned char		type;
	unsigned short          len;
	keytype                 rid;
	/*AID, Checksum, autype, autication  */
};

struct helo_header{
	unsigned short		interval;
};

struct lsa_header{
	keytype			aid;
	unsigned int		seq;
	unsigned int 		len;
};

/* two versions of struct lsa_body are defined.
 * lsa_body_etx and lsa_body_ori. 
 */
struct lsa_body_etx{
	keytype			id;
	unsigned char		drate;
	unsigned char		type;
};

struct lsa_body_ori{
	keytype			id;
	unsigned char		state;
	unsigned char		type;
};

struct lsa_list{
	node			n;
	keytype			id;
	unsigned short		seq;
	unsigned short		bseq;
	list 			lsa;
};

/* two versions of struct lsa_entry are defined.
 * lsa_entry_etx and lsa_entry_ori. 
 */

struct lsa_entry_etx{
	node			n;
	struct lsa_body_etx	body;
	struct ospf_tnode_etx*  tnode;
	double			etx;
	///* marked by jclin to prevent from segmentation fault. 
        unsigned long		timeout;
	unsigned char		state;
        //*/
};

struct lsa_entry_ori{
	node			n;
	struct lsa_body_ori	body;
};

typedef struct ospf_d_etx {
	struct ospf_header 	header;
	list			self_lsas;
	list			lsa_lists;
	list			neighbors;
	struct ospf_tnode_etx*	tree;
	unsigned short		helo_interval;
	unsigned short		seq;
	unsigned short		bseq;
	struct mesh_if		*ifaces[32]; 
	int			ifaces_num;
	int			adhoc_iface;
	struct mesh_rtb		*route;
	unsigned char		state;
} ospf_d_etx;

typedef struct ospf_d_ori {
	struct ospf_header 	header;
	list			self_lsas;
	list			lsa_lists;
	list			neighbors;
	struct ospf_tnode_ori*	tree;
	unsigned short		helo_interval;
	unsigned short		seq;
	unsigned short		bseq;
	struct mesh_if		*ifaces[32]; 
	int			ifaces_num;
	int			adhoc_iface;
	struct mesh_rtb		*route;
	unsigned char		state;
} ospf_d_ori;

#define OSPF_HELO_STATE 	0
#define OSPF_NORMAL_STATE  	1

#define ospf_adhoc_iface(od) (od->ifaces[od->adhoc_iface])

ospf_d_etx* ospf_init_etx( keytype id, unsigned short interval, struct mesh_if** ifaces, int n);
ospf_d_ori* ospf_init_ori( keytype id, unsigned short interval, struct mesh_if** ifaces, int n);

void ospf_helo_etx( ospf_d_etx *od);
void ospf_helo_ori( ospf_d_ori *od);
void ospf_flood_packet_etx( ospf_d_etx *od, char* buf, int s);
void ospf_flood_packet_ori( ospf_d_ori *od, char* buf, int s);
void ospf_flood_lsa_etx( ospf_d_etx *od);
void ospf_flood_lsa_ori( ospf_d_ori *od);
void ospf_forward_lsa_etx(ospf_d_etx* od, char* buf, int s);
void ospf_forward_lsa_ori(ospf_d_ori* od, char* buf, int s);
void ospf_recv_etx( ospf_d_etx *od, char* buf, int size);
void ospf_recv_ori( ospf_d_ori *od, char* buf, int size);
void ospf_recv_broadcast_etx( ospf_d_etx *od, char* buf, int size);
void ospf_recv_broadcast_ori( ospf_d_ori *od, char* buf, int size);
void ospf_recv_lsa_etx( ospf_d_etx *od, char* buf,int size);
void ospf_recv_lsa_ori( ospf_d_ori *od, char* buf,int size);
void ospf_recv_helo_etx( ospf_d_etx *od, char* buf,int size);
void ospf_recv_helo_ori( ospf_d_ori *od, char* buf,int size);

struct lsa_list* ospf_get_lsa_list_etx( ospf_d_etx* od, keytype k);
struct lsa_list* ospf_get_lsa_list_ori( ospf_d_ori* od, keytype k);


struct lsa_list* ospf_flush_lsa_list_etx( ospf_d_etx* od, keytype k);
struct lsa_list* ospf_flush_lsa_list_ori( ospf_d_ori* od, keytype k);

struct lsa_entry_etx* ospf_get_lsa_entry_etx( ospf_d_etx* od, struct lsa_list* l, keytype k);
struct lsa_entry_ori* ospf_get_lsa_entry_ori( ospf_d_ori* od, struct lsa_list* l, keytype k);

/* two versions of struct lsa_entry are defined.
 * ospf_tnode_etx and ospf_tnode_ori. 
 */

struct ospf_tnode_etx{
	struct ospf_tnode_etx *parent, *child, *sibling,*rsibling;
	struct lsa_body_etx body;
	double		etx;
};

struct ospf_tnode_ori{
	struct ospf_tnode_ori *parent, *child, *sibling;
	struct lsa_body_ori    body;
};

/* two versions of struct lsa_entry are defined.
 * ospf_build_tree_etx and ospf_build_tree_ori.
 */

void ospf_build_tree_etx( ospf_d_etx* od);
void ospf_build_tree_ori( ospf_d_ori* od);

/* two versions of struct lsa_entry are defined.
 * ospf_flush_tree_etx and ospf_flush_tree_ori.
 */

void ospf_flush_tree_etx( ospf_d_etx *od);
void ospf_flush_tree_ori( ospf_d_ori *od);

/* two versions of struct lsa_entry are defined.
 * ospf_free_tree_etx and ospf_free_tree_ori.
 */

void ospf_free_tree_etx(struct ospf_tnode_etx* tn, struct lsa_list* l, struct ospf_tnode_etx** ots);

void ospf_free_tree_ori(struct ospf_tnode_ori* tn);

void ospf_make_route_etx(ospf_d_etx *od);
void ospf_make_route_ori(ospf_d_ori *od);


void ospf_set_route_etx ( ospf_d_etx *od, struct mesh_rtb* rtb, struct ospf_tnode_etx *tn, struct mesh_rte* prte);
void ospf_set_route_ori ( ospf_d_ori *od, struct mesh_rtb* rtb, struct ospf_tnode_ori *tn, struct mesh_rte* prte);

/* two versions of struct lsa_entry are defined.
 * ospf_new_neighbor_etx and ospf_new_neighbor_ori
 */

struct neighbor* ospf_new_neighbor_etx( ospf_d_etx* od, keytype id, struct mesh_if* iface);
struct neighbor* ospf_new_neighbor_ori( ospf_d_ori* od, keytype id, struct mesh_if* iface);

int mesharp_ospf_helo_ori( struct mesh_if* if_p);

#endif  /* _OSPF_H_ */
