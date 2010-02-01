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
#include "./linklist.h"

#include <netinet/in.h>
#include <arpa/inet.h>

struct linkList * 
newList()
{
	struct linkList *plist;


	plist = (struct linkList *)malloc(sizeof(struct linkList));

	if( plist != NULL){
		plist->head = NULL;
		plist->tail = NULL;
		plist->pCurrent = NULL;
		plist->no = 0;
	}
	/* fprintf(stderr, "newList: %p\n", plist); */

	return plist;
}

int
isEmpty(struct linkList *plist)
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
printList(char *str, struct linkList *plist)
{
	struct linkListEntry *p;

	p = plist->head;

	fprintf(stderr, "%sentry#(%d), ", str, plist->no);

	while(p){
		printEntry(p);
		p = p->next;
	}

	fprintf(stderr, "\n");

	return 0;
}

int
printEntry(struct linkListEntry *e)
{
	/* printf("%08X, %08X ", e->addr.s_addr); */
	fprintf(stderr, "%s ", inet_ntoa(e->addr));

	return 0;
}

int 
freeList(struct linkList *plist)
{
	struct linkListEntry *prev, *current;


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

struct linkListEntry *
newListEntry(char *a)
{
	struct linkListEntry *pEntry;

	pEntry = (struct linkListEntry *)malloc(sizeof(struct linkListEntry));

	if(pEntry == NULL)  
		return NULL;

	/* Fill the data member. */
	pEntry->addr.s_addr  = inet_addr(a);
	/* printf("pEntry: %08X ", pEntry->addr.s_addr);
	printf("newListEntry(a): %s, newListEntry(b): %s ", a, b); */
	/* if b is not an empty string. */
	/* if(!strlen(b)) */
	/* pending */
	/* pEntry->coa.s_addr   = inet_addr(b);
	pEntry->haddr.s_addr = inet_addr(c);
	pEntry->state        = s;
	pEntry->lifetime     = d; */
	pEntry->next         = NULL;

	return pEntry;
}

struct linkListEntry *
newListEntry2(struct in_addr *a)
{
	struct linkListEntry *pEntry;

	pEntry = (struct linkListEntry *)malloc(sizeof(struct linkListEntry));

	if(pEntry == NULL)  
		return NULL;

	/* Fill the data member. */
	pEntry->addr.s_addr  = a->s_addr;
	/* printf("pEntry: %08X ", pEntry->addr.s_addr);
	printf("newListEntry(a): %s, newListEntry(b): %s ", a, b); */
	/* if b is not an empty string. */
	/* if(!strlen(b)) */
	/* pending */
	/* if(b != NULL)
		pEntry->coa.s_addr   = b->s_addr;
	if(c != NULL)
		pEntry->haddr.s_addr = c->s_addr;
	pEntry->state        = s;
	pEntry->lifetime     = d; */
	pEntry->next         = NULL;

	return pEntry;
}

int
addEntry(struct linkList *plist, char *a)
{
	struct linkListEntry *p;


	if(plist == NULL){
		fprintf(stderr, "[X] fail to addEntry().\n");
		return -1;
	}

	p = newListEntry(a);
	if(p == NULL){
		fprintf(stderr, "[X] fail to newListEntry().\n");
		return -1;
	}else if( isEmpty(plist) ){ 
		plist->head = p;
		plist->tail = p;
		/* p->next = plist->head;
		plist->head = p;
		plist->pCurrent = plist->head; */
	}else{
		plist->tail->next = p;
		plist->tail = p;
	}
	(plist->no)++;

	return 0;
}

int
addEntry2(struct linkList *plist, struct in_addr *a)
{
	struct linkListEntry *p;


	if(plist == NULL){
		fprintf(stderr, "[X] fail to addEntry().\n");
		return -1;
	}

	p = newListEntry2(a);
	if(p == NULL){
		fprintf(stderr, "[X] fail to newListEntry().\n");
		return -1;
	}else if( isEmpty(plist) ){ 
		plist->head = p;
		plist->tail = p;
		/* p->next = plist->head;
		plist->head = p;
		plist->pCurrent = plist->head; */
	}else{
		plist->tail->next = p;
		plist->tail = p;
	}
	(plist->no)++;

	return 0;
}

/* pending. */
int 
delEntry(struct linkList *plist, struct linkListEntry *e, struct linkListEntry *pre)
{
	if((plist == NULL) || isEmpty(plist)){
		return -1;
	}

	(plist->no)--;
	if((plist->head == e) && (plist->tail == e)){ /* delete the only entry. */
		plist->head = NULL;
		plist->tail = NULL;
	}else if(plist->head == e){ /* delete the first entry. */
		plist->head = e->next;
	}else if(plist->tail == e){ /* delete the last entry. */
		plist->tail = pre;
		plist->tail->next = NULL;
	}else{
		pre->next = e->next;
	}

	free(e);
	
	return 0;
	
}


/* return the pCurrent point, then advance to next entry. */
struct linkListEntry * 
getCurrentEntry(struct linkList *plist)
{
	struct linkListEntry *cur;


	if((plist == NULL) || isEmpty(plist) || (plist->pCurrent == NULL)){
		return NULL;
	}else{
		cur = plist->pCurrent;
		plist->pCurrent = plist->pCurrent->next;
		return cur;
	}
}

/* struct linkListEntry * 
getNextEntry(struct linkList *plist)
{
	if((plist == NULL) || isEmpty(plist)){
		return NULL;
	}else{
		plist->pCurrent = plist->pCurrent->next;
		return plist->pCurrent;
	}
} */

int
rewindList(struct linkList *plist)
{
	if((plist == NULL) || isEmpty(plist)){
		return 1;
	}else{
		plist->pCurrent = plist->head;
	}

	return 0;
}

struct linkListEntry *
searchEntry(struct linkList *plist, struct in_addr *a)
{
	struct linkListEntry *cur;

		/* printf("plist = %p\n", plist);
		printf("plist->head = %p\n", plist->head);
		printf("isEmpty() = %d\n", isEmpty(plist)); */

	if(rewindList(plist)){
		/* fprintf(stderr, "list is empty.\n"); */
		return NULL;
	}

	while((cur = getCurrentEntry(plist)) != NULL){
		/* fprintf(stderr, "searchEntry: %08X\n", cur->addr.s_addr); */
		if(cur->addr.s_addr == a->s_addr){
			return cur;
		}
	}

	return NULL; /* not found. */

}


