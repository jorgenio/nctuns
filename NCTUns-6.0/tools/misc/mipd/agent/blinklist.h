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

#ifndef MIP_BLINKLIST
#define MIP_BLINKLIST
#endif
#include <unistd.h>
#include <netinet/in.h>

struct blinkList{
	struct blinkListEntry *head;
	struct blinkListEntry *tail;
	struct blinkListEntry *pCurrent;
	int                   no;
};
typedef struct blinkList BLLIST;

struct blinkListEntry{
	struct in_addr        mnaddr;
	struct in_addr        coa;
	struct in_addr        haddr;
	/* MnState               state;  */
	long int              lifetime;
	struct blinkListEntry *next;
};

BLLIST *                newBList();
int                     isEmptyBL(BLLIST *);
int                     printBList(char *, BLLIST *);
int                     freeBList(BLLIST *);

struct blinkListEntry * newBListEntry(char *, char *, char *, long int);
struct blinkListEntry * newBListEntry2(struct in_addr *, struct in_addr *, struct in_addr *, long int);
int                     addBEntry(BLLIST *, char *, char *, char *, long int);
int                     addBEntry2(BLLIST *, struct in_addr *, struct in_addr *, struct in_addr *, long int);
int                     delBEntry(BLLIST *, struct blinkListEntry *, struct blinkListEntry *);
int                     printBEntry(struct blinkListEntry *);
struct blinkListEntry * getCurrentBEntry(BLLIST *);
struct blinkListEntry * getNextBEntry(BLLIST *);
int                     rewindBList(BLLIST *);
struct blinkListEntry * searchBEntry(BLLIST *, struct in_addr *);
