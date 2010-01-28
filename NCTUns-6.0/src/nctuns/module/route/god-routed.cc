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

/* Last update: 2004-1012 by sylin*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <nctuns_api.h>
#include <route/god-routed.h>


MODULE_GENERATOR(GodRouted);

bool			GodRouted::IsLoadedMrt = false ;
struct mrtInfo *GodRouted::mrtHead = NULL ;


GodRouted::GodRouted(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: myRouted(type, id, pl, name)
{
	s_flowctl = DISABLED;
  	r_flowctl = DISABLED;

	vBind("ShortestPathFileName",&fileName);
}


GodRouted::~GodRouted() 
{
	struct mrtInfo	*p;
	
	/* mrtHead should be Nil */
	while( mrtHead )
	{
		p = mrtHead->next ;
		free(mrtHead);
		mrtHead = p ;
	}
}

int GodRouted::init() {

	double			time;
	u_int32_t		nid, dnid, nhop, mynid;
	u_int64_t		timeInTick;
	Event			*ep; 
	struct routeInfo	*rinfo; 
	struct mrtInfo		*p, *q;
 	BASE_OBJTYPE(type);  
 	int count = 0;
 
 	if( !IsLoadedMrt )
 	{
 		assert( !mrtHead );

		/* get .mrt file and read it */
		char *filepath = (char *)malloc( strlen(GetConfigFileDir())+ strlen(fileName) + 1);
		sprintf(filepath,"%s%s", GetConfigFileDir(), fileName);

 		Read_mrt( filepath );
 		free(filepath);
 	}

 	mynid = get_nid();
 	p = mrtHead ; q = NULL ;
 	while( p )
 	{
		nid  = p->nid ;
		dnid = p->dnid;
		nhop = p->nhop;
		time = p->time;

		if( nid != mynid )
		{
			q = p;
			p = p->next ;
 			continue;
		}
		count++;

		if( q )
		{
			q->next = p->next;
			free(p);
			p = q->next;
		}
		else
		{
			mrtHead = p->next;
			free(p);
			p = mrtHead;
		}

		/*
		 * Create an event to update Mobile Node
		 * routing Table.
		 */
		type = POINTER_TO_MEMBER(GodRouted, UpdateRouteTBL);
		SEC_TO_TICK(timeInTick, time); 
		ep = createEvent();
		rinfo = (struct routeInfo *) malloc(sizeof(struct routeInfo));
		assert(rinfo);
 		rinfo->dst_nid = dnid;
  		rinfo->next_hop = nhop;
  		setObjEvent(ep, timeInTick, 0, this, type, (void *)rinfo);
	}
	/* get my ip address */
	mip = GET_REG_VAR(get_port(), "IP", u_long *);
	return(1);
}


int GodRouted::Read_mrt(char *fname)
{
	FILE 			*fp;
 	char			buf[101];
 	int				i;
	u_int32_t		nid, snid, dnid, nhop;
	double			time;
	struct mrtInfo	*newInfo;

	fp = fopen(fname, "r");

	if( fp ==NULL )
	{
		printf("Warning: Can't read file: %s\n",fname);
	}
	else
	{
		while( !feof(fp) ) 
		{
			buf[0] = '\0'; fgets(buf, sizeof(buf)-1, fp);
			if( (buf[0]=='\0')||(buf[0]=='#') )
				continue; 

 			i = sscanf(buf, "$node_(%d) %lf set-next-hop %d %d %d",
				&nid, &time, &snid, &dnid, &nhop); 
			if (i == 0) 
				continue;

                        /* nid should equal to snid */
			/* C.C. Lin: this assertion is temporarily disabled 
			 * for the SuperNode implementation.
			 */

			//assert(nid == snid);

			newInfo = (struct mrtInfo *) malloc( sizeof(struct mrtInfo) );
			assert(newInfo);

			newInfo->nid  = nid;
			newInfo->time = time;
			newInfo->dnid = dnid;
			newInfo->nhop = nhop;

			newInfo->next = mrtHead;
			mrtHead = newInfo;
		}
		fclose(fp); 
		IsLoadedMrt = true;
	}
	return 1;	// Always success
}

int GodRouted::recv(ePacket_ *pkt) {

	return(myRouted::recv(pkt)); 
}


int GodRouted::send(ePacket_ *pkt) {

	return(myRouted::send(pkt)); 
}



int GodRouted::UpdateRouteTBL(Event_ *ep) {

	struct routeInfo	*rinfo;
 	u_long			dstip, nxtip; 


	assert(ep);
 	rinfo = (struct routeInfo *)ep->DataInfo_;
	
	assert((dstip=nodeid_to_ipv4addr(rinfo->dst_nid, 1)) > 0);
	if (rinfo->next_hop == 999999) {
		/* unreachable destination */
		assert(update_rtbl(dstip, rinfo->next_hop));  
	}
	else {
		assert((nxtip=nodeid_to_ipv4addr(rinfo->next_hop, 1)) > 0);
		assert(update_rtbl(dstip, nxtip) > 0);
	}

 	free(rinfo);
 	ep->DataInfo_ = 0;
  	freeEvent(ep); 
	return(1); 
}
