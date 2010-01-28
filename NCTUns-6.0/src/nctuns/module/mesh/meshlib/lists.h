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

#ifndef _LISTS_H_
#define _LISTS_H_

#undef _CXX_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>


typedef struct node {
  struct node *next, *prev;
} node;

typedef struct list {			/* In fact two overlayed nodes */
  struct node *head, *null, *tail;
} list;

#define NODE (node *)
#define HEAD(list) ((void *)((list).head))
#define TAIL(list) ((void *)((list).tail))
#define WALK_LIST(n,list, type) for(n= (type) HEAD(list);(NODE (n))->next; \
				n=(type) ((NODE (n))->next))
#define WALK_LIST_DELSAFE(n,nxt,list, type) \
     for( n=(type) HEAD(list); (nxt=(type)((NODE (n))->next)); n= (type)nxt)

#define EMPTY_LIST(list) (!(list).head->next)

void add_tail(list *, node *);
void add_head(list *, node *);
void rem_node(node *);
void add_tail_list(list *, list *);
void init_list(list *);
void insert_node(node *, node *);

#ifdef __cplusplus
}
#endif

#endif /* _LISTS_H_ */
