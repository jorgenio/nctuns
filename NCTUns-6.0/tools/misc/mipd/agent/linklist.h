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

#ifndef MIP_LINKLIST
#define MIP_LINKLIST
#endif
#include <unistd.h>
#include <netinet/in.h>

struct linkList{
	struct linkListEntry *head;
	struct linkListEntry *tail;
	struct linkListEntry *pCurrent;
	int                   no;
};
typedef struct linkList LLIST;

struct linkListEntry{
	struct in_addr        addr;
	struct linkListEntry *next;
};

LLIST *                newList();
int                    isEmpty(LLIST *);
int                    printList(char *, LLIST *);
int                    freeList(LLIST *);

struct linkListEntry * newListEntry(char *);
struct linkListEntry * newListEntry2(struct in_addr *);
int                    addEntry(LLIST *, char *);
int                    addEntry2(LLIST *, struct in_addr *);
int                    delEntry(LLIST *, struct linkListEntry *, struct linkListEntry *);
int                    printEntry(struct linkListEntry *);
struct linkListEntry * getCurrentEntry(LLIST *);
struct linkListEntry * getNextEntry(LLIST *);
int                    rewindList(LLIST *);
struct linkListEntry * searchEntry(LLIST *, struct in_addr *);

