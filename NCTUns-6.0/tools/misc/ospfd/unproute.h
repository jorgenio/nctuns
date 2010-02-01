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

#ifndef __UNPROUTE_H
#define __UNPROUTE_H

#include	"unp.h"
#include	<net/if.h>			/* if_msghdr{} */
#include	<net/route.h>		/* RTA_xxx constants */
#include	<sys/param.h>

#ifdef	HAVE_SYS_SYSCTL_H
#include	<sys/sysctl.h>		/* sysctl() */
#endif

#include  "unpifi.h"

			/* function prototypes */
void	 get_rtaddrs(int, struct sockaddr *, struct sockaddr **);
char	*net_rt_iflist(int, int, size_t *);
char	*net_rt_dump(int, int, size_t *);
char	*sock_masktop(struct sockaddr *, socklen_t);

			/* wrapper functions */
char	*Net_rt_iflist(int, int, size_t *);
char	*Net_rt_dump(int, int, size_t *);
#define	Sock_masktop(a,b)		sock_masktop((a), (b))

#endif
