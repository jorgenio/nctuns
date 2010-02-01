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

#include "nctuns_mip.h"
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

int
ip2ssdd(struct in_addr src, struct in_addr dst, char *ssdd)
{
	unsigned char *p, *q; /* "unsigned" char is needed! */


	p = (unsigned char *)&(src.s_addr);
	q = (unsigned char *)&(dst.s_addr);

	/* s.s.d.d format */
	sprintf(ssdd, "%u.%u.%u.%u", p[2], p[3], q[2], q[3]); 

	return 0;
}

struct in_addr *  
ssdd2dst(struct in_addr *ssdd)
{
	unsigned char *p;
	struct in_addr *dst;

	dst = (struct in_addr *)malloc(sizeof(struct in_addr));
	if(dst != NULL){
		memset(dst, 0, sizeof(dst));
		dst->s_addr = ssdd->s_addr;
		p = (unsigned char *)dst;
		p[0] = 1; p[1] = 0;
	}

	return dst;
}

struct in_addr *  
ssdd2src(struct in_addr *ssdd)
{
	unsigned char *p;
	struct in_addr *src;

	src = (struct in_addr *)malloc(sizeof(struct in_addr));
	if(src != NULL){
		memset(src, 0, sizeof(src));
		src->s_addr = ssdd->s_addr;
		p = (unsigned char *)src;
		p[2] = p[0]; p[3] = p[1];
		p[0] = 1; p[1] = 0;
	}

	return src;
}

char *            
addr2str(struct in_addr *addr)
{
	return 0;
}

int               
cpaddr(struct in_addr *src, struct in_addr *dst)
{
	src->s_addr = dst->s_addr;

	return 0;
}

struct in_addr *
srcFrmPkt(char *pkt)
{
	struct ip *ip;


	ip = (struct ip *)pkt;
	
	return &(ip->ip_src);
}

struct in_addr *
dstFrmPkt(char *pkt)
{
	struct ip *ip;


	ip = (struct ip *)pkt;
	
	return &(ip->ip_dst);
}


