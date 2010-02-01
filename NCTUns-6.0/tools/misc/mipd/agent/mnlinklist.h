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

#ifndef MIP_MNLINKLIST
#define MIP_MNLINKLIST
#endif
#ifndef MIP_MNSTATE
#define MIP_MNSTATE
typedef enum{ATHOME, FOREIGN, AWAY, NOTCHILD, NA} MnState;
#endif
#include <unistd.h>
#include <netinet/in.h>

struct mnlinkList{
	struct mnlinkListEntry *head;
	struct mnlinkListEntry *tail;
	struct mnlinkListEntry *pCurrent;
	int                    no;
};
typedef struct mnlinkList MNLLIST;

struct mnlinkListEntry{
	struct in_addr         mnaddr;
	struct in_addr         coa;
	/* struct in_addr         haddr; */
	MnState                state; 
	/* int                    lifetime; */
	struct mnlinkListEntry *next;
};

MNLLIST *              newMNList();
int                    isEmptyMNL(MNLLIST *);
int                    printMNList(char *, MNLLIST *);
int                    freeMNList(MNLLIST *);

struct mnlinkListEntry * newMNListEntry(char *, char *, MnState);
struct mnlinkListEntry * newMNListEntry2(struct in_addr *, struct in_addr *, MnState);
int                    addMNEntry(MNLLIST *, char *, char *, MnState);
int                    addMNEntry2(MNLLIST *, struct in_addr *, struct in_addr *, MnState);
int                    delMNEntry(MNLLIST *, struct mnlinkListEntry *, struct mnlinkListEntry *);
int                    printMNEntry(struct mnlinkListEntry *);
struct mnlinkListEntry * getCurrentMNEntry(MNLLIST *);
struct mnlinkListEntry * getMNNextEntry(MNLLIST *);
int                    rewindMNList(MNLLIST *);
struct mnlinkListEntry * searchMNEntry(MNLLIST *, struct in_addr *);
