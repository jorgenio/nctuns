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

#ifndef __NCTUNS_maptable_h__
#define __NCTUNS_maptable_h__

#include <mylist.h>

/*--------------------------------------------------------------------------
 * Define maptable of system call action enumeration number
 *--------------------------------------------------------------------------*/       
enum maptable_action_enum {
	syscall_NSC_mt_ADD = 0x01,
	syscall_NSC_mt_FLUSH,
	syscall_NSC_mt_DISPLAY
};

#define START_PORT		5001
#define ENDING_PORT		65535


/* Define Map Table Data structure */
struct if_info {
	SLIST_ENTRY(if_info)	nextif; 	/* point to next if */
	u_int32_t		tid; 		/* tunnel ID */
	u_int32_t		portid;		/* port ID */
	u_long			*ip;		/* ipv4 addr. */
	u_long			*netmask; 	/* netmask */
	u_char			*mac;		/* ieee802 mac addr. */
	char			name[10]; 	/* device name */
}; 
 
struct maptable {
	SLIST_ENTRY(maptable)	nextnode;	/* to next node's if */
	SLIST_HEAD(, if_info)	ifinfo; 	/* to if belongs to this node */
	u_short			s_port;		/* port mapping, start port */
	u_int32_t		nodeID; 	/* node ID */
	u_int32_t		sequ;
	u_short			portnum;   	/* total port */
}; 
 

int umtbl_add			(u_int32_t, u_int32_t, u_int32_t, 
				     u_long *, u_long *, u_char *); 
int umtbl_configtun		(void); 
int umtbl_cpytokern		(void);
u_long mtbl_mactoip		(u_char *); 
u_char * mtbl_iptomac		(u_long); 
char *mtbl_getifnamebytunid	(u_int32_t); 
u_int32_t mtbl_getportbytunid	(u_int32_t);
u_int32_t mtbl_iptonid		(u_long);
u_int32_t mtbl_iptopid          (u_long);
 u_int32_t mtbl_nidtoip          (u_int32_t, u_int32_t); 
u_char mtbl_isbroadcast		(u_int32_t, u_long); 
u_int32_t mtbl_nidtoip		(u_int32_t, u_int32_t); 
struct maptable 
    *mtbl_getnidinfo		(u_int32_t);

void mtbl_display();

#endif /* __NCTUNS_maptable_h__ */
