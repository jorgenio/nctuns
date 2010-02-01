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

#include <stdio.h>
#include <stdlib.h>
#include "./blinklist.h"
#include "linklist.h"

#include <netinet/in.h>
#include <arpa/inet.h>

struct blinkList * 
newBList()
{
	struct blinkList *plist;


	plist = (struct blinkList *)malloc(sizeof(struct blinkList));

	if( plist != NULL){
		plist->head = NULL;
		plist->pCurrent = NULL;
	}

	return plist;
}

int
isEmptyBL(struct blinkList *plist)
{
	/* error! */
	if(plist == NULL)
		return -1;

	if(plist->head == NULL)
		return 1;
	else
		return 0;
}

int
printBList(struct blinkList *plist)
{
	struct blinkListEntry *p;

	p = plist->head;

	while(p){
		printEntry((struct linkListEntry *)p);
		p = p->next;
	}

	return 0;
}

int 
freeBList(struct blinkList *plist)
{
	struct blinkListEntry *prev, *current;


	if(plist == NULL){
		return -1;
	}

	current = plist->head;

	/* pending */
	while(current != NULL){
		prev = current;
		current = current->next;
		free(prev);
	}

	return 0;
}

struct blinkListEntry *
newBListEntry(char *a, char *b)
{
	struct blinkListEntry *pEntry;

	pEntry = (struct blinkListEntry *)malloc(sizeof(struct blinkListEntry));

	if(pEntry == NULL)  
		return NULL;

	/* Fill the data member. */
	pEntry->mnaddr.s_addr = inet_addr(a);
	/* if b is not an empty string. */
	/* if(!strlen(b)) */
		pEntry->coa.s_addr  = inet_addr(b);
	pEntry->next = NULL;

	return pEntry;
}

struct blinkListEntry *
newBListEntry2(struct in_addr *a, struct in_addr *b)
{
	struct blinkListEntry *pEntry;

	pEntry = (struct blinkListEntry *)malloc(sizeof(struct blinkListEntry));

	if(pEntry == NULL)  
		return NULL;

	/* Fill the data member. */
	pEntry->mnaddr.s_addr = a->s_addr;
	/* if b is not an empty string. */
	/* if(!strlen(b)) */
		pEntry->coa.s_addr  = b->s_addr;
	pEntry->next = NULL;

	return pEntry;
}

int
addBEntry(struct blinkList *plist, char *a, char *b)
{
	struct blinkListEntry *p;


	if(plist == NULL){
		return -1;
	}

	p = newBListEntry(a, b);
	if(p == NULL)
		return -1;
	else{
		p->next = plist->head;
		plist->head = p;
		plist->pCurrent = plist->head;
	}

	return 0;
}

int
addBEntry2(struct blinkList *plist, struct in_addr *a, struct in_addr *b)
{
	struct blinkListEntry *p;


	if(plist == NULL){
		return -1;
	}

	p = newBListEntry2(a, b);
	if(p == NULL)
		return -1;
	else{
		p->next = plist->head;
		plist->head = p;
		plist->pCurrent = plist->head;
	}

	return 0;
}

/* pending. */
int 
delBEntry(struct blinkList *plist)
{
	if((plist == NULL) || isEmptyBL(plist)){
		return -1;
	}

	return 0;
}

int
printBEntry(struct blinkListEntry *e)
{
	printf("%08X,%08X ", e->mnaddr.s_addr, e->coa.s_addr);

	return 0;
}

/* return the pCurrent point, then advance to next entry. */
struct blinkListEntry * 
getCurrentBEntry(struct blinkList *plist)
{
	struct blinkListEntry *cur;


	if((plist == NULL) || isEmptyBL(plist) || (plist->pCurrent == NULL)){
		return NULL;
	}else{
		cur = plist->pCurrent;
		plist->pCurrent = plist->pCurrent->next;
		return cur;
	}

}

struct blinkListEntry * 
getNextBEntry(struct blinkList *plist)
{
	if((plist == NULL) || isEmptyBL(plist)){
		return NULL;
	}else{
		plist->pCurrent = plist->pCurrent->next;
		return plist->pCurrent;
	}
}

int
rewindBList(struct blinkList *plist)
{
	if((plist == NULL) || isEmptyBL(plist)){
		return 1;
	}else{
		plist->pCurrent = plist->head;
	}

	return 0;
}

struct in_addr * 
searchBEntry(struct blinkList *plist, struct in_addr *a)
{
	struct blinkListEntry *cur;


	if(rewindBList(plist)){
		fprintf(stderr, "list(@%p) is empty.\n", plist);
		return NULL;
	}

	while((cur = getCurrentBEntry(plist)) != NULL){
		if(cur->mnaddr.s_addr == a->s_addr){
			return &(cur->coa);
		}
	}

	return NULL; /* not found. */

}
