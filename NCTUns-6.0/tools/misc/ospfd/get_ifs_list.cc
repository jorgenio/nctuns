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

#define	 	HAVE_SYS_SYSCTL_H
#include	"unproute.h"

#include	<ifaddrs.h>

struct ifi_info* get_ifi_info(int family , int doaliases)
{

	struct ifaddrs *ifap0, *ifap;
	struct ifi_info *ifi, *ifi_head, **ifi_nextp;

	if( getifaddrs(&ifap0) ) {
		printf("getifaddrs() failed\n");
	}

	ifi_head = NULL;
	ifi_nextp = &ifi_head;

	for(ifap=ifap0; ifap!=NULL; ifap=ifap->ifa_next) {
		//char *buf;

		if( !ifap->ifa_addr || ifap->ifa_addr->sa_family != AF_INET ){
			//printf("%s is NULL or not AF_INET\n", ifap->ifa_name);
			continue;
		}

		// ignore interfaces which are down
		if( (ifap->ifa_flags & IFF_UP) == 0 ) {
			//printf("%s is down\n", ifap->ifa_name);
			continue;
		}

		ifi = (struct ifi_info*) calloc(1 , sizeof(struct ifi_info));
		*ifi_nextp = ifi;
		ifi_nextp = &ifi->ifi_next;

		//buf = inet_ntoa(((struct sockaddr_in *)ifap->ifa_addr)->sin_addr);

		snprintf(ifi->ifi_name, IFI_NAME, "%s", ifap->ifa_name);
		//printf("%s: %s\n", ifi->ifi_name, buf);

		ifi->ifi_flags = ifap->ifa_flags;

		ifi->ifi_addr = (struct sockaddr *) malloc(sizeof(struct sockaddr));
		memcpy(ifi->ifi_addr, ifap->ifa_addr, sizeof(struct sockaddr));

		ifi->ifi_brdaddr = (struct sockaddr *) malloc(sizeof(struct sockaddr));
		memcpy(ifi->ifi_brdaddr, ifap->ifa_broadaddr, sizeof(struct sockaddr));

		ifi->ifi_dstaddr = (struct sockaddr *) malloc(sizeof(struct sockaddr));
		memcpy(ifi->ifi_dstaddr, ifap->ifa_dstaddr, sizeof(struct sockaddr));
	}

	freeifaddrs(ifap0);

	return ifi_head;

}
